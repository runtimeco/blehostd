#ifndef H_PARSE_
#define H_PARSE_

#include <inttypes.h>

int parse_arg_byte_stream(char *val, int max_len, uint8_t *dst, int *out_len);
int parse_arg_byte_stream_exact_length(char *val, uint8_t *dst, int len);
int parse_arg_mac(char *val, uint8_t *dst);

#endif
