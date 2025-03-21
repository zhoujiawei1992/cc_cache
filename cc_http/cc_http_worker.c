#include "cc_http/cc_http_worker.h"
#include "cc_http/cc_http_client_request.h"
#include "cc_net/cc_acceptor.h"
#include "cc_util/cc_config.h"
void cc_free_http_worker(cc_worker_t* worker) {
  cc_http_worker_t* http_worker = (cc_http_worker_t*)(worker);
  while (http_worker->acceptor_list.next) {
    cc_slist_node_t* node = http_worker->acceptor_list.next;
    cc_slist_delete(&http_worker->acceptor_list);
    cc_tcp_acceptor_free(cc_list_data(node, cc_tcp_acceptor_t, node));
  }
  if (http_worker->tcp_connector_pool != NULL) {
    cc_tcp_connector_pool_free(http_worker->tcp_connector_pool);
  }
  if (worker->event_loop != NULL) {
    cc_event_loop_delete(worker->event_loop);
  }
  debug_log("http worker free. %d", http_worker->worker.id);
  cc_free(http_worker);
}
int cc_start_http_worker(cc_worker_t* worker) {
  cc_http_worker_t* http_worker = (cc_http_worker_t*)(worker);
  debug_log("http worker start. %d", http_worker->worker.id);
  cc_main(worker->event_loop);
  return 0;
}

void cc_stop_http_worker(cc_worker_t* worker) {
  cc_http_worker_t* http_worker = (cc_http_worker_t*)(worker);
  cc_stop(worker->event_loop);
  debug_log("http worker stoped. %d", http_worker->worker.id);
}

cc_worker_t* cc_http_worker_create(WorkerType worker_type) {
  cc_http_worker_t* http_worker = (cc_http_worker_t*)cc_alloc(1, sizeof(cc_http_worker_t));
  if (http_worker == NULL) {
    error_log("create http worker failed, cc_alloc %d failed.", sizeof(cc_http_worker_t));
    return NULL;
  }

  cc_init_worker(&http_worker->worker, worker_type);

  http_worker->worker.event_loop = cc_event_loop_create();
  if (http_worker->worker.event_loop == NULL) {
    error_log("create http worker failed, cc_event_loop_create failed.");
    cc_free_http_worker((cc_worker_t*)http_worker);
    return NULL;
  }

  http_worker->worker.free = cc_free_http_worker;
  http_worker->worker.start = cc_start_http_worker;
  http_worker->worker.stop = cc_stop_http_worker;

  http_worker->tcp_connector_pool =
      cc_tcp_connector_pool_create(&http_worker->worker, CONFIG_TCP_CONNECTOR_POOL_CACHE_MAX_NUM_PER_CONNECTOR);
  if (http_worker->tcp_connector_pool == NULL) {
    error_log("create http worker failed, cc_tcp_connector_pool_create failed.");
    cc_free_http_worker((cc_worker_t*)http_worker);
    return NULL;
  }

  cc_slist_init(&http_worker->acceptor_list);
  if (worker_type == CC_HTTP_WORKER) {
    cc_tcp_acceptor_t* acceptor =
        cc_tcp_acceptor_create((cc_worker_t*)http_worker, "127.0.0.1", 8080, 1024, cc_http_client_accept);
    if (acceptor == NULL) {
      error_log("create http worker failed, cc_tcp_acceptor_create failed.");
      cc_free_http_worker((cc_worker_t*)http_worker);
      return NULL;
    }
    cc_slist_insert(&http_worker->acceptor_list, &acceptor->node);
  } else if (worker_type == CC_HTTP_ADMIN_WORKER) {
    cc_tcp_acceptor_t* acceptor =
        cc_tcp_acceptor_create((cc_worker_t*)http_worker, "127.0.0.1", 8081, 1024, cc_http_client_accept);
    if (acceptor == NULL) {
      error_log("create http worker failed, cc_tcp_acceptor_create failed.");
      cc_free_http_worker((cc_worker_t*)http_worker);
      return NULL;
    }
    cc_slist_insert(&http_worker->acceptor_list, &acceptor->node);
  }
  debug_log("http worker created. %d", http_worker->worker.id);
  return &http_worker->worker;
}