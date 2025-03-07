#include "cc_net/cc_acceptor.h"
#include <arpa/inet.h>
#include "cc_event/cc_event.h"
#include "cc_http/cc_http_client_request.h"
#include "cc_net/cc_net.h"
#include "cc_util/cc_util.h"
cc_tcp_acceptor_t *cc_tcp_acceptor_create(cc_worker_t *worker, const char *ip, int port, int backlog,
                                          cc_socket_proc_t *accept_done) {
  cc_tcp_acceptor_t *acceptor = (cc_tcp_acceptor_t *)cc_alloc(1, sizeof(cc_tcp_acceptor_t));
  if (acceptor == NULL) {
    error_log("cc_tcp_acceptor_create: cc_alloc failed %d", sizeof(cc_tcp_acceptor_t));
    return NULL;
  }
  acceptor->worker = worker;
  acceptor->accept_done = accept_done;

  int ret = cc_net_tcp_server(ip, port, &acceptor->listen_fd);
  if (ret != CC_NET_OK) {
    error_log("cc_tcp_acceptor_create: cc_net_tcp_server failed, errno: %d, error: %s", cc_net_get_last_errno,
              cc_net_get_last_error);
    cc_tcp_acceptor_free(acceptor);
    return NULL;
  }

  if (cc_net_non_block(acceptor->listen_fd) == CC_NET_ERR) {
    error_log("cc_tcp_acceptor_create: cc_net_non_block failed, errno: %d, error: %s", cc_net_get_last_errno,
              cc_net_get_last_error);
    cc_tcp_acceptor_free(acceptor);
    return NULL;
  }

  if (cc_net_listen(acceptor->listen_fd, backlog)) {
    error_log("cc_tcp_acceptor_create: cc_net_listen failed, errno: %d, error: %s", cc_net_get_last_errno,
              cc_net_get_last_error);
    cc_tcp_acceptor_free(acceptor);
    return NULL;
  }

  if (cc_set_socket_event(worker->event_loop, acceptor->listen_fd, SS_LISTENING,
                          CC_EVENT_READABLE | CC_EVENT_ERROR | CC_EVENT_HIGH_PRIORITY, cc_tcp_acceptor_accept, acceptor,
                          NULL, NULL) != CC_EVENT_OK) {
    error_log("cc_tcp_acceptor_create: cc_set_socket_event failed, errno: %d, error: %s", cc_net_get_last_errno,
              cc_net_get_last_error);
    cc_tcp_acceptor_free(acceptor);
    return NULL;
  }

  debug_log("cc_tcp_acceptor_create done, ip: %s, port: %d, fd: %d", ip, port, acceptor->listen_fd);
  return acceptor;
}

void cc_tcp_acceptor_free(cc_tcp_acceptor_t *acceptor) {
  if (acceptor->listen_fd > 0) {
    if (acceptor->worker->event_loop->events[acceptor->listen_fd].state != SS_NONE) {
      cc_del_socket_event(acceptor->worker->event_loop, acceptor->listen_fd, CC_EVENT_READABLE);
      cc_net_close(acceptor->listen_fd);
    } else {
      cc_net_close(acceptor->listen_fd);
    }
    acceptor->listen_fd = 0;
  }
  cc_free(acceptor);
  debug_log("cc_tcp_acceptor_free, fd=%d", acceptor->listen_fd);
}

void cc_tcp_acceptor_accept(cc_event_loop_t *event_loop, int fd, void *client_data, int mask) {
  cc_tcp_acceptor_t *acceptor = (cc_tcp_acceptor_t *)client_data;
  if (acceptor->accept_done) {
    acceptor->accept_done(event_loop, fd, client_data, mask);
  }
}