#include "cc_util/cc_buffer.h"
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