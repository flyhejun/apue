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
extern int callback(void *NotUsed,int argc, char *argv[], char **azColName);

extern int db_write(sqlite3 *db, char *json_buf);

extern sqlite3_stmt* data_exist(sqlite3 *db);

extern int db_open(sqlite3 **db);

extern void db_read(sqlite3 *db, char *buf, size_t buf_size, int fd);

extern int db_delete(sqlite3 *db, const char *table_name);

extern int db_close(sqlite3 *db);

#ifdef __cplusplus

}
#endif

#endif
