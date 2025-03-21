#include "cc_net/cc_connector_pool.h"
#include "cc_http/cc_http_worker.h"
#include "cc_util/cc_config.h"
#include "cc_util/cc_snprintf.h"
#include "cc_util/cc_util.h"

#define POOL_HASH_BUCKET_SIZE (1024)

void cc_tcp_connetor_pool_clear_connector(cc_hash_node_t* node) {
  cc_tcp_connector_t* connector = cc_hash_data(node, cc_tcp_connector_t, hash_node);
  cc_tcp_connector_free(connector);
}
cc_tcp_connector_pool_t* cc_tcp_connector_pool_create(cc_worker_t* worker, int npools) {
  cc_tcp_connector_pool_t* pool = cc_malloc(sizeof(cc_tcp_connector_pool_t));
  if (pool == NULL) {
    error_log("cc_tcp_connector_pool_create failed, cc_malloc failed");
    return NULL;
  }
  pool->pools = (cc_hash_table_t**)cc_alloc(npools, sizeof(cc_hash_table_t*));
  if (pool->pools == NULL) {
    cc_tcp_connector_pool_free(pool);
    error_log("cc_tcp_connector_pool_create failed, cc_alloc failed, npools=%d", npools);
    return NULL;
  }
  pool->worker = worker;
  pool->npools = 0;
  for (int i = 0; i < npools; ++i) {
    pool->pools[i] = cc_hash_table_create(POOL_HASH_BUCKET_SIZE, NULL, NULL, cc_tcp_connetor_pool_clear_connector);
    if (pool->pools[i] == NULL) {
      cc_tcp_connector_pool_free(pool);
      error_log("cc_tcp_connector_pool_create failed, cc_hash_table_create failed, npools=%d, i=%d", npools, i);
      return NULL;
    }
    pool->npools++;
  }
  debug_log("cc_tcp_connector_pool_create done, npools=%d, pool=%x", pool->npools, pool);
  return pool;
}

void cc_tcp_connector_pool_free(cc_tcp_connector_pool_t* pool) {
  debug_log("cc_tcp_connector_pool_free done, pool=%x", pool);
  if (pool->pools != NULL) {
    for (int i = 0; i < pool->npools; ++i) {
      cc_hash_table_free(pool->pools[i]);
    }
    cc_free(pool->pools);
    pool->pools = NULL;
  }
  cc_free(pool);
}

int cc_tcp_connector_pool_get_connector(cc_tcp_connector_pool_t* pool, const char* ip, int port,
                                        cc_tcp_connector_t** connector) {
  char key[CC_CONNECTOR_IP_PORT_LEN];
  char* endp = cc_snprintf(key, CC_CONNECTOR_IP_PORT_LEN, "%s:%d", ip, port);
  unsigned int key_size = endp - key;
  cc_hash_node_t* node = NULL;
  for (int i = 0; i < pool->npools; ++i) {
    if (cc_hash_table_lookup(pool->pools[i], key, key_size, &node) == CC_HASH_OK) {
      if (connector) {
        *connector = cc_hash_data(node, cc_tcp_connector_t, hash_node);
        cc_hash_table_remove(pool->pools[i], node);
        (*connector)->idx_in_pool = -1;
        cc_del_time_event(pool->worker->event_loop, &(*connector)->timeout_in_pool_timer);
      }
      return 0;
    }
  }
  return 0;
}

void cc_tcp_connector_pool_connector_timeout(cc_event_loop_t* event_loop, void* client_data) {
  cc_tcp_connector_t* connector = client_data;
  cc_tcp_connector_pool_t* pool = ((cc_http_worker_t*)connector->worker)->tcp_connector_pool;
  assert(connector->idx_in_pool >= 0);
  assert(pool != NULL);
  assert(connector->idx_in_pool < pool->npools);

  cc_hash_table_remove(pool->pools[connector->idx_in_pool], &connector->hash_node);
  cc_tcp_connector_free(connector);
}
int cc_tcp_connector_pool_put_connector(cc_tcp_connector_pool_t* pool, cc_tcp_connector_t* connector) {
  for (int i = 0; i < pool->npools; ++i) {
    if (cc_hash_table_lookup(pool->pools[i], connector->hash_node.key.data, connector->hash_node.key.size, NULL) ==
        CC_HASH_OK) {
      continue;
    }
    connector->idx_in_pool = i;
    cc_hash_table_insert(pool->pools[i], &connector->hash_node);

    connector->timeout_in_pool_timer.node.key =
        pool->worker->event_loop->current_msec + CONFIG_TCP_CONNECTOR_POOL_CACHE_TIME_MSEC;
    connector->timeout_in_pool_timer.timer_proc = cc_tcp_connector_pool_connector_timeout;
    connector->timeout_in_pool_timer.client_data = connector;
    cc_add_time_event(pool->worker->event_loop, &connector->connect_timer);

    return 0;
  }
  // 如果满了，则直接释放
  cc_tcp_connector_free(connector);
  return 0;
}
