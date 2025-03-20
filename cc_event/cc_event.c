#include "cc_event/cc_event.h"
#include "cc_event/cc_epoll.h"
#include "cc_net/cc_net.h"
#include "cc_util/cc_util.h"

#define CC_MAX_FD_SIZE (1024 * 128)

// _cc_event_loop_s
void cc_event_loop_delete(cc_event_loop_t *event_loop) {
  debug_log("cc_event_loop_delete, event_loop=%x", event_loop);
  if (event_loop->api_data) {
    cc_api_free(event_loop);
  }
  if (event_loop->events) {
    cc_free(event_loop->events);
  }
  if (event_loop->actived_events) {
    cc_free(event_loop->actived_events);
  }
  if (event_loop->high_priority_actived_events) {
    cc_free(event_loop->high_priority_actived_events);
  }
  cc_free(event_loop);
}

cc_event_loop_t *cc_event_loop_create() {
  cc_event_loop_t *event_loop = (cc_event_loop_t *)cc_alloc(1, sizeof(cc_event_loop_t));
  if (!event_loop) {
    error_log("cc_event_loop_create failed");
    cc_event_loop_delete(event_loop);
    return NULL;
  }
  if (cc_api_create(event_loop)) {
    error_log("cc_event_loop_create failed, event_loop=%x", event_loop);
    cc_event_loop_delete(event_loop);
    return NULL;
  }
  event_loop->events = (cc_socket_event_t *)cc_alloc(CC_MAX_FD_SIZE, sizeof(cc_socket_event_t));
  if (!event_loop->events) {
    error_log("cc_event_loop_create failed, event_loop=%x", event_loop);
    cc_event_loop_delete(event_loop);
    return NULL;
  }
  event_loop->nevents = CC_MAX_FD_SIZE;
  event_loop->actived_events =
      (cc_active_event_t *)cc_alloc(cc_api_get_max_size(event_loop), sizeof(cc_active_event_t));
  if (!event_loop->actived_events) {
    error_log("cc_event_loop_create failed, event_loop=%x", event_loop);

    cc_event_loop_delete(event_loop);
    return NULL;
  }
  event_loop->high_priority_actived_events =
      (cc_active_event_t *)cc_alloc(cc_api_get_max_size(event_loop), sizeof(cc_active_event_t));
  if (!event_loop->high_priority_actived_events) {
    error_log("cc_event_loop_create failed, event_loop=%x", event_loop);

    cc_event_loop_delete(event_loop);
    return NULL;
  }
  cc_rbtree_init(&event_loop->rbtree, &event_loop->sentinel, cc_rbtree_insert_timer_value);
  debug_log("cc_event_loop_create, event_loop=%x", event_loop);
  return event_loop;
}

void cc_stop(cc_event_loop_t *event_loop) {
  event_loop->stop = 1;
  debug_log("cc_stop, event_loop=%x", event_loop);
}

static void process_actived_events(cc_event_loop_t *event_loop, int actived_nevents,
                                   cc_active_event_t *actived_events) {
  int i, mask, fd, so_errno = 0;
  cc_socket_event_t *socket_event;
  for (i = 0; i < actived_nevents; ++i) {
    fd = actived_events[i].fd;
    mask = actived_events[i].mask;
    socket_event = event_loop->events + actived_events[i].fd;
    if (socket_event->mask & mask & CC_EVENT_READABLE) {
      socket_event->can_read = 1;
      socket_event->read_proc(event_loop, fd, socket_event->read_proc_data, mask);
    }
    if (socket_event->mask & mask & CC_EVENT_WRITEABLE) {
      socket_event->can_write = 1;
      socket_event->write_proc(event_loop, fd, socket_event->write_proc_data, mask);
    }
  }
}

static void process_timer_events(cc_event_loop_t *event_loop) {
  cc_time_event_t *timer_event = NULL;
  cc_rbtree_node_t *node = NULL;
  while (1) {
    if (event_loop->rbtree.root == event_loop->rbtree.sentinel) {
      break;
    }
    node = cc_rbtree_min(event_loop->rbtree.root, event_loop->rbtree.sentinel);
    if (node->key <= event_loop->current_msec) {
      timer_event = cc_rbtree_data(node, cc_time_event_t, node);
      cc_del_time_event(event_loop, timer_event);
      cc_time_proc_t *timer_proc = timer_event->timer_proc;
      void *client_data = timer_event->client_data;
      if (timer_proc) {
        timer_proc(event_loop, client_data);
      }
      timer_event->is_done = 1;
    } else {
      break;
    }
  }
}

void cc_process_events(cc_event_loop_t *event_loop) {
  int ret = 0;
  int wait_msec = 0;
  event_loop->current_msec = get_curr_milliseconds();
  if (event_loop->rbtree.root != event_loop->rbtree.sentinel) {
    cc_rbtree_node_t *node = cc_rbtree_min(event_loop->rbtree.root, event_loop->rbtree.sentinel);
    if (node->key > event_loop->current_msec) {
      wait_msec = node->key - event_loop->current_msec;
    }
  } else {
    wait_msec = -1;
  }
  event_loop->actived_nevents = 0;
  event_loop->high_priority_actived_nevents = 0;
  ret = cc_api_poll(event_loop, wait_msec);
  if (ret == 0) {
    event_loop->current_msec = get_curr_milliseconds();
    if (event_loop->high_priority_actived_nevents > 0) {
      debug_log("cc_process_events process_actived_events high priority, event_loop=%x, actived_nevents=%d", event_loop,
                event_loop->high_priority_actived_nevents);
      process_actived_events(event_loop, event_loop->high_priority_actived_nevents,
                             event_loop->high_priority_actived_events);
    }
    if (event_loop->actived_nevents > 0) {
      debug_log("cc_process_events process_actived_events event_loop=%x, actived_nevents=%d", event_loop,
                event_loop->actived_nevents);
      process_actived_events(event_loop, event_loop->actived_nevents, event_loop->actived_events);
    }
    process_timer_events(event_loop);
  }
}

void cc_main(cc_event_loop_t *event_loop) {
  while (!event_loop->stop) {
    cc_process_events(event_loop);
  }
}

// _cc_socket_event_s
int cc_set_socket_event(cc_event_loop_t *event_loop, int fd, SocketState state, int mask, cc_socket_proc_t *read_proc,
                        void *read_proc_data, cc_socket_proc_t *write_proc, void *write_proc_data) {
  cc_socket_event_t *socket_event = &event_loop->events[fd];
  socket_event->fd = fd;
  socket_event->state = state;
  if (mask != CC_EVENT_NONE) {
    if (cc_api_add_event(event_loop, fd, mask) != CC_EVENT_OK) {
      error_log(
          "cc_set_socket_event, cc_api_add_event failed, event_loop=%x, fd=%d, state=%d, mask=%d, read_proc=%x, "
          "read_proc_data=%x, "
          "write_proc=%x, write_proc_data=%x",
          event_loop, fd, state, mask, read_proc, read_proc_data, write_proc, write_proc_data);
      return CC_EVENT_ERR;
    }
    if (mask & CC_EVENT_READABLE) {
      socket_event->read_proc_data = read_proc_data;
      socket_event->read_proc = read_proc;
      if (!(socket_event->mask & CC_EVENT_READABLE)) {
        socket_event->can_read = 0;
      }
    }
    if (mask & CC_EVENT_WRITEABLE) {
      socket_event->write_proc_data = write_proc_data;
      socket_event->write_proc = write_proc;
      if (!(socket_event->mask & CC_EVENT_WRITEABLE)) {
        socket_event->can_write = 0;
      }
    }
    socket_event->mask |= mask;
  }
  debug_log(
      "cc_set_socket_event done, event_loop=%x, fd=%d, state=%d, mask=%d, read_proc=%x, read_proc_data=%x, "
      "write_proc=%x, write_proc_data=%x",
      event_loop, fd, state, mask, read_proc, read_proc_data, write_proc, write_proc_data);
  return CC_EVENT_OK;
}

int cc_del_socket_event(cc_event_loop_t *event_loop, int fd, int delmask) {
  if (cc_api_del_event(event_loop, fd, delmask) != CC_EVENT_OK) {
    error_log("cc_del_socket_event cc_api_del_event failed, event_loop=%x, fd=%d, delmask=%d", event_loop, fd, delmask);
    return CC_EVENT_ERR;
  }
  cc_socket_event_t *socket_event = &event_loop->events[fd];
  if ((delmask & CC_EVENT_READABLE) && (delmask & CC_EVENT_WRITEABLE)) {
    socket_event->fd = 0;
    socket_event->state = SS_NONE;
    socket_event->mask = 0;
  }
  if (delmask & CC_EVENT_READABLE) {
    socket_event->mask &= ~CC_EVENT_READABLE;
    socket_event->read_proc_data = NULL;
    socket_event->read_proc = NULL;
    socket_event->can_read = 0;
  }
  if (delmask & CC_EVENT_WRITEABLE) {
    socket_event->mask &= ~CC_EVENT_WRITEABLE;
    socket_event->write_proc_data = NULL;
    socket_event->write_proc = NULL;
    socket_event->can_write = 0;
  }
  debug_log("cc_del_socket_event done, event_loop=%x, fd=%d, delmask=%d", event_loop, fd, delmask);
  return 0;
}

// _cc_time_event_s
void cc_add_time_event(cc_event_loop_t *event_loop, cc_time_event_t *timer_event) {
  if (timer_event->node.key > 0) {
    timer_event->is_done = 0;
    cc_rbtree_insert(&event_loop->rbtree, &timer_event->node);
    debug_log("cc_add_time_event done, event_loop=%x, timer_event=%x", event_loop, timer_event);
  }
}
void cc_del_time_event(cc_event_loop_t *event_loop, cc_time_event_t *timer_event) {
  if (timer_event->node.key > 0) {
    cc_rbtree_delete(&event_loop->rbtree, &timer_event->node);
    debug_log("cc_del_time_event done, event_loop=%x, timer_event=%x", event_loop, timer_event);
  }
}

void cc_add_async_io_file_event(cc_event_loop_t *event_loop, cc_async_io_file_event_t *async_io_file_event) { return; }
void cc_cancel_async_io_file_event(cc_event_loop_t *event_loop, cc_async_io_file_event_t *async_io_file_event) {
  return;
}