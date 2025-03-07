#include "cc_util/cc_cb_context.h"
#include "cc_util/cc_util.h"

cc_cb_context_t* cc_inner_cb_context_create(void* data, cc_cb_data_free_proc_t* free_proc, const char* func, int line) {
  cc_cb_context_t* cb_context = (cc_cb_context_t*)cc_inner_alloc(1, sizeof(cc_cb_context_t), func, line);
  if (cb_context == NULL) {
    return NULL;
  }
  cb_context->data = data;
  cb_context->free_proc = free_proc;
  cb_context->ref_count = 1;
  debug_log("cc_inner_cb_context_create[%s,%d] cb_context=%x, data=%x, free_proc=%x, ref_count=%d", func, line,
            cb_context, cb_context->data, cb_context->free_proc, cb_context->ref_count);
  return cb_context;
}
void cc_inner_cb_context_free(cc_cb_context_t* cb_context, const char* func, int line) {
  debug_log("cc_inner_cb_context_free[%s,%d] cb_context=%x, data=%x, free_proc=%x, ref_count=%d", func, line,
            cb_context, cb_context->data, cb_context->free_proc, cb_context->ref_count);
  if (cb_context->free_proc) {
    cb_context->free_proc(cb_context->data, func, line);
  }
  cc_inner_free(cb_context, func, line);
}
cc_cb_context_t* cc_inner_cb_context_link(cc_cb_context_t* cb_context, const char* func, int line) {
  if (cb_context->ref_count > 0) {
    atomic_add(&cb_context->ref_count, 1);
  }
  debug_log("cc_inner_cb_context_link[%s,%d] cb_context=%x, data=%x, ref_count=%d", func, line, cb_context,
            cb_context->data, cb_context->ref_count);
  return cb_context;
}
void cc_inner_cb_context_unlink(cc_cb_context_t* cb_context, const char* func, int line) {
  if (cb_context->ref_count > 0) {
    if (atomic_add(&cb_context->ref_count, -1) == 0) {
      cc_inner_cb_context_free(cb_context, func, line);
    }
  }
  debug_log("cc_inner_cb_context_unlink[%s,%d] cb_context=%x, data=%x, ref_count=%d", func, line, cb_context,
            cb_context->data, cb_context->ref_count);
}