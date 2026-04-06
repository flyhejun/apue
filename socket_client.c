/*********************************************************************************
 *      Copyright:  (C) 2026 He Junfei<hejunfei2005@163.com>
 *                  All rights reserved.
 *
 *       Filename:  socket_client.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(16/03/26)
 *         Author:  He Junfei <hejunfei2005@163.com>
 *      ChangeLog:  1, Release initial version on "16/03/26 14:27:17"
 *                 
 ********************************************************************************/
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <time.h>
#include <getopt.h>
#include <sqlite3.h>
#include "socket_client.h"
#include "ds18b20.h"
#include "packet.h"
#include "database.h"
#include "cJSON.h"
				
static void print_usage(char *program)
{
	printf("%s usage: \n", program);
	printf("  -i(--ipaddr): sepcify server port.\n");
	printf("  -p(--port): sepcify server port.\n");
	printf("  -s(--sleep: sleep time setting\n");
	printf("  -d(--dnr): donmain name resolution.\n");
	printf("  -h(--Help): print this help information.\n");
	printf("if already have dnr, ipaddr isnot necessary\n");
}

void get_time(char *time_str, size_t time_len)
{
	time_t 			t;
	struct tm		*lt = NULL;

	t = time(NULL);
	lt = localtime(&t);
	strftime(time_str, time_len, "%Y-%m-%d %H:%M:%S", lt);
}


int main(int argc, char *argv[])
{
	/*socket var*/
	char					*servip = NULL;
	char					ip_buf[64];
	char					*dns = "www.123.com";
	int						port = 0;
	char					port_buf[8];
	int						fd1 = -1;
	struct sockaddr_in 		serv_addr;

	char 					buf[512];	
	/*time var*/
	char					time[64];
 	/*temp var*/
	double					*temp;
	char					temp_buf[32];
	
	int 					rv;
	int						sleep_t = 5;

	int						ch;
	struct option opts[] = {
		{"ipaddr", required_argument, NULL, 'i'},
		{"port", required_argument, NULL, 'p'},
		{"help", no_argument, NULL, 'h'},
		{"sleeptime",required_argument, NULL, 's'},
		{"dnr", required_argument, NULL, 'd'},
		{NULL, 0, NULL, 0}
	};

	int 					rc;
	struct addrinfo 		hints;
	struct addrinfo			*result=NULL;
	struct sockaddr_in		*dnsip;
	
	int						rs = 0;
	int						cout = 0;
	sqlite3_stmt			*stmt;
	sqlite3 				*db;
	char					*sql = NULL;

	while ((ch = getopt_long(argc, argv, "i:p:h:s:d", opts, NULL)) != -1)
	{
		switch(ch)
		{
			case 'i':
				servip = optarg;
				break;

			case 'p':
				port = atoi(optarg);
				break;

			case 's':
				sleep_t = atoi(optarg);
				break;

			case 'd':
				dns = optarg;
				break;

			case 'h':
				print_usage(argv[0]);
				break;
		}
	}
	
	if( !servip || !port)
	{
		print_usage(argv[0]);
		return -1;
	}

	if(dns != "www.123.com")
	{
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
	
		snprintf(port_buf, sizeof(port_buf), "%d", port);
		if((rc = getaddrinfo(dns, port_buf, &hints, &result)) != 0)
		{
			printf("getaddrinfo [%s:%d] failure: %s\n", dns, port);
			return -2;
		}
	
		if(result != 0)
		{
			dnsip = (struct sockaddr_in *)result->ai_addr;	
			inet_ntop(AF_INET, &(dnsip->sin_addr), ip_buf, sizeof(ip_buf));
			strcpy(servip, ip_buf);
		}
	}

	fd1 = socket(AF_INET, SOCK_STREAM, 0);
	if(fd1 < 0)
	{
		printf("create socket failure: %s\n", strerror(errno));
		return -3;
	}

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	inet_aton(servip, &serv_addr.sin_addr);
	if(connect(fd1, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		printf("Connect to server [%s:%d] failure: %s\n", servip, port, strerror(errno));
		return -4;
	}
	printf("Connect to server [%s:%d] sucessfully!\n");

	while(1)
	{
		get_time(time, sizeof(time));
	
		if(read_temperature(temp) < 0)
		{
			printf("get temperature failed: %s\n", strerror(errno));
			return -5;
		}
	
		if(date_packet(time, temp, buf, sizeof(buf)) < 0)
		{
			printf("set data format failure: %s\n", strerror(errno));
			return -6;
		}

		rc = write(fd1, buf, strlen(buf));
		if(rc <= 0 && cout < 100)
		{
			const char 			*id_buf = NULL;
			const char			*time_buf = NULL;
			double				temp_buf = 0;
		
			printf("Connection closed by accident...try to connect\n");
			if((fd1=connect(fd1, (struct sockaddr *)&serv_addr, sizeof(serv_addr)))<0)
			{
				cout+=1;
				temp_data_in(db, buf);
			}
			else if(fd1 > 0)
			{		
				printf("Reconnect successfully, send data again\n");
				sql = "SELECT id, time, temperature FROM temp_recds";
				rs = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
				if(rs != SQLITE_OK)
				{
					printf("prepare failure；%s\n", sqlite3_errmsg(db));
					sqlite3_close(db);
					return -7;
				}
				while((rs = sqlite3_step(stmt)) == SQLITE_ROW)
				{
					memset(buf, 0, sizeof(buf));
					id_buf = sqlite3_column_text(stmt, 0);
					time_buf = sqlite3_column_text(stmt, 1);
					temp_buf = sqlite3_column_double(stmt, 2);
					char *buf_id = strdup(id_buf);
					char *buf_time = strdup(time_buf);
					date_packet(buf_time, &temp_buf, buf, sizeof(buf));
					write(fd1, buf, strlen(buf));
					
					free(buf_id);
					free(buf_time);
				}
				cout = 0;	

			}
		}
		else
		{
			printf("Write %d bytes data successfully\n", rc);
		}
		sleep(sleep_t);
	}
	
	close(fd1);
	return 0;
}
