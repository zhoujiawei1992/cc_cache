#ifndef __CC_UTIL_CC_CONFIG_H__
#define __CC_UTIL_CC_CONFIG_H__

#define CONFIG_HTTP_READ_TIMEOUT_MSEC (3000)
#define CONFIG_HTTP_WRITE_TIMEOUT_MSEC (30000)
#define CONFIG_HTTP_HEADER_BUFFER_SIZE (2 * 1024)
#define CONFIG_HTTP_BODY_BUFFER_SIZE (16 * 1024)
#define CONFIG_ADMIN_HTTP_BODY_BUFFER_SIZE (1 * 1024 * 1024)
#define CONFIG_TCP_CONNECTOR_POOL_CACHE_TIME_MSEC (30000)
#define CONFIG_TCP_CONNECTOR_POOL_CACHE_MAX_NUM_PER_CONNECTOR (100)

#endif