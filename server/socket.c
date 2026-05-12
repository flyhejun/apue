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
#include <sys/epoll.h>
#include <fcntl.h>
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
	freeaddrinfo(result);
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

int socket_bind(int fd, int port, struct sockaddr_in *serv_addr)
{
	memset(serv_addr, 0, sizeof(struct sockaddr_in));
	serv_addr->sin_family = AF_INET;
	serv_addr->sin_port = htons(port);
	serv_addr->sin_addr.s_addr = htonl(INADDR_ANY);

	if(bind(fd, (struct sockaddr *)serv_addr, sizeof(struct sockaddr_in)) < 0)
	{
		log_error("绑定到端口: %d 失败，错误分析: %s", port, strerror(errno));
		return -1;
	}

	log_info("绑定到指定端口[%d] 成功！", port);
	return 0;
}

int socket_epoll_init(int fd)
{
	int 				epfd;
	struct epoll_event 	ev;

	epfd = epoll_create1(0);
	if(epfd < 0)
	{
		log_error("创建epoll进程失败: %s", strerror(errno));
		return -1;
	}

	ev.events = EPOLLIN;
	ev.data.fd = fd;
	if(epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) < 0)
	{
		log_error("添加监听事件失败: %s", strerror(errno));
		return -2;
	}
	log_info("添加监听事件成功！");
	return epfd;
}

int socket_epoll(int epfd, int fd, struct epoll_event *events, size_t events_size, int *fds)
{
	struct epoll_event		ev;
	int 					nfds;
	int 					i;
	int 					j = 0;
	int 					ep_fd;
	int						cli_fd;
	struct sockaddr_in 		cli_addr;
	socklen_t				addr_len = sizeof(cli_addr);

	nfds = epoll_wait(epfd, events, events_size, -1);
	if(nfds <= 0)
	{
		log_error("无事件发生/发生错误: %s", strerror(errno));
		return 0;
	}

	for(i=0; i<nfds; i++)
	{
		ep_fd = events[i].data.fd;
		if(ep_fd == fd)
		{
			cli_fd = accept(fd, (struct sockaddr *)&cli_addr, &addr_len);
			if(cli_fd < 0)
			{
				log_error("获取新的client_fd 失败: %s", strerror(errno));
				continue;
			}
			log_info("连接上了新客户端[%s:%d], fd[%d]",
					inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), cli_fd);
			
			ev.events = EPOLLIN | EPOLLRDHUP;
			ev.data.fd = cli_fd;
			if(epoll_ctl(epfd, EPOLL_CTL_ADD, cli_fd, &ev) < 0)
			{
				log_error("添加新用户端失败");
				close(cli_fd);
			}
		}

		else if(events[i].events & EPOLLIN)
		{
			fds[j] = ep_fd;
			j++;
		}
	}
	return j;
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

int socket_accept(int fd)
{
	int						cli_fd;
	struct sockaddr_in		cli_addr;
	socklen_t				addr_len = sizeof(cli_addr);

	cli_fd = accept(fd, (struct sockaddr *)&cli_addr, &addr_len);
	if(cli_fd < 0)
	{
		log_error("接受新连接失败: %s", strerror(errno));
		return -1;
	}

	log_info("连接上了新客户端[%s:%d], fd[%d]",
			inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), cli_fd);
	return cli_fd;
}

int socket_recv(int fd, char *buf, size_t buf_len)
{
	int		rc;

	if(fd < 0 || !buf)
		return -1;

	rc = recv(fd, buf, buf_len - 1, 0);
	if(rc < 0)
	{
		log_error("接收数据失败: %s", strerror(errno));
		return -1;
	}

	if(rc > 0)
		buf[rc] = '\0';

	return rc;
}

int socket_send(int fd, const char *buf, size_t len)
{
	int		rc;

	if(fd < 0 || !buf)
		return -1;

	rc = send(fd, buf, len, 0);
	if(rc < 0)
	{
		log_error("发送数据失败: %s", strerror(errno));
		return -1;
	}

	return rc;
}

int socket_close(int fd)
{
	if(fd < 0)
		return -1;

	close(fd);
	return 0;
}
