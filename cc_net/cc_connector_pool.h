#ifndef __CC_NET_CC_CONNECTOR_POOL_H__
#define __CC_NET_CC_CONNECTOR_POOL_H__

#include "cc_event/cc_event.h"
#include "cc_net/cc_connector.h"
#include "cc_util/cc_hash.h"

typedef struct _cc_tcp_connector_pool_s {
  cc_worker_t* worker;
  cc_hash_table_t** pools;
  int npools;
} cc_tcp_connector_pool_t;

cc_tcp_connector_pool_t* cc_tcp_connector_pool_create(cc_worker_t* worker, int npools);

void cc_tcp_connector_pool_free(cc_tcp_connector_pool_t* pool);

int cc_tcp_connector_pool_get_connector(cc_tcp_connector_pool_t* pool, const char* ip, int port,
                                        cc_tcp_connector_t** connector);

int cc_tcp_connector_pool_put_connector(cc_tcp_connector_pool_t* pool, cc_tcp_connector_t* connector);

#endif