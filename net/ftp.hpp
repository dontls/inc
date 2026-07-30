#pragma once

#define NOLFS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#ifdef NOLFS
#define off64_t long
#endif

#define SETSOCKOPT_OPTVAL_TYPE (void *)

#define net_read read
#define net_write write
#define net_close close

using namespace std;

/* socket values */
#define FTPLIB_BUFSIZ 4 * 1024 // 1024
#define ACCEPT_TIMEOUT 30

/* io types */
#define FTPLIB_CONTROL 0
#define FTPLIB_READ 1
#define FTPLIB_WRITE 2

namespace libftp {

typedef int (*FtpCallbackXfer)(off64_t xfered, void *arg);
typedef int (*FtpCallbackIdle)(void *arg);
typedef void (*FtpCallbackLog)(char *str, void *arg, bool out);

struct context {
  char *cput, *cget;
  int handle;
  int cavail, cleft;
  char *buf;
  int dir;
  context *ctrl;
  int cmode;
  struct timeval idletime;
  FtpCallbackXfer xfercb;
  FtpCallbackIdle idlecb;
  FtpCallbackLog logcb;
  void *cbarg;
  off64_t xfered;
  off64_t cbbytes;
  off64_t xfered1;
  char response[1024];
  off64_t offset;
  bool correctpasv;
};

class Client {
public:
  enum accesstype {
    dir = 1,        //
    dirverbose,     //
    fileread,       //
    filewrite,      //
    filereadappend, //
    filewriteappend
  };

  enum transfermode {
    ascii = 'A', //
    image = 'I'
  };

  enum connmode {
    pasv = 1, //
    port
  };

  enum fxpmethod {
    defaultfxp = 0, //
    alternativefxp
  };

  enum dataencryption {
    unencrypted = 0, //
    secure
  };

  Client();
  ~Client();
  int Connect(const char *host);
  int Login(const char *user, const char *pass);
  int Mkdir(const char *path);
  int Size(const char *remotefile, int *size, transfermode mode);
  int Get(const char *outputfile, const char *path, transfermode mode,
          off64_t offset = 0);
  int Put(const char *localfile, const char *remotepath, transfermode mode,
          off64_t offset = 0);
  int Quit();
  // ½ø¶È»Øµ÷
  void SetCallbackXferFunction(FtpCallbackXfer pointer);
  void SetCallbackLogFunction(FtpCallbackLog pointer);
  void SetCallbackIdletime(int time);
  void SetConnmode(connmode mode);

private:
  char *LastResponse();
  int FtpXfer(const char *localfile, const char *path, context *nControl,
              accesstype type, transfermode mode);
  int FtpOpenPasv(context *nControl, context **nData, transfermode mode,
                  int dir, char *cmd);
  int FtpSendCmd(const char *cmd, char expresp, context *nControl);
  int FtpRead(void *buf, int max, context *nData);
  int FtpWrite(void *buf, int len, context *nData);
  int FtpAccess(const char *path, accesstype type, transfermode mode,
                context *nControl, context **nData);
  int FtpClose(context *nData);
  void ClearHandle();
  int CorrectPasvResponse(unsigned int *v);
  int AsynConnect(int sock, struct sockaddr *sockaddr, int namelen,
                  int tm_us = 30 * 1000 * 1000);

private:
  int socket_wait(context *ctl);
  int socket_wait_Control(context *ctl, int timeLen_us = 5 * 1000 * 1000);
  int readline(char *buf, int max, context *ctl);
  int writeline(char *buf, int len, context *nData);
  int readresp(char c, context *nControl);

private:
  context *context_;
  char ip_[128];
};
/*
 * Constructor
 */

inline Client::Client() {
  memset(ip_, 0, sizeof(ip_));

  context_ = static_cast<context *>(calloc(1, sizeof(context)));
  if (context_ == NULL) {
    perror("calloc");
  }
  context_->buf = static_cast<char *>(malloc(FTPLIB_BUFSIZ));
  if (context_->buf == NULL) {
    perror("calloc");
    free(context_);
  }

  ClearHandle();
}

/*
 * Destructor
 */

inline Client::~Client() {
  free(context_->buf);
  free(context_);
}

/*
 * socket_wait - wait for socket to receive or flush data
 *
 * return 1 if no user callback, otherwise, return value returned by
 * user callback
 */
inline int Client::socket_wait(context *ctl) {
  if (ctl->dir == FTPLIB_CONTROL) {
    return socket_wait_Control(ctl);
  }

  fd_set fd, *rfd = NULL, *wfd = NULL;
  struct timeval tv;
  int rv = 0;

  if (ctl->idlecb == NULL) {
    return 1;
  }

  if (ctl->dir == FTPLIB_WRITE) {
    wfd = &fd;
  } else {
    rfd = &fd;
  }

  FD_ZERO(&fd);
  do {
    FD_SET(ctl->handle, &fd);
    tv = ctl->idletime;
    rv = select(ctl->handle + 1, rfd, wfd, NULL, &tv);
    if (rv == -1) {
      rv = 0;
      strncpy(ctl->ctrl->response, strerror(errno),
              sizeof(ctl->ctrl->response));
      break;
    } else if (rv > 0) {
      rv = 1;
      break;
    }
  } while ((rv = ctl->idlecb(ctl->cbarg)));

  return rv;
}

inline int Client::socket_wait_Control(context *ctl, int timeLen_us) {
  fd_set fd;
  struct timeval tv;
  tv.tv_sec = timeLen_us / (1000 * 1000);
  tv.tv_usec = timeLen_us % (1000 * 1000);

  FD_ZERO(&fd);
  FD_SET(ctl->handle, &fd);
  int rv = select(ctl->handle + 1, &fd, NULL, NULL, &tv);

  return rv > 0 ? 1 : 0;
}

/*
 * read a line of text
 *
 * return -1 on error or bytecount
 */
inline int Client::readline(char *buf, int max, context *ctl) {
  int x, retval = 0;
  char *end, *bp = buf;
  int eof = 0;

  if ((ctl->dir != FTPLIB_CONTROL) && (ctl->dir != FTPLIB_READ)) {
    return -1;
  }

  if (max == 0) {
    return 0;
  }

  do {
    if (ctl->cavail > 0) {
      x = (max >= ctl->cavail) ? ctl->cavail : max - 1;
      end = static_cast<char *>(memccpy(bp, ctl->cget, '\n', x));
      if (end != NULL) {
        x = end - bp;
      }
      retval += x;
      bp += x;
      *bp = '\0';
      max -= x;
      ctl->cget += x;
      ctl->cavail -= x;
      if (end != NULL) {
        bp -= 2;
        if (strcmp(bp, "\r\n") == 0) {
          *bp++ = '\n';
          *bp++ = '\0';
          --retval;
        }
        break;
      }
    }
    if (max == 1) {
      *buf = '\0';
      break;
    }
    if (ctl->cput == ctl->cget) {
      ctl->cput = ctl->cget = ctl->buf;
      ctl->cavail = 0;
      ctl->cleft = FTPLIB_BUFSIZ;
    }
    if (eof) {
      if (retval == 0) {
        retval = -1;
      }
      break;
    }

    if (!socket_wait(ctl)) {
      return retval;
    }

    x = net_read(ctl->handle, ctl->cput, ctl->cleft);

    if (x == -1) {
      perror("read");
      retval = -1;
      break;
    }

    // LOGGING FUNCTIONALITY!!!

    if ((ctl->dir == FTPLIB_CONTROL) && (context_->logcb != NULL)) {
      *((ctl->cput) + x) = '\0';
      context_->logcb(ctl->cput, context_->cbarg, true);
    }

    if (x == 0) {
      eof = 1;
    }
    ctl->cleft -= x;
    ctl->cavail += x;
    ctl->cput += x;
  } while (1);

  return retval;
}

/*
 * write lines of text
 *
 * return -1 on error or bytecount
 */
inline int Client::writeline(char *buf, int len, context *nData) {
  int x, nb = 0, w;
  char *ubp = buf, *nbp;
  char lc = 0;

  if (nData->dir != FTPLIB_WRITE) {
    return -1;
  }
  nbp = nData->buf;
  for (x = 0; x < len; x++) {
    if ((*ubp == '\n') && (lc != '\r')) {
      if (nb == FTPLIB_BUFSIZ) {
        if (!socket_wait(nData)) {
          return x;
        }

        w = net_write(nData->handle, nbp, FTPLIB_BUFSIZ);

        if (w != FTPLIB_BUFSIZ) {
          printf("write(1) returned %d, errno = %d\n", w, errno);
          return (-1);
        }
        nb = 0;
      }
      nbp[nb++] = '\r';
    }
    if (nb == FTPLIB_BUFSIZ) {
      if (!socket_wait(nData)) {
        return x;
      }

      w = net_write(nData->handle, nbp, FTPLIB_BUFSIZ);

      if (w != FTPLIB_BUFSIZ) {
        printf("write(2) returned %d, errno = %d\n", w, errno);
        return (-1);
      }
      nb = 0;
    }
    nbp[nb++] = lc = *ubp++;
  }
  if (nb) {
    if (!socket_wait(nData)) {
      return x;
    }

    w = net_write(nData->handle, nbp, nb);

    if (w != nb) {
      printf("write(3) returned %d, errno = %d\n", w, errno);
      return (-1);
    }
  }
  return len;
}

/*
 * read a response from the server
 *
 * return 0 if first char doesn't match
 * return 1 if first char matches
 */
inline int Client::readresp(char c, context *nControl) {
  char match[5];

  if (readline(nControl->response, 256, nControl) == -1) {
    perror("Control socket read failed");
    return 0;
  }

  if (nControl->response[3] == '-') {
    strncpy(match, nControl->response, 3);
    match[3] = ' ';
    match[4] = '\0';
    do {
      if (readline(nControl->response, 256, nControl) == -1) {
        perror("Control socket read failed");
        return 0;
      }
    } while (strncmp(nControl->response, match, 4));
  }

  if (nControl->response[0] == c) {
    return 1;
  }

  return 0;
}

/*
 * FtpLastResponse - return a pointer to the last response received
 */
inline char *Client::LastResponse() {
  if ((context_) && (context_->dir == FTPLIB_CONTROL)) {
    return context_->response;
  }
  return NULL;
}

/*
 * Client::Connect - connect to remote server
 *
 * return 1 if connected, 0 if not
 */
inline int Client::Connect(const char *host) {
  struct sockaddr_in sin;
  struct servent *pse;
  linger linger_std;
  int one = 1, ret = 0, Ret = 0;
  int iSetOptReturnCode = 0;
  char *lhost = NULL, *pnum;
  context_->dir = FTPLIB_CONTROL;
  context_->ctrl = NULL;
  context_->xfered = 0;
  context_->xfered1 = 0;
  context_->offset = 0;
  context_->handle = 0;

  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  lhost = strdup(host);
  if (NULL == lhost) {
    perror(" NULL == lhost ");
    return 0;
  }

  pnum = strchr(lhost, ':');
  if (pnum == NULL) {
    if ((pse = getservbyname("ftp", "tcp")) == NULL) {
      perror("getservbyname");
      goto Done;
    }
    sin.sin_port = pse->s_port;
  } else {
    *pnum++ = '\0';
    if (isdigit(*pnum)) {
      sin.sin_port = htons(atoi(pnum));
    } else {
      pse = getservbyname(pnum, "tcp");
      sin.sin_port = pse->s_port;
    }
  }

  ret = inet_aton(lhost, &sin.sin_addr);
  if (ret == 0) {
    char *p = NULL;
    struct hostent hostbuf;
    struct hostent *hp = NULL;
    size_t hstbuflen = 1024;
    char tmphstbuf[1024] = {0};
    int herr = 0;
    if (gethostbyname_r(lhost, &hostbuf, tmphstbuf, hstbuflen, &hp, &herr) ||
        hp == NULL) {
      goto Done;
    }

    sin.sin_addr.s_addr = *((unsigned int *)hp->h_addr_list[0]);
    if ((p = inet_ntoa(sin.sin_addr)) != NULL) {
      strcpy(ip_, p);
    } else {
      goto Done;
    }
  } else {
    strcpy(ip_, lhost);
  }

  context_->handle = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

  if (context_->handle == -1) {
    perror("socket");
    goto Done;
  }

  linger_std.l_onoff = 0;
  linger_std.l_linger = 0;
  iSetOptReturnCode =
      setsockopt(context_->handle, SOL_SOCKET, SO_LINGER,
                 SETSOCKOPT_OPTVAL_TYPE & linger_std, sizeof(linger));
  if (iSetOptReturnCode == -1) {
    perror("setsockopt:SO_LINGER");
    goto Done;
  }

  iSetOptReturnCode = ::setsockopt(context_->handle, SOL_SOCKET, SO_KEEPALIVE,
                                   (char *)&one, sizeof(int));
  if (iSetOptReturnCode == -1) {
    perror("setsockopt:SO_KEEPALIVE");
    goto Done;
  }

  if (AsynConnect(context_->handle, (struct sockaddr *)&sin, sizeof(sin)) !=
      1) {
    perror("AsynConnectconnect");
    goto Done;
  }

  if (readresp('2', context_) == 0) {
    perror("readresp");
    goto Done;
  }

  Ret = 1;

Done:
  if (lhost)
    free(lhost);
  if (Ret == 0) {
    if (context_->handle > 0) {
      net_close(context_->handle);
      context_->handle = 0;
    }
  }
  return Ret;
}

/*
 * FtpSendCmd - send a command and wait for expected response
 *
 * return 1 if proper response received, 0 otherwise
 */
inline int Client::FtpSendCmd(const char *cmd, char expresp,
                              context *nControl) {

  char buf[256];
  int x;

  if (!nControl->handle) {
    return 0;
  }

  if (nControl->dir != FTPLIB_CONTROL) {
    return 0;
  }
  sprintf(buf, "%s\r\n", cmd);

  x = net_write(nControl->handle, buf, strlen(buf));

  if (x <= 0) {
    perror("write");
    return 0;
  }

  if (context_->logcb != NULL) {
    context_->logcb(buf, context_->cbarg, false);
  }

  return readresp(expresp, nControl);
}

/*
 * FtpLogin - log in to remote server
 *
 * return 1 if logged in, 0 otherwise
 */
inline int Client::Login(const char *user, const char *pass) {
  char tempbuf[64];

  if (((strlen(user) + 7) > sizeof(tempbuf)) ||
      ((strlen(pass) + 7) > sizeof(tempbuf))) {
    return 0;
  }
  sprintf(tempbuf, "USER %s", user);
  if (!FtpSendCmd(tempbuf, '3', context_)) {
    if (context_->ctrl != NULL) {
      return 1;
    }
    if (*LastResponse() == '2') {
      return 1;
    }
    return 0;
  }
  sprintf(tempbuf, "PASS %s", pass);

  return FtpSendCmd(tempbuf, '2', context_);
}

/*
 * FtpAccess - return a handle for a data stream
 *
 * return 1 if successful, 0 otherwise
 */
inline int Client::FtpAccess(const char *path, accesstype type,
                             transfermode mode, context *nControl,
                             context **nData) {
  char buf[256];
  int dir, ret;
  ret = 0;

  if ((path == NULL) &&
      ((type == Client::filewrite) || (type == Client::fileread) ||
       (type == Client::filereadappend) || (type == Client::filewriteappend))) {
    sprintf(nControl->response, "Missing path argument for file transfer\n");
    return 0;
  }
  sprintf(buf, "TYPE %c", mode);
  if (!FtpSendCmd(buf, '2', nControl)) {
    return 0;
  }

  switch (type) {
  case Client::dir:
    strcpy(buf, "NLST");
    dir = FTPLIB_READ;
    break;
  case Client::dirverbose:
    strcpy(buf, "LIST -aL");
    dir = FTPLIB_READ;
    break;
  case Client::filereadappend:
  case Client::fileread:
    strcpy(buf, "RETR");
    dir = FTPLIB_READ;
    break;
  case Client::filewriteappend:
  case Client::filewrite:
    strcpy(buf, "STOR");
    dir = FTPLIB_WRITE;
    break;
  default:
    sprintf(nControl->response, "Invalid open type %d\n", type);
    return 0;
  }
  if (path != NULL) {
    int i = strlen(buf);
    buf[i++] = ' ';
    if ((strlen(path) + i) >= sizeof(buf)) {
      return 0;
    }
    strcpy(&buf[i], path);
  }

  if (nControl->cmode == Client::pasv) {
    if (FtpOpenPasv(nControl, nData, mode, dir, buf) == -1) {
      return 0;
    }
  }

  if (nControl->cmode == Client::port) {
    return 0;
  }

  return 1;
}

/*
 * FtpOpenPasv - Establishes a PASV connection for data transfer
 *
 * return 1 if successful, -1 otherwise
 */
inline int Client::FtpOpenPasv(context *nControl, context **nData,
                               transfermode mode, int dir, char *cmd) {
  int sData;
  union {
    struct sockaddr sa;
    struct sockaddr_in in;
  } sin;

  struct linger lng = {0, 0};
  unsigned int l;
  int on = 1;
  context *ctrl = NULL;
  char *cp;
  unsigned int v[6];
  int ret, Ret = -1;

  if (nControl->dir != FTPLIB_CONTROL) {
    return -1;
  }
  if ((dir != FTPLIB_READ) && (dir != FTPLIB_WRITE)) {
    sprintf(nControl->response, "Invalid direction %d\n", dir);
    return -1;
  }
  if ((mode != Client::ascii) && (mode != Client::image)) {
    sprintf(nControl->response, "Invalid mode %c\n", mode);
    return -1;
  }

  l = sizeof(sin);
  memset(&sin, 0, l);
  sin.in.sin_family = AF_INET;

  if (!FtpSendCmd("PASV", '2', nControl)) {
    return -1;
  }
  cp = strchr(nControl->response, '(');
  if (cp == NULL) {
    return -1;
  }
  cp++;
  sscanf(cp, "%u,%u,%u,%u,%u,%u", &v[2], &v[3], &v[4], &v[5], &v[0], &v[1]);
  if (nControl->correctpasv)
    if (!CorrectPasvResponse(v)) {
      return -1;
    }
  printf("pasv ip:%d.%d.%d.%d %d %d\n", v[2], v[3], v[4], v[5], v[0], v[1]);

  if (strlen(ip_) > 0) {
    sscanf(ip_, "%u.%u.%u.%u", &v[2], &v[3], &v[4], &v[5]);
    // printf("------ip_:%s, pasv ip:%d %d %d %d %d %d\n", ip_, v[2],
    // v[3], v[4], v[5], v[0], v[1]);
  }

  sin.sa.sa_data[2] = v[2];
  sin.sa.sa_data[3] = v[3];
  sin.sa.sa_data[4] = v[4];
  sin.sa.sa_data[5] = v[5];

  sin.sa.sa_data[0] = v[0];
  sin.sa.sa_data[1] = v[1];

  if (context_->offset != 0) {
    char buf[256];
    sprintf(buf, "REST %ld", context_->offset);
    if (!FtpSendCmd(buf, '3', nControl)) {
      return 0;
    }
  }

  sData = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sData == -1) {
    perror("socket");
    goto Done;
  }

  if (setsockopt(sData, SOL_SOCKET, SO_REUSEADDR, SETSOCKOPT_OPTVAL_TYPE & on,
                 sizeof(on)) == -1) {
    perror("setsockopt");
    goto Done;
  }

  if (setsockopt(sData, SOL_SOCKET, SO_LINGER, SETSOCKOPT_OPTVAL_TYPE & lng,
                 sizeof(lng)) == -1) {
    perror("setsockopt");
    goto Done;
  }

  if (nControl->dir != FTPLIB_CONTROL) {
    perror(" nControl->dir != FTPLIB_CONTROL ");
    goto Done;
  }
  sprintf(cmd, "%s\r\n", cmd);

  ret = net_write(nControl->handle, cmd, strlen(cmd));

  if (ret <= 0) {
    perror("write");
    goto Done;
  }

  if (context_->logcb != NULL) {
    context_->logcb(cmd, context_->cbarg, false);
  }

  if (AsynConnect(sData, &sin.sa, sizeof(sin.sa)) != 1) {
    perror("connect");
    goto Done;
  }
  if (!readresp('1', nControl)) {
    goto Done;
  }
  ctrl = static_cast<context *>(calloc(1, sizeof(context)));
  if (ctrl == NULL) {
    perror("calloc");
    goto Done;
  }

  if ((mode == 'A') &&
      ((ctrl->buf = static_cast<char *>(malloc(FTPLIB_BUFSIZ))) == NULL)) {
    perror("calloc");
    goto Done;
  }

  ctrl->handle = sData;
  ctrl->dir = dir;
  ctrl->ctrl = (nControl->cmode == Client::pasv) ? nControl : NULL;
  ctrl->idletime = nControl->idletime;
  ctrl->cbarg = nControl->cbarg;
  ctrl->xfered = 0;
  ctrl->xfered1 = 0;
  ctrl->cbbytes = nControl->cbbytes;

  if (ctrl->idletime.tv_sec || ctrl->idletime.tv_usec) {
    ctrl->idlecb = nControl->idlecb;
  } else {
    ctrl->idlecb = NULL;
  }

  if (ctrl->cbbytes) {
    ctrl->xfercb = nControl->xfercb;
  } else {
    ctrl->xfercb = NULL;
  }

  ret = 1;
  *nData = ctrl;
Done:
  if (ret != 1) {
    if (sData > 0)
      net_close(sData);
    if (ctrl)
      free(ctrl);
  }
  return ret;
}

/*
 * FtpClose - close a data connection
 */
inline int Client::FtpClose(context *nData) {
  context *ctrl;
  if (NULL == nData) {
    return 1;
  }

  if (nData->dir == FTPLIB_WRITE) {
    if (nData->buf != NULL) {
      writeline(NULL, 0, nData);
    }
  } else if (nData->dir != FTPLIB_READ) {
    return 0;
  }

  if (nData->buf) {
    free(nData->buf);
  }
  shutdown(nData->handle, 2);
  net_close(nData->handle);

  ctrl = nData->ctrl;

  if (NULL != nData) {
    free(nData);
  }

  if (ctrl) {
    return readresp('2', ctrl);
  }

  return 1;
}

/*
 * FtpRead - read from a data connection
 */
inline int Client::FtpRead(void *buf, int max, context *nData) {
  int i;

  if (nData->dir != FTPLIB_READ) {
    return 0;
  }
  if (nData->buf) {
    i = readline(static_cast<char *>(buf), max, nData);
  } else {
    i = socket_wait(nData);
    if (i != 1) {
      return 0;
    }

    i = net_read(nData->handle, buf, max);
  }
  if (i == -1) {
    return 0;
  }
  nData->xfered += i;

  if (nData->xfercb && nData->cbbytes) {
    nData->xfered1 += i;
    if (nData->xfered1 > nData->cbbytes) {
      if (nData->xfercb(nData->xfered, nData->cbarg) == 0) {
        return 0;
      }
      nData->xfered1 = 0;
    }
  }
  return i;
}

/*
 * FtpWrite - write to a data connection
 */
inline int Client::FtpWrite(void *buf, int len, context *nData) {
  int i;

  if (nData->dir != FTPLIB_WRITE) {
    printf("nData->dir != FTPLIB_WRITE\n");
    return 0;
  }

  if (nData->buf) {
    i = writeline(static_cast<char *>(buf), len, nData);
  } else {
    socket_wait(nData);

    {
      i = net_write(nData->handle, buf, len);
      strerror(errno);
    }
  }

  if (i == -1) {
    printf("i == -1\n");
    return 0;
  }

  nData->xfered += i;

  if (nData->xfercb && nData->cbbytes) {
    nData->xfered1 += i;
    if (nData->xfered1 > nData->cbbytes) {
      if (nData->xfercb(nData->xfered, nData->cbarg) == 0) {
        printf("nData->xfercb(nData->xfered, nData->cbarg) == 0\n");
        return 0;
      }
      nData->xfered1 = 0;
    }
  }

  return i;
}

/*
 * FtpMkdir - create a directory at server
 *
 * return 1 if successful, 0 otherwise
 */
inline int Client::Mkdir(const char *path) {
  char buf[256];

  if ((strlen(path) + 6) > sizeof(buf)) {
    return 0;
  }
  sprintf(buf, "MKD %s", path);
  if (!FtpSendCmd(buf, '2', context_)) {
    return 0;
  }
  return 1;
}

/*
 * FtpXfer - issue a command and transfer data
 *
 * return 1 if successful, 0 otherwise
 */
inline int Client::FtpXfer(const char *localfile, const char *path,
                           context *nControl, accesstype type,
                           transfermode mode) {
  int l, c;
  char *dbuf;
  FILE *local = NULL;
  int rv = 1;

  if (localfile != NULL) {
    char ac[3] = "  ";
    if ((type == Client::dir) || (type == Client::dirverbose)) {
      ac[0] = 'w';
      ac[1] = '\0';
    }
    if (type == Client::fileread) {
      ac[0] = 'w';
      ac[1] = '\0';
    }
    if (type == Client::filewriteappend) {
      ac[0] = 'r';
      ac[1] = '\0';
    }
    if (type == Client::filereadappend) {
      ac[0] = 'a';
      ac[1] = '\0';
    }
    if (type == Client::filewrite) {
      ac[0] = 'r';
      ac[1] = '\0';
    }
    if (mode == Client::image) {
      ac[1] = 'b';
    }

    local = fopen(localfile, ac);
    if (local) {
      if (type == Client::filewriteappend) {
        fseek(local, context_->offset, SEEK_SET);
      }
    } else {
      strncpy(nControl->response, strerror(errno), sizeof(nControl->response));
      return 0;
    }
  }
  if (local == NULL)
    local = ((type == Client::filewrite) || (type == Client::filewriteappend))
                ? stdin
                : stdout;

  context *nData = new context;
  if (!FtpAccess(path, type, mode, nControl, &nData)) {
    printf("==FtpAccess == 0\n");
    delete nData;
    if ((NULL != local) && (stdin != local) && (stdout != local)) {
      fclose(local);
      local = NULL;
    }
    return 0;
  }

  int cnt = 0;
  dbuf = static_cast<char *>(malloc(FTPLIB_BUFSIZ));
  if ((type == Client::filewrite) || (type == Client::filewriteappend)) {
    while ((l = fread(dbuf, 1, FTPLIB_BUFSIZ, local)) > 0) {
      if ((c = FtpWrite(dbuf, l, nData)) < l) {
        rv = 0;
        free(dbuf);
        if (localfile != NULL) {
          fclose(local);
        }
        FtpClose(nData);
        printf("==c=%d---l=%d-\n", c, l);
        return rv;
      }

      if (((++cnt) % 100) == 0) {
        usleep(20000);
      }
    }
  } else {
    while ((l = FtpRead(dbuf, FTPLIB_BUFSIZ, nData)) > 0) {
      if (fwrite(dbuf, 1, l, local) <= 0) {
        perror("localfile write");
        break;
      }
      if (((++cnt) % 100) == 0) {
        usleep(20000);
      }
    }
  }

  free(dbuf);
  if (localfile != NULL) {
    fclose(local);
  }

  rv = FtpClose(nData);
  if (rv == 0) {
    rv = -1;
  }
  return rv;
}

/*
 * FtpSize - determine the size of a remote file
 *
 * return 1 if successful, 0 otherwise
 */
inline int Client::Size(const char *path, int *size, transfermode mode) {
  char cmd[256];
  int resp, sz, rv = 1;

  if ((strlen(path) + 7) > sizeof(cmd)) {
    return 0;
  }

  sprintf(cmd, "TYPE %c", mode);
  if (!FtpSendCmd(cmd, '2', context_)) {
    return 0;
  }

  sprintf(cmd, "SIZE %s", path);
  if (!FtpSendCmd(cmd, '2', context_)) {
    rv = 0;
  } else {
    if (sscanf(context_->response, "%d %d", &resp, &sz) == 2) {
      *size = sz;
    } else {
      rv = 0;
    }
  }
  return rv;
}

/*
 * FtpPut - issue a PUT command and send data from input
 *
 * return 1 if successful, 0 otherwise
 */

inline int Client::Put(const char *inputfile, const char *path,
                       transfermode mode, off64_t offset) {
  context_->offset = offset;

  if (offset == 0) {
    return FtpXfer(inputfile, path, context_, Client::filewrite, mode);
  } else {
    return FtpXfer(inputfile, path, context_, Client::filewriteappend, mode);
  }
}

inline int Client::Get(const char *outputfile, const char *path,
                       transfermode mode, off64_t offset) {
  context_->offset = offset;

  if (offset == 0) {
    return FtpXfer(outputfile, path, context_, Client::fileread, mode);
  } else {
    return FtpXfer(outputfile, path, context_, Client::filereadappend, mode);
  }
}

/*
 * FtpQuit - disconnect from remote
 *
 * return 1 if successful, 0 otherwise
 */
inline int Client::Quit() {
  int ret = 0;

  if (context_->dir != FTPLIB_CONTROL) {
    printf("context_->dir = %d, FTPLIB_CONTROL = %d", context_->dir,
           FTPLIB_CONTROL);
    goto Done;
  }

  if (context_->handle == 0) {
    strcpy(context_->response, "error: no anwser from server\n");
    printf("[%s,%d]:quit ftp no answer from server!\n", __FUNCTION__, __LINE__);
    goto Done;
  }

  if (!FtpSendCmd("QUIT", '2', context_)) {
    printf("[%s,%d]:quit ftp error!\n", __FUNCTION__, __LINE__);
  } else {
    ret = 1;
  }

Done:
  if (context_->handle > 0) {
    net_close(context_->handle);
    context_->handle = 0;
  }

  return ret;
}

inline void Client::SetCallbackXferFunction(FtpCallbackXfer pointer) {
  context_->xfercb = pointer;
}

inline void Client::SetCallbackLogFunction(FtpCallbackLog pointer) {
  context_->logcb = pointer;
}

inline void Client::SetCallbackIdletime(int time) {
  context_->idletime.tv_sec = time / 1000;
  context_->idletime.tv_usec = (time % 1000) * 1000;
}

inline void Client::SetConnmode(connmode mode) { context_->cmode = mode; }

inline void Client::ClearHandle() {
  context_->dir = FTPLIB_CONTROL;
  context_->ctrl = NULL;
  context_->cmode = Client::pasv;
  context_->idlecb = NULL;
  context_->idletime.tv_sec = context_->idletime.tv_usec = 0;
  context_->cbarg = NULL;
  context_->xfered = 0;
  context_->xfered1 = 0;
  context_->cbbytes = 1;
  context_->offset = 0;
  context_->handle = 0;
  context_->logcb = NULL;
  context_->xfercb = NULL;
  context_->correctpasv = false;
}

inline int Client::CorrectPasvResponse(unsigned int *v) {
  struct sockaddr ipholder;
  socklen_t ipholder_size = sizeof(ipholder);

  if (getpeername(context_->handle, &ipholder, &ipholder_size) == -1) {
    perror("getpeername");
    net_close(context_->handle);
    return 0;
  }

  for (int i = 2; i < 6; i++) {
    v[i] = ipholder.sa_data[i];
  }

  return 1;
}

inline int Client::AsynConnect(int sock, struct sockaddr *sockaddr, int namelen,
                               int tm_us) {
  int rv = 0;

  int flag = ::fcntl(sock, F_GETFL, 0);
  flag |= O_NONBLOCK;
  ::fcntl(sock, F_SETFL, flag);

  do {
    int iRet = ::connect(sock, sockaddr, namelen);
    if (iRet < 0) {
      if (errno != EINPROGRESS) {
        printf("[AsynConnect]connect Failed.[errno = %d]\n", errno);
        break;
      }

      int select_nums = tm_us / (1000 * 1000);
      if (select_nums == 0) {
        select_nums = 1;
      }

      int selects = 0;
      struct timeval tm;
      fd_set rset, wset;
      do {
        tm.tv_sec = 1;
        tm.tv_usec = 0;
        FD_ZERO(&rset);
        FD_ZERO(&wset);
        FD_SET(sock, &rset);
        FD_SET(sock, &wset);
        iRet = ::select(sock + 1, &rset, &wset, NULL, &tm);
        if (iRet == 0) {
          if (context_ && context_->idlecb) {
            if (!context_->idlecb(context_->cbarg)) {
              printf("[AsynConnect]idlecb Failed\n");
              break;
            }
          }
          ++selects;
        } else {
          break;
        }
      } while (selects < select_nums);

      if (iRet <= 0) {
        printf("[AsynConnect]select Failed[iRet = %d, errno = %d]\n", iRet,
               errno);
        break;
      }

      bool berr = false;
      if (FD_ISSET(sock, &rset) || FD_ISSET(sock, &wset)) {
        int ierror = 0;
        int ilen = sizeof(ierror);
        if (getsockopt(sock, SOL_SOCKET, SO_ERROR, (char *)&ierror,
                       (socklen_t *)&ilen) < 0) {
          berr = true;
          printf("[AsynConnect]getsockopt failed[errno = %d]\n", errno);
        } else {
          berr = ierror != 0;
        }
      } else {
        berr = true;
        printf("[AsynConnect]berr2 = true\n");
      }

      if (berr) {
        printf("[AsynConnect]berr = true, break;\n");
        break;
      }
    }

    rv = 1;
    break;
  } while (0);

  // set block
  flag = ::fcntl(sock, F_GETFL, 0);
  flag &= ~O_NONBLOCK;
  ::fcntl(sock, F_SETFL, flag);
  return rv;
}

} // namespace libftp
