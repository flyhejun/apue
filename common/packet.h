/********************************************************************************
 *      Copyright:  (C) 2026 xy
 *                  All rights reserved.
 *
 *       Filename:  packet.h
 *    Description:  This file 
 *
 *        Version:  1.0.0(29/03/26)
 *         Author:  xiao yang <809308758@qq.com>
 *      ChangeLog:  1, Release initial version on "29/03/26 19:15:24"
 *                 
 ********************************************************************************/

#include <stddef.h>
#include <stdlib.h>

#ifndef PACKET_H
#define PACKET_H

#ifdef __cplusplus
extern "C" {
#endif

int date_packet(char *time, double *temperature, char *buf, size_t buf_len);

#ifdef __cplusplus

}
#endif

#endif
