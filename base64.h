#ifndef BASE64_H
#include "textutils.h"
#define BASE64_H
unsigned char * base64_encode(const unsigned char *src, size_t len, size_t *out_len);
unsigned char * base64_decode(const unsigned char *src, size_t len,size_t *out_len);
#endif