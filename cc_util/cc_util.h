#ifndef __CC_UTIL_CC_UTIL_H__
#define __CC_UTIL_CC_UTIL_H__

#include <assert.h>
#include <malloc.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

#define CC_MIN(a, b) ((a) < (b) ? (a) : (b))

#ifdef CC_MEMCHECK
void memcheck_init();
void memcheck_show(char *buffer, unsigned int buffer_size, unsigned int *out_size);
#endif

void *cc_inner_malloc(size_t size, const char *func, int line);
void *cc_inner_alloc(size_t n, size_t size, const char *func, int line);
void *cc_inner_realloc(void *ptr, size_t size, const char *func, int line);
void *cc_inner_align_alloc(size_t align, size_t size, const char *func, int line);
void cc_inner_free(void *ptr, const char *func, int line);

#define cc_malloc(size) (cc_inner_malloc((size), (__func__), (__LINE__)))

#define cc_alloc(n, size) (cc_inner_alloc((n), (size), (__func__), (__LINE__)))

#define cc_realloc(ptr, size) (cc_inner_realloc((ptr), (size), (__func__), (__LINE__)))

#define cc_align_alloc(align, size) (cc_inner_align_alloc((align), (size), (__func__), (__LINE__)))

#define cc_free(ptr) (cc_inner_free((ptr), (__func__), (__LINE__)))

long long get_curr_milliseconds();

void cc_md5_upper_string(unsigned char result[16], unsigned char *data, unsigned int size);

void cc_md5_lower_string(unsigned char result[16], unsigned char *data, unsigned int size);

// 定义日志等级
typedef enum { LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR } LogLevel;

typedef struct _cc_log_ctx_s {
  FILE *log_file;
  LogLevel min_log_level;
} cc_log_ctx_t;

cc_log_ctx_t global_log_ctx;

// 初始化日志系统
void error_log_open(cc_log_ctx_t *ctx, const char *filename, LogLevel min_level);

// 关闭日志系统
void error_log_close(cc_log_ctx_t *ctx);

// 线程安全的日志记录函数
void log_line(cc_log_ctx_t *ctx, LogLevel level, const char *file, int line, const char *func, const char *format, ...);

#define ctx_debug_log(ctx, ...)                                                  \
  do {                                                                           \
    if ((ctx)->min_log_level <= LOG_DEBUG) {                                     \
      log_line(ctx, LOG_DEBUG, (__FILE__), (__LINE__), (__func__), __VA_ARGS__); \
    }                                                                            \
  } while (0);

#define ctx_info_log(ctx, ...)                                                  \
  do {                                                                          \
    if ((ctx)->min_log_level <= LOG_INFO) {                                     \
      log_line(ctx, LOG_INFO, (__FILE__), (__LINE__), (__func__), __VA_ARGS__); \
    }                                                                           \
  } while (0);

#define ctx_warn_log(ctx, ...)                                                  \
  do {                                                                          \
    if ((ctx)->min_log_level <= LOG_WARN) {                                     \
      log_line(ctx, LOG_WARN, (__FILE__), (__LINE__), (__func__), __VA_ARGS__); \
    }                                                                           \
  } while (0);

#define ctx_error_log(ctx, ...)                                                  \
  do {                                                                           \
    if ((ctx)->min_log_level <= LOG_ERROR) {                                     \
      log_line(ctx, LOG_ERROR, (__FILE__), (__LINE__), (__func__), __VA_ARGS__); \
    }                                                                            \
  } while (0);

#define debug_log(...) ctx_debug_log(&global_log_ctx, __VA_ARGS__)

#define info_log(...) ctx_info_log(&global_log_ctx, __VA_ARGS__)

#define warn_log(...) ctx_warn_log(&global_log_ctx, __VA_ARGS__)

#define error_log(...) ctx_error_log(&global_log_ctx, __VA_ARGS__)

#endif