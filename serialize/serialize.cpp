#include <stdio.h>
#include <stdlib.h>
#include "serialize.hpp"

// SERIALIZE

unsigned char *serialize_int(int n, unsigned char *buf)
{
    buf[0] = n >> 24;
    buf[1] = n >> 16;
    buf[2] = n >> 8;
    buf[3] = n;

    return buf + 4;
}

unsigned char *serialize_long_long(long long n, unsigned char *buf)
{
    buf[0] = n >> 56;
    buf[1] = n >> 48;
    buf[2] = n >> 40;
    buf[3] = n >> 32;
    buf[4] = n >> 24;
    buf[5] = n >> 16;
    buf[6] = n >> 8;
    buf[7] = n;

    return buf + 8;
}

unsigned char *serialize_char(const unsigned char *n,
                              unsigned char *buf, int char_len)
{
    int i = 0;
    while (n[i])
    {
        buf[i] = n[i];
        i++;
    }
    return buf + char_len;
}

unsigned char *serialize_message_struct(message_struct *msg,
                                        unsigned char *buf, int char_len)
{
    buf = serialize_int(msg->command, buf);
    buf = serialize_long_long(msg->sel_pesel, buf);
    buf = serialize_long_long(msg->id, buf);
    buf = serialize_char((unsigned const char *)msg->name, buf, char_len);
    buf = serialize_char((unsigned const char *)msg->surname, buf, char_len);
    buf = serialize_long_long(msg->pesel, buf);

    return buf;
}

// DESERIALIZE

unsigned char *deserialize_int(int *n, unsigned char *buf)
{
    *n = 0;
    int i;
    for (i = 0; i < 3; i++)
        *n = (buf[i] | *n) << 8;
    *n = (buf[i] | *n);
    return buf + i + 1;
}

unsigned char *deserialize_long_long(long long *n, unsigned char *buf)
{
    *n = 0;
    int i;
    for (i = 0; i < 7; i++)
        *n = (buf[i] | *n) << 8;
    *n = (buf[i] | *n);
    return buf + i + 1;
}

unsigned char *deserialize_char(unsigned char *n,
                                unsigned char *buf, int char_len)
{
    int i = 0;
    while (buf[i])
    {
        n[i] = buf[i];
        i++;
    }
    n[i] = 0;
    return buf + char_len;
}

unsigned char *deserialize_message_struct(message_struct *msg,
                                          unsigned char *buf, int char_len)
{
    buf = deserialize_int((int *)&msg->command, buf);
    buf = deserialize_long_long((long long *)&msg->sel_pesel, buf);
    buf = deserialize_long_long((long long *)&msg->id, buf);
    buf = deserialize_char((unsigned char *)msg->name, buf, char_len);
    buf = deserialize_char((unsigned char *)msg->surname, buf, char_len);
    buf = deserialize_long_long((long long *)&msg->pesel, buf);

    return buf;
}