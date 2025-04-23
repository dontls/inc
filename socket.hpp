#pragma once

// This file contains the definitions for the three socket classes: Basic, Data,
// and Listening.

// ========================================================================
//  Include Files
// ========================================================================
#ifdef _WIN32 // windows 95 and above
#include "Ws2tcpip.h"
#include "windows.h" // almost everything is contained in one file.

#ifndef socklen_t
typedef int socklen_t;
#endif

#else                   // UNIX/Linux
#include <sys/types.h>  // header containing all basic data types and
// typedefs
#include <sys/socket.h> // header containing socket data types and
// functions
#include <arpa/inet.h>  // contains all inet_* functions
#include <errno.h>      // contains the error functions
#include <fcntl.h>      // file control
#include <netdb.h>      // for DNS - gethostbyname()
#include <netinet/in.h> // IPv4 and IPv6 stuff
#include <unistd.h>     // for gethostname()
#endif

#include <exception>
#include "buffer.hpp"
#include <functional>

namespace libnet {
// ========================================================================
//  Globals and Typedefs
// ========================================================================
#ifdef _WIN32            // windows 95 and above
typedef SOCKET socket_t; // Although sockets are int's on unix,
                         // windows uses it's own typedef of
                         // SOCKET to represent them. If you look
                         // in the Winsock2 source code, you'll see
                         // that it is just a typedef for int, but
                         // there is absolutely no garuntee that it
                         // won't change in a later version.
                         // therefore, in order to avoid confusion,
                         // this library will create it's own basic
                         // socket descriptor typedef

#else // UNIX/Linux
typedef int socket_t; // see the description above.
#ifndef INVALID_SOCKET
#define INVALID_SOCKET (-1)
#endif
#endif

// ========================================================================
//  Ports will be in host byte order, but IP addresses in network byte
//  order. It's easier this way; ports are usually accessed as numbers,
//  but IP addresses are better accessed through the string functions.
// ========================================================================
typedef unsigned short int port;     // define the port type.
typedef unsigned long int ipaddress; // an IP address for IPv4

// ========================================================================
// Description: Error codes for the socket library.
// ========================================================================
enum Error {
  // errors that shouldn't happen, so if they do, something is wrong:
  ESeriousError,

  // these errors are common
  ENetworkDown,
  ENoSocketsAvailable,
  ENoMemory,
  EAddressNotAvailable,
  EAlreadyConnected,
  ENotConnected,
  EConnectionRefused,
  ENetworkUnreachable,
  ENetworkReset,
  EHostUnreachable,
  EHostDown,
  EConnectionAborted,
  EConnectionReset,
  EOperationWouldBlock,

  // DNS errors
  EDNSNotFound,
  EDNSError,
  ENoDNSData,

  // These errors are specific errors that should never or rarely occur.
  EInProgress,
  EInterrupted,
  EAccessDenied,
  EInvalidParameter,
  EAddressFamilyNotSupported,
  EProtocolFamilyNotSupported,
  EProtocolNotSupported,
  EProtocolNotSupportedBySocket,
  EOperationNotSupported,
  EInvalidSocketType,
  EInvalidSocket,
  EAddressRequired,
  EMessageTooLong,
  EBadProtocolOption,
  EAddressInUse,
  ETimedOut,
  EShutDown,

  // auxilliary socketlib errors
  ESocketLimitReached,
  ENotAvailable,
  EConnectionClosed
};

// ========================================================================
// Description: This translates error codes from the native platoform
//              format into the SocketLib format
// ========================================================================

inline Error TranslateError(int p_error, bool p_errno) {
#ifdef _WIN32
  switch (p_error) {
  case WSAEINTR:
    return EInterrupted;
  case WSAEACCES:
    return EAccessDenied;
  case WSAEFAULT:
  case WSAEINVAL:
    return EInvalidParameter;
  case WSAEMFILE:
    return ENoSocketsAvailable;
  case WSAEWOULDBLOCK:
    return EOperationWouldBlock;
  case WSAEINPROGRESS:
  case WSAEALREADY:
    return EInProgress;
  case WSAENOTSOCK:
    return EInvalidSocket;
  case WSAEDESTADDRREQ:
    return EAddressRequired;
  case WSAEMSGSIZE:
    return EMessageTooLong;
  case WSAEPROTOTYPE:
    return EProtocolNotSupportedBySocket;
  case WSAENOPROTOOPT:
    return EBadProtocolOption;
  case WSAEPROTONOSUPPORT:
    return EProtocolNotSupported;
  case WSAESOCKTNOSUPPORT:
    return EInvalidSocketType;
  case WSAEOPNOTSUPP:
    return EOperationNotSupported;
  case WSAEPFNOSUPPORT:
    return EProtocolFamilyNotSupported;
  case WSAEAFNOSUPPORT:
    return EAddressFamilyNotSupported;
  case WSAEADDRINUSE:
    return EAddressInUse;
  case WSAEADDRNOTAVAIL:
    return EAddressNotAvailable;
  case WSAENETDOWN:
    return ENetworkDown;
  case WSAENETUNREACH:
    return ENetworkUnreachable;
  case WSAENETRESET:
    return ENetworkReset;
  case WSAECONNABORTED:
    return EConnectionAborted;
  case WSAECONNRESET:
    return EConnectionReset;
  case WSAENOBUFS:
    return ENoMemory;
  case WSAEISCONN:
    return EAlreadyConnected;
  case WSAENOTCONN:
    return ENotConnected;
  case WSAESHUTDOWN:
    return EShutDown;
  case WSAETIMEDOUT:
    return ETimedOut;
  case WSAECONNREFUSED:
    return EConnectionRefused;
  case WSAEHOSTDOWN:
    return EHostDown;
  case WSAEHOSTUNREACH:
    return EHostUnreachable;
  case WSAHOST_NOT_FOUND:
    return EDNSNotFound;
  case WSATRY_AGAIN:
    return EDNSError;
  case WSANO_DATA:
    return ENoDNSData;
  default:
    return ESeriousError;
  }
#else
  // for the linux version, we need to check if we're using errno
  // or h_errno. Lucky for us, both error reporting mechanisms
  // return the same values for different errors (d'oh). So,
  // the code checks the errno and the h_errno error codes in
  // different switch statements.
  if (p_errno == true) {
    switch (p_error) {
    case EINTR:
      return EInterrupted;
    case EACCES:
      return EAccessDenied;
    case EFAULT:
    case EINVAL:
      return EInvalidParameter;
    case EMFILE:
      return ENoSocketsAvailable;
    case EWOULDBLOCK:
      return EOperationWouldBlock;
    case EINPROGRESS:
    case EALREADY:
      return EInProgress;
    case ENOTSOCK:
      return EInvalidSocket;
    case EDESTADDRREQ:
      return EAddressRequired;
    case EMSGSIZE:
      return EMessageTooLong;
    case EPROTOTYPE:
      return EProtocolNotSupportedBySocket;
    case ENOPROTOOPT:
      return EBadProtocolOption;
    case EPROTONOSUPPORT:
      return EProtocolNotSupported;
    case ESOCKTNOSUPPORT:
      return EInvalidSocketType;
    case EOPNOTSUPP:
      return EOperationNotSupported;
    case EPFNOSUPPORT:
      return EProtocolFamilyNotSupported;
    case EAFNOSUPPORT:
      return EAddressFamilyNotSupported;
    case EADDRINUSE:
      return EAddressInUse;
    case EADDRNOTAVAIL:
      return EAddressNotAvailable;
    case ENETDOWN:
      return ENetworkDown;
    case ENETUNREACH:
      return ENetworkUnreachable;
    case ENETRESET:
      return ENetworkReset;
    case ECONNABORTED:
      return EConnectionAborted;
    case ECONNRESET:
      return EConnectionReset;
    case ENOBUFS:
      return ENoMemory;
    case EISCONN:
      return EAlreadyConnected;
    case ENOTCONN:
      return ENotConnected;
    case ESHUTDOWN:
      return EShutDown;
    case ETIMEDOUT:
      return ETimedOut;
    case ECONNREFUSED:
      return EConnectionRefused;
    case EHOSTDOWN:
      return EHostDown;
    case EHOSTUNREACH:
      return EHostUnreachable;
    default:
      return ESeriousError;
    }
  } else {
    switch (p_error) {
    case HOST_NOT_FOUND:
      return EDNSNotFound;
    case TRY_AGAIN:
      return EDNSError;
    case NO_DATA:
      return ENoDNSData;
    default:
      return ESeriousError;
    }
  }
#endif
}

// ========================================================================
// Description: This function acts as a simple wrapper for retrieving
//              socket library errors from errno or h_errno.
// ========================================================================
inline Error GetError(bool p_errno = true) {
#ifdef _WIN32
  return TranslateError(WSAGetLastError(), p_errno);
#else
  if (p_errno == true)
    return TranslateError(errno, p_errno);
  else
    return TranslateError(h_errno, p_errno);
#endif

  return ESeriousError;
}

// ========================================================================
// Type:        Exception
// Purpose:     A generic socket exception class that holds an error, and
//              an optional text string describing the error in more detail
// ========================================================================
class Exception : public std::exception {
public:
  // ====================================================================
  // Function:    Exception
  // Purpose:     To initialize the socket exception with a specific
  //              error code.
  // ====================================================================
  Exception(Error p_code) { m_code = p_code; }

  // ====================================================================
  // Function:    Error
  // Purpose:     To retrieve the error code of the socket.
  // ====================================================================
  Error ErrorCode() { return m_code; }

  // ====================================================================
  // Function:    PrintError
  // Purpose:     Print the error message to a string
  // ====================================================================
  const char *PrintError() {
    switch (m_code) {
    case EOperationWouldBlock:
      return "Nonblocking socket operation would have blocked";
    case EInProgress:
      return "This operation is already in progress";
    case EInvalidSocket:
      return "The socket was not valid";
    case EAddressRequired:
      return "A destination address is required";
    case EMessageTooLong:
      return "The message was too long";
    case EProtocolNotSupported:
      return "The protocol is not supported";
    case EProtocolNotSupportedBySocket:
      return "The socket type is not supported";
    case EOperationNotSupported:
      return "The operation is not supported";
    case EProtocolFamilyNotSupported:
      return "The protocol family is not supported";
    case EAddressFamilyNotSupported:
      return "The operation is not supported by the address family";
    case EAddressInUse:
      return "The address is already in use";
    case EAddressNotAvailable:
      return "The address is not available to use";
    case ENetworkDown:
      return "The network is down";
    case ENetworkUnreachable:
      return "The destination network is unreachable";
    case ENetworkReset:
      return "The network connection has been reset";
    case EConnectionAborted:
      return "The network connection has been aborted due to software error";
    case EConnectionReset:
      return "Connection has been closed by the other side";
    case ENoMemory:
      return "There was insufficient system memory to complete the operation";
    case EAlreadyConnected:
      return "The socket is already connected";
    case ENotConnected:
      return "The socket is not connected";
    case EShutDown:
      return "The socket has already been shut down";
    case ETimedOut:
      return "The connection timed out";
    case EConnectionRefused:
      return "The connection was refused";
    case EHostDown:
      return "The host is down";
    case EHostUnreachable:
      return "The host is unreachable";
    case EDNSNotFound:
      return "DNS lookup is not found";
    case EDNSError:
      return "Host not found due to error; try again";
    case ENoDNSData:
      return "Address found, but has no valid data";
    case EInterrupted:
      return "Operation was interrupted";
    case ENoSocketsAvailable:
      return "No more sockets available";
    case EInvalidParameter:
      return "Operation has an invalid parameter";
    case EInvalidSocketType:
      return "Socket type is invalid";
    case EAccessDenied:
      return "Access to this operation was denied";
    case ESocketLimitReached:
      return "The manager has reached its maximum number of sockets";
    default:
      return "undefined or serious error";
    }
  }

protected:
  Error m_code;
};

#ifdef _WIN32
class WSInit {
public:
  WSInit() {
    WSADATA wsaData;
    if (WSAStartup(0x0002, &wsaData) == 0)
      is_valid_ = true;
  }

  ~WSInit() {
    if (is_valid_)
      WSACleanup();
  }

  bool is_valid_ = false;
};

static WSInit wstup_;
#endif

// ========================================================================
// Class:       Socket
// Purpose:     A very basic socket base class that will give the user
//              the ability to get port and IP information, but not much
//              else.
// ========================================================================
class Socket {
public:
  void Create() {
    if (m_sock == INVALID_SOCKET) {
      m_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    }
  }

  // ====================================================================
  // Function:    GetSock
  // Purpose:     this function returns the internal API socket
  //              representation. Used by classes and functions that need
  //              to interface directly with the BSD Sockets or Winsock
  //              libraries.
  // ====================================================================
  socket_t GetSock() const { return m_sock; }

  // ====================================================================
  // Function:    GetLocalPort
  // Purpose:     gets the local port of the socket
  // ====================================================================
  port GetLocalPort() const { return ntohs(m_localinfo.sin_port); }

  // ====================================================================
  // Function:    GetLocalAddress
  // Purpose:     gets the local address of the socket
  // ====================================================================
  ipaddress GetLocalAddress() const { return m_localinfo.sin_addr.s_addr; }

  // ====================================================================
  // Function:    Close
  // Purpose:     closes the socket.
  // ====================================================================
  void Close() {
// WinSock uses "closesocket" instead of "close", since it treats
// sockets as completely separate objects to files, whereas unix
// treats files and sockets exactly the same.
#ifdef _WIN32
    closesocket(m_sock);
#else
    close(m_sock);
#endif
    // invalidate the socket
    m_sock = -1;
  }

  // ====================================================================
  // Function:    SetBlocking
  // Purpose:     sets whether the socket is blocking or not.
  // ====================================================================
  void SetBlocking(bool p_blockmode) {
    int err;

#ifdef _WIN32
    unsigned long mode = !p_blockmode;
    err = ioctlsocket(m_sock, FIONBIO, &mode);
#else
    // get the flags
    int flags = fcntl(m_sock, F_GETFL, 0);

    // set or clear the non-blocking flag
    if (p_blockmode == false) {
      flags |= O_NONBLOCK;
    } else {
      flags &= ~O_NONBLOCK;
    }
    err = fcntl(m_sock, F_SETFL, flags);
#endif

    if (err == -1) {
      throw(Exception(GetError()));
    }

    m_isblocking = p_blockmode;
  }

protected:
  // ====================================================================
  // Function:    Socket
  // Purpose:     hidden constructor, meant to prevent people from
  //              instantiating this class. You should be using direct
  //              implementations of this class instead, such as
  //              Server and Conn.
  // ====================================================================
  Socket(socket_t p_socket = -1);

  socket_t m_sock; // this is the underlying representation
                   // of the actual socket.

  struct sockaddr_in m_localinfo; // structure containing information
                                  // about the local connection

  bool m_isblocking; // this tells whether the socket is
                     // blocking or not.
};

inline Socket::Socket(socket_t p_socket)
    : m_sock(p_socket), m_localinfo{}, m_isblocking(false) {
  if (p_socket != INVALID_SOCKET) {
    socklen_t s = sizeof(m_localinfo);
    getsockname(p_socket, (sockaddr *)(&m_localinfo), &s);
  }

  // the socket is blocking by default
  m_isblocking = true;
}

// ========================================================================
// Class:       TcpConn
// Purpose:     A variation of the BasicSocket base class that handles
//              TCP/IP data connections.
// ========================================================================
class TcpConn : public Socket {
public:
  // ====================================================================
  // Function:    TcpConn
  // Purpose:     Constructs the data socket with optional values
  // ====================================================================
  TcpConn(socket_t p_socket = INVALID_SOCKET);

  // ====================================================================
  // Function:    GetRemoteAddress
  // Purpose:     get the IP address of the remote host.
  // ====================================================================
  ipaddress GetRemoteAddress() const { return m_remoteinfo.sin_addr.s_addr; }

  // ====================================================================
  // Function:    GetRemotePort
  // Purpose:     gets the remote port of the socket
  // ====================================================================
  port GetRemotePort() const { return ntohs(m_remoteinfo.sin_port); }

  // ====================================================================
  // Function:    IsConnected
  // Purpose:     Determines if the socket is connected or not.
  // ====================================================================
  bool IsConnected() const { return m_connected; }

  // ====================================================================
  // Function:    Connect
  // Purpose:     Connects this socket to another socket. This will fail
  //              if the socket is already connected, or the server
  //              rejects the connection.
  // ====================================================================
  void Dial(const char *p_addr, port p_port);

  // ====================================================================
  // Function:    Send
  // Purpose:     Attempts to send data, and returns the number of
  //              of bytes sent
  // ====================================================================
  int Write(const char *p_buffer, int p_size, long timeout = 0);

  // ====================================================================
  // Function:    Receive
  // Purpose:     Attempts to receive data from a socket, and returns the
  //              amount of data received.
  // ====================================================================
  int Read(char *p_buffer, int p_size, long timeout = 0);

  // ====================================================================
  // Function:    Close
  // Purpose:     closes the socket.
  // ====================================================================
  void Close();

  // ====================================================================
  // Function:    LoopRead
  // Purpose:     LoopRead with protocol handler, return protocol length
  // ====================================================================
  void LoopRead(std::function<int(libyte::Buffer &)> handler, int timeout = 0);

protected:
  bool m_connected; // is the socket connected?

  struct sockaddr_in m_remoteinfo; // structure containing information
                                   // about the remote connection

private:
  int setDealTimeout(long timeout, bool isread) {
    int err = -1;
    struct timeval tout;
    tout.tv_sec = timeout / 1000;
    tout.tv_usec = (timeout % 1000) * 1000;
    int nsocket = int(m_sock);
#ifdef __linux__
    nsocket += 1;
#endif // __linux__
    fd_set fd{};
    fd_set *fdr = NULL, *fdw = NULL;
    isread ? fdr = &fd : fdw = &fd;
    while (1) {
      FD_ZERO(&fd);
      FD_SET(m_sock, &fd);
      int rc = select(nsocket, fdr, fdw, NULL, &tout);
      if (rc == -1) {
        if (EINTR == errno || EAGAIN == errno) {
          continue;
        }
      }
      if (FD_ISSET(m_sock, &fd)) {
        err = 0;
      }
      break;
    }
    return err;
  }
};

inline TcpConn::TcpConn(socket_t p_socket)
    : Socket(p_socket), m_connected(false), m_remoteinfo{} {
  if (p_socket != INVALID_SOCKET) {
    socklen_t s = sizeof(m_remoteinfo);
    getpeername(p_socket, (sockaddr *)(&m_remoteinfo), &s);
    m_connected = true;
  }
}
// ====================================================================
// Function:    Connect
// Purpose:     Connects this socket to another socket. This will fail
//              if the socket is already connected, or the server
//              rejects the connection.
// ====================================================================
inline void TcpConn::Dial(const char *p_addr, port p_port) {
  int err;

  // if the socket is already connected...
  if (m_connected == true) {
    throw Exception(EAlreadyConnected);
  }

  // first try to obtain a socket descriptor from the OS, if
  // there isn't already one.
  Socket::Create();
  // throw an exception if the socket could not be created
  if (m_sock == INVALID_SOCKET) {
    throw Exception(GetError());
  }
  // set up the socket address structure
  m_remoteinfo.sin_family = AF_INET;
  m_remoteinfo.sin_port = htons(p_port);
  m_remoteinfo.sin_addr.s_addr = inet_addr(p_addr);
  memset(&(m_remoteinfo.sin_zero), 0, 8);

  // now the socket is created, so connect it.
  socklen_t s = sizeof(struct sockaddr);
  err = connect(m_sock, (struct sockaddr *)(&m_remoteinfo), s);
  if (err == -1) {
    throw Exception(GetError());
  }

  m_connected = true;

  // to get the local port, you need to do a little more work
  err = getsockname(m_sock, (struct sockaddr *)(&m_localinfo), &s);
  if (err != 0) {
    throw Exception(GetError());
  }
}

// ====================================================================
// Function:    Send
// Purpose:     Attempts to send data, and returns the number of
//              of bytes sent
// ====================================================================
inline int TcpConn::Write(const char *p_buffer, int p_size, long timeout) {

  // make sure the socket is connected first.
  if (m_connected == false) {
    throw Exception(ENotConnected);
  }

  if (timeout > 0 && setDealTimeout(timeout, false) < 0) {
    throw Exception(ETimedOut);
  }
  // attempt to send the data
  int err = send(m_sock, p_buffer, p_size, 0);
  if (err == -1) {
    Error e = GetError();
    if (e != EOperationWouldBlock) {
      throw Exception(e);
    }

    // if the socket is nonblocking, we don't want to send a terminal
    // error, so just set the number of bytes sent to 0, assuming
    // that the Conn will be able to handle that.
    err = 0;
  }

  // return the number of bytes successfully sent
  return err;
}

// ====================================================================
// Function:    Receive
// Purpose:     Attempts to recieve data from a socket, and returns the
//              amount of data received.
// ====================================================================
inline int TcpConn::Read(char *p_buffer, int p_size, long timeout) {
  // make sure the socket is connected first.
  if (m_connected == false) {
    throw Exception(ENotConnected);
  }
  if (timeout > 0 && setDealTimeout(timeout, true) < 0) {
    throw Exception(ETimedOut);
  }
  // attempt to recieve the data
  int err = recv(m_sock, p_buffer, p_size, 0);
  if (err == 0) {
    throw Exception(EConnectionClosed);
  }
  if (err == -1) {
    throw Exception(GetError());
  }

  // return the number of bytes successfully recieved
  return err;
}

// ====================================================================
// Function:    Close
// Purpose:     closes the socket.
// ====================================================================
inline void TcpConn::Close() {
  if (m_connected == true) {
    shutdown(m_sock, 2);
  }

  // close the socket
  Socket::Close();

  m_connected = false;
}

// ====================================================================
// Function:    LoopRead
// Purpose:     LoopRead with protocol handler
// ====================================================================
inline void TcpConn::LoopRead(std::function<int(libyte::Buffer &)> handler,
                              int timeout) {
  libyte::Buffer rbuf;
  for (;;) {
    char buf[2048] = {0};
    int n = Read(buf, 2048, timeout);
    rbuf.Write(buf, n);
    for (;;) {
      int l = handler(rbuf);
      if (l < 0) {
        return;
      }
      if (l == 0) {
        break;
      }
      rbuf.Remove(l);
    }
  }
}

// ========================================================================
// Class:       Server
// Purpose:     A variation of the BasicSocket base class that handles
//              incomming TCP/IP connection requests.
// ========================================================================
class TcpServer : public Socket {
public:
  // ====================================================================
  // Function:    Server
  // Purpose:     Constructor. Constructs the socket with initial values
  // ====================================================================
  TcpServer() { m_listening = false; }

  // ====================================================================
  // Function:    Listen
  // Purpose:     this function will tell the socket to listen on a
  //              certain port
  // p_port:      This is the port that the socket will listen on.
  // ====================================================================
  void Listen(port p_port);

  // ====================================================================
  // Function:    Accept
  // Purpose:     This is a blocking function that will accept an
  //              incomming connection and return a data socket with info
  //              about the new connection.
  // ====================================================================
  TcpConn Accept() {
    struct sockaddr_in socketaddress;

    // try to accept a connection
    socklen_t size = sizeof(struct sockaddr);
    socket_t s = accept(m_sock, (struct sockaddr *)&socketaddress, &size);
    if (s == INVALID_SOCKET) {
      throw Exception(GetError());
    }

    // return the newly created socket.
    return TcpConn(s);
  }

  // ====================================================================
  // Function:    IsListening
  // Purpose:     Determines if the socket is listening or not.
  // ====================================================================
  bool IsListening() const { return m_listening; }

  // ====================================================================
  // Function:    Close
  // Purpose:     closes the socket.
  // ====================================================================
  void Close() {
    // close the socket
    Socket::Close();

    // invalidate the variables
    m_listening = false;
  }

protected:
  bool m_listening; // is the socket listening?
};

// ====================================================================
// Function:    Listen
// Purpose:     this function will tell the socket to listen on a
//              certain port
// p_port:      This is the port that the socket will listen on.
// ====================================================================
inline void TcpServer::Listen(port p_port) {
  int err;

  // first try to obtain a socket descriptor from the OS, if
  // there isn't already one.
  Socket::Create();
  // throw an exception if the socket could not be created
  if (m_sock == INVALID_SOCKET) {
    throw Exception(GetError());
  }

  // set the SO_REUSEADDR option on the socket, so that it doesn't
  // hog the port after it closes.
  int reuse = 1;
  err = setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (char *)(&reuse),
                   sizeof(reuse));
  if (err != 0) {
    throw Exception(GetError());
  }

  // set up the socket address structure
  m_localinfo.sin_family = AF_INET;
  m_localinfo.sin_port = htons(p_port);
  m_localinfo.sin_addr.s_addr = htonl(INADDR_ANY);
  memset(&(m_localinfo.sin_zero), 0, 8);

  // bind the socket
  err = bind(m_sock, (struct sockaddr *)&m_localinfo, sizeof(struct sockaddr));
  if (err == -1) {
    throw Exception(GetError());
  }

  // now listen on the socket. There is a very high chance that this will
  // be successful if it got to this point, but always check for errors
  // anyway. Set the queue to 8; a reasonable number.
  err = listen(m_sock, 8);
  if (err == -1) {
    throw Exception(GetError());
  }

  m_listening = true;
}

} // namespace libnet
