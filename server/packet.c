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

int date_packet(char *time, double *temperature, char *buf, size_t buf_len)
{
	char			*id = "rpi3b001";
	char			*json_str = NULL;

	cJSON *root = cJSON_CreateObject();
	if(!root)
	{
		printf("Create cJSON object failure: %s\n", strerror(errno));
		return -1;
	}

	cJSON_AddStringToObject(root, "ID", id);
	cJSON_AddStringToObject(root, "TIME", time);
	cJSON_AddNumberToObject(root, "TEMPERATURE", *temperature);
	
	json_str = cJSON_Print(root);
	if(!json_str)
	{
		printf("cJson to string failure: %s\n", strerror(errno));
		cJSON_Delete(root);
		return -2;
	}

	memset(buf, 0, buf_len);
	strcpy(buf, json_str);
	
	free(json_str);
	cJSON_Delete(root);
	return 0;
	
}
