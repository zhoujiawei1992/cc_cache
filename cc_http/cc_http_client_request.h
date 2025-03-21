#ifndef __CC_HTTP_CC_HTTP_CLIENT_REQUEST_H__
#define __CC_HTTP_CC_HTTP_CLIENT_REQUEST_H__

#include <arpa/inet.h>
#include "cc_event/cc_event.h"
#include "cc_http/cc_http_client_reply.h"
#include "cc_http/cc_http_upstream.h"
#include "cc_http/cc_http_worker.h"
#include "cc_util/cc_buffer.h"
#include "cc_util/cc_list.h"
#include "third/http_parser/http_parser.h"

extern cc_string_t g_crlf;
extern cc_string_t g_server_header;
extern cc_string_t g_connection_close_header;
extern cc_string_t g_connection_keepalive;
extern cc_string_t g_cache_control_no_cache_header;
extern cc_string_t g_pragma_no_cache_header;
extern cc_string_t g_transfer_encoding_chunked_header;
extern cc_string_t g_cache_control_cache_header_prefix;

extern cc_string_t g_transfer_encoding_chunked_field;
extern cc_string_t g_transfer_encoding_chunked_value;
extern cc_string_t g_content_length_header_field;
extern cc_string_t g_connection_header_field;
extern cc_string_t g_cache_control_header_field;
extern cc_string_t g_date_header_field;
extern cc_string_t g_server_header_field;
extern cc_string_t g_expire_header_field;

typedef enum {
  HTTP_STATE_REQUEST_WAIT,
  HTTP_STATE_REQUEST_HEADER_WAIT,
  HTTP_STATE_REQUEST_HEADER_DONE,
  HTTP_STATE_REQUEST_BODY_DONE,
} HttpState;

typedef struct _cc_http_context_s cc_http_context_t;

typedef void cc_http_parse_done_proc_t(cc_http_context_t *http_context);

typedef void cc_http_parse_on_body_proc_t(cc_http_context_t *http_context, const char *buffer, unsigned int length);

typedef void cc_http_send_done_proc_t(cc_http_context_t *http_context, ssize_t nbytes, int flag);

typedef struct _cc_http_header_s {
  cc_slist_node_t node;
  cc_string_t field;
  cc_string_t value;
} cc_http_header_t;

typedef struct _cc_http_request_s {
  cc_string_t url;
  cc_string_t parsing_header_field;
  cc_slist_node_t header_list;
  size_t content_length;
  int bytes_recv;
  int header_size;
  unsigned int chunk_header : 1;
  unsigned int chunk_complete : 1;
} cc_http_request_t;

typedef struct _cc_send_data_node_s {
  cc_dlist_node_t node;
  cc_buffer_t *buffer;
  unsigned int offset;
  unsigned int size;
  unsigned int flag;
} cc_send_data_node_t;

typedef struct _cc_http_upstream_s {
  cc_http_reply_t http_reply;
  cc_http_request_t http_request;
  http_parser http_parser;
} cc_http_upstream_t;

typedef struct _cc_http_context_s {
  cc_http_worker_t *http_worker;
  cc_buffer_t *header_buffer;
  cc_buffer_t *body_buffer;
  char client_addr[INET_ADDRSTRLEN];

  cc_time_event_t read_timer;
  cc_time_event_t write_timer;

  cc_http_reply_t http_reply;
  cc_http_request_t http_request;
  cc_http_upstream_t http_upstream;

  http_parser http_parser;
  cc_dlist_node_t send_data_list;
  cc_http_send_done_proc_t *send_done_proc;

  cc_http_parse_done_proc_t *on_header_done_proc;
  cc_http_parse_done_proc_t *on_message_done_proc;
  cc_http_parse_on_body_proc_t *on_body_proc;
  cc_http_parse_done_proc_t *on_chunk_header;
  cc_http_parse_done_proc_t *on_chunk_complete;

  int total_bytes_recv;
  int total_bytes_sent;

  HttpState http_state;
  int client_fd;

} cc_http_context_t;

#define cc_http_context_create(http_context) cc_inner_http_context_create(http_context, __func__, __LINE__)
#define cc_http_context_free(http_context) cc_inner_http_context_free(http_context, __func__, __LINE__)
#define cc_http_request_create(http_request) cc_inner_http_request_create(http_request, __func__, __LINE__)
#define cc_http_request_free(http_request) cc_inner_http_request_free(http_request, __func__, __LINE__)

#define cc_http_client_http_context_close(http_context) \
  cc_inner_http_client_http_context_close(http_context, __func__, __LINE__)

cc_http_context_t *cc_inner_http_context_create(cc_http_worker_t *http_context, const char *func, int line);
void cc_inner_http_context_free(cc_http_context_t *http_context, const char *func, int line);
void cc_http_context_reset(cc_http_context_t *http_context);

void cc_http_client_clear_request(cc_http_context_t *http_context, cc_http_request_t *request);
void cc_http_client_reset_request(cc_http_context_t *http_context, cc_http_request_t *request);

void cc_http_context_set_timer(cc_http_context_t *http_context, cc_time_event_t *timer, cc_time_proc_t *proc,
                               int timeout);
void cc_http_context_del_timer(cc_http_context_t *http_context, cc_time_event_t *timer);

void cc_http_client_read_timeout(cc_event_loop_t *event_loop, void *client_data);

void cc_http_client_read(cc_event_loop_t *event_loop, int fd, void *client_data, int mask);

void cc_http_client_accept(cc_event_loop_t *event_loop, int fd, void *client_data, int mask);
void cc_http_client_connect_done(cc_event_loop_t *event_loop, int fd, void *client_data, int mask);

int cc_http_client_start_read_and_write(cc_http_context_t *http_context);

void cc_inner_http_client_http_context_close(cc_http_context_t *http_context, const char *func, int line);

int cc_http_client_request_do_parse(cc_http_context_t *http_context, const char *buffer, unsigned int length,
                                    size_t *nbytes);

void cc_http_client_request_parse_on_header_done(cc_http_context_t *http_context);

void cc_http_client_request_parse_on_message_done(cc_http_context_t *http_context);

void cc_http_client_request_parse_on_body(cc_http_context_t *http_context, const char *buffer, unsigned int length);

void cc_http_client_admin_request_parse_on_header_done(cc_http_context_t *http_context);

void cc_http_client_admin_request_parse_on_message_done(cc_http_context_t *http_context);

#endif