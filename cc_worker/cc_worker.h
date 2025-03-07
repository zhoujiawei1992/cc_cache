#ifndef __CC_WORKER_CC_WORKER_H__
#define __CC_WORKER_CC_WORKER_H__

#include <pthread.h>
#include "cc_event/cc_event.h"
#include "cc_util/cc_atomic.h"

#define CC_MAX_WORKER (1024)

typedef enum {
  CC_DEFAULT_WORKER,
  CC_HTTP_WORKER,
} WorkerType;

typedef struct _cc_worker_s cc_worker_t;

typedef void cc_worker_free_proc_t(cc_worker_t* worker);
typedef int cc_worker_start_proc_t(cc_worker_t* worker);
typedef void cc_worker_stop_proc_t(cc_worker_t* worker);

typedef struct _cc_worker_s {
  unsigned short int id;
  WorkerType type;
  pthread_t tid;
  cc_event_loop_t* event_loop;
  cc_worker_free_proc_t* free;
  cc_worker_start_proc_t* start;
  cc_worker_stop_proc_t* stop;
} cc_worker_t;

void cc_init_worker(cc_worker_t* worker, WorkerType worker_type);

int cc_start_worker(cc_worker_t* worker);
void cc_join_worker(cc_worker_t* worker);
cc_worker_t* cc_create_worker(WorkerType worker_type);

#endif