#ifndef __CC_UTIL_CC_MD5_H__
#define __CC_UTIL_CC_MD5_H__

typedef struct _cc_md5_s {
  unsigned long long bytes;
  unsigned int a, b, c, d;
  unsigned char buffer[64];
} cc_md5_t;

void cc_md5_init(cc_md5_t *ctx);
void cc_md5_update(cc_md5_t *ctx, const void *data, unsigned long long size);
void cc_md5_final(unsigned char result[16], cc_md5_t *ctx);

#endif