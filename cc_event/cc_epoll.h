#ifndef __CC_EVENT_CC_EPOLL_H__
#define __CC_EVENT_CC_EPOLL_H__

#include "cc_event/cc_event.h"

int cc_api_create(cc_event_loop_t *event_loop);
void cc_api_free(cc_event_loop_t *event_loop);
int cc_api_get_max_size(cc_event_loop_t *event_loop);
int cc_api_add_event(cc_event_loop_t *event_loop, int fd, int mask);
int cc_api_del_event(cc_event_loop_t *event_loop, int fd, int delmask);
int cc_api_poll(cc_event_loop_t *event_loop, int wait_msec);

#endif