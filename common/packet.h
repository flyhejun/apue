#ifndef PACKET_H
#define PACKET_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdlib.h>

typedef struct data_s
{
    char        time[64];
    double      temperature;
    char        id[16];
} data_t;

void get_devid(data_t *data, int sn);

int date_packet(data_t *data, char *buf, size_t buf_len);

#ifdef __cplusplus

}
#endif

#endif
