#include "cc_util/cc_snprintf.h"
#include "cc_util/cc_buffer.h"
#include "cc_util/cc_util.h"

#define CC_INT64_LEN ((sizeof("-9223372036854775808") - 1) + 2)
#define CC_CPYMEM(dst, src, n) (((char *)memcpy(dst, src, n)) + (n))
#define CC_STRING_NULL "NULL"

static void cc_reverse(char *start, char *end);

static char *cc_sprintf_num(char *buf, char *last, uint64_t ui64, char zero, uint32_t hexadecimal, uint32_t width,
                            uint32_t left_fill, uint32_t sign_flag);
char *cc_snprintf(char *buf, size_t max, const char *fmt, ...) {
  char *p;
  va_list args;

  va_start(args, fmt);
  p = cc_vslprintf(buf, buf + max, fmt, args);
  va_end(args);

  return p;
}

char *cc_vslprintf(char *buf, char *last, const char *fmt, va_list args) {
  char *p, zero;
  int d;
  double f;
  size_t len, slen;
  int64_t i64;
  uint64_t ui64, frac;
  uint32_t width, sign, sign_flag, hex, max_width, frac_width, scale, n, is_long, left_fill;
  cc_string_t *v;
  char *start;

  while (*fmt && buf < last) {
    if (*fmt == '%') {
      i64 = 0;
      ui64 = 0;
      left_fill = 0;
      start = NULL;

      if (*(fmt + 1) == '-') {
        left_fill = 1;
        fmt++;
      }
      zero = (char)((*++fmt == '0') ? '0' : ' ');
      width = 0;
      sign = 1;
      sign_flag = 0;
      hex = 0;
      max_width = 0;
      frac_width = 0;
      is_long = 0;
      slen = (size_t)-1;

      while (*fmt >= '0' && *fmt <= '9') {
        width = width * 10 + *fmt++ - '0';
      }

      for (;;) {
        switch (*fmt) {
          case 'u':
            sign = 0;
            fmt++;
            continue;
          case 'm':
            max_width = 1;
            fmt++;
            continue;
          case 'l':
            is_long = 1;
            fmt++;
            continue;
          case '.':
            fmt++;
            while (*fmt >= '0' && *fmt <= '9') {
              frac_width = frac_width * 10 + *fmt++ - '0';
            }
            break;
          case '*':
            slen = va_arg(args, size_t);
            fmt++;
            continue;
          default:
            break;
        }
        break;
      }

      switch (*fmt) {
        case 'V':
          v = va_arg(args, cc_string_t *);
          if (v->buf == NULL) {
            len = CC_MIN(((size_t)(last - buf)), 4);  // 4 is len of NULL
            buf = CC_CPYMEM(buf, CC_STRING_NULL, len);
          } else {
            len = CC_MIN(((size_t)(last - buf)), v->len);
            buf = CC_CPYMEM(buf, v->buf, len);
          }
          fmt++;
          continue;
        case 's':
          p = va_arg(args, char *);
          if (p == NULL) {
            p = (char *)CC_STRING_NULL;
          }
          if (slen == (size_t)-1) {
            while (*p && buf < last) {
              *buf++ = *p++;
            }
          } else {
            len = CC_MIN(((size_t)(last - buf)), slen);
            buf = CC_CPYMEM(buf, p, len);
          }
          fmt++;
          continue;
        case 'O':
          i64 = (int64_t)va_arg(args, off_t);
          sign = 1;
          break;
        case 'T':
          i64 = (int64_t)va_arg(args, time_t);
          sign = 1;
          break;
        case 'z':
          if (sign) {
            i64 = (int64_t)va_arg(args, ssize_t);
          } else {
            ui64 = (uint64_t)va_arg(args, size_t);
          }
          break;
        case 'd':
          if (is_long) {
            if (sign) {
              i64 = va_arg(args, int64_t);
            } else {
              ui64 = va_arg(args, uint64_t);
            }
          } else {
            if (sign) {
              i64 = (int64_t)va_arg(args, int);
            } else {
              ui64 = (uint64_t)va_arg(args, u_int);
            }
          }
          break;
        case 'D':
          if (sign) {
            i64 = (int64_t)va_arg(args, int32_t);
          } else {
            ui64 = (uint64_t)va_arg(args, uint32_t);
          }
          break;

        case 'f':
          f = va_arg(args, double);
          if (f < 0) {
            *buf++ = '-';
            f = -f;
          }
          ui64 = (int64_t)f;
          frac = 0;
          if (frac_width == 0) {
            frac_width = 6;
          }
          if (frac_width) {
            scale = 1;
            for (n = frac_width; n; n--) {
              scale *= 10;
            }
            frac = (uint64_t)((f - (double)ui64) * scale + 0.5);
            if (frac == scale) {
              ui64++;
              frac = 0;
            }
          }
          buf = cc_sprintf_num(buf, last, ui64, zero, 0, width, 0, 0);
          if (frac_width) {
            if (buf < last) {
              *buf++ = '.';
            }
            buf = cc_sprintf_num(buf, last, frac, '0', 0, frac_width, 0, 0);
          }
          fmt++;
          continue;
        case 'X':
          ui64 = (uintptr_t)va_arg(args, void *);
          hex = 2;
          sign = 0;
          break;
        case 'x':
          ui64 = (uintptr_t)va_arg(args, void *);
          hex = 1;
          sign = 0;
          break;
        case 'p':
          ui64 = (uintptr_t)va_arg(args, void *);
          hex = 1;
          sign = 0;
          break;
        case 'c':
          d = va_arg(args, int);
          *buf++ = (char)(d & 0xff);
          fmt++;
          continue;
        case 'Z':
          *buf++ = '\0';
          fmt++;
          continue;
        case '%':
          *buf++ = '%';
          fmt++;
          continue;
        default:
          *buf++ = *fmt++;
          continue;
      }

      if (sign) {
        if (i64 < 0) {
          sign_flag = 1;
          ui64 = (uint64_t)-i64;
        } else {
          ui64 = (uint64_t)i64;
        }
      }
      buf = cc_sprintf_num(buf, last, ui64, zero, hex, width, left_fill, sign_flag);
      fmt++;
    } else {
      *buf++ = *fmt++;
    }
  }
  return buf;
}

static char *cc_sprintf_num(char *buf, char *last, uint64_t ui64, char zero, uint32_t hexadecimal, uint32_t width,
                            uint32_t left_fill, uint32_t sign_flag) {
  char *p, temp[CC_INT64_LEN + 1];
  char *start, *mid, *end;
  char *buf_ret;

  size_t len;
  uint32_t ui32;
  static char hex[] = "0123456789abcdef";
  static char HEX[] = "0123456789ABCDEF";

  p = temp + CC_INT64_LEN;
  start = buf;
  mid = NULL;
  end = NULL;

  if (hexadecimal == 0) {
    if (ui64 <= (uint64_t)0xffffffff) {
      ui32 = (uint32_t)ui64;

      do {
        *--p = (char)(ui32 % 10 + '0');
      } while (ui32 /= 10);

    } else {
      do {
        *--p = (char)(ui64 % 10 + '0');
      } while (ui64 /= 10);
    }

  } else if (hexadecimal == 1) {
    do {
      *--p = hex[(uint32_t)(ui64 & 0xf)];

    } while (ui64 >>= 4);
    *--p = 'x';
    *--p = '0';
  } else { /* hexadecimal == 2 */

    do {
      *--p = HEX[(uint32_t)(ui64 & 0xf)];

    } while (ui64 >>= 4);
    *--p = 'X';
    *--p = '0';
  }

  if (sign_flag) {
    *--p = '-';
  }

  len = (temp + CC_INT64_LEN) - p;

  end = mid;

  while (len++ < width && buf < last) {
    *buf++ = zero;
    mid = buf;
  }

  len = (temp + CC_INT64_LEN) - p;

  if (buf + len > last) {
    len = last - buf;
  }

  if (mid > last) {
    mid = last;
  }

  end = mid + len;

  buf_ret = CC_CPYMEM(buf, p, len);
  if (left_fill == 1 && start && mid && end && mid - start > 0 && end - mid > 0) {
    cc_reverse(start, mid);
    cc_reverse(mid, end);
    cc_reverse(start, end);
  }
  // return CC_CPYMEM(buf, p, len);
  return buf_ret;
}

static void cc_reverse(char *start, char *end) {
  char temp;
  end--;
  while (start < end) {
    temp = *start;
    *start = *end;
    *end = temp;
    ++start;
    --end;
  }
}