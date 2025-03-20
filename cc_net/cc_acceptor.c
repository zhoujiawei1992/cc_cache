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
    error_log("cc_tcp_acceptor_create, cc_alloc failed, worker=%x, ip=%s, port=%d, backlog=%d, accept_done=%x", worker,
              ip, port, backlog, accept_done);
    return NULL;
  }
  acceptor->worker = worker;
  acceptor->accept_done = accept_done;

  int ret = cc_net_tcp_server(ip, port, &acceptor->listen_fd);
  if (ret != CC_NET_OK) {
    error_log(
        "cc_tcp_acceptor_create, cc_net_tcp_server failed, worker=%x, ip=%s, port=%d, backlog=%d, accept_done=%x, "
        "errno: %d, error: %s",
        worker, ip, port, backlog, accept_done, cc_net_get_last_errno, cc_net_get_last_error);
    cc_tcp_acceptor_free(acceptor);
    return NULL;
  }

  if (cc_net_non_block(acceptor->listen_fd) == CC_NET_ERR) {
    error_log(
        "cc_tcp_acceptor_create, cc_net_non_block failed, worker=%x, ip=%s, port=%d, backlog=%d, accept_done=%x, "
        "errno: %d, error: %s",
        worker, ip, port, backlog, accept_done, cc_net_get_last_errno, cc_net_get_last_error);
    cc_tcp_acceptor_free(acceptor);
    return NULL;
  }

  if (cc_net_listen(acceptor->listen_fd, backlog)) {
    error_log(
        "cc_tcp_acceptor_create, cc_net_listen failed, worker=%x, ip=%s, port=%d, backlog=%d, accept_done=%x, errno: "
        "%d, error: %s",
        worker, ip, port, backlog, accept_done, cc_net_get_last_errno, cc_net_get_last_error);
    cc_tcp_acceptor_free(acceptor);
    return NULL;
  }

  if (cc_set_socket_event(worker->event_loop, acceptor->listen_fd, SS_LISTENING,
                          CC_EVENT_READABLE | CC_EVENT_ERROR | CC_EVENT_HIGH_PRIORITY, cc_tcp_acceptor_accept, acceptor,
                          NULL, NULL) != CC_EVENT_OK) {
    error_log(
        "cc_tcp_acceptor_create, cc_set_socket_event failed, worker=%x, ip=%s, port=%d, backlog=%d, accept_done=%x, "
        "errno: %d, error: %s",
        worker, ip, port, backlog, accept_done, cc_net_get_last_errno, cc_net_get_last_error);
    cc_tcp_acceptor_free(acceptor);
    return NULL;
  }

  debug_log("cc_tcp_acceptor_create done, worker=%x, ip=%s, port=%d, backlog=%d, accept_done=%x ", worker, ip, port,
            backlog, accept_done);
  return acceptor;
}

void cc_tcp_acceptor_free(cc_tcp_acceptor_t *acceptor) {
  if (acceptor->listen_fd > 0) {
    cc_del_socket_event(acceptor->worker->event_loop, acceptor->listen_fd, CC_EVENT_READABLE);
    cc_net_close(acceptor->listen_fd);
  }
  cc_free(acceptor);
  debug_log("cc_tcp_acceptor_free, acceptor=%x, fd=%d", acceptor, acceptor->listen_fd);
}

void cc_tcp_acceptor_accept(cc_event_loop_t *event_loop, int fd, void *client_data, int mask) {
  if (fd >= event_loop->nevents) {
    error_log("cc_tcp_acceptor_accept fd size is limited, event_loop=%x, fd=%d, client_data=%x, mask=%d", event_loop,
              fd, client_data, mask);
    cc_net_close(fd);
    return;
  }

  if (!(event_loop->events[fd].mask & CC_EVENT_READABLE)) {
    debug_log("cc_tcp_acceptor_accept ignore, event_loop=%x, fd=%d, client_data=%x, mask=%d", event_loop, fd,
              client_data, mask);
    return;
  }
  cc_tcp_acceptor_t *acceptor = (cc_tcp_acceptor_t *)client_data;
  if (acceptor->accept_done) {
    acceptor->accept_done(event_loop, fd, client_data, mask);
  }
}