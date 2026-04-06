/********************************************************************************
 *      Copyright:  (C) 2026 qyc
 *                  All rights reserved.
 *
 *       Filename:  ds18b20.h
 *    Description:  This file 
 *
 *        Version:  1.0.0(27/03/26)
 *         Author:  qingyuechuan <360744619@qq.com>
 *      ChangeLog:  1, Release initial version on "27/03/26 12:59:00"
 *                 
 ********************************************************************************/


#ifndef DS18B20_H
#define DS18B20_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

int read_temperature(double *temperature);

#ifdef __cplusplus

}
#endif

#endif
