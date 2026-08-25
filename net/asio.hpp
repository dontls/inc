#pragma once

// 基于 asio-1.30.2 的异步 TCP 封装：
//   Context   - 封装 asio::io_context，多个 Server / Client 共享同一个
//               Context 时，所有 IO 事件都在同一个 io_context 中统一调度。
//   Conn   - 一条已建立的 TCP 连接（会话），异步收发，通过 shared_ptr
//   自持生命周期。 Server - 异步 TCP 服务器，接受连接并生成 Conn 会话。
//   Client - 异步 TCP 客户端，与多个客户端共享同一个 Context。

// 使用推荐的新式 asio API（同步操作返回 void 而非被标记 nodiscard 的
// error_code）
#include <cstddef>
#define ASIO_NO_DEPRECATED

#include <asio.hpp>

#include "../buffer.hpp"
#include "../log.hpp"

#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <map>
#include <string>
#include <thread>
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

// ========================================================================
// 消息处理器：在事件循环线程中调用，conn 为触发该回调的连接。
// 从缓冲区消费数据，返回本次消费的字节数：
//   > 0 : 消费 n 字节，循环继续解析下一条消息；
//   0   : 数据不足，等待更多数据；
//   < 0 : 关闭该连接。
// ========================================================================
using ConnMsgHandler = std::function<int(ConnPtr, char *, size_t n)>;

// ========================================================================
// 客户端连接成功回调
// ========================================================================
using ClientConnectHandler = std::function<void(ClientPtr)>;
// 客户端连接关闭 / 失败回调
using ClientCloseHandler =
    std::function<void(ClientPtr, const std::error_code &)>;
using ClientMsgHandler = std::function<int(ClientPtr, char *, size_t n)>;

// ========================================================================
// Context : 封装 asio::io_context。
// 多个 Server / Client 共享同一个 Context 时，所有 IO 事件都由该
// Context 统一调度，通常通过 std::shared_ptr 传递以保持其生命周期。
// ========================================================================
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

  // 阻塞运行事件循环，直到 Stop() 或没有待处理任务
  void Run() { io_.run(); }

  // 在一个线程中异步运行事件循环（幂等）
  void RunAsync() {
    if (thread_.joinable()) {
      return;
    }
    thread_ = std::thread([this] { io_.run(); });
  }

  // 停止事件循环（线程安全）
  void Stop() {
    io_.stop();
    std::map<int, ClientPtr> clients;
    {
      std::lock_guard<std::mutex> lock(clients_mutex_);
      clients.swap(clients_);
    }
    // Release Context's ownership on shutdown.  This also breaks the
    // Context -> Client -> Context ownership cycle.
    clients.clear();
  }

  // Stop() 之后需要 Restart() 才能再次 Run()
  void Restart() { io_.restart(); }

  // 等待 RunAsync() 启动的线程结束
  void Join() {
    if (thread_.joinable()) {
      thread_.join();
    }
  }

  // 向事件循环投递一个任务（线程安全）
  template <typename F> void Post(F &&f) {
    asio::post(io_, std::forward<F>(f));
  }

  // 创建客户端并发起连接（便捷方法，阻塞式同步连接）
  ClientPtr Dial(const char *host, uint16_t port, int id = 0);

  // 创建并启动服务器（便捷方法）
  ServerPtr Listen(uint16_t port);

  // 全局默认回调，Dial/Listen 创建的客户端/服务端会自动继承
  ClientConnectHandler OnConnect = nullptr;
  ClientMsgHandler OnMessage = nullptr;
  ClientCloseHandler OnClose = nullptr;

private:
  friend class Client;

  void KeepClient(int id, const ClientPtr &client) {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    clients_[id] = client;
  }

  void ReleaseClient(int id) {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    clients_.erase(id);
  }

  asio::io_context io_;
  // Keep run()/run_one() alive even when no async operation is pending.
  // Without this, RunAsync() may return before a later Context::Post(),
  // leaving tasks such as Conn::Send() unexecuted.
  asio::executor_work_guard<asio::io_context::executor_type> work_guard_;
  std::thread thread_;
  std::mutex clients_mutex_;
  std::map<int, ClientPtr> clients_;
};

// 连接关闭回调
using ConnCloseHandler = std::function<void(ConnPtr)>;

// ========================================================================
// Conn : 一条已建立的 TCP 连接（会话）。
// 通过 shared_ptr 自持生命周期：只要有未完成的异步操作，对象就不会被销毁。
// 请通过服务器 Accept 或客户端连接得到 ConnPtr 使用。
// ========================================================================
class Conn : public std::enable_shared_from_this<Conn> {
public:
  Conn(ContextPtr ctx, tcp::socket sock)
      : ctx_(std::move(ctx)), sock_(std::move(sock)),
        strand_(ctx_->Io().get_executor()) {}

  // 析构时直接关闭底层 socket（此时对象已无引用，不能再调用 shared_from_this）
  ~Conn() { close_now(); }

  Conn(const Conn &) = delete;
  Conn &operator=(const Conn &) = delete;

  // 底层 socket
  tcp::socket &Socket() { return sock_; }

  // 远端地址 "ip:port"
  std::string Remote() const {
    std::error_code ec;
    tcp::endpoint ep = sock_.remote_endpoint(ec);
    if (ec) {
      return std::string();
    }
    return ep.address().to_string() + ":" + std::to_string(ep.port());
  }

  // 本地地址 "ip:port"
  std::string Local() const {
    std::error_code ec;
    tcp::endpoint ep = sock_.local_endpoint(ec);
    if (ec) {
      return std::string();
    }
    return ep.address().to_string() + ":" + std::to_string(ep.port());
  }

  bool IsOpen() const { return sock_.is_open(); }

  // 设置消息处理器
  void SetMessageHandler(ConnMsgHandler h) { handler_ = std::move(h); }

  // 设置连接关闭回调
  void SetCloseHandler(ConnCloseHandler h) { on_close_ = std::move(h); }

  // 启动异步读取循环（最多调用一次）
  void Start() {
    auto self = shared_from_this();
    asio::post(strand_, [self, this]() {
      if (started_ || closed_) {
        return;
      }
      started_ = true;
      do_read();
    });
  }

  // 发送数据（线程安全，可在任意线程调用）
  void Send(const char *data, size_t len) { Send(std::string(data, len)); }
  void Send(const std::string &msg) {
    auto self = shared_from_this();
    // C++11 has no init-capture; keep the moved payload alive through Post().
    auto payload = std::make_shared<std::string>(msg);
    asio::post(strand_, [self, this, payload]() {
      if (closed_ || !sock_.is_open()) {
        return;
      }
      outbox_.push_back(std::move(*payload));
      if (!writing_) {
        do_write();
      }
    });
  }

  // 关闭连接（线程安全）
  void Close() {
    auto self = shared_from_this();
    asio::post(strand_, [self, this]() { handle_close(std::error_code()); });
  }

private:
  void do_read() {
    auto self = shared_from_this();
    sock_.async_read_some(
        asio::buffer(buf_, sizeof(buf_)),
        asio::bind_executor(
            strand_, [self, this](const std::error_code &ec, size_t n) {
              if (ec) {
                handle_close(ec);
                return;
              }
              rbuf_.Write(buf_, n);
              for (;;) {
                int consumed =
                    handler_(shared_from_this(), rbuf_.Bytes(), rbuf_.Len());
                if (consumed < 0) {
                  handle_close(ec);
                  return;
                }
                if (consumed == 0) {
                  break;
                }
                rbuf_.Remove((size_t)consumed);
              }
              do_read();
            }));
  }

  void do_write() {
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
                              // close_now() may clear the queue while an async
                              // write is being cancelled.  Its completion
                              // handler still runs, so never pop an
                              // already-cleared queue.
                              if (!outbox_.empty()) {
                                outbox_.pop_front();
                              }
                              if (ec) {
                                writing_ = false;
                                handle_close(ec);
                                return;
                              }
                              if (outbox_.empty()) {
                                writing_ = false;
                              } else {
                                do_write();
                              }
                            }));
  }

  void handle_close(const std::error_code &ec) {
    if (closed_) {
      return;
    }
    close_now();
    if (on_close_) {
      on_close_(shared_from_this());
    }
  }

  // 立即关闭底层 socket（不触发回调，析构或事件循环线程内使用）
  void close_now() {
    if (closed_) {
      return;
    }
    closed_ = true;
    std::error_code ignored;
    sock_.shutdown(tcp::socket::shutdown_both, ignored);
    sock_.close(ignored);
    outbox_.clear();
  }

  ContextPtr ctx_;
  tcp::socket sock_;
  asio::strand<asio::io_context::executor_type> strand_;
  libyte::Buffer rbuf_;            // 接收缓冲
  std::deque<std::string> outbox_; // 发送队列
  char buf_[8192];                 // 单次读取缓冲
  bool writing_ = false;
  bool closed_ = false;
  bool started_ = false;
  ConnMsgHandler handler_;
  ConnCloseHandler on_close_;
};

// ========================================================================
// Server : 基于 asio 的异步 TCP 服务器。
// 生命周期要求：Start() 之后的接受循环会捕获 this，使用期间 Server
// 对象必须保持存活（通常在程序 / 服务生命周期内持有）。
// ========================================================================
class Server {
public:
  explicit Server(ContextPtr ctx)
      : ctx_(std::move(ctx)), acceptor_(ctx_->Io()) {}

  ~Server() { Stop(); }

  Server(const Server &) = delete;
  Server &operator=(const Server &) = delete;

  // 绑定并监听端口，失败返回 false
  bool Listen(uint16_t port, const std::string &host = "0.0.0.0");

  // 实际监听端口（端口传 0 时由系统分配）
  uint16_t Port() const {
    std::error_code ec;
    tcp::endpoint ep = acceptor_.local_endpoint(ec);
    return ec ? 0 : ep.port();
  }

  // 开始异步接受连接
  void Start() { do_accept(); }

  // 停止接受并关闭监听 socket
  void Stop() {
    std::error_code ec;
    acceptor_.close(ec);
  }

  // 新连接回调（可在其中定制该连接的处理器）
  void OnAccept(std::function<void(ConnPtr)> h) { on_accept_ = std::move(h); }
  // 默认消息回调，作用于每个新连接
  void OnMessage(ConnMsgHandler h) { on_msg_ = std::move(h); }
  // 连接关闭回调
  void OnClose(ConnCloseHandler h) { on_close_ = std::move(h); }

private:
  void do_accept() {
    acceptor_.async_accept([this](const std::error_code &ec, tcp::socket sock) {
      if (ec) {
        if (ec != asio::error::operation_aborted) {
          LogError(true, "accept failed: %s", ec.message().c_str());
        }
        return;
      }
      auto conn = std::make_shared<Conn>(ctx_, std::move(sock));
      conn->SetCloseHandler(on_close_);
      conn->SetMessageHandler(on_msg_);
      if (on_accept_) {
        on_accept_(conn); // 用户可在此时覆盖该连接的处理逻辑
      }
      conn->Start();
      do_accept();
    });
  }

  ContextPtr ctx_;
  tcp::acceptor acceptor_;
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

// ========================================================================
// Client : 基于 asio 的同步 TCP 客户端。
// 多个客户端共享同一个 Context（事件循环），收发在该线程中完成。
// 由于回调需要保证对象存活，请通过 Create() 以 shared_ptr 创建。
// ========================================================================
class Client : public std::enable_shared_from_this<Client> {
public:
  static ClientPtr Create(ContextPtr ctx, int id = 0) {
    return ClientPtr(new Client(std::move(ctx), id));
  }

  ~Client() { Close(); }

  Client(const Client &) = delete;
  Client &operator=(const Client &) = delete;

  bool ConnectSync(const char *host, uint16_t port);
  void Connect(const char *host, uint16_t port) { ConnectSync(host, port); }
  void Reconnect(int) {
    if (!connected_ && !host_.empty())
      Connect(host_.c_str(), port_);
  }

  void Send(const char *data, size_t len) { Send(std::string(data, len)); }
  void Send(const std::string &s) {
    if (conn_) {
      conn_->Send(s);
    }
  }

  void Close() {
    manual_close_ = true;
    connected_ = false;
    auto c = conn_;
    conn_.reset();
    if (c) {
      c->Close();
    }
  }

  bool IsConnected() const { return connected_; }
  int Id() const { return id_; }

  void OnConnect(ClientConnectHandler h) { on_connect_ = std::move(h); }
  void OnMessage(ClientMsgHandler h) { on_msg_ = std::move(h); }
  void OnClose(ClientCloseHandler h) { on_close_ = std::move(h); }

private:
  Client(ContextPtr ctx, int id) : ctx_(std::move(ctx)), id_(id) {}

  ContextPtr ctx_;
  const int id_;
  ConnPtr conn_;
  bool connected_ = false;
  bool manual_close_ = false;
  std::string host_;
  uint16_t port_ = 0;
  ClientConnectHandler on_connect_;
  ClientMsgHandler on_msg_;
  ClientCloseHandler on_close_;
};

inline bool Client::ConnectSync(const char *host, uint16_t port) {
  host_ = host;
  port_ = port;
  manual_close_ = false;
  std::error_code ec;
  tcp::resolver resolver(ctx_->Io());
  tcp::resolver::results_type endpoints =
      resolver.resolve(host, std::to_string(port), ec);
  if (ec) {
    return false;
  }
  tcp::socket sock(ctx_->Io());
  asio::connect(sock, endpoints, ec);
  if (ec) {
    return false;
  }
  conn_ = std::make_shared<Conn>(ctx_, std::move(sock));
  conn_->SetMessageHandler([this](ConnPtr c, char *data, size_t n) {
    return on_msg_ ? on_msg_(shared_from_this(), data, n) : -1;
  });
  std::weak_ptr<Client> weak = shared_from_this();
  conn_->SetCloseHandler([weak](ConnPtr) {
    auto client = weak.lock();
    if (!client) {
      return;
    }
    bool mc = client->manual_close_;
    client->connected_ = false;
    client->conn_.reset();
    if (!mc && client->on_close_) {
      client->on_close_(client,
                        std::make_error_code(std::errc::connection_reset));
    }
  });
  conn_->Start();
  connected_ = true;
  if (on_connect_) {
    on_connect_(shared_from_this());
  }
  return connected_;
}

inline ClientPtr Context::Dial(const char *host, uint16_t port, int id) {
  auto c = Client::Create(shared_from_this(), id);
  KeepClient(id, c);
  c->OnConnect(this->OnConnect);
  c->OnMessage(this->OnMessage);
  ClientCloseHandler user_close = this->OnClose;
  std::weak_ptr<Context> weak_ctx = shared_from_this();
  c->OnClose(
      [weak_ctx, user_close, id](ClientPtr client, const std::error_code &ec) {
        if (user_close) {
          user_close(client, ec);
        }
        auto ctx = weak_ctx.lock();
        if (ctx) {
          ctx->ReleaseClient(id);
        }
      });
  if (!c->ConnectSync(host, port)) {
    ReleaseClient(id);
  }
  return c;
}

inline ServerPtr Context::Listen(uint16_t port) {
  auto s = std::make_shared<Server>(shared_from_this());
  if (!s->Listen(port)) {
    return nullptr;
  }
  return s;
}

} // namespace libtcp
