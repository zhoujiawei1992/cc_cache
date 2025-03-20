#include "cc_util/cc_util.h"
#include <pthread.h>
#include "cc_util/cc_atomic.h"
#include "cc_util/cc_list.h"
#include "cc_util/cc_snprintf.h"

#ifdef CC_MEMCHECK

#define CC_MAX_MEM_NODE (1023)
#define CC_MAX_CONTROL_BLOCK (64)
#define CC_ALIGN_NUM (8)
#define CC_FORCE_ALIGN_NUM (64)
typedef struct _cc_mem_node_s {
  cc_slist_node_t node;
  pthread_mutex_t mutex;
} cc_mem_node_t;

typedef struct _cc_mem_control_block_s {
  cc_mem_node_t node[CC_MAX_MEM_NODE];
} cc_mem_control_block_t;

typedef struct _cc_mem_prefix_s {
  cc_slist_node_t node;
  void *ptr;
  size_t size;
  const char *func;
  unsigned int line;
} cc_mem_prefix_t;

cc_mem_control_block_t control_block[CC_MAX_CONTROL_BLOCK];
int next_cb_idx = -1;

pthread_key_t memcheck_cb_key;
void memcheck_init() { pthread_key_create(&memcheck_cb_key, NULL); }

void memcheck_alloc(cc_mem_prefix_t *prefix_node) {
  cc_mem_control_block_t *cb = NULL;
  void *value = pthread_getspecific(memcheck_cb_key);
  if (value != NULL) {
    cb = (cc_mem_control_block_t *)value;
  } else {
    int idx = atomic_add(&next_cb_idx, 1);
    cb = control_block + idx;
    for (int i = 0; i < CC_MAX_MEM_NODE; ++i) {
      pthread_mutex_init(&cb->node[i].mutex, NULL);
      cc_slist_init(&cb->node[i].node);
    }
    debug_log("control_block alloc, idx=%d, cb=%x", idx, cb);
    pthread_setspecific(memcheck_cb_key, cb);
  }
  cc_mem_node_t *node = &cb->node[(size_t)prefix_node % CC_MAX_MEM_NODE];
  pthread_mutex_lock(&node->mutex);
  cc_slist_insert(&node->node, &prefix_node->node);
  pthread_mutex_unlock(&node->mutex);
}

void memcheck_free(cc_mem_prefix_t *prefix_node) {
  cc_mem_control_block_t *cb = NULL;
  void *value = pthread_getspecific(memcheck_cb_key);
  assert(value != NULL);
  cb = (cc_mem_control_block_t *)value;
  cc_mem_node_t *node = &cb->node[(size_t)prefix_node % CC_MAX_MEM_NODE];
  pthread_mutex_lock(&node->mutex);
  cc_slist_delete_node(&node->node, &prefix_node->node);
  pthread_mutex_unlock(&node->mutex);
}

void memcheck_show(char *buffer, unsigned int buffer_size, unsigned int *out_size) {
  char *endp = buffer;
  size_t sum[CC_MAX_CONTROL_BLOCK] = {0};
  for (int i = 0; i <= next_cb_idx; ++i) {
    for (int j = 0; j < CC_MAX_MEM_NODE; ++j) {
      pthread_mutex_lock(&control_block[i].node[j].mutex);
      cc_slist_node_t *node = control_block[i].node[j].node.next;
      while (node != NULL) {
        cc_mem_prefix_t *prefix = cc_list_data(node, cc_mem_prefix_t, node);
        endp = cc_snprintf(endp, buffer + buffer_size - endp, "%2d\t%10uz\t%s:%d\n", i, prefix->size, prefix->func,
                           prefix->line);
        node = node->next;
        sum[i] += prefix->size;
      }
      pthread_mutex_unlock(&control_block[i].node[j].mutex);
    }
  }
  endp = cc_snprintf(endp, buffer + buffer_size - endp,
                     "------------------------------------------------------------------------------------------\n");
  for (int i = 0; i <= next_cb_idx; ++i) {
    endp = cc_snprintf(endp, buffer + buffer_size - endp, "%2d\t%10uzB\t%10uzKB\t%10uzMB\n", i, sum[i], sum[i] / 1024,
                       sum[i] / 1024 / 1024);
  }
  *out_size = endp - buffer;
}

void *cc_inner_malloc(size_t size, const char *func, int line) {
  size_t real_size = sizeof(cc_mem_prefix_t) + (size + CC_ALIGN_NUM - 1) & ~(CC_ALIGN_NUM - 1);
  void *ptr = malloc(real_size);
  if (ptr) {
    ((cc_mem_prefix_t *)ptr)->size = real_size;
    ((cc_mem_prefix_t *)ptr)->ptr = ptr;
    ((cc_mem_prefix_t *)ptr)->func = func;
    ((cc_mem_prefix_t *)ptr)->line = line;
    memcheck_alloc(ptr);
    ptr += sizeof(cc_mem_prefix_t);
  }
  return ptr;
}

void *cc_inner_alloc(size_t n, size_t size, const char *func, int line) {
  void *ptr = cc_inner_malloc(n * size, func, line);
  if (ptr) {
    memset(ptr, 0, n * size);
  }
  return ptr;
}

void *cc_inner_realloc(void *ptr, size_t size, const char *func, int line) {
  if (ptr == NULL) {
    return cc_inner_malloc(size, func, line);
  }
  if (size == 0) {
    cc_inner_free(ptr, func, line);
    return NULL;
  }

  void *real_ptr = ptr - sizeof(cc_mem_prefix_t);
  size_t original_size = ((cc_mem_prefix_t *)real_ptr)->size - sizeof(cc_mem_prefix_t);

  void *new_ptr = cc_inner_malloc(size, func, line);
  if (new_ptr == NULL) {
    return NULL;
  }

  size_t copy_size = original_size < size ? original_size : size;
  memcpy(new_ptr, ptr, copy_size);

  cc_inner_free(ptr, func, line);
  return new_ptr;
}

void *cc_inner_align_alloc(size_t align, size_t size, const char *func, int line) {
  size_t real_align = CC_FORCE_ALIGN_NUM;
  size_t real_size = size + real_align;
  void *ptr = memalign(real_align, real_size);
  if (ptr) {
    cc_mem_prefix_t *prefix = ptr + real_align - sizeof(cc_mem_prefix_t);
    prefix->size = real_size;
    prefix->ptr = ptr;
    prefix->func = func;
    prefix->line = line;
    memcheck_alloc(prefix);
    return ptr + real_align;
  }
  return NULL;
}

void cc_inner_free(void *ptr, const char *func, int line) {
  cc_mem_prefix_t *prefix = ptr - sizeof(cc_mem_prefix_t);
  memcheck_free(prefix);
  return free(prefix->ptr);
}

#else
void *cc_inner_malloc(size_t size, const char *func, int line) { return malloc(size); }
void *cc_inner_alloc(size_t n, size_t size, const char *func, int line) { return calloc(n, size); }
void *cc_inner_realloc(void *ptr, size_t size, const char *func, int line) { return realloc(ptr, size); }
void *cc_inner_align_alloc(size_t align, size_t size, const char *func, int line) { return memalign(align, size); }
void cc_inner_free(void *ptr, const char *func, int line) { return free(ptr); }
#endif

long long get_curr_milliseconds() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000LL + tv.tv_usec / 1000;
}
void cc_md5_upper_string(unsigned char result[16], unsigned char *data, unsigned int size) {
  static const char upper_hex_chars[] = "0123456789abcdef";
  for (int i = 0; i < 16; ++i) {
    if (i * 2 < size && i * 2 + 1 < size) {
      data[i * 2] = upper_hex_chars[(result[i] >> 4) & 0x0F];
      data[i * 2 + 1] = upper_hex_chars[result[i] & 0x0F];
    }
  }
}
void cc_md5_lower_string(unsigned char result[16], unsigned char *data, unsigned int size) {
  static const char lower_hex_chars[] = "0123456789ABCDEF";
  for (int i = 0; i < 16; ++i) {
    if (i * 2 < size && i * 2 + 1 < size) {
      data[i * 2] = lower_hex_chars[(result[i] >> 4) & 0x0F];
      data[i * 2 + 1] = lower_hex_chars[result[i] & 0x0F];
    }
  }
}

// 获取日志等级的字符串表示
const char *log_level_to_string(LogLevel level) {
  switch (level) {
    case LOG_DEBUG:
      return "DEBUG";
    case LOG_INFO:
      return "INFO";
    case LOG_WARN:
      return "WARN";
    case LOG_ERROR:
      return "ERROR";
    default:
      return "UNKNOWN";
  }
}

// 初始化日志系统
void error_log_open(cc_log_ctx_t *ctx, const char *filename, LogLevel min_level) {
  if (ctx != NULL) {
    ctx->min_log_level = min_level;
    ctx->log_file = NULL;
    if (filename) {
      ctx->log_file = fopen(filename, "a");
    }
  }
}

// 关闭日志系统
void error_log_close(cc_log_ctx_t *ctx) {
  if (ctx != NULL && ctx->log_file != NULL) {
    fclose(ctx->log_file);
    ctx->log_file = NULL;
  }
}

// 线程安全的日志记录函数
void log_line(cc_log_ctx_t *ctx, LogLevel level, const char *file, int line, const char *func, const char *format,
              ...) {
  if (ctx) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm *tm_info = localtime(&tv.tv_sec);
    char time_buf[32] = {0};
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);

    char buf[1024];
    char *buf_pos = cc_snprintf(buf, 1024, "[%s.%03ld] [%uld] [%s] [%s:%d] [%s] ", time_buf, tv.tv_usec / 1000,
                                (unsigned long)pthread_self(), log_level_to_string(level), file, line, func);
    va_list args;
    va_start(args, format);
    buf_pos = cc_vslprintf(buf_pos, buf + 1024 - 1, format, args);
    va_end(args);

    *buf_pos = '\n';
    ++buf_pos;

    FILE *fp = (ctx->log_file != NULL ? ctx->log_file : stderr);
    fwrite(buf, 1, buf_pos - buf, fp);
    // if (level >= LOG_WARN) {
    fflush(fp);
    // }
  }
}