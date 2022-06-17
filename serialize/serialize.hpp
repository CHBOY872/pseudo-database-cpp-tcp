#ifndef SERIALIZE_SENTER
#define SERIALIZE_SENTER

#include "../server/server.hpp"

unsigned char *serialize_int(int n, char *buf);
unsigned char *serialize_long_long(long long n, char *buf);
unsigned char *serialize_char(const char *n, char *buf, int len);
unsigned char *serialize_message_struct(message_struct *msg, unsigned char *buf,
                               int char_len);

unsigned char *deserialize_int(int *n, unsigned char *buf);
unsigned char *deserialize_long_long(long long *n, unsigned char *buf);
unsigned char *deserialize_char(unsigned char *n,
                                unsigned char *buf, int char_len);
unsigned char *deserialize_message_struct(message_struct *msg,
                                          unsigned char *buf, int char_len);

#endif