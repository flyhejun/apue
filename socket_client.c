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
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <time.h>
#include <getopt.h>
#include "socket_client.h"

void print_usage(char *program);

double get_temp();

int main(int argc, char *argv[])
{
	char					*servip = NULL;
	char					*dns = "www.hejunfei.com";
	int						port = 0;
	int						fd1 = -1;
	struct sockaddr_in 		serv_addr;
	char 					buf[1024];
	char					buf_ls[512];
	time_t					t;	
	struct tm				*lt;	
	double					temp = 23.0;
	int 					rv;
	int						sleep_t;

	int						ch;
	struct option opts[] = {
		{"ipaddr", required_argument, NULL, 'i'},
		{"port", required_argument, NULL, 'p'},
		{"help", no_argument, NULL, 'h'},
		{"sleeptime",required_argument, NULL, 's'},
		{"dnr", optional_argument, NULL, 'd'},
		{NULL, 0, NULL, 0}
	};

	int 					rc;
	struct addrinfo 		hints;
	struct addrinfo			*result=NULL;
	struct sockaddr_in		*dnsip;
	
	while ((ch = getopt_long(argc, argv, "i:p:h", opts, NULL)) != -1)
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

	if(dns != "www.hejunfei.com")
	{
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
	
		if((rc = getaddrinfo(dns, (char *)port, &hints, &result)) != 0)
		{
			printf("getaddrinfo [%s:%d] failure: %s\n", dns, port);
			return -2;
		}
	
		if(result != 0)
		{
			dnsip = (struct sockaddr_in *)result->ai_addr;	
			inet_ntop(AF_INET, &(dnsip->sin_addr), servip, sizeof(servip));
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
	if(connect(fd1, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0);
	{
		printf("Connect to server [%s:%d] failure: %s\n", servip, port, strerror(errno));
		return -4;
	}
	printf("Connect to server [%s:%d] sucessfully!\n");

	while(1)
	{
		t = time(NULL);
		lt = localtime(&t);
		strftime(buf_ls, sizeof(buf_ls), "%Y-%m-%d %H:%M:%S", lt);

		strncat(buf, "rpi3b001,", sizeof(buf)-strlen(buf)-1);
		strncat(buf, buf_ls, sizeof(buf)-strlen(buf)-1);
		memset(buf_ls, 0, sizeof(buf_ls));
		if((temp = get_temp()) <= 0)
		{
			printf("get temperature failed: %s\n", strerror(errno));
			return -5;
		}
		strncat(buf, ",", sizeof(buf)-strlen(buf)-1);
		strcpy(buf_ls, (char *)&temp);
		strncat(buf, buf_ls, sizeof(buf)-strlen(buf)-1);

		if(write(fd1, buf, sizeof(buf)) < 0)
		{
			printf("Write data to server [%s:%d] failure: %s\n", servip, port, strerror(errno));		
			goto cleanup;
		}
		printf("Write to server[%s:%d] successfully\n", servip, port);

		memset(buf, 0, sizeof(buf));
		rv = read(fd1, buf, sizeof(buf));
		if(rv < 0)
		{
			printf("read from [%s:%d] failure: %s\n", servip, port, strerror(errno));
			return -6;
		}
		else if( rv == 0)
		{	printf("lose connect with [%s:%d]\n", servip, port);
			goto cleanup;
		}
		printf("read %d bytes: %s\n", rv, buf);

		sleep(sleep_t);
	}
	

cleanup:
	close(fd1);
	return 0;
}
