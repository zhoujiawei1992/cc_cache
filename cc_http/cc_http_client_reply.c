#include "cc_http/cc_http_client_reply.h"
#include "cc_http/cc_http_client_request.h"
#include "cc_net/cc_net.h"
#include "cc_util/cc_config.h"
#include "cc_util/cc_snprintf.h"
#include "cc_util/cc_util.h"

#include <string.h>
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

  if (reply->content_length != CONTENT_LENGTH_NO_OUTPUT) {
    endp = cc_snprintf(endp, reply->header_buffer->data + reply->header_buffer->capacity - endp, "%V: %ld%V",
                       &g_content_length_header_field, reply->content_length, &g_crlf);
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
    return -1;
  }
  node->need_free = 1;
  node->offset = 0;
  node->size = http_context->http_reply.header_buffer->size;
  node->data = http_context->http_reply.header_buffer->data;
  cc_dlist_insert_forward(&http_context->send_data_list, &node->node);

  http_context->send_done_proc = cc_http_client_send_header_done;
  http_context->http_reply.header_buffer = NULL;

  cc_http_context_set_timer(http_context, &http_context->write_timer, cc_http_client_write_timeout,
                            CONFIG_HTTP_WRITE_TIMEOUT_MSEC);
  cc_http_client_write(http_context->http_worker->worker.event_loop, http_context->client_fd, http_context, 0);
  return 0;
}

void cc_http_client_send_header_done(cc_http_context_t *http_context, size_t nbytes) {
  debug_log("cc_http_client_send_header_done, http_context=%x, nbytes=%ld, content_length=%ld, connection=%d",
            http_context, nbytes, http_context->http_reply.content_length, http_context->http_reply.connection);
  if (nbytes < 0) {
    cc_http_client_http_context_close(http_context);
    return;
  }
  if (http_context->http_reply.content_length > 0) {
    cc_http_client_start_send_body(http_context);
    return;
  }
  if (http_context->http_reply.connection == CONNCETION_CLOSE) {
    cc_http_client_http_context_close(http_context);
    return;
  }
  if (!cc_dlist_empty(&http_context->send_data_list)) {
    error_log(
        "cc_http_client_send_header_done, cc_http_client_http_context_close send_data_list has data, http_context=%x, "
        "nbytes=%ld, content_length=%ld, connection=%d",
        http_context, nbytes, http_context->http_reply.content_length, http_context->http_reply.connection);
    cc_http_client_http_context_close(http_context);
    return;
  }
  cc_http_context_reset(http_context);
}

int cc_http_client_start_send_body(cc_http_context_t *http_context) { return 0; }

void cc_http_client_send_body_done(cc_http_context_t *http_context, size_t nbytes) {}
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
  }

  if (cc_dlist_empty(&http_context->send_data_list)) {
    debug_log("cc_http_client_write done, emppty send list, event_loop=%x, fd=%d, http_context=%x, mask=%d", event_loop,
              fd, http_context, mask);
    return;
  }

  struct iovec iov[32] = {0};
  int ret = 0;
  ssize_t nbytes = 0;
  ssize_t bytes_sent = 0;
  do {
    size_t iov_num = 0;
    cc_dlist_node_t *node = http_context->send_data_list.next;
    while (node != &http_context->send_data_list && iov_num < sizeof(iov)) {
      cc_send_data_node_t *send_data_node = cc_list_data(node, cc_send_data_node_t, node);
      ssize_t last_size = send_data_node->size - send_data_node->offset;
      if (bytes_sent >= last_size) {
        bytes_sent -= last_size;
        cc_dlist_delete(&http_context->send_data_list, node);
        node = http_context->send_data_list.next;
        if (send_data_node->need_free) {
          cc_free(send_data_node->data);
        }
        cc_free(send_data_node);
      } else {
        send_data_node->offset += bytes_sent;
        iov[iov_num].iov_base = send_data_node->data + send_data_node->offset;
        iov[iov_num].iov_len = send_data_node->size - send_data_node->offset;
        ++iov_num;
        node = node->next;
      }
    }
    if (iov_num == 0) {
      debug_log("cc_http_client_write, data write done, event_loop=%x, fd=%d, http_context=%x, mask=%d", event_loop, fd,
                http_context, mask);
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
    }
    if (cc_dlist_empty(&http_context->send_data_list)) {
      if (http_context->send_done_proc) {
        http_context->send_done_proc(http_context, nbytes);
      }
    } else {
      cc_http_context_set_timer(http_context, &http_context->write_timer, cc_http_client_write_timeout,
                                CONFIG_HTTP_WRITE_TIMEOUT_MSEC);
    }
  } else {
    error_log(
        "cc_http_client_write cc_net_writev failed, event_loop=%x,  fd=%d, http_context=%x, mask=%d, errno=%d, "
        "strerror=%s",
        event_loop, fd, http_context, mask, cc_net_get_last_errno, cc_net_get_last_error);
    if (http_context->send_done_proc) {
      http_context->send_done_proc(http_context, CC_NET_ERR);
    }
  }
}

void cc_http_client_write_timeout(cc_event_loop_t *event_loop, void *client_data) {
  cc_http_context_t *http_context = (cc_http_context_t *)(client_data);
  info_log("cc_http_client_write_timeout, http_context=%x", http_context);
  cc_http_client_http_context_close(http_context);
}