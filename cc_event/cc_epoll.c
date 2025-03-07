#include "cc_event/cc_epoll.h"
#include <errno.h>
#include <sys/epoll.h>
#include <unistd.h>
#include "cc_net/cc_net.h"
#include "cc_util/cc_util.h"

#define CC_MAX_GET_EVENT_SIZE 1024

typedef struct _cc_api_state_s {
  int epfd;
  struct epoll_event *events;
  int max_size;
} cc_api_state_t;

int cc_api_create(cc_event_loop_t *event_loop) {
  cc_api_state_t *state = cc_malloc(sizeof(cc_api_state_t));
  if (!state) {
    error_log("cc_api_create epoll_create failed, event_loop=%x ", event_loop);
    return CC_EVENT_ERR;
  };
  state->max_size = CC_MAX_GET_EVENT_SIZE;
  state->events = cc_malloc(sizeof(struct epoll_event) * state->max_size);
  if (!state->events) {
    error_log("cc_api_create epoll_create failed, event_loop=%x ", event_loop);
    cc_free(state);
    return CC_EVENT_ERR;
  }
  state->epfd = epoll_create(1024);
  if (state->epfd == -1) {
    error_log("cc_api_create epoll_create failed, event_loop=%x, err=%d, err_str=%s", event_loop, cc_net_get_last_errno,
              cc_net_get_last_error);
    cc_free(state->events);
    cc_free(state);
    return CC_EVENT_ERR;
  }
  // anetCloexec(state->epfd);
  event_loop->api_data = state;
  debug_log("cc_api_create, event_loop=%x, efd=%d", event_loop, state->epfd);
  return CC_EVENT_OK;
}

void cc_api_free(cc_event_loop_t *event_loop) {
  cc_api_state_t *state = event_loop->api_data;
  debug_log("cc_api_free, event_loop=%x, efd=%d", event_loop, state->epfd);
  close(state->epfd);
  cc_free(state->events);
  cc_free(state);
}

int cc_api_get_max_size(cc_event_loop_t *event_loop) {
  cc_api_state_t *state = event_loop->api_data;
  return state->max_size;
}

int cc_api_add_event(cc_event_loop_t *event_loop, int fd, int mask) {
  cc_api_state_t *state = event_loop->api_data;
  struct epoll_event ee = {0};
  ee.events = 0;
  mask |= event_loop->events[fd].mask;
  if (mask & CC_EVENT_READABLE) {
    ee.events |= EPOLLIN;
  }
  if (mask & CC_EVENT_WRITEABLE) {
    ee.events |= EPOLLOUT;
  }
  if (mask & CC_EVENT_READABLE || mask & CC_EVENT_WRITEABLE) {
    if (mask & CC_EVENT_ERROR) {
      ee.events |= EPOLLERR;
    }
    if (mask & CC_EVENT_CLOSE) {
      ee.events |= EPOLLRDHUP;
    }
    ee.events |= EPOLLET;
  }
  ee.data.fd = fd;

  if (event_loop->events[fd].mask == CC_EVENT_NONE) {
    if (epoll_ctl(state->epfd, EPOLL_CTL_ADD, fd, &ee) == -1) {
      error_log(
          "cc_api_add_event epoll_ctl_add failed, event_loop=%x, efd=%d, fd=%d, op=%d, mask=%d, err=%d, err_str=%s",
          event_loop, state->epfd, fd, EPOLL_CTL_ADD, mask, cc_net_get_last_errno, cc_net_get_last_error);
      return CC_EVENT_ERR;
    }
    debug_log("cc_api_add_event epoll_ctl_add, event_loop=%x, efd=%d, fd=%d, op=%d, mask=%d", event_loop, state->epfd,
              fd, EPOLL_CTL_ADD, mask);
  } else {
    if (epoll_ctl(state->epfd, EPOLL_CTL_MOD, fd, &ee) == -1) {
      error_log(
          "cc_api_add_event epoll_ctl_mod failed, event_loop=%x, efd=%d, fd=%d, op=%d, mask=%d, err=%d, err_str=%s",
          event_loop, state->epfd, fd, EPOLL_CTL_MOD, mask, cc_net_get_last_errno, cc_net_get_last_error);
      return CC_EVENT_ERR;
    }
    debug_log("cc_api_add_event epoll_ctl_mod, event_loop=%x, efd=%d, fd=%d, op=%d, mask=%d", event_loop, state->epfd,
              fd, EPOLL_CTL_MOD, mask);
  }
  return 0;
}

int cc_api_del_event(cc_event_loop_t *event_loop, int fd, int delmask) {
  cc_api_state_t *state = event_loop->api_data;
  struct epoll_event ee = {0};
  int mask = event_loop->events[fd].mask & (~delmask);
  ee.events = 0;
  if (mask & CC_EVENT_READABLE) {
    ee.events |= EPOLLIN;
  }
  if (mask & CC_EVENT_WRITEABLE) {
    ee.events |= EPOLLOUT;
  }
  if (mask & CC_EVENT_READABLE || mask & CC_EVENT_WRITEABLE) {
    if (mask & CC_EVENT_ERROR) {
      ee.events |= EPOLLERR;
    }
    if (mask & CC_EVENT_CLOSE) {
      ee.events |= EPOLLHUP;
    }
    ee.events |= EPOLLET;
  }
  ee.data.fd = fd;
  if (ee.events != 0) {
    if (epoll_ctl(state->epfd, EPOLL_CTL_MOD, fd, &ee) == -1) {
      error_log(
          "cc_api_del_event epoll_ctl_mod failed, event_loop=%x, efd=%d, fd=%d, op=%d, mask=%d, err=%d, err_str=%s",
          event_loop, state->epfd, fd, EPOLL_CTL_MOD, mask, cc_net_get_last_errno, cc_net_get_last_error);
      return CC_EVENT_ERR;
    }
    debug_log("cc_api_del_event epoll_ctl_mod, event_loop=%x, efd=%d, fd=%d, op=%d, mask=%d", event_loop, state->epfd,
              fd, EPOLL_CTL_MOD, mask);
  } else {
    if (epoll_ctl(state->epfd, EPOLL_CTL_DEL, fd, &ee) == -1) {
      error_log(
          "cc_api_del_event epoll_ctl_del failed, event_loop=%x, efd=%d, fd=%d, op=%d, mask=%d, err=%d, err_str=%s",
          event_loop, state->epfd, fd, EPOLL_CTL_DEL, mask, cc_net_get_last_errno, cc_net_get_last_error);
      return CC_EVENT_ERR;
    }
    debug_log("cc_api_del_event epoll_ctl_del, event_loop=%x, efd=%d, fd=%d, op=%d, mask=%d", event_loop, state->epfd,
              fd, EPOLL_CTL_DEL, mask);
  }
  return 0;
}

int cc_api_poll(cc_event_loop_t *event_loop, int wait_msec) {
  cc_api_state_t *state = event_loop->api_data;
  cc_active_event_t *active_event = NULL;
  cc_socket_event_t *socket_event = NULL;
  int ret, i, numevents = 0;
  debug_log("cc_api_poll, event_loop=%x, efd=%d, wait_msec=%d", event_loop, state->epfd, wait_msec);
  ret = epoll_wait(state->epfd, state->events, CC_MAX_GET_EVENT_SIZE, wait_msec);
  if (ret > 0) {
    debug_log("cc_api_poll epoll_wait done, ret=%d, event_loop=%x, efd=%d", ret, event_loop, state->epfd);
    numevents = ret;
    for (i = 0; i < numevents; ++i) {
      int mask = 0;
      struct epoll_event *e = state->events + i;
      if (e->events & EPOLLIN) {
        mask |= CC_EVENT_READABLE;
      }
      if (e->events & EPOLLOUT) {
        mask |= CC_EVENT_WRITEABLE;
      }
      if (e->events & EPOLLERR) {
        mask |= CC_EVENT_ERROR;
      }
      if (e->events & EPOLLHUP) {
        mask |= CC_EVENT_CLOSE;
      }
      if (event_loop->events[e->data.fd].mask & CC_EVENT_HIGH_PRIORITY) {
        active_event = event_loop->high_priority_actived_events + event_loop->high_priority_actived_nevents;
        ++event_loop->high_priority_actived_nevents;
      } else {
        active_event = event_loop->actived_events + event_loop->actived_nevents;
        ++event_loop->actived_nevents;
      }
      active_event->fd = e->data.fd;
      active_event->mask = mask;
    }
  } else if (ret == -1 && errno != EINTR) {
    error_log("cc_api_poll epoll_wait failed, event_loop=%x, efd=%d, err=%d, err_str=%s", event_loop, state->epfd,
              cc_net_get_last_errno, cc_net_get_last_error);
    return CC_EVENT_ERR;
  }
  return 0;
}