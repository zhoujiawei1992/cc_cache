#include "cc_http/cc_http_client_request.h"
#include "cc_net/cc_acceptor.h"
#include "cc_net/cc_net.h"
#include "cc_util/cc_cb_context.h"
#include "cc_util/cc_config.h"
#include "cc_util/cc_util.h"
void cc_http_client_read(cc_event_loop_t *event_loop, int fd, void *client_data, int mask) {
  if (!(event_loop->events[fd].mask & CC_EVENT_READABLE)) {
    info_log("cc_http_client_read, ignore, fd=%d", fd);
    return;
  }

  cc_cb_context_t *cb_context = (cc_cb_context_t *)(client_data);
  cc_http_context_t *http_context = (cc_http_context_t *)(cb_context->data);
  info_log("cc_http_client_read, fd=%d, mask=%d, http_context=%x", fd, mask, http_context);

  cc_http_context_del_timer(http_context, &http_context->read_timer);

  if ((mask & CC_EVENT_CLOSE) || (mask & CC_EVENT_ERROR)) {
    int so_error = 0;
    if (mask & CC_EVENT_ERR) {
      so_error = cc_net_so_errno(fd);
    }
    cc_http_client_http_context_stop_read(http_context, cb_context);
    cc_http_client_http_context_stop_write(http_context, cb_context);
    cc_http_context_del_timer(http_context, &http_context->write_timer);
    info_log("cc_http_client_read, close, fd=%d, mask=%d, http_context=%x, so_error=%d, strerror=%s", fd, mask,
             http_context, so_error, strerror(so_error));
  }

  do {
    void *buf = http_context->header_buffer->data + http_context->header_buffer->size;
    size_t buf_size = http_context->header_buffer->capacity - http_context->header_buffer->size;

    debug_log("cc_http_client_read, begin read, fd=%d", fd);
    if (buf_size > 0) {
      size_t nbytes = 0;
      int ret = cc_net_read(fd, buf, buf_size, &nbytes);
      debug_log("cc_http_client_read, end read, fd=%d, ret=%d", fd, ret);
      if (ret == CC_EVENT_OK) {
        http_context->header_buffer->size += nbytes;
        cc_string_t temp;
        temp.buf = buf;
        temp.len = nbytes;
        info_log("cc_http_client_read, buf=%V", &temp);

      } else if (ret == CC_NET_ERR || ret == CC_NET_CLOSE) {
        cc_http_client_http_context_stop_read(http_context, cb_context);
        cc_http_client_http_context_stop_write(http_context, cb_context);
        cc_http_context_del_timer(http_context, &http_context->write_timer);
        error_log("cc_http_client_read, close, buf not size, fd=%d, http_context=%x, buf=%x, buf_size=%d", fd,
                  http_context, buf, buf_size);
        break;
      } else {
        // ret == CC_NET_AGAIN
        debug_log("cc_http_client_read, read no more, fd=%d, ret=%d", fd, ret);
        break;
      }
    } else {
      cc_http_client_http_context_stop_read(http_context, cb_context);
      cc_http_client_http_context_stop_write(http_context, cb_context);
      cc_http_context_del_timer(http_context, &http_context->write_timer);
      info_log("cc_http_client_read, close, buf not size, fd=%d, http_context=%x, buf=%x, buf_size=%d", fd,
               http_context, buf, buf_size);
      break;
    }
  } while (1);

  cc_http_context_set_timer(http_context, cb_context, &http_context->read_timer, cc_http_client_read_timeout,
                            CONFIG_HTTP_READ_TIMEOUT_MSEC);
}

void cc_http_client_read_timeout(cc_event_loop_t *event_loop, void *client_data) {
  cc_cb_context_t *cb_context = (cc_cb_context_t *)(client_data);
  cc_http_context_t *http_context = (cc_http_context_t *)(cb_context->data);
  cc_http_client_http_context_stop_read(http_context, cb_context);
  cc_http_client_http_context_stop_write(http_context, cb_context);
  cc_http_context_del_timer(http_context, &http_context->write_timer);
  info_log("cc_http_client_read_timeout, http_context=%x", http_context);
  cc_cb_context_unlink(cb_context);
}

void cc_http_client_write(cc_event_loop_t *event_loop, int fd, void *client_data, int mask) {}

void cc_http_client_write_timeout(cc_event_loop_t *event_loop, void *client_data) {}

cc_http_context_t *cc_inner_http_context_create(cc_http_worker_t *http_worker, const char *func, int line) {
  cc_http_context_t *http_context = cc_inner_alloc(1, sizeof(cc_http_context_t), func, line);
  if (http_context == NULL) {
    return NULL;
  }
  http_context->start_read = 0;
  http_context->start_write = 0;
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
  cc_slist_init(&http_context->send_data_list_head);
  return http_context;
}
void cc_inner_http_context_free(cc_http_context_t *http_context, const char *func, int line) {
  if (http_context->client_fd > 0) {
    cc_del_socket_event(http_context->http_worker->worker.event_loop, http_context->client_fd,
                        CC_EVENT_READABLE | CC_EVENT_WRITEABLE);
    cc_net_close(http_context->client_fd);
  }

  if (http_context->http_request) {
    cc_inner_http_request_free(http_context->http_request, func, line);
  }
  if (http_context->header_buffer) {
    cc_buffer_free(http_context->header_buffer);
  }
  if (http_context->body_buffer) {
    cc_buffer_free(http_context->body_buffer);
  }
  cc_inner_free(http_context, func, line);
}

cc_http_request_t *cc_inner_http_request_create(cc_http_context_t *http_context, const char *func, int line) {
  cc_http_request_t *http_request = (cc_http_request_t *)cc_inner_alloc(1, sizeof(cc_http_request_t), func, line);
  http_request->http_context = http_context;
  return http_request;
}
void cc_inner_http_request_free(cc_http_request_t *http_request, const char *func, int line) {
  cc_inner_free(http_request, func, line);
}

void cc_http_context_set_timer(cc_http_context_t *http_context, cc_cb_context_t *cb_context, cc_time_event_t *timer,
                               cc_time_proc_t *proc, int timeout) {
  cc_event_loop_t *event_loop = http_context->http_worker->worker.event_loop;
  timer->node.key = event_loop->current_msec + timeout;
  timer->timer_proc = proc;
  timer->client_data = cb_context;
  cc_cb_context_link(cb_context);
  cc_add_time_event(event_loop, timer);
  debug_log("cc_http_context_set_timer, http_context=%x, timer=%x", http_context, timer);
}
void cc_http_context_del_timer(cc_http_context_t *http_context, cc_time_event_t *timer) {
  if (timer->node.key > 0) {
    cc_event_loop_t *event_loop = http_context->http_worker->worker.event_loop;
    cc_del_time_event(event_loop, timer);
    cc_cb_context_t *cb_context = (cc_cb_context_t *)timer->client_data;
    cc_cb_context_unlink(cb_context);
  }
  debug_log("cc_http_context_del_timer, http_context=%x, timer=%x", http_context, timer);
}

void cc_http_client_accept(cc_event_loop_t *event_loop, int fd, void *client_data, int mask) {
  cc_tcp_acceptor_t *acceptor = (cc_tcp_acceptor_t *)client_data;
  do {
    char addr_buf[INET_ADDRSTRLEN] = {0};
    int client_fd = 0;
    int err = cc_net_accept(acceptor->listen_fd, &client_fd, addr_buf, sizeof(addr_buf));
    if (err == CC_NET_AGAIN) {
      event_loop->events[fd].can_read = 0;
      debug_log("cc_http_client_accept, cc_net_accept no more, server_fd=%d, errno=%d, err=%s", acceptor->listen_fd,
                cc_net_get_last_errno, cc_net_get_last_error);
      break;
    }
    if (err == CC_NET_ERR) {
      // TODO 需要研究什么情况下 accept会失败，且处理方式是啥
      error_log("cc_http_client_accept, cc_net_accept failed, server_fd=%d, errno=%d, err=%s", acceptor->listen_fd,
                cc_net_get_last_errno, cc_net_get_last_error);
      break;
    }
    if (err == CC_NET_OK) {
      int ret = cc_net_non_block(client_fd);
      if (ret != CC_NET_OK) {
        error_log("cc_http_client_accept cc_net_non_block failed, server_fd=%d, client_fd=%d", acceptor->listen_fd,
                  client_fd);
        cc_net_close(client_fd);
        continue;
      }

      debug_log("cc_http_client_accept, server_fd=%d, client_fd=%d, addr=%s", acceptor->listen_fd, client_fd, addr_buf);
      cc_http_context_t *http_context = cc_http_context_create((cc_http_worker_t *)acceptor->worker);
      if (http_context == NULL) {
        error_log("cc_http_client_accept failed, server_fd=%d, client_fd=%d, http_context=%x", acceptor->listen_fd,
                  client_fd, http_context);
        cc_net_close(client_fd);
        continue;
      }

      http_context->http_state = HTTP_STATE_REQUEST_WAIT;
      http_context->client_fd = client_fd;
      memcpy(http_context->client_addr, addr_buf, INET_ADDRSTRLEN);

      if (cc_http_client_start_read(http_context) != 0) {
        error_log("cc_http_client_accept failed, server_fd=%d, client_fd=%d, http_context=%x", acceptor->listen_fd,
                  client_fd, http_context);
        cc_http_context_free(http_context);
        cc_net_close(client_fd);
        continue;
      }
    }
  } while (1);
  return;
}

void cc_http_client_connect_done(cc_event_loop_t *event_loop, int fd, void *client_data, int mask) {}

int cc_http_client_start_read(cc_http_context_t *http_context) {
  cc_cb_context_t *cb_context = cc_cb_context_create(http_context, cc_inner_http_context_free);
  if (cb_context == NULL) {
    error_log("cc_http_client_start_read cc_cb_context_create, failed,http_context=%x", http_context);
    return CC_EVENT_ERR;
  }
  if (cc_set_socket_event(http_context->http_worker->worker.event_loop, http_context->client_fd, SS_CONNECTED,
                          CC_EVENT_READABLE | CC_EVENT_CLOSE | CC_EVENT_ERROR, cc_http_client_read, cb_context, NULL,
                          NULL) != CC_EVENT_OK) {
    error_log("cc_http_client_start_read cc_set_socket_event, failed, http_context=%x", http_context);
    cc_cb_context_free(cb_context);
  }
  http_context->start_read = 1;
  cc_http_context_set_timer(http_context, cb_context, &http_context->read_timer, cc_http_client_read_timeout,
                            CONFIG_HTTP_READ_TIMEOUT_MSEC);

  return 0;
}
int cc_http_client_start_write(cc_http_context_t *http_context) { return 0; }

void cc_http_client_http_context_stop_read(cc_http_context_t *http_context, cc_cb_context_t *cb_context) {
  if (http_context->start_read) {
    if (cc_del_socket_event(http_context->http_worker->worker.event_loop, http_context->client_fd, CC_EVENT_READABLE) ==
        CC_EVENT_OK) {
      cc_cb_context_unlink(cb_context);
      http_context->start_read = 0;
    }
  }
}
void cc_http_client_http_context_stop_write(cc_http_context_t *http_context, cc_cb_context_t *cb_context) {
  if (http_context->start_write) {
    if (cc_del_socket_event(http_context->http_worker->worker.event_loop, http_context->client_fd,
                            CC_EVENT_WRITEABLE) == CC_EVENT_OK) {
      cc_cb_context_unlink(cb_context);
      http_context->start_write = 0;
    }
  }
}