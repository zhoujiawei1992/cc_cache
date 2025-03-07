#ifndef __CC_NET_CC_NET_H__
#define __CC_NET_CC_NET_H__

#include <errno.h>
#include <string.h>
#include <sys/uio.h>

#define CC_NET_OK 0
#define CC_NET_ERR -1
#define CC_NET_AGAIN 11
#define CC_NET_CONTINUE 10
#define CC_NET_CLOSE 12

#define cc_net_get_last_errno (errno)
#define cc_net_get_last_error strerror(cc_net_get_last_errno)

int cc_net_tcp_server(const char* ip, int port, int* server_fd);
int cc_net_tcp_connect(const char* ip, int port, int* upstream_fd);
int cc_net_listen(int fd, int backlog);
int cc_net_non_block(int fd);
int cc_net_block(int fd);
int cc_net_close(int fd);
int cc_net_reuseport(int fd);
int cc_net_reuseaddr(int fd);
int cc_net_accept(int fd, int* client_fd, char* buf, int buf_size);
int cc_net_so_errno(int fd);
int cc_net_read(int fd, void* buf, size_t count, size_t* out_size);
int cc_net_writev(int fd, struct iovec* iov, int iovcnt, size_t* out_size);

#endif