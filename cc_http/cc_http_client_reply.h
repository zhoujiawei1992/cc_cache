#ifndef __CC_HTTP_CC_HTTP_CLIENT_REPLY_H__
#define __CC_HTTP_CC_HTTP_CLIENT_REPLY_H__

#include <arpa/inet.h>
#include "cc_event/cc_event.h"
#include "cc_http/cc_http_worker.h"
#include "cc_util/cc_buffer.h"
#include "cc_util/cc_list.h"
#include "third/http_parser/http_parser.h"

#define CONNCETION_CLOSE 0
#define CONNCETION_KEEPALIVE 1
#define CACHE_CONTROL_NO_CACHE 0
#define CONTENT_LENGTH_NO_OUTPUT -1

typedef struct _cc_http_context_s cc_http_context_t;

typedef struct _cc_http_reply_s {
  enum http_status status;
  cc_slist_node_t header_list;
  cc_buffer_t *header_buffer;
  cc_buffer_t *body_buffer;
  size_t bytes_sent;
  unsigned int header_size;
  unsigned int cache_control;
  unsigned int age;
  ssize_t content_length;
  unsigned int connection : 1;
  unsigned int chunk_header : 1;
  unsigned int chunk_complete : 1;
} cc_http_reply_t;

void cc_http_client_clear_reply(cc_http_context_t *http_context, cc_http_reply_t *reply);

void cc_http_client_reset_reply(cc_http_context_t *http_context, cc_http_reply_t *reply);

void cc_http_client_write_timeout(cc_event_loop_t *event_loop, void *client_data);

void cc_http_client_write(cc_event_loop_t *event_loop, int fd, void *client_data, int mask);

int cc_http_client_start_send_header(cc_http_context_t *http_context);

void cc_http_client_send_header_done(cc_http_context_t *http_context, ssize_t nbytes, int flag);

int cc_http_client_start_send_body(cc_http_context_t *http_context, const char *body, size_t body_size);

void cc_http_client_send_body_done(cc_http_context_t *http_context, ssize_t nbytes, int flag);

int cc_http_client_start_send_chunked_body(cc_http_context_t *http_context, const char *body, size_t body_size);

int cc_http_client_start_send_last_chunked(cc_http_context_t *http_context);

void cc_http_client_send_chunked_body_done(cc_http_context_t *http_context, ssize_t nbytes, int flag);

void cc_http_client_send_error_page(cc_http_context_t *http_context, enum http_status status);

void cc_http_client_send_reply(cc_http_context_t *http_context);

#endif