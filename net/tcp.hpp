#pragma once

// Linux epoll TCP 网络库：Context（事件循环）、Conn（连接）、
// Server（监听）、Client（客户端连接 + 自动重连）

#include "../buffer.hpp"
#include "../log.hpp"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <unistd.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <system_error>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

namespace libtcp {

namespace detail {
inline bool WouldBlock(int error) {
  return error == EAGAIN || error == EWOULDBLOCK;
}
inline void CloseFd(int &fd) {
  if (fd >= 0) {
    ::close(fd);
    fd = -1;
  }
}
} // namespace detail

class Context;
using ContextPtr = std::shared_ptr<Context>;
class Conn;
using ConnPtr = std::shared_ptr<Conn>;
class Server;
using ServerPtr = std::shared_ptr<Server>;
class Client;
using ClientPtr = std::shared_ptr<Client>;

// 错误代码
enum class ErrorCode { Success = 0, Closed = 1, ConnectFailed = 2 };

// 回调类型
using MsgHandler = std::function<int(ConnPtr, libyte::Buffer &)>;
using ConnCloseHandler = std::function<void(ConnPtr)>;
using ClientConnectHandler = std::function<void(ClientPtr)>;
using ClientCloseHandler =
    std::function<void(ClientPtr, const std::error_code &)>;

// ====================================================================
// Context：epoll 事件循环，多个 Server / Client 共享
// ====================================================================
class Context : public std::enable_shared_from_this<Context> {
public:
  Context() { epfd_ = ::epoll_create1(EPOLL_CLOEXEC); }
  ~Context() {
    Stop();
    Join();
    if (epfd_ >= 0)
      ::close(epfd_);
  }

  Context(const Context &) = delete;
  Context &operator=(const Context &) = delete;

  void Run() { Loop(); }
  void RunAsync() {
    if (!thread_.joinable())
      thread_ = std::thread([this] { Loop(); });
  }

  // 停止事件循环并清理内部客户端引用（避免循环引用）
  void Stop() {
    running_ = false;
    std::unordered_map<int, ClientPtr> temp;
    {
      std::lock_guard<std::mutex> lock(clients_mutex_);
      temp.swap(clients_);
    }
  }

  void Join() {
    if (thread_.joinable())
      thread_.join();
  }

  ClientPtr Dial(const char *host, uint16_t port, int id = 0);
  ServerPtr Listen(uint16_t port);

  // 全局默认回调
  ClientConnectHandler OnConnect;
  MsgHandler OnMessage;
  ClientCloseHandler OnClose;

private:
  friend class Conn;
  friend class Server;
  friend class Client;

  void KeepClient(int id, const ClientPtr &client) {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    clients_[id] = client;
  }

  void DropClient(int id) {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    clients_.erase(id);
  }

  void Loop() {
    running_ = true;
    while (running_) {
      epoll_event evs[64];
      int n = ::epoll_wait(epfd_, evs, 64, 100);
      if (n < 0) {
        if (errno == EINTR)
          continue;
        break;
      }
      for (int i = 0; i < n; ++i) {
        std::function<void(uint32_t)> cb;
        {
          std::lock_guard<std::mutex> lock(fd_mutex_);
          auto it = fd_handlers_.find(evs[i].data.fd);
          if (it != fd_handlers_.end())
            cb = it->second;
        }
        if (cb)
          cb(evs[i].events);
      }
    }
    running_ = false;
  }

  bool AddFd(int fd, std::function<void(uint32_t)> cb,
             uint32_t events = EPOLLIN) {
    epoll_event ev{};
    ev.events = events;
    ev.data.fd = fd;
    if (::epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ev) != 0) {
      LogError(true, "epoll_ctl ADD fd %d failed: %s", fd, strerror(errno));
      return false;
    }
    {
      std::lock_guard<std::mutex> lock(fd_mutex_);
      fd_handlers_[fd] = std::move(cb);
    }
    return true;
  }

  void ModFd(int fd, uint32_t events) {
    epoll_event ev{};
    ev.events = events;
    ev.data.fd = fd;
    ::epoll_ctl(epfd_, EPOLL_CTL_MOD, fd, &ev);
  }

  void DelFd(int fd) {
    {
      std::lock_guard<std::mutex> lock(fd_mutex_);
      fd_handlers_.erase(fd);
    }
    ::epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, nullptr);
  }

  int epfd_ = -1;
  bool running_ = false;
  std::thread thread_;
  std::mutex fd_mutex_;
  std::unordered_map<int, std::function<void(uint32_t)>> fd_handlers_;
  std::mutex clients_mutex_;
  std::unordered_map<int, ClientPtr> clients_;
};

// 将 fd 设为非阻塞
inline void SetNonBlock(int fd) {
  const int flags = ::fcntl(fd, F_GETFL, 0);
  if (flags >= 0)
    ::fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// fd 的 "ip:port"（peer=true 取远端，否则取本地）
inline std::string SockName(int fd, bool peer) {
  struct sockaddr_in addr;
  socklen_t len = sizeof(addr);
  if (fd < 0)
    return {};
  int rc = peer ? ::getpeername(fd, (struct sockaddr *)&addr, &len)
                : ::getsockname(fd, (struct sockaddr *)&addr, &len);
  if (rc != 0)
    return {};
  char ip[INET_ADDRSTRLEN];
  ::inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));
  return std::string(ip) + ":" + std::to_string(ntohs(addr.sin_port));
}

// ====================================================================
// Conn：连接会话，异步收发
// ====================================================================
class Conn : public std::enable_shared_from_this<Conn> {
public:
  Conn(ContextPtr ctx, int fd) : ctx_(std::move(ctx)), fd_(fd) {}
  ~Conn() { close_now(false); }

  Conn(const Conn &) = delete;
  Conn &operator=(const Conn &) = delete;

  int Fd() const { return fd_; }
  std::string Remote() const { return SockName(fd_, true); }
  std::string Local() const { return SockName(fd_, false); }
  bool IsOpen() const { return fd_ >= 0 && !closed_; }

  void SetMessageHandler(MsgHandler h) { handler_ = std::move(h); }
  void SetCloseHandler(ConnCloseHandler h) { on_close_ = std::move(h); }

  void Start() {
    if (started_)
      return;
    started_ = true;
    auto self = shared_from_this();
    ctx_->AddFd(fd_, [self](uint32_t ev) { self->HandleEvent(ev); });
  }

  void Send(const char *data, size_t len) {
    if (!closed_ && fd_ >= 0) {
      outbox_.Write((char *)data, len);
      HandleWrite();
    }
  }

  void Close() { close_now(true); }

private:
  void HandleEvent(uint32_t ev) {
    if (ev & (EPOLLERR | EPOLLHUP)) {
      close_now(true);
      return;
    }
    if (ev & EPOLLIN)
      HandleRead();
    if (ev & EPOLLOUT)
      HandleWrite();
  }

  void HandleRead() {
    char buf[8192];
    for (;;) {
      ssize_t n = ::recv(fd_, buf, sizeof(buf), 0);
      if (n > 0) {
        rbuf_.Write(buf, (size_t)n);
        if (!DispatchMessages())
          return;
        continue;
      }
      if (n == 0) {
        close_now(true);
        return;
      }
      if (errno == EINTR)
        continue;
      if (detail::WouldBlock(errno))
        break;
      close_now(true);
      return;
    }
  }

  bool DispatchMessages() {
    if (!handler_) {
      rbuf_.Reset();
      return true;
    }
    for (;;) {
      const size_t available = rbuf_.Len();
      const int consumed = handler_(shared_from_this(), rbuf_);
      if (consumed < 0) {
        close_now(true);
        return false;
      }
      if (consumed == 0)
        return true;
      if (static_cast<size_t>(consumed) > available) {
        LogError(false, "message handler consumed %d bytes from %zu", consumed,
                 available);
        close_now(true);
        return false;
      }
      rbuf_.Remove(static_cast<size_t>(consumed));
    }
  }

  void HandleWrite() {
    if (closed_ || fd_ < 0)
      return;
    while (!outbox_.Empty()) {
      ssize_t n = ::send(fd_, outbox_.Bytes(), outbox_.Len(), 0);
      if (n > 0) {
        outbox_.Remove((size_t)n);
        continue;
      }
      if (n < 0 && detail::WouldBlock(errno))
        break;
      if (n < 0 && errno != EINTR) {
        close_now(true);
      }
      return;
    }
  }

  void close_now(bool notify) {
    if (closed_)
      return;
    closed_ = true;
    if (fd_ >= 0) {
      if (notify)
        ctx_->DelFd(fd_);
      detail::CloseFd(fd_);
    }
    outbox_.Reset();
    if (notify && on_close_)
      on_close_(shared_from_this());
  }

  ContextPtr ctx_;
  int fd_ = -1;
  libyte::Buffer rbuf_;
  libyte::Buffer outbox_;
  bool closed_ = false;
  bool started_ = false;
  MsgHandler handler_;
  ConnCloseHandler on_close_;
};

// ====================================================================
// Server：异步服务器
// ====================================================================
class Server : public std::enable_shared_from_this<Server> {
public:
  explicit Server(ContextPtr ctx) : ctx_(std::move(ctx)) {}
  ~Server() { Stop(); }

  Server(const Server &) = delete;
  Server &operator=(const Server &) = delete;

  bool Listen(uint16_t port, const std::string &host = "0.0.0.0");

  uint16_t Port() const {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    if (listen_fd_ < 0 ||
        ::getsockname(listen_fd_, (struct sockaddr *)&addr, &len) != 0)
      return 0;
    return ntohs(addr.sin_port);
  }

  void Start() {
    if (listen_fd_ < 0 || started_)
      return;
    started_ = true;
    auto self = shared_from_this();
    ctx_->AddFd(listen_fd_, [self](uint32_t ev) {
      if (ev & (EPOLLERR | EPOLLHUP))
        self->Stop();
      else if (ev & EPOLLIN)
        self->HandleAccept();
    });
  }

  void Stop() {
    if (listen_fd_ >= 0) {
      ctx_->DelFd(listen_fd_);
      ::close(listen_fd_);
      listen_fd_ = -1;
    }
  }

  void OnAccept(std::function<void(ConnPtr)> h) { on_accept_ = std::move(h); }
  void OnMessage(MsgHandler h) { on_msg_ = std::move(h); }
  void OnClose(ConnCloseHandler h) { on_close_ = std::move(h); }

private:
  void HandleAccept() {
    for (;;) {
      int fd = ::accept(listen_fd_, nullptr, nullptr);
      if (fd < 0) {
        if (errno == EINTR)
          continue;
        if (errno == EAGAIN || errno == EWOULDBLOCK)
          break;
        LogError(true, "accept failed: %s", strerror(errno));
        break;
      }
      SetNonBlock(fd);
      auto conn = std::make_shared<Conn>(ctx_, fd);
      conn->SetCloseHandler(on_close_);
      conn->SetMessageHandler(on_msg_);
      if (on_accept_)
        on_accept_(conn);
      conn->Start();
    }
  }

  ContextPtr ctx_;
  int listen_fd_ = -1;
  bool started_ = false;
  std::function<void(ConnPtr)> on_accept_;
  MsgHandler on_msg_;
  ConnCloseHandler on_close_;
};

inline bool Server::Listen(uint16_t port, const std::string &host) {
  int fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    LogError(true, "socket failed: %s", strerror(errno));
    return false;
  }

  int reuse = 1;
  ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = (host.empty() || host == "0.0.0.0")
                             ? htonl(INADDR_ANY)
                             : inet_addr(host.c_str());

  SetNonBlock(fd);
  if (::bind(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
    LogError(true, "bind %s:%u failed: %s", host.c_str(), port,
             strerror(errno));
    ::close(fd);
    return false;
  }
  if (::listen(fd, 128) != 0) {
    LogError(true, "listen failed: %s", strerror(errno));
    ::close(fd);
    return false;
  }
  listen_fd_ = fd;
  return true;
}

// ====================================================================
// Client：异步客户端，支持自动重连
// ====================================================================
class Client : public std::enable_shared_from_this<Client> {
public:
  static ClientPtr Create(ContextPtr ctx, int id = 0) {
    return ClientPtr(new Client(std::move(ctx), id));
  }
  ~Client() { Close(); }

  Client(const Client &) = delete;
  Client &operator=(const Client &) = delete;

  void Connect(const char *host, uint16_t port);

  void Send(const char *data, size_t len) {
    if (conn_)
      conn_->Send(data, len);
  }
  void Send(const std::string &s) { Send(s.data(), s.size()); }

  // 在连接关闭后发起重连（毫秒，<=0 表示取消重连）。
  // 通常在 OnClose 回调中调用。
  void Reconnect(int interval_ms);

  void Close() {
    manual_close_ = true;
    cancel_timer();
    if (connect_fd_ >= 0) {
      if (auto ctx = ctx_.lock())
        ctx->DelFd(connect_fd_);
      ::close(connect_fd_);
      connect_fd_ = -1;
    }
    auto c = conn_;
    conn_.reset();
    if (auto ctx = ctx_.lock())
      ctx->DropClient(id_);
    if (c)
      c->Close();
  }

  bool IsConnected() const { return connected_; }
  int Id() const { return id_; }

  void OnConnect(ClientConnectHandler h) { on_connect_ = std::move(h); }
  void OnMessage(MsgHandler h) { on_msg_ = std::move(h); }
  void OnClose(ClientCloseHandler h) { on_close_ = std::move(h); }

private:
  Client(ContextPtr ctx, int id) : ctx_(std::move(ctx)), id_(id) {}

  void cancel_timer() {
    if (timerfd_ >= 0) {
      if (auto ctx = ctx_.lock())
        ctx->DelFd(timerfd_);
      ::close(timerfd_);
      timerfd_ = -1;
    }
  }

  void schedule_reconnect(int reconnect_ms_) {
    if (manual_close_ || connecting_)
      return;
    connecting_ = true;
    timerfd_ = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (timerfd_ < 0)
      return;
    struct itimerspec its{};
    its.it_value.tv_sec = reconnect_ms_ / 1000;
    its.it_value.tv_nsec = (reconnect_ms_ % 1000) * 1000000;
    ::timerfd_settime(timerfd_, 0, &its, nullptr);
    if (auto ctx = ctx_.lock()) {
      auto self = shared_from_this();
      ctx->AddFd(timerfd_, [self](uint32_t) { self->on_timer(); });
    }
  }

  void on_timer() {
    connecting_ = false;
    cancel_timer();
    if (manual_close_) {
      return;
    }
    // 如果已有解析结果，直接用；否则异步解析
    if (!addrs_.empty()) {
      addr_idx_ = 0;
      try_connect_next();
    } else {
      std::thread([this]() {
        auto self = shared_from_this();
        do_resolve_and_connect(host_.c_str(), port_);
      }).detach();
    }
  }

  // 在后台线程中执行 DNS 解析和连接
  void do_resolve_and_connect(const char *host, uint16_t port) {
    struct addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo *res = nullptr;
    if (::getaddrinfo(host, std::to_string(port).c_str(), &hints, &res) != 0) {
      if (on_close_) {
        on_close_(shared_from_this(),
                  std::make_error_code(std::errc::host_unreachable));
      }
      return;
    }

    std::vector<sockaddr_in> new_addrs;
    for (struct addrinfo *rp = res; rp; rp = rp->ai_next) {
      if (rp->ai_family == AF_INET && rp->ai_addrlen >= sizeof(sockaddr_in)) {
        sockaddr_in sa;
        memcpy(&sa, rp->ai_addr, sizeof(sa));
        new_addrs.push_back(sa);
      }
    }
    ::freeaddrinfo(res);

    if (new_addrs.empty()) {
      if (on_close_) {
        on_close_(shared_from_this(),
                  std::make_error_code(std::errc::host_unreachable));
      }
      return;
    }

    // DNS 解析完成，保存结果并回到主线程执行连接
    addrs_ = std::move(new_addrs);
    addr_idx_ = 0;
    try_connect_next();
  }

  void try_connect_next();
  void on_connect_event(uint32_t);
  void on_connect_done(int fd);

  std::weak_ptr<Context> ctx_;
  const int id_;
  ConnPtr conn_;
  std::string host_;
  uint16_t port_ = 0;
  bool connected_ = false;
  bool manual_close_ = false;
  bool connecting_ = false;
  int timerfd_ = -1;
  std::vector<sockaddr_in> addrs_;
  size_t addr_idx_ = 0;
  int connect_fd_ = -1;
  ClientConnectHandler on_connect_;
  MsgHandler on_msg_;
  ClientCloseHandler on_close_;
};

inline void Client::Connect(const char *host, uint16_t port) {
  auto ctx = ctx_.lock();
  if (!ctx)
    return;

  manual_close_ = false;
  connecting_ = true;
  host_ = host;
  port_ = port;
  cancel_timer();

  if (connect_fd_ >= 0) {
    ctx->DelFd(connect_fd_);
    ::close(connect_fd_);
    connect_fd_ = -1;
  }

  // 检查是否需要重新解析（host/port 改变或首次连接）
  if (addrs_.empty() || addr_idx_ >= addrs_.size()) {
    addr_idx_ = 0;
    do_resolve_and_connect(host, port);
  } else {
    // 复用已有的地址列表
    addr_idx_ = 0;
    try_connect_next();
  }
}

inline void Client::Reconnect(int interval_ms) {
  manual_close_ = false;
  cancel_timer();
  if (!connected_ && !connecting_)
    schedule_reconnect(interval_ms);
}

inline void Client::try_connect_next() {
  auto ctx = ctx_.lock();
  if (!ctx) {
    connecting_ = false;
    if (on_close_) {
      on_close_(shared_from_this(),
                std::make_error_code(std::errc::operation_canceled));
    }
    return;
  }

  if (connect_fd_ >= 0) {
    ctx->DelFd(connect_fd_);
    ::close(connect_fd_);
    connect_fd_ = -1;
  }

  for (;;) {
    if (manual_close_)
      return;
    if (addr_idx_ >= addrs_.size()) {
      connecting_ = false;
      if (on_close_) {
        on_close_(shared_from_this(),
                  std::make_error_code(std::errc::connection_refused));
      }
      return;
    }

    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
      ++addr_idx_;
      continue;
    }

    SetNonBlock(fd);
    const sockaddr_in &sa = addrs_[addr_idx_];
    int rc = ::connect(fd, (const struct sockaddr *)&sa, sizeof(sa));

    if (rc == 0) {
      on_connect_done(fd);
      return;
    }

    if (errno == EINPROGRESS || errno == EWOULDBLOCK) {
      connect_fd_ = fd;
      auto self = shared_from_this();
      ctx->AddFd(
          fd, [self](uint32_t ev) { self->on_connect_event(ev); }, EPOLLOUT);
      return;
    }

    ::close(fd);
    ++addr_idx_;
  }
}

inline void Client::on_connect_event(uint32_t) {
  auto ctx = ctx_.lock();
  if (!ctx) {
    connecting_ = false;
    return;
  }

  int fd = connect_fd_;
  connect_fd_ = -1;
  if (fd < 0)
    return;

  ctx->DelFd(fd);

  if (manual_close_) {
    connecting_ = false;
    ::close(fd);
    return;
  }

  int err = 0;
  socklen_t len = sizeof(err);
  ::getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len);

  if (err != 0) {
    ::close(fd);
    ++addr_idx_;
    try_connect_next();
    return;
  }

  on_connect_done(fd);
}

inline void Client::on_connect_done(int fd) {
  auto ctx = ctx_.lock();
  if (!ctx) {
    connecting_ = false;
    ::close(fd);
    return;
  }

  if (manual_close_) {
    connecting_ = false;
    ::close(fd);
    return;
  }

  connecting_ = false;
  conn_ = std::make_shared<Conn>(ctx, fd);
  conn_->SetMessageHandler(on_msg_);

  std::weak_ptr<Client> weak = shared_from_this();
  conn_->SetCloseHandler([weak](ConnPtr) {
    auto client = weak.lock();
    if (!client)
      return;
    client->connected_ = false;
    client->conn_.reset();
    if (!client->manual_close_ && client->on_close_)
      client->on_close_(client,
                        std::make_error_code(std::errc::connection_reset));
  });

  conn_->Start();
  connected_ = true;
  if (on_connect_)
    on_connect_(shared_from_this());
}

inline ClientPtr Context::Dial(const char *host, uint16_t port, int id) {
  auto c = Client::Create(shared_from_this(), id);
  KeepClient(id, c);
  c->OnConnect(OnConnect);
  c->OnMessage(OnMessage);
  c->OnClose(OnClose);
  c->Connect(host, port);
  return c;
}

inline ServerPtr Context::Listen(uint16_t port) {
  auto s = std::make_shared<Server>(shared_from_this());
  if (!s->Listen(port))
    return nullptr;
  return s;
}

} // namespace libtcp
