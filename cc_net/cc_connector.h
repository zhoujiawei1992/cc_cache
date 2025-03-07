#ifndef __CC_HTTP_CC_CONNECTOR_H__
#define __CC_HTTP_CC_CONNECTOR_H__

#include "cc_event/cc_event.h"
#include "cc_util/cc_list.h"
#include "cc_worker/cc_worker.h"

typedef enum {
  CONNECTOR_STATE_CLOSED,
  CONNECTOR_STATE_CONNECTING,
  CONNECTOR_STATE_CONNECTED,
} ConnectorState;

typedef void cc_connect_proc_t(cc_event_loop_t* event_loop, int fd, void* client_data, int error_code);

typedef struct _cc_tcp_connector_s {
  cc_worker_t* worker;
  cc_connect_proc_t* connect_done;
  cc_time_event_t connect_timer;
  int remote_fd;
  ConnectorState state;
} cc_tcp_connector_t;

cc_tcp_connector_t* cc_tcp_connector_create(cc_worker_t* worker, const char* ip, int port,
                                            cc_socket_proc_t* connect_done, int timeout);

void cc_tcp_connector_free(cc_tcp_connector_t* connector);

void cc_tcp_connector_close(cc_tcp_connector_t* connector);

void cc_tcp_connector_connect_done(cc_event_loop_t* event_loop, int fd, void* client_data, int mask);

void cc_tcp_connector_connect_timeout(cc_event_loop_t* event_loop, void* client_data);

#endif