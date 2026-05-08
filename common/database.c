/*********************************************************************************
 *      Copyright:  (C) 2026 fanjingyu<14784765013@163.com>
 *                  All rights reserved.
 *
 *       Filename:  database.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(30/03/26)
 *         Author:  fanjingyu <14784765013@163.com>
 *      ChangeLog:  1, Release initial version on "30/03/26 21:45:25"
 *                 
 ********************************************************************************/


#include <stdio.h>
#include <sqlite3.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "packet.h"
#include "database.h"
#include "cJSON.h"
#include "log.h"
				

int callback(void *NotUsed, int argc, char *argv[], char **azColName)
{
	int	i;
	for(i=0; i<argc; i++)
	{
		printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : NULL);
	}
	printf("\n");
	return 0;
}

int table_exist(sqlite3 *db)
{
	const char 			*table_name = "TEMP_RECDS";
	const char			*sql = "SELECT name FROM sqlite_master WHERE type='table' AND name=?";
	sqlite3_stmt		*stmt;
	int					exist = 0;
	
	int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	if(rc != SQLITE_OK)
	{
		printf("prepare failure: %s\n", sqlite3_errmsg(db));
		return -1;
	}

	sqlite3_bind_text(stmt, 1, table_name, -1, SQLITE_STATIC);

	if(sqlite3_step(stmt) == SQLITE_ROW)
	{
		exist = 1;
	}

	sqlite3_finalize(stmt);

	return exist;
}

sqlite3_stmt* data_exist(sqlite3 *db)
{
    char            *sql = "SELECT DATA FROM TEMP_RECDS";
    int             rs;
    sqlite3_stmt    *stmt;

    rs = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

    if(rs != SQLITE_OK)
    {
        log_error("SQL准备失败: %s", sqlite3_errmsg(db));
        return NULL;
    }

    if(stmt == NULL)
    {
        sqlite3_close(db);
        return NULL;
    }

    return stmt;
}

int temporary_repo(sqlite3 **db)
{
    char            *zErrMsg = NULL;
    char            *sql = NULL;

    int rc = sqlite3_open("temp.db", db);
    if(rc)
    {
        printf("Create or open database failure: %s\n", sqlite3_errmsg(*db));
        return -1;
    }

    if(table_exist(*db) != 1)
    {
        sql = "CREATE TABLE TEMP_RECDS(DATA TEXT NOT NULL);";

        rc = sqlite3_exec(*db, sql, callback, 0, &zErrMsg);
        if(rc != SQLITE_OK)
        {
            log_error("SQL操作失败: %s", zErrMsg);
            sqlite3_free(zErrMsg);
            return -1;
        }
    }
    return 0;
}

int temp_data_in(sqlite3 *db, char *json_buf)
{
    char         *sql = NULL;
    sqlite3_stmt *stmt;

    sql = "INSERT INTO TEMP_RECDS (DATA) VALUES(?);";
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if(rc != SQLITE_OK)
    {
        printf("prepare failure: %s\n", sqlite3_errmsg(db));
        return rc;
    }

    sqlite3_bind_text(stmt, 1, json_buf, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if(rc != SQLITE_OK)
    {
        printf("step to table failure: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return -3;
    }

    sqlite3_finalize(stmt);
    return 0;
}

void tempo_updata(sqlite3 *db, char *buf, size_t buf_size, int fd)
{
    sqlite3_stmt        *stmt;
    const unsigned char *data = NULL;

    stmt = data_exist(db);
    if(!stmt) return;

    if(sqlite3_step(stmt) != SQLITE_ROW)
    {
        sqlite3_finalize(stmt);
        log_error("库内无数据");
        return ;
    }

    memset(buf, 0, buf_size);
    data = sqlite3_column_text(stmt, 0);
    if(data)
    {
        strncpy(buf, (const char*)data, buf_size - 1);
    }

    sqlite3_finalize(stmt);

    if(write(fd, buf, strlen(buf)) < 0)
    {
        log_error("失去连接，错误: %s", strerror(errno));
        return ;          
    }
    log_info("data reupdata.");
    return ;
}

int old_data_delete(sqlite3 *db, const char *table_name)
{
	char 		sql[256];

	sprintf(sql, "DELETE FROM %s WHERE rowid IN "
				 "(SELECT MIN(rowid) FROM %s)",
				 table_name, table_name);
	if(sqlite3_exec(db, sql, NULL,NULL, NULL) != SQLITE_OK)
	{
		log_error("delete oldest data failed: %s", sqlite3_errmsg(db));
		return -1;
	}
	return 0;
}
