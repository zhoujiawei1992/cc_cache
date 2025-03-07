#ifndef __CC_HTTP_CC_HTTP_CLIENT_REQUEST_H__
#define __CC_HTTP_CC_HTTP_CLIENT_REQUEST_H__

#include <arpa/inet.h>
#include "cc_event/cc_event.h"
#include "cc_http/cc_http_worker.h"
#include "cc_util/cc_buffer.h"
#include "cc_util/cc_cb_context.h"
#include "cc_util/cc_hash.h"
#include "cc_util/cc_list.h"

typedef enum {
  HTTP_STATE_REQUEST_WAIT,
  HTTP_STATE_REQUEST_PROCESS,
  HTTP_STATE_RESPONSE_SEND,
} HttpState;

typedef struct _cc_http_context_s cc_http_context_t;

typedef struct _cc_http_request_s {
  cc_http_context_t *http_context;
} cc_http_request_t;

typedef struct _cc_http_context_s {
  cc_http_worker_t *http_worker;
  cc_http_request_t *http_request;
  int client_fd;
  HttpState http_state;
  cc_time_event_t read_timer;
  cc_time_event_t write_timer;
  char client_addr[INET_ADDRSTRLEN];
  cc_buffer_t *header_buffer;
  cc_buffer_t *body_buffer;
  cc_slist_node_t send_data_list_head;

  int start_read : 1;
  int start_write : 1;

} cc_http_context_t;

#define cc_http_context_create(http_context) cc_inner_http_context_create(http_context, __func__, __LINE__)
#define cc_http_context_free(http_context) cc_inner_http_context_free(http_context, __func__, __LINE__)
#define cc_http_request_create(http_request) cc_inner_http_request_create(http_request, __func__, __LINE__)
#define cc_http_request_free(http_request) cc_inner_http_request_free(http_request, __func__, __LINE__)

cc_http_context_t *cc_inner_http_context_create(cc_http_worker_t *http_context, const char *func, int line);
void cc_inner_http_context_free(cc_http_context_t *http_context, const char *func, int line);

cc_http_request_t *cc_inner_http_request_create(cc_http_context_t *http_context, const char *func, int line);
void cc_inner_http_request_free(cc_http_request_t *http_request, const char *func, int line);

void cc_http_context_set_timer(cc_http_context_t *http_context, cc_cb_context_t *cb_context, cc_time_event_t *timer,
                               cc_time_proc_t *proc, int timeout);
void cc_http_context_del_timer(cc_http_context_t *http_context, cc_time_event_t *timer);

void cc_http_client_read_timeout(cc_event_loop_t *event_loop, void *client_data);
void cc_http_client_write_timeout(cc_event_loop_t *event_loop, void *client_data);

void cc_http_client_read(cc_event_loop_t *event_loop, int fd, void *client_data, int mask);
void cc_http_client_write(cc_event_loop_t *event_loop, int fd, void *client_data, int mask);

void cc_http_client_accept(cc_event_loop_t *event_loop, int fd, void *client_data, int mask);
void cc_http_client_connect_done(cc_event_loop_t *event_loop, int fd, void *client_data, int mask);

int cc_http_client_start_read(cc_http_context_t *http_context);
int cc_http_client_start_write(cc_http_context_t *http_context);

void cc_http_client_http_context_stop_read(cc_http_context_t *http_context, cc_cb_context_t *cb_context);
void cc_http_client_http_context_stop_write(cc_http_context_t *http_context, cc_cb_context_t *cb_context);

#endif