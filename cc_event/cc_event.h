#ifndef __CC_EVENT_CC_EVENT_H__
#define __CC_EVENT_CC_EVENT_H__

#include "cc_util/cc_rbtree.h"

#define CC_EVENT_OK 0
#define CC_EVENT_ERR -1

#define CC_EVENT_NONE 0
#define CC_EVENT_READABLE 1
#define CC_EVENT_WRITEABLE 2
#define CC_EVENT_ERROR 4
#define CC_EVENT_CLOSE 8
#define CC_EVENT_HIGH_PRIORITY 128

typedef struct _cc_event_loop_s cc_event_loop_t;

typedef void cc_socket_proc_t(cc_event_loop_t *event_loop, int fd, void *client_data, int mask);

typedef void cc_file_read_done_proc_t(cc_event_loop_t *event_loop, void *client_data);

typedef void cc_file_write_done_proc_t(cc_event_loop_t *event_loop, void *client_data);

typedef void cc_time_proc_t(cc_event_loop_t *event_loop, void *client_data);

typedef struct _cc_active_event_s {
  int mask;
  int fd;
} cc_active_event_t;

typedef struct _cc_time_event_s {
  cc_rbtree_node_t node;
  void *client_data;
  cc_time_proc_t *timer_proc;
  int is_done;
} cc_time_event_t;

typedef enum { SS_NONE, SS_LISTENING, SS_CONNECTING, SS_CONNECTED } SocketState;

typedef struct _cc_socket_event_s {
  void *read_proc_data;
  cc_socket_proc_t *read_proc;
  void *write_proc_data;
  cc_socket_proc_t *write_proc;
  int fd;
  int mask;
  SocketState state;
  unsigned int can_read : 1;
  unsigned int can_write : 1;
} cc_socket_event_t;

typedef struct _cc_async_io_file_event_s {
  size_t offset;
  size_t length;
  size_t nbytes;
  char *buffer;
  cc_file_read_done_proc_t *read_proc;
  cc_file_write_done_proc_t *write_proc;
  void *client_data;
  int fd;
  int error;
} cc_async_io_file_event_t;

typedef struct _cc_event_loop_s {
  int stop;
  int flags;
  int nevents;
  int actived_nevents;
  int high_priority_actived_nevents;
  long long current_msec;
  cc_socket_event_t *events;
  cc_active_event_t *actived_events;
  cc_active_event_t *high_priority_actived_events;
  void *api_data;
  cc_rbtree_node_t sentinel;
  cc_rbtree_t rbtree;
} cc_event_loop_t;

// _cc_event_loop_s
cc_event_loop_t *cc_event_loop_create();
void cc_event_loop_delete(cc_event_loop_t *event_loop);
void cc_stop(cc_event_loop_t *event_loop);
void cc_process_events(cc_event_loop_t *event_loop);
void cc_main(cc_event_loop_t *event_loop);

// _cc_socket_event_s
int cc_set_socket_event(cc_event_loop_t *event_loop, int fd, SocketState state, int mask, cc_socket_proc_t *read_proc,
                        void *read_proc_data, cc_socket_proc_t *write_proc, void *write_proc_data);

int cc_del_socket_event(cc_event_loop_t *event_loop, int fd, int delmask);

// _cc_time_event_s
void cc_add_time_event(cc_event_loop_t *event_loop, cc_time_event_t *timer_event);
void cc_del_time_event(cc_event_loop_t *event_loop, cc_time_event_t *timer_event);

// _cc_file_event_s
void cc_add_async_io_file_event(cc_event_loop_t *event_loop, cc_async_io_file_event_t *async_io_file_event);
void cc_del_async_io_file_event(cc_event_loop_t *event_loop, cc_async_io_file_event_t *async_io_file_event);
#endif