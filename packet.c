/*********************************************************************************
 *      Copyright:  (C) 2026 xy
 *                  All rights reserved.
 *
 *       Filename:  packet.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(29/03/26)
 *         Author:  xiao yang <809308758@qq.com>
 *      ChangeLog:  1, Release initial version on "29/03/26 19:17:57"
 *                 
 ********************************************************************************/

#include <string.h>
#include <stdio.h>
#include "packet.h"

void date_packet(char *time, float *temperature, char *buf, size_t buf_len)
{
		char		temp_buf[32];
		
		memset(buf, 0, buf_len);
		strncat(buf, "rpi3b001", buf_len);
		strncat(buf, time, buf_len-strlen(buf));
		strncat(buf, ",", buf_len-strlen(buf));
		snprint(temp_buf, sizeof(temp_buf), "%.2f", temperature);
		strncat(buf, temp_buf, buf_len-strlen(buf));
}

