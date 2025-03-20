#include "cc_http/cc_http_client_request.h"
#include "cc_net/cc_acceptor.h"
#include "cc_net/cc_net.h"
#include "cc_util/cc_config.h"
#include "cc_util/cc_snprintf.h"
#include "cc_util/cc_util.h"

cc_string_t g_crlf = {"\r\n", 2};
cc_string_t g_server_header = {"Server: cc_cache/1.0.0.0", 24};
cc_string_t g_connection_close_header = {"Connection: close", 17};
cc_string_t g_connection_keepalive = {"Connection: keep-alive", 22};
cc_string_t g_pragma_no_cache_header = {"Pragma: no-cache", 16};
cc_string_t g_cache_control_no_cache_header = {
    "Cache-Control: private, no-cache, no-store, proxy-revalidate, no-transform", 74};
cc_string_t g_transfer_encoding_chunked_header = {"Transfer-Encoding: chunked", 26};
cc_string_t g_cache_control_cache_header_prefix = {"Cache-Control: max-age=", 23};

cc_string_t g_transfer_encoding_chunked_field = {"Transfer-Encoding", 17};
cc_string_t g_transfer_encoding_chunked_value = {"chunked", 7};

cc_string_t g_content_length_header_field = {"Content-Length", 14};
cc_string_t g_date_header_field = {"Date", 4};
cc_string_t g_connection_header_field = {"Connection", 10};
cc_string_t g_cache_control_header_field = {"Cache-Control", 13};
cc_string_t g_server_header_field = {"Server", 6};
cc_string_t g_expire_header_field = {"Expires", 7};

extern int cc_http_client_execute_parse(cc_http_context_t *http_context, const char *buffer, unsigned int length,
                                        size_t *nbytes);
void cc_http_client_read(cc_event_loop_t *event_loop, int fd, void *client_data, int mask) {
  if (!(event_loop->events[fd].mask & CC_EVENT_READABLE)) {
    debug_log("cc_http_client_read, ignore, event_loop=%x, fd=%d, client_data=%x, mask=%d", event_loop, fd, client_data,
              mask);
    return;
  }
  cc_http_context_t *http_context = (cc_http_context_t *)(client_data);
  debug_log("cc_http_client_read, event_loop=%x, fd=%d, http_context=%x, mask=%d", event_loop, fd, http_context, mask);

  cc_http_context_del_timer(http_context, &http_context->read_timer);

  if ((mask & CC_EVENT_CLOSE) || (mask & CC_EVENT_ERROR)) {
    int so_error = 0;
    if (mask & CC_EVENT_ERR) {
      so_error = cc_net_so_errno(fd);
    }
    cc_http_client_http_context_close(http_context);
    error_log("cc_http_client_read close, event_loop=%x, fd=%d, http_context=%x, mask=%d", event_loop, fd, http_context,
              mask);
    return;
  }

  do {
    cc_buffer_t *recv_buffer = (http_context->http_state < HTTP_STATE_REQUEST_HEADER_DONE ? http_context->header_buffer
                                                                                          : http_context->body_buffer);
    if (recv_buffer->capacity <= recv_buffer->size) {
      if (http_context->http_state < HTTP_STATE_REQUEST_HEADER_DONE) {
        cc_http_client_http_context_close(http_context);
        error_log(
            "cc_http_client_read, has not header buffer last, recv_buffer=%x, capacity=%ud, size=%ud, event_loop=%x, "
            "fd=%d, "
            "http_context=%x, mask=%d",
            recv_buffer, recv_buffer->capacity, recv_buffer->size, event_loop, fd, http_context, mask);
        break;
      } else {
        debug_log(
            "cc_http_client_read, body buffer is full, recv_buffer=%x, capacity=%ud, size=%ud, "
            "event_loop=%x, fd=%d, "
            "http_context=%x, mask=%d",
            recv_buffer, recv_buffer->capacity, recv_buffer->size, event_loop, fd, http_context, mask);
        break;
      }
    }
    const char *buffer = recv_buffer->data + recv_buffer->size;
    size_t length = 0;
    do {
      size_t nbytes = 0;
      int ret =
          cc_net_read(fd, recv_buffer->data + recv_buffer->size, recv_buffer->capacity - recv_buffer->size, &nbytes);
      if (ret == CC_EVENT_OK) {
        length += nbytes;
        recv_buffer->size += nbytes;
        http_context->total_bytes_recv += nbytes;
      } else if (ret == CC_NET_ERR || ret == CC_NET_CLOSE) {
        error_log("cc_http_client_read close, read data failed, ret=%d, event_loop=%x, fd=%d, http_context=%x, mask=%d",
                  ret, event_loop, fd, http_context, mask);
        cc_http_client_http_context_close(http_context);
        return;
      } else {
        event_loop->events[fd].can_read = 0;
        if (http_context->http_state < HTTP_STATE_REQUEST_HEADER_DONE ||
            (http_context->http_request.content_length > 0 &&
             http_context->http_state < HTTP_STATE_REQUEST_BODY_DONE)) {
          cc_http_context_set_timer(http_context, &http_context->read_timer, cc_http_client_read_timeout,
                                    CONFIG_HTTP_READ_TIMEOUT_MSEC);
        }
        debug_log("cc_http_client_read, read data no more, ret=%d, event_loop=%x, fd=%d, http_context=%x, mask=%d", ret,
                  event_loop, fd, http_context, mask);
        break;
      }
    } while (recv_buffer->capacity > recv_buffer->size);

    if (length > 0) {
      size_t nbytes = 0;
      if (cc_http_client_request_do_parse(http_context, recv_buffer->data + recv_buffer->offset,
                                          recv_buffer->size - recv_buffer->offset, &nbytes) != 0) {
        error_log("cc_http_client_request_do_parse failed, nbytes=%uz", nbytes);
        break;
      }
      if (event_loop->events[fd].can_read != 0) {
        recv_buffer->offset += nbytes;
        if (recv_buffer->data == http_context->body_buffer->data) {
          if (recv_buffer->size == recv_buffer->capacity) {
            cc_buffer_free(http_context->body_buffer);
            http_context->body_buffer = cc_buffer_create(CONFIG_HTTP_BODY_BUFFER_SIZE);
            if (http_context->body_buffer == NULL) {
              error_log("cc_http_client_read, cc_buffer_create failed, http_context=%x", http_context);
              break;
            }
            if (recv_buffer->offset < recv_buffer->size) {
              assert(0);
            }
          }
        }
      }
    }
  } while (event_loop->events[fd].can_read != 0);
}

void cc_http_client_read_timeout(cc_event_loop_t *event_loop, void *client_data) {
  cc_http_context_t *http_context = (cc_http_context_t *)(client_data);
  info_log("cc_http_client_read_timeout, http_context=%x", http_context);
  cc_http_client_http_context_close(http_context);
}

cc_http_context_t *cc_inner_http_context_create(cc_http_worker_t *http_worker, const char *func, int line) {
  cc_http_context_t *http_context = cc_inner_alloc(1, sizeof(cc_http_context_t), func, line);
  if (http_context == NULL) {
    return NULL;
  }
  http_context->http_worker = http_worker;
  http_context->header_buffer = cc_buffer_create(CONFIG_HTTP_HEADER_BUFFER_SIZE);
  if (http_context->header_buffer == NULL) {
    cc_inner_http_context_free(http_context, func, line);
    return NULL;
  }
  http_context->body_buffer = cc_buffer_create(CONFIG_HTTP_BODY_BUFFER_SIZE);
  if (http_context->body_buffer == NULL) {
    cc_inner_http_context_free(http_context, func, line);
    return NULL;
  }
  cc_dlist_init(&http_context->send_data_list);

  http_context->total_bytes_recv = 0;
  http_context->total_bytes_sent = 0;
  return http_context;
}
void cc_inner_http_context_free(cc_http_context_t *http_context, const char *func, int line) {
  if (http_context->header_buffer) {
    cc_buffer_free(http_context->header_buffer);
    http_context->header_buffer = NULL;
  }
  if (http_context->body_buffer) {
    cc_buffer_free(http_context->body_buffer);
    http_context->body_buffer = NULL;
  }

  cc_http_client_clear_request(http_context, &http_context->http_request);

  cc_dlist_node_t *node = http_context->send_data_list.next;
  while (node != &http_context->send_data_list) {
    cc_dlist_delete(&http_context->send_data_list, node);
    cc_send_data_node_t *send_data_node = cc_list_data(node, cc_send_data_node_t, node);
    cc_buffer_free(send_data_node->buffer);
    cc_free(send_data_node);
    node = http_context->send_data_list.next;
  }
  cc_inner_free(http_context, func, line);
}

void cc_http_context_reset(cc_http_context_t *http_context) {
  debug_log("cc_http_context_reset, http_context=%x, fd=%d", http_context, http_context->client_fd);
  cc_http_client_reset_request(http_context, &http_context->http_request);
  cc_http_client_reset_reply(http_context, &http_context->http_reply);
  cc_http_context_del_timer(http_context, &http_context->write_timer);
  cc_http_context_set_timer(http_context, &http_context->read_timer, cc_http_client_read_timeout,
                            CONFIG_HTTP_READ_TIMEOUT_MSEC);
  http_context->http_state = HTTP_STATE_REQUEST_WAIT;
  http_context->send_done_proc = NULL;
  http_context->header_buffer->size = 0;
  http_context->body_buffer->size = 0;
}

void cc_http_client_clear_request(cc_http_context_t *http_context, cc_http_request_t *request) {
  cc_slist_node_t *node = http_context->http_request.header_list.next;
  while (node != NULL) {
    http_context->http_request.header_list.next = node->next;
    cc_http_header_t *header = cc_list_data(node, cc_http_header_t, node);
    cc_free(header);
    node = http_context->http_request.header_list.next;
  }
}

void cc_http_client_reset_request(cc_http_context_t *http_context, cc_http_request_t *request) {
  cc_http_client_clear_request(http_context, &http_context->http_request);
  memset(request, 0, sizeof(cc_http_request_t));
  cc_slist_init(&http_context->http_request.header_list);
}

void cc_http_context_set_timer(cc_http_context_t *http_context, cc_time_event_t *timer, cc_time_proc_t *proc,
                               int timeout) {
  // 防止设置两个定时器
  cc_http_context_del_timer(http_context, timer);

  cc_event_loop_t *event_loop = http_context->http_worker->worker.event_loop;
  timer->node.key = event_loop->current_msec + timeout;
  timer->timer_proc = proc;
  timer->client_data = http_context;
  cc_add_time_event(event_loop, timer);
  debug_log("cc_http_context_set_timer, http_context=%x, timer=%x", http_context, timer);
}
void cc_http_context_del_timer(cc_http_context_t *http_context, cc_time_event_t *timer) {
  if (timer->node.key > 0) {
    cc_event_loop_t *event_loop = http_context->http_worker->worker.event_loop;
    cc_del_time_event(event_loop, timer);
    debug_log("cc_http_context_del_timer, http_context=%x, timer=%x", http_context, timer);
  }
}

void cc_http_client_accept(cc_event_loop_t *event_loop, int fd, void *client_data, int mask) {
  cc_tcp_acceptor_t *acceptor = (cc_tcp_acceptor_t *)client_data;
  do {
    char addr_buf[INET_ADDRSTRLEN] = {0};
    int client_fd = 0;
    int ret = cc_net_accept(fd, &client_fd, addr_buf, sizeof(addr_buf));
    if (ret == CC_NET_AGAIN) {
      event_loop->events[fd].can_read = 0;
      debug_log(
          "cc_http_client_accept, cc_net_accept no more, event_loop=%x, fd=%d, client_data=%x, mask=%d, errno=%d, "
          "error=%s",
          event_loop, fd, client_data, mask, cc_net_get_last_errno, cc_net_get_last_error);
      break;
    }
    if (ret == CC_NET_ERR) {
      error_log(
          "cc_http_client_accept, cc_http_client_accept failed, ret=%d, event_loop=%x, fd=%d, client_data=%x, mask=%d, "
          "errno=%d, error=%s",
          ret, event_loop, fd, client_data, mask, cc_net_get_last_errno, cc_net_get_last_error);
      break;
    }
    if (ret == CC_NET_OK) {
      ret = cc_net_non_block(client_fd);
      if (ret != CC_NET_OK) {
        error_log(
            "cc_http_client_accept, cc_net_non_block failed, ret=%d, event_loop=%x, fd=%d, client_data=%x, mask=%d, "
            "errno=%d, error=%s",
            ret, event_loop, fd, client_data, mask, cc_net_get_last_errno, cc_net_get_last_error);
        cc_net_close(client_fd);
        continue;
      }

      debug_log("cc_http_client_accept, client_fd=%d, addr=%s, event_loop=%x, fd=%d, client_data=%x, mask=%d",
                client_fd, addr_buf, event_loop, fd, client_data, mask);

      cc_http_context_t *http_context = cc_http_context_create((cc_http_worker_t *)acceptor->worker);
      if (http_context == NULL) {
        error_log("cc_http_client_accept, cc_http_context_create failed, event_loop=%x, fd=%d, client_data=%x, mask=%d",
                  event_loop, fd, client_data, mask);
        cc_net_close(client_fd);
        continue;
      }

      http_context->http_state = HTTP_STATE_REQUEST_WAIT;
      http_context->client_fd = client_fd;
      memcpy(http_context->client_addr, addr_buf, INET_ADDRSTRLEN);

      if (acceptor->worker->type == CC_HTTP_WORKER) {
        http_context->on_header_done_proc = cc_http_client_request_parse_on_header_done;
        http_context->on_body_proc = cc_http_client_request_parse_on_body;
        http_context->on_chunk_header = NULL;
        http_context->on_chunk_complete = NULL;
        http_context->on_message_done_proc = cc_http_client_request_parse_on_message_done;
      } else {
        http_context->on_header_done_proc = cc_http_client_admin_request_parse_on_header_done;
        http_context->on_body_proc = NULL;
        http_context->on_chunk_header = NULL;
        http_context->on_chunk_complete = NULL;
        http_context->on_message_done_proc = cc_http_client_admin_request_parse_on_message_done;
      }

      if (cc_http_client_start_read_and_write(http_context) != 0) {
        error_log(
            "cc_http_client_accept, cc_http_client_start_read_and_write failed, client_fd=%d, addr=%s, event_loop=%x, "
            "fd=%d, "
            "client_data=%x, mask=%d, errno=%d, error=%s",
            client_fd, addr_buf, event_loop, fd, client_data, mask, cc_net_get_last_errno, cc_net_get_last_error);
        cc_http_context_free(http_context);
        cc_net_close(client_fd);
        continue;
      }
    }
  } while (1);
  return;
}

void cc_http_client_connect_done(cc_event_loop_t *event_loop, int fd, void *client_data, int mask) {}

int cc_http_client_start_read_and_write(cc_http_context_t *http_context) {
  if (cc_set_socket_event(http_context->http_worker->worker.event_loop, http_context->client_fd, SS_CONNECTED,
                          CC_EVENT_READABLE | CC_EVENT_WRITEABLE | CC_EVENT_CLOSE | CC_EVENT_ERROR, cc_http_client_read,
                          http_context, cc_http_client_write, http_context) != CC_EVENT_OK) {
    error_log("cc_http_client_start_read_and_write cc_set_socket_event, failed, http_context=%x", http_context);
  }
  cc_http_context_set_timer(http_context, &http_context->read_timer, cc_http_client_read_timeout,
                            CONFIG_HTTP_READ_TIMEOUT_MSEC);

  return 0;
}

void cc_inner_http_client_http_context_close(cc_http_context_t *http_context, const char *func, int line) {
  cc_http_client_reset_request(http_context, &http_context->http_request);
  cc_http_client_reset_reply(http_context, &http_context->http_reply);

  cc_http_context_del_timer(http_context, &http_context->read_timer);
  cc_http_context_del_timer(http_context, &http_context->write_timer);

  if (http_context->client_fd > 0) {
    if (cc_del_socket_event(http_context->http_worker->worker.event_loop, http_context->client_fd,
                            CC_EVENT_READABLE | CC_EVENT_WRITEABLE)) {
      error_log("cc_http_client_http_context_close cc_del_socket_event, failed, http_context=%x, fd=%d, delmask=%d",
                http_context, http_context->client_fd);
    }
    cc_net_close(http_context->client_fd);
  }
  debug_log("cc_http_client_http_context_close[%s:%d], http_context=%x", func, line, http_context);
  cc_http_context_free(http_context);
}

int cc_http_client_request_do_parse(cc_http_context_t *http_context, const char *buffer, unsigned int length,
                                    size_t *nbytes) {
  if (http_context->http_state == HTTP_STATE_REQUEST_WAIT ||
      http_context->http_state == HTTP_STATE_REQUEST_HEADER_WAIT ||
      (http_context->http_state == HTTP_STATE_REQUEST_HEADER_DONE &&
       (http_context->http_request.content_length > 0 || http_context->http_request.chunk_header))) {
    HttpState state = http_context->http_state;
    if (cc_http_client_execute_parse(http_context, buffer, length, nbytes)) {
      error_log("cc_http_client_request_do_parse cc_http_client_execute_parse, failed, http_context=%x", http_context);
      if (http_context->http_reply.status > 0) {
        cc_http_client_http_context_close(http_context);
      } else {
        cc_http_client_send_error_page(http_context, HTTP_STATUS_BAD_REQUEST);
      }
    } else {
      debug_log("cc_http_client_request_do_parse done, nbytes=%uz", *nbytes);
      return 0;
    }
  } else {
    error_log("cc_http_client_request_do_parse error state, failed, http_context=%x, http_state=%d, content_length=%d",
              http_context, http_context->http_state, http_context->http_parser.content_length);
    if (http_context->http_reply.status > 0) {
      cc_http_client_http_context_close(http_context);
    } else {
      cc_http_client_send_error_page(http_context, HTTP_STATUS_BAD_REQUEST);
    }
  }
  return -1;
}

void cc_http_client_request_parse_on_message_done(cc_http_context_t *http_context) {
  cc_http_context_del_timer(http_context, &http_context->read_timer);
  if (http_context->http_request.chunk_header) {
    if (cc_http_client_start_send_last_chunked(http_context) != 0) {
      cc_http_client_http_context_close(http_context);
      return;
    }
  }
}

void cc_http_client_request_parse_on_header_done(cc_http_context_t *http_context) {
  if (http_context->http_request.content_length > 0 || http_context->http_request.chunk_header) {
    cc_http_reply_t *reply = &http_context->http_reply;
    reply->status = HTTP_STATUS_OK;
    reply->connection = http_should_keep_alive(&http_context->http_parser) ? CONNCETION_KEEPALIVE : CONNCETION_CLOSE;
    reply->cache_control = CACHE_CONTROL_NO_CACHE;
    if (http_context->http_request.chunk_header) {
      http_context->http_reply.chunk_header = 1;
    } else {
      reply->content_length = http_context->http_request.content_length;
    }
    cc_http_client_send_reply(http_context);
  } else {
    cc_http_client_send_error_page(http_context, HTTP_STATUS_OK);
  }
}
void cc_http_client_request_parse_on_body(cc_http_context_t *http_context, const char *buffer, unsigned int length) {
  debug_log("cc_http_client_request_parse_on_body, http_context=%x, buffer=%x, length=%d", http_context, buffer,
            length);
  if (http_context->http_request.chunk_header) {
    if (cc_http_client_start_send_chunked_body(http_context, buffer, length) != 0) {
      cc_http_client_http_context_close(http_context);
      return;
    }
  } else {
    if (cc_http_client_start_send_body(http_context, buffer, length) != 0) {
      cc_http_client_http_context_close(http_context);
      return;
    }
  }
}

void cc_http_client_admin_request_parse_on_header_done(cc_http_context_t *http_context) {
  cc_http_reply_t *reply = &http_context->http_reply;
  reply->status = HTTP_STATUS_OK;
  reply->connection = CONNCETION_CLOSE;
  reply->cache_control = CACHE_CONTROL_NO_CACHE;
  http_context->http_reply.chunk_header = 1;
  cc_http_client_send_reply(http_context);
}

void cc_http_client_admin_request_parse_on_message_done(cc_http_context_t *http_context) {
  cc_http_reply_t *reply = &http_context->http_reply;
  reply->body_buffer = cc_buffer_create(CONFIG_ADMIN_HTTP_BODY_BUFFER_SIZE);
#ifdef CC_MEMCHECK
  memcheck_show(reply->body_buffer->data, reply->body_buffer->capacity, &reply->body_buffer->size);
#else
  char *endp =
      cc_snprintf(reply->body_buffer->data, reply->body_buffer->capacity, "Not Support, need enable CC_MEMCHECK\n");
  reply->body_buffer->size = endp - reply->body_buffer->data;
#endif

  if (cc_http_client_start_send_chunked_body(http_context, reply->body_buffer->data, reply->body_buffer->size) != 0) {
    cc_http_client_http_context_close(http_context);
    return;
  }
  if (cc_http_client_start_send_last_chunked(http_context) != 0) {
    cc_http_client_http_context_close(http_context);
    return;
  }
}