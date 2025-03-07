#ifndef __CC_HTTP_CC_WORKER_H__
#define __CC_HTTP_CC_WORKER_H__

#include "cc_event/cc_event.h"
#include "cc_util/cc_list.h"
#include "cc_util/cc_util.h"
#include "cc_worker/cc_worker.h"

typedef struct _cc_http_worker_s {
  cc_worker_t worker;
  cc_slist_node_t acceptor_list;
} cc_http_worker_t;

void cc_free_http_worker(cc_worker_t* worker);
int cc_start_http_worker(cc_worker_t* worker);
void cc_stop_http_worker(cc_worker_t* worker);
cc_worker_t* cc_http_worker_create();

#endif