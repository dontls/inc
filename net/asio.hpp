#pragma once

// 基于 asio-1.30.2 的现代异步 TCP 网络库：Context（事件循环）、Conn（连接）、
// Server（监听）、Client（客户端连接 + 自动重连）

#include "../buffer.hpp"
#include "../log.hpp"

#define ASIO_NO_DEPRECATED
#include <asio.hpp>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <system_error>
#include <thread>
#include <unordered_map>
#include <utility>

namespace libtcp {

using asio::ip::tcp;

class Context;
using ContextPtr = std::shared_ptr<Context>;
class Conn;
using ConnPtr = std::shared_ptr<Conn>;
class Server;
using ServerPtr = std::shared_ptr<Server>;
class Client;
using ClientPtr = std::shared_ptr<Client>;

// Conn回调
using ConnMsgHandler = std::function<int(ConnPtr, char *, size_t n)>;
using ConnCloseHandler = std::function<void(ConnPtr)>;

// 客户端回调类型
using ClientMsgHandler = std::function<int(ClientPtr, char *, size_t n)>;
using ClientConnectHandler = std::function<void(ClientPtr)>;
using ClientCloseHandler =
    std::function<void(ClientPtr, const std::error_code &)>;

// ====================================================================
// Context：基于 asio::io_context 的现代事件循环，多个 Server / Client 共享
// 使用 any_io_executor 和 executor_work_guard
// ====================================================================
class Context : public std::enable_shared_from_this<Context> {
public:
  Context() : work_guard_(asio::make_work_guard(io_)) {}
  ~Context() {
    Stop();
    Join();
  }

  Context(const Context &) = delete;
  Context &operator=(const Context &) = delete;

  // 底层 io_context
  asio::io_context &Io() { return io_; }

  void Run() { io_.run(); }
  void RunAsync() {
    if (!thread_.joinable())
      thread_ = std::thread([this] { io_.run(); });
  }

  // 停止事件循环并清理内部客户端引用（避免循环引用）
  void Stop() {
    work_guard_.reset();
    io_.stop();
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

  // 向事件循环投递一个任务（线程安全）
  template <typename F> void Post(F &&f) {
    asio::post(io_, std::forward<F>(f));
  }

  ClientPtr Dial(const char *host, uint16_t port, int id = 0);
  ServerPtr Listen(uint16_t port);

  // 全局默认回调
  ClientConnectHandler OnConnect;
  ClientMsgHandler OnMessage;
  ClientCloseHandler OnClose;

private:
  friend class Client;

  void KeepClient(int id, const ClientPtr &client) {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    clients_[id] = client;
  }

  void DropClient(int id) {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    clients_.erase(id);
  }

  asio::io_context io_;
  asio::executor_work_guard<asio::io_context::executor_type> work_guard_;
  std::thread thread_;
  std::mutex clients_mutex_;
  std::unordered_map<int, ClientPtr> clients_;
};

// ====================================================================
// Conn：连接会话，异步收发，使用 any_io_executor 和现代 strand
// ====================================================================
class Conn : public std::enable_shared_from_this<Conn> {
public:
  Conn(ContextPtr ctx, tcp::socket sock)
      : ctx_(std::move(ctx)), sock_(std::move(sock)),
        strand_(asio::make_strand(ctx_->Io().get_executor())) {}

  ~Conn() { CloseNow(); }

  Conn(const Conn &) = delete;
  Conn &operator=(const Conn &) = delete;

  tcp::socket &Socket() { return sock_; }

  std::string Remote() const {
    std::error_code ec;
    tcp::endpoint ep = sock_.remote_endpoint(ec);
    if (ec)
      return {};
    return ep.address().to_string() + ":" + std::to_string(ep.port());
  }

  std::string Local() const {
    std::error_code ec;
    tcp::endpoint ep = sock_.local_endpoint(ec);
    if (ec)
      return {};
    return ep.address().to_string() + ":" + std::to_string(ep.port());
  }

  bool IsOpen() const { return sock_.is_open() && !closed_; }

  void SetMessageHandler(ConnMsgHandler h) { handler_ = std::move(h); }
  void SetCloseHandler(ConnCloseHandler h) { on_close_ = std::move(h); }

  void Start() {
    auto self = shared_from_this();
    asio::post(strand_, [self, this]() {
      if (started_ || closed_)
        return;
      started_ = true;
      DoRead();
    });
  }

  void Send(const char *data, size_t len) { Send(std::string(data, len)); }
  void Send(const std::string &msg) {
    auto self = shared_from_this();
    auto payload = std::make_shared<std::string>(msg);
    asio::post(strand_, [self, this, payload]() {
      if (closed_ || !sock_.is_open())
        return;
      outbox_.push_back(std::move(*payload));
      if (!writing_)
        DoWrite();
    });
  }

  void Close() {
    auto self = shared_from_this();
    asio::post(strand_, [self, this]() { HandleClose(); });
  }

private:
  void DoRead() {
    auto self = shared_from_this();
    sock_.async_read_some(
        asio::buffer(buf_, sizeof(buf_)),
        asio::bind_executor(strand_,
                            [self, this](const std::error_code &ec, size_t n) {
                              if (ec) {
                                HandleClose();
                                return;
                              }
                              rbuf_.Write(buf_, n);
                              if (!DispatchMessages())
                                return;
                              DoRead();
                            }));
  }

  bool DispatchMessages() {
    for (;;) {
      const int consumed =
          handler_(shared_from_this(), rbuf_.Bytes(), rbuf_.Len());
      if (consumed < 0) {
        HandleClose();
        return false;
      }
      if (consumed == 0)
        return true;
      rbuf_.Remove(static_cast<size_t>(consumed));
    }
  }

  void DoWrite() {
    if (outbox_.empty() || closed_ || !sock_.is_open()) {
      writing_ = false;
      return;
    }
    writing_ = true;
    auto self = shared_from_this();
    asio::async_write(
        sock_, asio::buffer(outbox_.front()),
        asio::bind_executor(strand_,
                            [self, this](const std::error_code &ec, size_t) {
                              if (!outbox_.empty())
                                outbox_.pop_front();
                              if (ec) {
                                writing_ = false;
                                HandleClose();
                                return;
                              }
                              if (outbox_.empty()) {
                                writing_ = false;
                              } else {
                                DoWrite();
                              }
                            }));
  }

  void HandleClose() {
    if (closed_)
      return;
    CloseNow();
    if (on_close_)
      on_close_(shared_from_this());
  }

  void CloseNow() {
    if (closed_)
      return;
    closed_ = true;
    std::error_code ignored;
    sock_.shutdown(tcp::socket::shutdown_both, ignored);
    sock_.close(ignored);
    outbox_.clear();
  }

  ContextPtr ctx_;
  tcp::socket sock_;
  asio::strand<asio::any_io_executor> strand_;
  libyte::Buffer rbuf_;
  std::deque<std::string> outbox_;
  char buf_[8192];
  bool writing_ = false;
  bool closed_ = false;
  bool started_ = false;
  ConnMsgHandler handler_;
  ConnCloseHandler on_close_;
};

// ====================================================================
// Server：异步服务器，使用现代 asio 特性
// ====================================================================
class Server : public std::enable_shared_from_this<Server> {
public:
  explicit Server(ContextPtr ctx)
      : ctx_(std::move(ctx)), acceptor_(ctx_->Io()) {}

  ~Server() { Stop(); }

  Server(const Server &) = delete;
  Server &operator=(const Server &) = delete;

  bool Listen(uint16_t port, const std::string &host = "0.0.0.0");

  uint16_t Port() const {
    std::error_code ec;
    tcp::endpoint ep = acceptor_.local_endpoint(ec);
    return ec ? 0 : ep.port();
  }

  void Start() {
    if (!acceptor_.is_open() || started_)
      return;
    started_ = true;
    DoAccept();
  }

  void Stop() {
    std::error_code ec;
    acceptor_.close(ec);
  }

  void OnAccept(std::function<void(ConnPtr)> h) { on_accept_ = std::move(h); }
  void OnMessage(ConnMsgHandler h) { on_msg_ = std::move(h); }
  void OnClose(ConnCloseHandler h) { on_close_ = std::move(h); }

private:
  void DoAccept() {
    acceptor_.async_accept([this](const std::error_code &ec, tcp::socket sock) {
      if (ec) {
        if (ec != asio::error::operation_aborted)
          LogError(true, "accept failed: %s", ec.message().c_str());
        return;
      }
      auto conn = std::make_shared<Conn>(ctx_, std::move(sock));
      conn->SetCloseHandler(on_close_);
      conn->SetMessageHandler(on_msg_);
      if (on_accept_)
        on_accept_(conn);
      conn->Start();
      DoAccept();
    });
  }

  ContextPtr ctx_;
  tcp::acceptor acceptor_;
  bool started_ = false;
  std::function<void(ConnPtr)> on_accept_;
  ConnMsgHandler on_msg_;
  ConnCloseHandler on_close_;
};

inline bool Server::Listen(uint16_t port, const std::string &host) {
  std::error_code ec;
  asio::ip::address addr = asio::ip::make_address(host, ec);
  if (ec) {
    LogError(true, "make_address(%s) failed: %s", host.c_str(),
             ec.message().c_str());
    return false;
  }
  tcp::endpoint ep(addr, port);
  acceptor_.open(ep.protocol(), ec);
  if (ec) {
    LogError(true, "acceptor open failed: %s", ec.message().c_str());
    return false;
  }
  acceptor_.set_option(asio::socket_base::reuse_address(true), ec);
  if (ec) {
    LogError(true, "set reuse_address failed: %s", ec.message().c_str());
    return false;
  }
  acceptor_.bind(ep, ec);
  if (ec) {
    LogError(true, "bind %s:%u failed: %s", host.c_str(), port,
             ec.message().c_str());
    return false;
  }
  acceptor_.listen(asio::socket_base::max_listen_connections, ec);
  if (ec) {
    LogError(true, "listen failed: %s", ec.message().c_str());
    return false;
  }
  return true;
}

// ====================================================================
// Client：异步客户端，支持自动重连，使用 asio::steady_timer
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

  // 通常在 OnClose 回调中调用
  void Reconnect(int interval_ms);

  void Send(const char *data, size_t len) {
    if (conn_)
      conn_->Send(data, len);
  }
  void Send(const std::string &s) { Send(s.data(), s.size()); }

  void Close() {
    manual_close_ = true;
    CancelTimer();
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
  void OnMessage(ClientMsgHandler h) { on_msg_ = std::move(h); }
  void OnClose(ClientCloseHandler h) { on_close_ = std::move(h); }

private:
  Client(ContextPtr ctx, int id)
      : timer_(ctx->Io()), ctx_(std::move(ctx)), id_(id) {}

  void CancelTimer() { timer_.cancel(); }

  void ScheduleReconnect(int reconnect_ms);
  void DoConnect(const char *host, uint16_t port);
  void OnConnectDone(tcp::socket sock);

  asio::steady_timer timer_;
  std::weak_ptr<Context> ctx_;
  const int id_;
  ConnPtr conn_;
  std::string host_;
  uint16_t port_ = 0;
  bool connected_ = false;
  bool manual_close_ = false;
  bool connecting_ = false;
  ClientConnectHandler on_connect_;
  ClientMsgHandler on_msg_;
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
  CancelTimer();

  auto self = shared_from_this();
  ctx->Post([self, this, host, port]() { DoConnect(host, port); });
}

inline void Client::Reconnect(int interval_ms) {
  manual_close_ = false;
  CancelTimer();
  if (!connected_ && !connecting_)
    ScheduleReconnect(interval_ms);
}

inline void Client::ScheduleReconnect(int reconnect_ms) {
  if (manual_close_ || connected_ || connecting_)
    return;
  connecting_ = true;
  timer_.expires_after(std::chrono::milliseconds(reconnect_ms));
  auto self = shared_from_this();
  timer_.async_wait([self, this](const std::error_code &ec) {
    if (ec)
      return;
    connecting_ = false;
    if (!manual_close_)
      DoConnect(host_.c_str(), port_);
  });
}

inline void Client::DoConnect(const char *host, uint16_t port) {
  auto ctx = ctx_.lock();
  if (!ctx) {
    connecting_ = false;
    if (on_close_)
      on_close_(shared_from_this(),
                std::make_error_code(std::errc::operation_canceled));
    return;
  }

  if (manual_close_) {
    connecting_ = false;
    return;
  }

  // 使用同步DNS解析，然后异步连接
  std::error_code ec;
  tcp::resolver resolver(ctx->Io());
  tcp::resolver::results_type endpoints =
      resolver.resolve(host, std::to_string(port), ec);
  if (ec) {
    connecting_ = false;
    if (on_close_)
      on_close_(shared_from_this(), ec);
    return;
  }

  auto sock = std::make_shared<tcp::socket>(ctx->Io());
  auto self = shared_from_this();
  asio::async_connect(
      *sock, endpoints,
      [self, this, sock](const std::error_code &ec, const tcp::endpoint &) {
        connecting_ = false;
        if (ec) {
          if (on_close_)
            on_close_(self, ec);
          return;
        }
        if (manual_close_)
          return;
        OnConnectDone(std::move(*sock));
      });
}

inline void Client::OnConnectDone(tcp::socket sock) {
  auto ctx = ctx_.lock();
  if (!ctx) {
    connecting_ = false;
    return;
  }

  if (manual_close_) {
    connecting_ = false;
    return;
  }

  connecting_ = false;
  conn_ = std::make_shared<Conn>(ctx, std::move(sock));
  std::weak_ptr<Client> weak = shared_from_this();
  conn_->SetMessageHandler([weak](ConnPtr, char *data, size_t n) {
    auto client = weak.lock();
    if (!client || !client->on_msg_)
      return -1;
    return client->on_msg_(client, data, n);
  });

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
  std::weak_ptr<Context> weak_ctx = shared_from_this();
  ClientCloseHandler user_close = OnClose;
  c->OnClose(
      [weak_ctx, user_close, id](ClientPtr client, const std::error_code &ec) {
        if (user_close)
          user_close(client, ec);
        auto ctx = weak_ctx.lock();
        if (ctx)
          ctx->DropClient(id);
      });
  c->Connect(host, port);
  return c;
}

inline ServerPtr Context::Listen(uint16_t port) {
  auto s = std::make_shared<Server>(shared_from_this());
  if (!s->Listen(port))
    return nullptr;
  s->Start(); // 自动启动服务器
  return s;
}

} // namespace libtcp