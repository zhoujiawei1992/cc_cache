#ifndef __CC_HTTP_CC_ACCEPTOR_H__
#define __CC_HTTP_CC_ACCEPTOR_H__

#include "cc_util/cc_list.h"
#include "cc_worker/cc_worker.h"

typedef struct _cc_tcp_acceptor_s {
  cc_slist_node_t node;
  cc_worker_t* worker;
  int listen_fd;
  cc_socket_proc_t* accept_done;
} cc_tcp_acceptor_t;

cc_tcp_acceptor_t* cc_tcp_acceptor_create(cc_worker_t* worker, const char* ip, int port, int backlog,
                                          cc_socket_proc_t* accept_done);

void cc_tcp_acceptor_free(cc_tcp_acceptor_t* acceptor);

void cc_tcp_acceptor_accept(cc_event_loop_t* event_loop, int fd, void* client_data, int error_code);

#endif