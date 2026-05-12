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
#include <sqlite3.h>

struct socket_s;
typedef struct socket_s socket_t;

extern int db_write(sqlite3 *db, char *json_buf);

extern sqlite3_stmt* db_exist(sqlite3 *db);

extern int db_open(sqlite3 **db);

extern int db_read(sqlite3 *db, char *buf, size_t buf_size, socket_t *sock);

extern int db_delete(sqlite3 *db, const char *table_name);

extern int db_close(sqlite3 *db);

#ifdef __cplusplus

}
#endif

#endif
