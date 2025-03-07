#include "cc_http/cc_http_client_reply.h"
#include "cc_http/cc_http_client_request.h"
#include "cc_net/cc_net.h"
#include "cc_util/cc_config.h"
#include "cc_util/cc_snprintf.h"
#include "cc_util/cc_util.h"

#include <string.h>

#define MAX_IOV_NUM (32)
void cc_http_client_send_error_page(cc_http_context_t *http_context, enum http_status status) {
  cc_http_reply_t *reply = &http_context->http_reply;
  reply->status = status;
  reply->connection = CONNCETION_CLOSE;
  reply->cache_control = CACHE_CONTROL_NO_CACHE;
  reply->content_length = CONTENT_LENGTH_NO_OUTPUT;
  cc_http_client_send_reply(http_context);
}

void cc_http_client_send_reply(cc_http_context_t *http_context) {
  cc_http_reply_t *reply = &http_context->http_reply;
  time_t now = http_context->http_worker->worker.event_loop->current_msec / 1000;

  debug_log("cc_http_client_send_reply, http_context=%x, fd=%d, status=%d", http_context, http_context->client_fd,
            reply->status);

  reply->header_buffer = cc_buffer_create(CONFIG_HTTP_HEADER_BUFFER_SIZE);

  char *endp = cc_snprintf(reply->header_buffer->data, reply->header_buffer->capacity, "HTTP/%d.%d %d %s%V",
                           http_context->http_parser.http_major, http_context->http_parser.http_minor, reply->status,
                           http_status_str(reply->status), &g_crlf);

  cc_slist_node_t *node = reply->header_list.next;
  while (node != NULL) {
    cc_http_header_t *header = cc_list_data(node, cc_http_header_t, node);
    if (strncasecmp(header->field.buf, g_content_length_header_field.buf, header->field.len) != 0 &&
        strncasecmp(header->field.buf, g_connection_header_field.buf, header->field.len) != 0 &&
        strncasecmp(header->field.buf, g_cache_control_header_field.buf, header->field.len) != 0 &&
        strncasecmp(header->field.buf, g_date_header_field.buf, header->field.len) != 0 &&
        strncasecmp(header->field.buf, g_server_header_field.buf, header->field.len) != 0 &&
        strncasecmp(header->field.buf, g_expire_header_field.buf, header->field.len) != 0) {
      endp = cc_snprintf(endp, reply->header_buffer->data + reply->header_buffer->capacity - endp, "%V: %V%V",
                         &header->field, &header->value, &g_crlf);
    }
    node = node->next;
  }

  if (reply->chunk_header) {
    endp = cc_snprintf(endp, reply->header_buffer->data + reply->header_buffer->capacity - endp, "%V%V",
                       &g_transfer_encoding_chunked_header, &g_crlf);
  } else {
    if (reply->content_length != CONTENT_LENGTH_NO_OUTPUT) {
      endp = cc_snprintf(endp, reply->header_buffer->data + reply->header_buffer->capacity - endp, "%V: %ld%V",
                         &g_content_length_header_field, reply->content_length, &g_crlf);
    }
  }

  if (reply->connection == CONNCETION_CLOSE) {
    endp = cc_snprintf(endp, reply->header_buffer->data + reply->header_buffer->capacity - endp, "%V%V",
                       &g_connection_close_header, &g_crlf);
  } else {
    endp = cc_snprintf(endp, reply->header_buffer->data + reply->header_buffer->capacity - endp, "%V%V",
                       &g_connection_keepalive, &g_crlf);
  }

  if (reply->cache_control == 0) {
    endp = cc_snprintf(endp, reply->header_buffer->data + reply->header_buffer->capacity - endp, "%V%V",
                       &g_cache_control_no_cache_header, &g_crlf);
    endp = cc_snprintf(endp, reply->header_buffer->data + reply->header_buffer->capacity - endp, "%V%V",
                       &g_pragma_no_cache_header, &g_crlf);
    time_t expire_time = 0;
    char expire_date_buf[64] = {0};
    TimeStamp2GmtTime(expire_time, expire_date_buf, sizeof(expire_date_buf));
    endp = cc_snprintf(endp, reply->header_buffer->data + reply->header_buffer->capacity - endp, "%V: %s%V",
                       &g_expire_header_field, expire_date_buf, &g_crlf);
  } else {
    unsigned int last_age = 0;
    if (reply->cache_control > reply->age) {
      last_age = reply->cache_control - reply->age;
    }
    endp = cc_snprintf(endp, reply->header_buffer->data + reply->header_buffer->capacity - endp, "%V%ud",
                       &g_cache_control_cache_header_prefix, last_age);

    time_t expire_time = now + last_age;
    char expire_date_buf[64] = {0};
    TimeStamp2GmtTime(expire_time, expire_date_buf, sizeof(expire_date_buf));
    endp = cc_snprintf(endp, reply->header_buffer->data + reply->header_buffer->capacity - endp, "%V: %s%V",
                       &g_expire_header_field, expire_date_buf, &g_crlf);
  }

  endp = cc_snprintf(endp, reply->header_buffer->data + reply->header_buffer->capacity - endp, "%V%V", &g_server_header,
                     &g_crlf);

  char date_buf[64] = {0};
  TimeStamp2GmtTime(now, date_buf, sizeof(date_buf));
  endp = cc_snprintf(endp, reply->header_buffer->data + reply->header_buffer->capacity - endp, "%V: %s%V",
                     &g_date_header_field, date_buf, &g_crlf);

  endp = cc_snprintf(endp, reply->header_buffer->data + reply->header_buffer->capacity - endp, "%V", &g_crlf);

  reply->header_buffer->size = endp - reply->header_buffer->data;
  reply->header_size = reply->header_buffer->size;

  if (cc_http_client_start_send_header(http_context) != 0) {
    error_log("cc_http_client_send_reply cc_http_client_start_send_header failed, http_context=%x", http_context);
    cc_http_client_http_context_close(http_context);
  }
}

void cc_http_client_clear_reply(cc_http_context_t *http_context, cc_http_reply_t *reply) {
  if (reply->header_buffer) {
    cc_buffer_free(reply->header_buffer);
    reply->header_buffer = NULL;
  }
  if (reply->body_buffer) {
    cc_buffer_free(reply->body_buffer);
    reply->body_buffer = NULL;
  }

  cc_slist_node_t *node = reply->header_list.next;
  while (node != NULL) {
    reply->header_list.next = node->next;
    cc_http_header_t *header = cc_list_data(node, cc_http_header_t, node);
    cc_free(header);
    node = reply->header_list.next;
  }
}

void cc_http_client_reset_reply(cc_http_context_t *http_context, cc_http_reply_t *reply) {
  cc_http_client_clear_reply(http_context, reply);
  memset(reply, 0, sizeof(cc_http_reply_t));
  cc_slist_init(&reply->header_list);
}

int cc_http_client_start_send_header(cc_http_context_t *http_context) {
  cc_send_data_node_t *node = (cc_send_data_node_t *)cc_alloc(1, sizeof(cc_send_data_node_t));
  if (node == NULL) {
    error_log("cc_http_client_start_send_header, cc_alloc failed send_data_list has data, http_context=%x",
              http_context);
    return -1;
  }

  node->offset = 0;
  node->size = http_context->http_reply.header_buffer->size;
  cc_buffer_assgin(&node->buffer, http_context->http_reply.header_buffer);
  cc_dlist_insert_forward(&http_context->send_data_list, &node->node);

  http_context->send_done_proc = cc_http_client_send_header_done;

  cc_socket_event_t *socket_event = &http_context->http_worker->worker.event_loop->events[http_context->client_fd];
  debug_log("cc_http_client_start_send_header, http_context=%x, can_write=%ld", http_context, socket_event->can_write);
  if (socket_event->can_write) {
    cc_http_client_write(http_context->http_worker->worker.event_loop, http_context->client_fd, http_context, 0);
  }
  return 0;
}

void cc_http_client_send_header_done(cc_http_context_t *http_context, ssize_t nbytes, int flag) {
  debug_log("cc_http_client_send_header_done, http_context=%x, nbytes=%ld, content_length=%ld, connection=%ld, flag=%d",
            http_context, nbytes, http_context->http_reply.content_length, http_context->http_reply.connection, flag);
  if (nbytes < 0) {
    cc_http_client_http_context_close(http_context);
    return;
  }
  http_context->http_reply.bytes_sent += nbytes;
  if (http_context->http_reply.content_length > 0 ||
      (http_context->http_reply.chunk_header && http_context->http_reply.chunk_complete == 0)) {
    return;
  }
  if (http_context->http_reply.connection == CONNCETION_CLOSE) {
    cc_http_client_http_context_close(http_context);
    return;
  }
  if (!cc_dlist_empty(&http_context->send_data_list)) {
    error_log(
        "cc_http_client_send_header_done, cc_http_client_http_context_close send_data_list has data, http_context=%x, "
        "nbytes=%ld, content_length=%ld, connection=%ld",
        http_context, nbytes, http_context->http_reply.content_length, http_context->http_reply.connection);
    cc_http_client_http_context_close(http_context);
    return;
  }
  cc_http_context_reset(http_context);
}

int cc_http_client_start_send_body(cc_http_context_t *http_context, const char *body, size_t body_size) {
  debug_log("cc_http_client_start_send_body, http_context=%x, body_size=%lud", http_context, body_size);
  http_context->send_done_proc = cc_http_client_send_body_done;
  cc_socket_event_t *socket_event = &http_context->http_worker->worker.event_loop->events[http_context->client_fd];
  if (body_size > 0 && body > http_context->header_buffer->data &&
      body < http_context->header_buffer->data + http_context->header_buffer->size) {
    cc_send_data_node_t *node = (cc_send_data_node_t *)cc_alloc(1, sizeof(cc_send_data_node_t));
    if (node == NULL) {
      error_log("cc_http_client_start_send_body, cc_alloc failed send_data_list has data, http_context=%x",
                http_context);
      return -1;
    }
    node->offset = body - http_context->header_buffer->data;
    node->size = body + body_size - http_context->header_buffer->data;
    cc_buffer_assgin(&node->buffer, http_context->header_buffer);
    cc_dlist_insert_forward(&http_context->send_data_list, &node->node);

    if (socket_event->can_write) {
      cc_http_client_write(http_context->http_worker->worker.event_loop, http_context->client_fd, http_context, 0);
    }

  } else if (body_size > 0) {
    cc_send_data_node_t *node = (cc_send_data_node_t *)cc_alloc(1, sizeof(cc_send_data_node_t));
    if (node == NULL) {
      error_log("cc_http_client_start_send_body, cc_alloc failed send_data_list has data, http_context=%x",
                http_context);
      return -1;
    }

    node->offset = body - http_context->body_buffer->data;
    node->size = body + body_size - http_context->body_buffer->data;
    cc_buffer_assgin(&node->buffer, http_context->body_buffer);
    cc_dlist_insert_forward(&http_context->send_data_list, &node->node);

    if (socket_event->can_write) {
      cc_http_client_write(http_context->http_worker->worker.event_loop, http_context->client_fd, http_context, 0);
    }
  }
  return 0;
}

void cc_http_client_send_body_done(cc_http_context_t *http_context, ssize_t nbytes, int flag) {
  debug_log("cc_http_client_send_body_done, http_context=%x, nbytes=%ld, content_length=%ld, connection=%ld, flag=%d",
            http_context, nbytes, http_context->http_reply.content_length, http_context->http_reply.connection, flag);
  if (nbytes < 0) {
    cc_http_client_http_context_close(http_context);
    return;
  }
  http_context->http_reply.bytes_sent += nbytes;

  if (http_context->http_reply.content_length + http_context->http_reply.header_size >
      http_context->http_reply.bytes_sent) {
    debug_log(
        "cc_http_client_send_body_done, has data need send, http_context=%x, nbytes=%ld, bytes_sent=%ld, "
        "content_length=%ld, header_size=%d",
        http_context, nbytes, http_context->http_reply.bytes_sent, http_context->http_reply.content_length,
        http_context->http_reply.header_size);
    return;
  }
  debug_log(
      "cc_http_client_send_body_done, all data sent, http_context=%x, nbytes=%ld, bytes_sent=%ld, "
      "content_length=%ld, header_size=%d",
      http_context, nbytes, http_context->http_reply.bytes_sent, http_context->http_reply.content_length,
      http_context->http_reply.header_size);
  if (http_context->http_reply.connection == CONNCETION_CLOSE) {
    cc_http_client_http_context_close(http_context);
    return;
  }
  if (!cc_dlist_empty(&http_context->send_data_list)) {
    error_log(
        "cc_http_client_send_body_done, cc_http_client_http_context_close send_data_list has data, http_context=%x, "
        "nbytes=%ld, content_length=%ld, connection=%ld",
        http_context, nbytes, http_context->http_reply.content_length, http_context->http_reply.connection);
    cc_http_client_http_context_close(http_context);
    return;
  }
  cc_http_context_reset(http_context);
}

int cc_http_client_start_send_chunked_body(cc_http_context_t *http_context, const char *body, size_t body_size) {
  debug_log("cc_http_client_start_send_chunked_body, http_context=%x, body_size=%lud", http_context, body_size);
  http_context->send_done_proc = cc_http_client_send_chunked_body_done;
  cc_socket_event_t *socket_event = &http_context->http_worker->worker.event_loop->events[http_context->client_fd];
  if (body_size > 0) {
    {
      cc_send_data_node_t *node = (cc_send_data_node_t *)cc_alloc(1, sizeof(cc_send_data_node_t));
      if (node == NULL) {
        error_log("cc_http_client_start_send_body, cc_alloc failed, http_context=%x", http_context);
        return -1;
      }

      node->buffer = cc_buffer_create(32);
      if (node->buffer == NULL) {
        error_log("cc_http_client_start_send_chunked_body, cc_buffer_create failed, http_context=%x", http_context);
        return -1;
      }
      char *endp = cc_snprintf(node->buffer->data, node->buffer->capacity, "%x%V", body_size, &g_crlf);
      node->buffer->size = endp - node->buffer->data;

      node->offset = 2;
      node->size = node->buffer->size;
      cc_dlist_insert_forward(&http_context->send_data_list, &node->node);
    }
    if (body > http_context->header_buffer->data &&
        body < http_context->header_buffer->data + http_context->header_buffer->size) {
      cc_send_data_node_t *node = (cc_send_data_node_t *)cc_alloc(1, sizeof(cc_send_data_node_t));
      if (node == NULL) {
        error_log("cc_http_client_start_send_body, cc_alloc failed send_data_list has data, http_context=%x",
                  http_context);
        return -1;
      }
      node->offset = body - http_context->header_buffer->data;
      node->size = body + body_size - http_context->header_buffer->data;
      cc_buffer_assgin(&node->buffer, http_context->header_buffer);
      cc_dlist_insert_forward(&http_context->send_data_list, &node->node);
    } else {
      cc_send_data_node_t *node = (cc_send_data_node_t *)cc_alloc(1, sizeof(cc_send_data_node_t));
      if (node == NULL) {
        error_log("cc_http_client_start_send_body, cc_alloc failed send_data_list has data, http_context=%x",
                  http_context);
        return -1;
      }
      node->offset = body - http_context->body_buffer->data;
      node->size = body + body_size - http_context->body_buffer->data;
      cc_buffer_assgin(&node->buffer, http_context->body_buffer);
      cc_dlist_insert_forward(&http_context->send_data_list, &node->node);
    }
    {
      cc_send_data_node_t *node = (cc_send_data_node_t *)cc_alloc(1, sizeof(cc_send_data_node_t));
      if (node == NULL) {
        error_log("cc_http_client_start_send_body, cc_alloc failed send_data_list has data, http_context=%x",
                  http_context);
        return -1;
      }
      node->buffer = cc_buffer_create(8);
      if (node->buffer == NULL) {
        error_log("cc_http_client_start_send_chunked_body, cc_buffer_create failed, http_context=%x", http_context);
        return -1;
      }
      char *endp = cc_snprintf(node->buffer->data, node->buffer->capacity, "%V", &g_crlf);
      node->buffer->size = endp - node->buffer->data;
      node->offset = 0;
      node->size = node->buffer->size;
      cc_dlist_insert_forward(&http_context->send_data_list, &node->node);
    }
    if (socket_event->can_write) {
      cc_http_client_write(http_context->http_worker->worker.event_loop, http_context->client_fd, http_context, 0);
    }
  }
  return 0;
}

int cc_http_client_start_send_last_chunked(cc_http_context_t *http_context) {
  debug_log("cc_http_client_start_send_last_chunked, http_context=%x", http_context);
  http_context->send_done_proc = cc_http_client_send_chunked_body_done;
  cc_socket_event_t *socket_event = &http_context->http_worker->worker.event_loop->events[http_context->client_fd];
  cc_send_data_node_t *node = (cc_send_data_node_t *)cc_alloc(1, sizeof(cc_send_data_node_t));
  if (node == NULL) {
    error_log("cc_http_client_start_send_body, cc_alloc failed send_data_list has data, http_context=%x", http_context);
    return -1;
  }

  node->buffer = cc_buffer_create(8);
  if (node->buffer == NULL) {
    error_log("cc_http_client_start_send_last_chunked, cc_buffer_create failed, http_context=%x", http_context);
    return -1;
  }
  char *endp = cc_snprintf(node->buffer->data, node->buffer->capacity, "0%V%V", &g_crlf, &g_crlf);
  node->buffer->size = endp - node->buffer->data;
  node->flag = 1;
  node->offset = 0;
  node->size = node->buffer->size;
  cc_dlist_insert_forward(&http_context->send_data_list, &node->node);

  if (socket_event->can_write) {
    cc_http_client_write(http_context->http_worker->worker.event_loop, http_context->client_fd, http_context, 0);
  }
  return 0;
}

void cc_http_client_send_chunked_body_done(cc_http_context_t *http_context, ssize_t nbytes, int flag) {
  debug_log(
      "cc_http_client_send_chunked_body_done, http_context=%x, nbytes=%ld, content_length=%ld, connection=%ld, flag=%d",
      http_context, nbytes, http_context->http_reply.content_length, http_context->http_reply.connection, flag);
  if (nbytes < 0) {
    cc_http_client_http_context_close(http_context);
    return;
  }
  http_context->http_reply.bytes_sent += nbytes;
  int last_chunked = flag;
  if (last_chunked == 0) {
    debug_log(
        "cc_http_client_send_chunked_body_done, has data need send, http_context=%x, nbytes=%ld, bytes_sent=%ld, "
        "last_chunked=%d, header_size=%d",
        http_context, nbytes, http_context->http_reply.bytes_sent, last_chunked, http_context->http_reply.header_size);
    return;
  }
  debug_log(
      "cc_http_client_send_chunked_body_done, all data sent, http_context=%x, nbytes=%ld, bytes_sent=%ld, "
      "last_chunked=%d, header_size=%d, connection=%d",
      http_context, nbytes, http_context->http_reply.bytes_sent, last_chunked, http_context->http_reply.header_size,
      http_context->http_reply.connection);
  if (http_context->http_reply.connection == CONNCETION_CLOSE) {
    cc_http_client_http_context_close(http_context);
    return;
  }
  if (!cc_dlist_empty(&http_context->send_data_list)) {
    error_log(
        "cc_http_client_send_chunked_body_done, cc_http_client_http_context_close send_data_list has data, "
        "http_context=%x, "
        "nbytes=%ld, content_length=%ld, connection=%ld",
        http_context, nbytes, http_context->http_reply.content_length, http_context->http_reply.connection);
    cc_http_client_http_context_close(http_context);
    return;
  }
  cc_http_context_reset(http_context);
}

void cc_http_client_write(cc_event_loop_t *event_loop, int fd, void *client_data, int mask) {
  if (!(event_loop->events[fd].mask & CC_EVENT_WRITEABLE)) {
    debug_log("cc_http_client_write, ignore, event_loop=%x, fd=%d, client_data=%x, mask=%d", event_loop, fd,
              client_data, mask);
    return;
  }

  if (event_loop->events[fd].can_write == 0) {
    debug_log("cc_http_client_write, can not write, wait, event_loop=%x, fd=%d, client_data=%x, mask=%d", event_loop,
              fd, client_data, mask);
    return;
  }

  cc_http_context_t *http_context = (cc_http_context_t *)(client_data);
  debug_log("cc_http_client_write, event_loop=%x, fd=%d, http_context=%x, mask=%d", event_loop, fd, http_context, mask);

  cc_http_context_del_timer(http_context, &http_context->write_timer);

  if ((mask & CC_EVENT_CLOSE) || (mask & CC_EVENT_ERROR)) {
    int so_error = 0;
    if (mask & CC_EVENT_ERR) {
      so_error = cc_net_so_errno(fd);
    }
    cc_http_client_http_context_close(http_context);
    error_log("cc_http_client_write close, event_loop=%x, fd=%d, http_context=%x, mask=%d", event_loop, fd,
              http_context, mask);
    return;
  }

  if (cc_dlist_empty(&http_context->send_data_list)) {
    debug_log("cc_http_client_write done, empty send list, event_loop=%x, fd=%d, http_context=%x, mask=%d", event_loop,
              fd, http_context, mask);
    if (http_context->send_done_proc) {
      http_context->send_done_proc(http_context, 0, 0);
    }
    return;
  }
  struct iovec iov[MAX_IOV_NUM] = {0};
  int ret = 0;
  ssize_t nbytes = 0;
  ssize_t bytes_sent = 0;
  int flag = 0;
  do {
    size_t iov_num = 0;
    cc_dlist_node_t *node = http_context->send_data_list.next;
    while (node != &http_context->send_data_list && iov_num < MAX_IOV_NUM) {
      cc_send_data_node_t *send_data_node = cc_list_data(node, cc_send_data_node_t, node);
      assert(send_data_node->size != 0);
      ssize_t last_size = send_data_node->size - send_data_node->offset;
      if (bytes_sent >= last_size) {
        bytes_sent -= last_size;
        cc_buffer_free(send_data_node->buffer);
        cc_dlist_delete(&http_context->send_data_list, node);
        node = http_context->send_data_list.next;
      } else {
        flag = send_data_node->flag;
        send_data_node->offset += bytes_sent;
        iov[iov_num].iov_base = send_data_node->buffer->data + send_data_node->offset;
        iov[iov_num].iov_len = send_data_node->size - send_data_node->offset;
        debug_log("cc_http_client_write, iov_num=%d, iov_base=%x, iov_len=%d", iov_num, iov[iov_num].iov_base,
                  iov[iov_num].iov_len);
        ++iov_num;
        node = node->next;
      }
    }
    if (iov_num == 0) {
      debug_log("cc_http_client_write, data write done, event_loop=%x, fd=%d, http_context=%x, flag=%d, mask=%d",
                event_loop, fd, http_context, flag, mask);
      break;
    }
    do {
      ret = cc_net_writev(http_context->client_fd, iov, iov_num, &bytes_sent);
      if (ret == 0) {
        nbytes += bytes_sent;
        if (nbytes == 0) {
          // 特别注意下
          error_log(
              "cc_http_client_write waring 0 bytes return, ret=%d, bytes_sent=%ld, iov_num=%ld, event_loop=%x, "
              "fd=%d, "
              "http_context=%x, mask=%d",
              ret, bytes_sent, iov_num, event_loop, fd, http_context, mask);
        }
      }
    } while (ret == CC_NET_CONTINUE);
    if (ret != CC_NET_OK) {
      break;
    }
  } while (1);
  if (ret == CC_NET_OK || ret == CC_NET_AGAIN) {
    if (ret == CC_NET_AGAIN) {
      event_loop->events[fd].can_write = 0;
      if (!cc_dlist_empty(&http_context->send_data_list)) {
        cc_http_context_set_timer(http_context, &http_context->write_timer, cc_http_client_write_timeout,
                                  CONFIG_HTTP_WRITE_TIMEOUT_MSEC);
      }
    }
    if (cc_dlist_empty(&http_context->send_data_list)) {
      if (http_context->send_done_proc) {
        http_context->send_done_proc(http_context, nbytes, flag);
      }
    }
  } else {
    error_log(
        "cc_http_client_write cc_net_writev failed, event_loop=%x,  fd=%d, http_context=%x, mask=%d, errno=%d, "
        "strerror=%s",
        event_loop, fd, http_context, mask, cc_net_get_last_errno, cc_net_get_last_error);
    if (http_context->send_done_proc) {
      http_context->send_done_proc(http_context, -1, 0);
    }
  }
}

void cc_http_client_write_timeout(cc_event_loop_t *event_loop, void *client_data) {
  cc_http_context_t *http_context = (cc_http_context_t *)(client_data);
  info_log("cc_http_client_write_timeout, http_context=%x", http_context);
  cc_http_client_http_context_close(http_context);
}