/*********************************************************************************
 *      Copyright:  (C) 2026 fly studio
 *                  All rights reserved.
 *
 *       Filename:  socket_cli.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(01/05/26)
 *         Author:  He Junfei <hejunfei2005@163.com>
 *      ChangeLog:  1, Release initial version on "01/05/26 15:38:00"
 *                 
 ********************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include "log.h"

int socket_init()
{
	int		fd;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if(fd < 0)
	{
		log_error("创建socket失败: %s", strerror(errno));
		return -1;
	}

	return fd;
}

int domain_handle(char *domain_name, int port, char *serv_ip)
{
	int						rs;
	char					ip_buf[INET_ADDRSTRLEN];
	char					port_buf[8];
	struct addrinfo			hints;
	struct addrinfo			*result;		
	struct sockaddr_in		*domain_ip;

	if(strcmp(domain_name, "www.123.com") == 0)
	{
		return 0; 
	}
	
	else 
	{
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		
		memset(port_buf, 0, sizeof(port_buf));
		snprintf(port_buf, sizeof(port_buf), "%d", port);
		
		if((rs = getaddrinfo(domain_name, port_buf, &hints, &result)) != 0)
		{
			log_error("域名解析失败: %s:%s, 错误解析: %s", domain_name, port_buf, gai_strerror(rs));
			return -1;
		}
		
		domain_ip = (struct sockaddr_in *)result->ai_addr;
		inet_ntop(AF_INET, &(domain_ip->sin_addr), ip_buf, sizeof(ip_buf));
		strcpy(serv_ip, ip_buf);
	}

	return 1;
}

int socket_connect(int fd, char *serv_ip, int port, struct sockaddr_in *serv_addr)
{
	memset(serv_addr, 0, sizeof(struct sockaddr_in));
	serv_addr->sin_family = AF_INET;
	serv_addr->sin_port = htons(port);
	inet_aton(serv_ip, &serv_addr->sin_addr);

	if(connect(fd, (struct sockaddr *)serv_addr, sizeof(struct sockaddr_in)) < 0)
	{
		log_error("与服务器端 %s:%d 连接失败: %s", serv_ip, port, strerror(errno));
		return -1;
	}

	log_info("成功链接服务器 %s:%d", serv_ip, port);
	return 0;
}

int socket_reconnect(struct sockaddr_in *serv_addr, int cout)
{
	int 		cli_fd;

	cli_fd = socket_init();

	if(connect(cli_fd, (struct sockaddr *)serv_addr, sizeof(struct sockaddr_in)) < 0)
	{
		log_error("重连失败: %s", strerror(errno));
		cout++;
		close(cli_fd);
		return -1;
	}
	
	else
	{
		log_info("重连(第%d次)成功,将本地数据传入服务器", cout);
	}

	return cli_fd;
}
