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
#include <errno.h>
#include "packet.h"
#include "cJSON.h"

void get_devid(data_t *data, int sn)
{
    int             id_size = sizeof(data->id);

    memset(data->id, 0, id_size);
    snprintf(data->id, id_size, "RPI#%04d", sn);
}

int date_packet(data_t *data, char *buf, size_t buf_len)
{
	char			*json_str = NULL;

	cJSON *root = cJSON_CreateObject();
	if(!root)
	{
		printf("Create cJSON object failure: %s\n", strerror(errno));
		return -1;
	}

	cJSON_AddStringToObject(root, "ID", data->id);
	cJSON_AddStringToObject(root, "TIME", data->time);
	cJSON_AddNumberToObject(root, "TEMPERATURE", data->temperature);
	
	json_str = cJSON_Print(root);
	if(!json_str)
	{
		printf("cJson to string failure: %s\n", strerror(errno));
		cJSON_Delete(root);
		return -2;
	}

	memset(buf, 0, buf_len);
	strncpy(buf, json_str, buf_len - 1);
	
	free(json_str);
	cJSON_Delete(root);
	return 0;
	
}
