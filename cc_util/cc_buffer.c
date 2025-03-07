#include "cc_util/cc_buffer.h"
#include <time.h>
#include "cc_util/cc_atomic.h"
#include "cc_util/cc_util.h"
cc_buffer_t* cc_inner_buffer_create(unsigned int capacity, const char* func, int line) {
  cc_buffer_t* buffer = (cc_buffer_t*)cc_inner_align_alloc(sizeof(void*), sizeof(cc_buffer_t), func, line);
  if (buffer == NULL) {
    return NULL;
  }
  buffer->data = (char*)cc_inner_malloc(capacity, func, line);
  if (buffer->data == NULL) {
    cc_inner_free(buffer, func, line);
    return NULL;
  }
  buffer->capacity = capacity;
  buffer->size = 0;
  buffer->offset = 0;
  buffer->ref = 0;
  cc_inner_buffer_ref(buffer, func, line);
  debug_log("cc_inner_buffer_create[%s:%d]. buffer=%x, data=%x, capacity=%ld, ref=%d", func, line, buffer, buffer->data,
            buffer->capacity, buffer->ref);
  return buffer;
}
void cc_inner_buffer_free(cc_buffer_t* buffer, const char* func, int line) {
  int ref = cc_inner_buffer_unref(buffer, func, line);
  debug_log("cc_inner_buffer_free[%s:%d]. buffer=%x, data=%x, capacity=%ld, ref=%d", func, line, buffer, buffer->data,
            buffer->capacity, buffer->ref);
  if (ref == 0) {
    cc_inner_free(buffer->data, func, line);
    cc_inner_free(buffer, func, line);
  }
}

void cc_inner_buffer_assgin(cc_buffer_t** dst, cc_buffer_t* src, const char* func, int line) {
  *dst = src;
  cc_inner_buffer_ref(*dst, func, line);
  debug_log("cc_inner_buffer_ref[%s:%d]. buffer=%x, data=%x, capacity=%ld, ref=%d", func, line, *dst, (*dst)->data,
            (*dst)->capacity, (*dst)->ref);
}

int cc_inner_buffer_ref(cc_buffer_t* buffer, const char* func, int line) {
  int ref = atomic_add(&buffer->ref, 1);
  // debug_log("cc_inner_buffer_ref[%s:%d]. buffer=%x, data=%x, capacity=%ld, ref=%d", func, line, buffer, buffer->data,
  //          buffer->capacity, buffer->ref);
  return ref;
}
int cc_inner_buffer_unref(cc_buffer_t* buffer, const char* func, int line) {
  int ref = atomic_add(&buffer->ref, -1);
  // debug_log("cc_inner_buffer_unref[%s:%d]. buffer=%x, data=%x, capacity=%ld, ref=%d", func, line, buffer,
  // buffer->data,
  //          buffer->capacity, buffer->ref);
  return ref;
}

int str2ll(const char* data, unsigned int len, long long* value) {
  long long out = 0;
  int sign = 1;
  for (int i = 0; i < len; ++i) {
    if (i == 0 && data[i] == '-') {
      sign = -1;
    } else if (i == 0 && data[i] == '+') {
      sign = 1;
    } else if (data[i] >= '0' && data[i] <= '9') {
      out = out * 10 + (data[i] - '0');
    } else {
      return -1;
    }
  }
  *value = out * sign;
  return 0;
}
int str2ull(const char* data, unsigned int len, unsigned long long* value) {
  unsigned long long out = 0;
  for (int i = 0; i < len; ++i) {
    if (i == 0 && data[i] == '+') {
      continue;
    } else if (data[i] >= '0' && data[i] <= '9') {
      out = out * 10 + (data[i] - '0');
    } else {
      return -1;
    }
  }
  *value = out;
  return 0;
}

void TimeStamp2GmtTime(time_t time_in_second, char* buf, unsigned int buf_size) {
  struct tm tm_gmt;
  gmtime_r(&time_in_second, &tm_gmt);
  strftime(buf, buf_size, "%a, %d %b %Y %H:%M:%S GMT", &tm_gmt);
}