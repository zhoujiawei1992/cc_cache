#ifndef __CC_UTIL_CC_SNPRINTF_H_
#define __CC_UTIL_CC_SNPRINTF_H_

#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

char *cc_vslprintf(char *buf, char *last, const char *fmt, va_list args);
char *cc_snprintf(char *buf, size_t max, const char *fmt, ...);

#endif