#ifndef __CC_UTIL_CC_BUFFER_H__
#define __CC_UTIL_CC_BUFFER_H__

#include <stdlib.h>

typedef struct _cc_string_s {
  const char *data;
  unsigned int size;
} cc_string_t;

typedef struct _cc_buffer_s {
  char *data;
  unsigned int size;
  unsigned int capacity;
  unsigned int offset;
  int ref;
} cc_buffer_t;

#define cc_buffer_create(capacity) (cc_inner_buffer_create((capacity), (__func__), (__LINE__)))
#define cc_buffer_free(buffer) (cc_inner_buffer_free((buffer), (__func__), (__LINE__)))
#define cc_buffer_ref(buffer) (cc_inner_buffer_ref((buffer), (__func__), (__LINE__)))
#define cc_buffer_unref(buffer) (cc_inner_buffer_unref((buffer), (__func__), (__LINE__)))
#define cc_buffer_assgin(dst, src) (cc_inner_buffer_assgin((dst), (src), (__func__), (__LINE__)))

void cc_inner_buffer_assgin(cc_buffer_t **dst, cc_buffer_t *src, const char *func, int line);
void cc_inner_buffer_swap(cc_buffer_t **dst, cc_buffer_t **src, const char *func, int line);

cc_buffer_t *cc_inner_buffer_create(unsigned int capacity, const char *func, int line);
void cc_inner_buffer_free(cc_buffer_t *buffer, const char *func, int line);

int cc_inner_buffer_ref(cc_buffer_t *buffer, const char *func, int line);
int cc_inner_buffer_unref(cc_buffer_t *buffer, const char *func, int line);

int str2ll(const char *data, unsigned int len, long long *value);
int str2ull(const char *data, unsigned int len, unsigned long long *value);

void TimeStamp2GmtTime(time_t time_in_second, char *buf, unsigned int buf_size);

#endif