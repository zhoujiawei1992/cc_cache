#include "cc_util/cc_buffer.h"
#include <time.h>
#include "cc_util/cc_util.h"
cc_buffer_t* cc_inner_buffer_create(unsigned int capacity, const char* func, int line) {
  cc_buffer_t* buffer = (cc_buffer_t*)cc_inner_malloc(sizeof(cc_buffer_t), func, line);
  if (buffer == NULL) {
    return NULL;
  }
  if (cc_inner_buffer_init(buffer, capacity, func, line) == NULL) {
    cc_inner_free(buffer, func, line);
    return NULL;
  }
  return buffer;
}
void cc_inner_buffer_free(cc_buffer_t* buffer, const char* func, int line) {
  if (buffer) {
    cc_inner_buffer_clear(buffer, func, line);
    cc_inner_free(buffer, func, line);
  }
}

cc_buffer_t* cc_inner_buffer_init(cc_buffer_t* buffer, unsigned int capacity, const char* func, int line) {
  buffer->data = (char*)cc_inner_malloc(capacity, func, line);
  if (buffer->data == NULL) {
    return NULL;
  }
  buffer->size = 0;
  buffer->capacity = capacity;
  return buffer;
}
void cc_inner_buffer_clear(cc_buffer_t* buffer, const char* func, int line) {
  if (buffer->data) {
    cc_inner_free(buffer->data, func, line);
    buffer->data = NULL;
    buffer->size = 0;
    buffer->capacity = 0;
  }
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