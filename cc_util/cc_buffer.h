#ifndef __CC_UTIL_CC_BUFFER_H__
#define __CC_UTIL_CC_BUFFER_H__

#include <stdlib.h>

typedef struct _cc_string_s {
  const char *buf;
  unsigned int len;
} cc_string_t;

typedef struct _cc_buffer_s {
  char *data;
  unsigned int size;
  unsigned int capacity;
} cc_buffer_t;

#define cc_buffer_create(capacity) (cc_inner_buffer_create((capacity), (__func__), (__LINE__)))
#define cc_buffer_free(buffer) (cc_inner_buffer_free((buffer), (__func__), (__LINE__)))

cc_buffer_t *cc_inner_buffer_create(unsigned int capacity, const char *func, int line);
void cc_inner_buffer_free(cc_buffer_t *buffer, const char *func, int line);

cc_buffer_t *cc_inner_buffer_init(cc_buffer_t *buffer, unsigned int capacity, const char *func, int line);
void cc_inner_buffer_clear(cc_buffer_t *buffer, const char *func, int line);
#endif