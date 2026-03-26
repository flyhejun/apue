/********************************************************************************
 *      Copyright:  (C) 2026 He Junfei<hejunfei2005@163.com>
 *                  All rights reserved.
 *
 *       Filename:  socket_client.h
 *    Description:  This file 
 *
 *        Version:  1.0.0(16/03/26)
 *         Author:  He Junfei <hejunfei2005@163.com>
 *      ChangeLog:  1, Release initial version on "16/03/26 15:14:21"
 *                 
 ********************************************************************************/
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>

void print_usage(char *progname)
{
	printf("%s usage: \n", progname);
	printf("  -i(--ipaddr): sepcify server IP address\n");
	printf("  -p(--port): sepcify server port.\n");
	printf("  -s(--sleep: sleep time setting\n");
	printf("  -d(--dnr): donmain name resolution.\n");
	printf("  -h(--Help): print this help information.\n");

	return ;
}

double get_temp()
{
	char                    w1_path[50] = "/sys/devices/w1_bus_master1/";
	char					buf[128];
	struct dirent			*direntp;
	DIR						*dirp;
	int 					find;
	int 					fd;
	char					*tp;
	double 					temp;

	if(!temp)
	{
		return -1;
	}

	if((dirp = opendir(w1_path)) == NULL)
	{
		printf("open dir failure: %s\n", strerror(errno));
		return -2;
	}

	while((direntp = readdir(dirp)) != NULL)  
	{
		strstr(direntp->d_name, "28-");
		strcpy(buf, direntp->d_name);
		find = 1;
		break;
	}

	closedir(dirp);

	if(!find)
	{
		printf("open da18b20 file failure\n");
		return -3;
	}

	strncat(w1_path, buf, sizeof(w1_path)-strlen(w1_path)-1);
	strncat(w1_path, "/w1_slave", sizeof(w1_path)-strlen(w1_path)-1);

	if((fd = open(w1_path, O_RDONLY)) < 0)
	{
		printf("open %s error %s\n", w1_path, strerror(errno));
		return -4;
	}

	memset(buf, 0, sizeof(buf));
	if(read(fd, buf, sizeof(buf)) < 0)
	{
		printf("read data from %s failure: %s\n", w1_path, strerror(errno));
		return -5;
		goto cleanup;
	}

	tp = strstr(buf, "t=");
	if(!tp)
	{
		printf("fail to get temperature: %s\n", strerror(errno));
		goto cleanup;
	}

	tp+=2;
	temp = atof(tp)/1000; 
	return temp;

	cleanup:
		close(fd);
		return 0;

}


