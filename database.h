/********************************************************************************
 *      Copyright:  (C) 2026 fanjingyu<14784765013@163.com>
 *                  All rights reserved.
 *
 *       Filename:  database.h
 *    Description:  This file 
 *
 *        Version:  1.0.0(30/03/26)
 *         Author:  fanjingyu <14784765013@163.com>
 *      ChangeLog:  1, Release initial version on "30/03/26 21:34:19"
 *                 
 ********************************************************************************/

#ifndef DATABASE_H
#define DATABASE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
int callback(void *NotUsed,int argc, char *argv[], char **azColName);

int temp_data_in(sqlite3 *db, char	*json_buf);

#ifdef __cplusplus

}
#endif

#endif
