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

static int table_exist(sqlite3 *db)
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

void temporary_repo(sqlite3 *db)
{
	char			*zErrMsg = NULL;
	char 			*sql = NULL;

	int	rc = sqlite3_open("temp.db", &db);
	if(rc)
	{
		printf("Create or open database failure: %s\n", sqlite3_errmsg(db));
		_exit(1);
	}

	if(table_exist(db) != 1)
	{
		sql = "CREATE TABLE TEMP_RECDS(" \
			   "ID				TEXT	NOT NULL," \
			   "TIME			TEXT	NOT NULL," \
			   "TEMPERATURE		REAL	NOT NULL);";
	
		rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
		if(rc != SQLITE_OK)
		{
			log_error("SQL操作失败: %s", zErrMsg);
			sqlite3_free(zErrMsg);
			_exit(1);
		}
	}
}

int temp_data_in(sqlite3 *db, char *json_buf)
{
	char			*sql = NULL;
	sqlite3_stmt	*stmt;
	cJSON			*root = NULL;
	char			*id_item = NULL;
	char			*time_item = NULL;
	double			*temp_item = NULL;
	
	temporary_repo(db);
	sql = "INSERT INTO TEMP_RECDS (ID, TIME, TEMPERATURE) VALUES(?, ?, ?);";
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

	if(!cJSON_IsString(id_str)||!cJSON_IsString(time_str)||!cJSON_IsNumber(temp_num))
	{
		printf("change format fail\n");
		cJSON_Delete(root);
		return -2;
	}

	strcpy(id_item, id_str->valuestring);
	strcpy(time_item, time_str->valuestring);
	*temp_item = (double)temp_num->valuedouble;

	cJSON_Delete(root);

	sqlite3_bind_text(stmt, 1, id_item, -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 2, time_item, -1, SQLITE_STATIC);
	sqlite3_bind_double(stmt, 3, *temp_item);

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

void tempo_data_in(sqlite3_stmt *stmt, char *buf, size_t buf_size)
{
	const unsigned char 		*id_buf = NULL;
	const unsigned char			*time_buf = NULL;
	double 						temp_buf = 0;

	char						*buf_id = NULL;
	char						*buf_time = NULL;

	memset(buf, 0, buf_size);
	id_buf = sqlite3_column_text(stmt, 0);
	time_buf = sqlite3_column_text(stmt, 1);
	temp_buf = sqlite3_column_double(stmt, 2);
	buf_id = strdup((const char*)id_buf);
	buf_time = strdup((const char*)time_buf);
	date_packet(buf_time, &temp_buf, buf, buf_size);
	log_trace("缓存记录上传: ID:%S, 时间: %s, 温度: .2f", 
							id_buf, time_buf, temp_buf);
							
	free(buf_id);
	free(buf_time);
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
