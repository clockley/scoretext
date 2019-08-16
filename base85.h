#ifndef BASE85_H
#define BASE85_H
int decode_85(char *dst, const char *buffer, int len);
void encode_85(char *buf, const unsigned char *data, int bytes);
#endif