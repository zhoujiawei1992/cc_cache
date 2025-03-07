#ifndef __CC_UTIL_CC_CB_CONTEXT_H__
#define __CC_UTIL_CC_CB_CONTEXT_H__

#include "cc_util/cc_atomic.h"

typedef void cc_cb_data_free_proc_t(void* data, const char* func, int line);

typedef struct _cc_cb_context_s {
  void* data;
  cc_cb_data_free_proc_t* free_proc;
  int ref_count;
} cc_cb_context_t;

#define cc_cb_context_create(data, free_proc) \
  (cc_inner_cb_context_create((data), (cc_cb_data_free_proc_t*)(free_proc), (__func__), (__LINE__)))
#define cc_cb_context_free(context) (cc_inner_cb_context_free((context), (__func__), (__LINE__)))
#define cc_cb_context_link(context) (cc_inner_cb_context_link((context), (__func__), (__LINE__)))
#define cc_cb_context_unlink(context) (cc_inner_cb_context_unlink((context), (__func__), (__LINE__)))

cc_cb_context_t* cc_inner_cb_context_create(void* data, cc_cb_data_free_proc_t* free_proc, const char* func, int line);
void cc_inner_cb_context_free(cc_cb_context_t* cb_context, const char* func, int line);
cc_cb_context_t* cc_inner_cb_context_link(cc_cb_context_t* cb_context, const char* func, int line);
void cc_inner_cb_context_unlink(cc_cb_context_t* cb_context, const char* func, int line);

#endif