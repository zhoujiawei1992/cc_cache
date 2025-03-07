#include "cc_worker/cc_worker.h"
#include <errno.h>
#include <sys/prctl.h>
#include "cc_util/cc_snprintf.h"
#include "cc_util/cc_util.h"
const char* worker_type_to_string(WorkerType worker_type) {
  switch (worker_type) {
    case CC_DEFAULT_WORKER:
      return "default";
    case CC_HTTP_WORKER:
      return "http";
    default:
      return "UNKNOWN";
  }
}

void cc_init_worker(cc_worker_t* worker, WorkerType worker_type) {
  static int worker_id = -1;
  worker->id = atomic_add(&worker_id, 1);
  worker->type = worker_type;
  worker->event_loop = NULL;
}

cc_worker_t* cc_create_worker(WorkerType worker_type) {
  cc_worker_t* worker = (cc_worker_t*)cc_alloc(1, sizeof(cc_worker_t));
  if (worker) {
    error_log("cc_create_worker failed, worker_type=%s", worker_type_to_string(worker->type));
    cc_init_worker(worker, worker_type);
  }
  debug_log("cc_create_worker done, worker_type=%s, id=%d", worker_type_to_string(worker->type), worker->id);
  return worker;
}

static void* thread_fn(void* arg) {
  cc_worker_t* worker = (cc_worker_t*)arg;
  debug_log("worker thread_fn %s_%d start", worker_type_to_string(worker->type), worker->id);
  worker->start(worker);
  debug_log("worker thread_fn %s_%d end", worker_type_to_string(worker->type), worker->id);
  return NULL;
}
int cc_start_worker(cc_worker_t* worker) {
  int ret = pthread_create(&worker->tid, NULL, thread_fn, (void*)worker);
  if (ret != 0) {
    error_log("cc_start_worker pthread_create. failed, ret=%d, errno=%d, strerror=%s", ret, errno, strerror(errno));
    return ret;
  }
  char buf[32] = {0};
  cc_snprintf(buf, sizeof(buf), "%s_%d", worker_type_to_string(worker->type), worker->id);
  // pthread_setname_np(worker->tid, buf);
  prctl(PR_SET_NAME, buf, NULL, NULL, NULL);
  debug_log("cc_start_worker done, worker_type=%s, worker_id=%d", worker_type_to_string(worker->type), worker->id);
  return 0;
}
void cc_join_worker(cc_worker_t* worker) {
  pthread_join(worker->tid, NULL);
  worker->free(worker);
}
