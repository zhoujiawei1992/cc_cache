#include "cc_util/cc_util.h"
#include "cc_util/cc_snprintf.h"

void *cc_inner_malloc(size_t size, const char *func, int line) { return malloc(size); }

void *cc_inner_alloc(size_t n, size_t size, const char *func, int line) { return calloc(n, size); }

void *cc_inner_realloc(void *ptr, size_t size, const char *func, int line) { return realloc(ptr, size); }

void *cc_inner_align_alloc(size_t align, size_t size, const char *func, int line) { return memalign(align, size); }

void cc_inner_free(void *ptr, const char *func, int line) { return free(ptr); }

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