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
#include <string.h>
#include "database.h"
static int table_exist(sqlite3 *db)
{
	const char 			*table_name = "TEMP";
	const char			*sql = "SELECT name FROM sqlite_master WHERE type='table' AND name=?";
	sqlite3_stmt		*stmt;
	int					exist = 0;
	
	int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	if(rc != SQLTIE_OK)
	{
		printf("prepare failure: %s\n", sqlite3_errmsg(db));
		return -1
	}

	sqlite3_bind_text(stmt, 1, table_name, -1, SQLITE_STATIC);

	if(sqlite3_step(stmt) == SQLITE_ROW)
	{
		exist = 1;
	}

	sqlite3_finalize(stmt);

	return exist;
}

static void temporary_repo(sqlite3 *db)
{
	char			*zErrMSg = NULL;
	char 			*sql = NULL;
	sqlite3_stmt	*stmt;

	int	rc = sqlite3_open("temporary_date", &db);
	if(rc)
	{
		printf("Create or open database failure: %s\n", sqlite3_errmsg(db));
		_exit(0);
	}

	if(table_exist(db) != 1)
	{
		sql = "CREATE TABLE TEMP(" \
			   "ID				TEXT	NOT NULL," \
			   "TIME			TEXT	NOT NULL," \
			   "TEMPERATURE		REAL	NOT NULL);";
	
		rc = sqlite3_exec(db, sql, callback, 0, &zErrMSg);
		if(rc != SQLITE_OK)
		{
			printf("create table failure: %s\n", zErrMSg);
			sqlite3_free(zErrMSg);
			_exit(1);
		}
	}
}

int temporary_data_in(sqlite3 *db, char *json_buf)
{
	char			*sql = NULL;
	sqlite3_stmt	*stmt;
	cJSON			*root = NULL;
	char			*id_item;
	char			*time_item;
	float			*temp_item;
	
	temporary_repo(db);
	sql = "INSERT INTO TEMP (ID, TIME, TEMPERATURE) VALUES(?, ?, ?);";
	int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	if(rc != SQLITE_OK)
	{
		printf("prepare failure: %s\n", sqlite3_errmsg(db));
		return rc;
	}
	
	root = cJSON_Parse(json_buf);
	if(root == NULL)
	{
		printf("json parse fail");
		return -1;
	}

	cJSON *id_str = cJSON_GetObjectItem(root, "ID");
	cJSON *time_str = cJSON_GetObjectItem(root, "TIME");
	cJSON *temp_num = cJSON_GetObjectItem(root, "TEMPERATURE");

	if(!cJSON_IsString(id_str)||!cJSON_IsString(time_str)||!cJSON_IsNumnber(temp_num))
	{
		printf("change format fail\n");
		cJSON_Delete(root);
		return -2;
	}

	strcpy(id_item, id_str->valuestring);
	strcpy(time_item, time_str->valuestring);
	*temp_item = (float)temp_num->valuredouble;

	cJSON_Delete(root);

	sqlite3_bind_text(stmt, 1, id_item, -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 2, time_item, -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 3, temp_item, -1, SQLITE_STATIC);

	rc = sqlite3_step(stmt);
	if(rc != SQLTIE_OK)
	{
		printf("step to table failure: %s\n", sqlite3_errmsg(db));
		sqlite3_finalize(stmt);
		return -3;
	}

	sqlite3_finalize(stmt);
}


