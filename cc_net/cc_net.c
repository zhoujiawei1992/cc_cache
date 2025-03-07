#include "cc_net/cc_net.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "cc_util/cc_snprintf.h"
#include "cc_util/cc_util.h"

int cc_net_tcp_server(const char* ip, int port, int* server_fd) {
  int fd;
  struct sockaddr_in addr;
  fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd == -1) {
    return CC_NET_ERR;
  }

  if (cc_net_reuseaddr(fd) != CC_NET_OK) {
    cc_net_close(fd);
    return CC_NET_ERR;
  }

  if (cc_net_reuseport(fd) != CC_NET_OK) {
    cc_net_close(fd);
    return CC_NET_ERR;
  }

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr(ip);
  addr.sin_port = htons(port);

  if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    cc_net_close(fd);
    return CC_NET_ERR;
  }
  *server_fd = fd;
  return CC_NET_OK;
}

int cc_net_tcp_connect(const char* ip, int port, int* upstream_fd) {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (cc_net_non_block(fd) != CC_NET_OK) {
    cc_net_close(fd);
    return CC_NET_ERR;
  }

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr(ip);
  addr.sin_port = htons(port);

  int ret = connect(fd, (struct sockaddr*)&addr, sizeof(addr));
  if (ret != 0 && errno != EINPROGRESS) {
    cc_net_close(fd);
    return CC_NET_ERR;
  }
  *upstream_fd = fd;
  return ret == 0 ? CC_NET_OK : CC_NET_AGAIN;
}

int cc_net_listen(int fd, int backlog) { return listen(fd, backlog) == 0 ? CC_NET_OK : CC_NET_ERR; }

static int cc_net_set_block(int fd, int non_block) {
  int flags;
  if ((flags = fcntl(fd, F_GETFL)) == -1) {
    return CC_NET_ERR;
  }
  if (!!(flags & O_NONBLOCK) == !!non_block) {
    return CC_NET_OK;
  }
  if (non_block) {
    flags |= O_NONBLOCK;
  } else {
    flags &= ~O_NONBLOCK;
  }
  if (fcntl(fd, F_SETFL, flags) == -1) {
    return CC_NET_ERR;
  }
  return CC_NET_OK;
}

int cc_net_non_block(int fd) { return cc_net_set_block(fd, 1); }
int cc_net_block(int fd) { return cc_net_set_block(fd, 0); }
int cc_net_close(int fd) {
  close(fd);
  debug_log("close fd=%d", fd);
}

int cc_net_reuseport(int fd) {
  int reuse_port = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &reuse_port, sizeof(reuse_port)) == -1) {
    return CC_NET_ERR;
  }
  return CC_NET_OK;
}

int cc_net_reuseaddr(int fd) {
  int reuse = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
    return CC_NET_ERR;
  }
  return CC_NET_OK;
}

int cc_net_accept(int server_fd, int* client_fd, char* buf, int buf_size) {
  struct sockaddr_in client_addr;
  socklen_t client_addr_len = sizeof(client_addr);
  int fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_len);
  if (fd < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return CC_NET_AGAIN;
    } else if (errno == EINTR) {
      return CC_NET_CONTINUE;
    } else {
      return CC_NET_ERR;
    }
  } else {
    *client_fd = fd;
    if (inet_ntop(AF_INET, &(client_addr.sin_addr), buf, buf_size) != NULL) {
      int len = strlen(buf);
      cc_snprintf(buf + len, buf_size - len, ":%d", ntohs(client_addr.sin_port));
    }
    return 0;
  }
}

int cc_net_so_errno(int fd) {
  int so_error;
  socklen_t len = sizeof(so_error);
  if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &so_error, &len) < 0) {
    return errno;
  }
  return so_error;
}

int cc_net_read(int fd, void* buf, size_t buf_size, size_t* out_size) {
  int ret = read(fd, buf, buf_size);
  if (ret > 0) {
    if (out_size) {
      *out_size = ret;
    }
    return CC_NET_OK;
  } else if (ret == 0) {
    return CC_NET_CLOSE;
  }
  if (errno == EAGAIN || errno == EWOULDBLOCK) {
    return CC_NET_AGAIN;
  } else if (errno == EINTR) {
    return CC_NET_CONTINUE;
  } else {
    return CC_NET_ERR;
  }
}

int cc_net_writev(int fd, struct iovec* iov, int iovcnt, size_t* out_size) {
  ssize_t ret = writev(fd, iov, iovcnt);
  if (ret >= 0) {
    if (out_size) {
      *out_size = (size_t)ret;
    }
    return CC_NET_OK;
  } else {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return CC_NET_AGAIN;
    } else if (errno == EINTR) {
      return CC_NET_CONTINUE;
    } else {
      return CC_NET_ERR;
    }
  }
}