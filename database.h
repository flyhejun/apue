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
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

void temperary_repo(sqlite *db);

int temperary_data_in(sqlite3 *db, char	*json_buf);

#ifdef __cplusplus

}
#endif

#endif
