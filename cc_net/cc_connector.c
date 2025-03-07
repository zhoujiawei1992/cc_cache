#include "cc_net/cc_connector.h"
#include "cc_net/cc_net.h"
#include "cc_util/cc_util.h"
cc_tcp_connector_t* cc_tcp_connector_create(cc_worker_t* worker, const char* ip, int port,
                                            cc_socket_proc_t* connect_done, int timeout) {
  cc_tcp_connector_t* connector = (cc_tcp_connector_t*)cc_alloc(1, sizeof(cc_tcp_connector_t));
  if (connector == NULL) {
    error_log("cc_tcp_connector_create, failed, worker=%x", worker);
    return NULL;
  }
  connector->worker = worker;
  connector->state = CONNECTOR_STATE_CLOSED;
  connector->connect_done = connect_done;

  int ret = cc_net_tcp_connect(ip, port, &connector->remote_fd);
  if (ret == CC_NET_ERR) {
    error_log("cc_tcp_connector_create, cc_net_tcp_connect, failed, errno=%d, strerror=%s", cc_net_get_last_errno,
              cc_net_get_last_error);
    cc_tcp_connector_free(connector);
    return NULL;
  }
  if (ret == CC_NET_OK) {
    connector->state = CONNECTOR_STATE_CONNECTED;
    if (connector->connect_done) {
      connector->connect_done(connector->worker->event_loop, connector->remote_fd, connector, 0);
    }
    return 0;
  }
  connector->state = CONNECTOR_STATE_CONNECTING;

  if (cc_set_socket_event(worker->event_loop, connector->remote_fd, SS_CONNECTING,
                          CC_EVENT_WRITEABLE | CC_EVENT_ERROR | CC_EVENT_CLOSE, NULL, NULL,
                          cc_tcp_connector_connect_done, connector) != CC_EVENT_OK) {
    error_log("cc_tcp_connector_create, cc_set_socket_event failed, connector=%x, errno: %d, error: %s", connector,
              cc_net_get_last_errno, cc_net_get_last_error);
    cc_tcp_connector_free(connector);
    return NULL;
  }
  connector->connect_timer.node.key = worker->event_loop->current_msec + timeout;
  connector->connect_timer.timer_proc = cc_tcp_connector_connect_timeout;
  connector->connect_timer.client_data = connector;
  cc_add_time_event(worker->event_loop, &connector->connect_timer);
  debug_log("cc_tcp_connector_create done, connector=%x", connector);
  return connector;
}

void cc_tcp_connector_free(cc_tcp_connector_t* connector) {
  debug_log("cc_tcp_connector_free done, connector=%x, state=%d", connector, connector->state);
  if (connector->state == CONNECTOR_STATE_CLOSED) {
    cc_free(connector);
  } else {
    cc_tcp_connector_close(connector);
    cc_free(connector);
  }
}

void cc_tcp_connector_close(cc_tcp_connector_t* connector) {
  debug_log("cc_tcp_connector_free done, connector=%x, state=%d, fd=%d", connector, connector->state,
            connector->remote_fd);
  if (connector->remote_fd > 0) {
    cc_del_socket_event(connector->worker->event_loop, connector->remote_fd, CC_EVENT_WRITEABLE);
    cc_net_close(connector->remote_fd);
    connector->remote_fd = 0;
    connector->state = CONNECTOR_STATE_CLOSED;
    return;
  }
}
void cc_tcp_connector_connect_done(cc_event_loop_t* event_loop, int fd, void* client_data, int mask) {
  if (!(event_loop->events[fd].mask & CC_EVENT_WRITEABLE)) {
    debug_log("cc_tcp_connector_connect_done ignore, event_loop=%x, fd=%d, client_data=%x, mask=%d", event_loop, fd,
              client_data, mask);
    return;
  }
  debug_log("cc_tcp_connector_connect_done, event_loop=%x, fd=%d, client_data=%x, mask=%d", event_loop, fd, client_data,
            mask);
  cc_tcp_connector_t* connector = (cc_tcp_connector_t*)client_data;

  cc_del_time_event(event_loop, &connector->connect_timer);

  int so_error = cc_net_so_errno(fd);
  if (!so_error) {
    connector->state = CONNECTOR_STATE_CONNECTED;
  }
  if (connector->connect_done) {
    connector->connect_done(event_loop, fd, client_data, so_error);
  }
}

void cc_tcp_connector_connect_timeout(cc_event_loop_t* event_loop, void* client_data) {
  cc_tcp_connector_t* connector = (cc_tcp_connector_t*)client_data;
  if (cc_del_socket_event(event_loop, connector->remote_fd, CC_EVENT_WRITEABLE) != CC_EVENT_OK) {
    cc_tcp_connector_free(connector);
    error_log("cc_tcp_connector_connect_timeout: cc_del_socket_event failed, connector=%x", connector);
  }
  connector->state = CONNECTOR_STATE_CLOSED;
  if (connector->connect_done) {
    connector->connect_done(event_loop, connector->remote_fd, client_data, -1);
  }
}