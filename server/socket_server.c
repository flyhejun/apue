/*********************************************************************************
 *      Copyright:  (C) 2026 He Junfei<hejunfei2005@163.com>
 *                  All rights reserved.
 *
 *       Filename:  socket_server.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(2026年03月25日)
 *         Author:  He Junfei <hejunfei2005@163.com>
 *      ChangeLog:  1, Release initial version on "2026年03月25日 12时32分06秒"
 *                 
 ********************************************************************************/

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <errno.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <signal.h>
#include <sys/types.h>        
#include <sys/socket.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/epoll.h>
#include "socket_server.h"
#include "database.h"
#include "packet.h"
#include "cJSON.h"
#include "log.h"

#define MAX_EVENTS		1024
#define	BACKLOG			13			/*MAX LISTEN FDS  */

int 	g_stop = 0;

void sig_handler(int signum)
{
	switch(signum)
	{
		case SIGTERM:
			write(STDERR_FILENO, "SIGTERM signal detected(kill)\n", 31);
			break;

		case SIGSEGV:
			write(STDERR_FILENO, "SIGSEGV signal detected\n", 24);
			_exit(1);
			break;
			 
		case SIGPIPE:
			write(STDERR_FILENO, "SIGPIPE signal detected(socket error)\n", 38);
			break;
		case SIGINT:
			write(STDERR_FILENO, "SIGINT signal detected(Ctrl0+c)\n", 33);
			break;
	}
	g_stop = 1;
}

void print_usage(char *program)
{
	printf("%s usage: \n", program);
	printf("  -p(--port): sepcify listen port.\n");
	printf("  -h(--Help): print this help information.\n");
	return ; 
}


int main (int argc, char **argv)
{
	/*SOCEKT var*/
	int						listen_fd = -1;
	int						client_fd = -1;
	int 					reuse = 1;
	struct sockaddr_in		serv_addr;
	struct sockaddr_in		cli_addr;
	socklen_t				addr_len = sizeof(cli_addr);
	int						port = 0;

	/*sqlite3 var*/
	sqlite3					*db = NULL;
	char					buf[512];
	int						rv;

	/*the others*/
	int						debug = 0;
	struct rlimit			limit;
	int						ch;
	struct option			opts[] = {
							{"port", required_argument, NULL, 'p'},
							{"help", no_argument, NULL, 'h'},
							{NULL, 0, NULL, 0}
	};

	/*epoll var*/
	int						epfd;
	int						nfds = 0;
	int						fd;
	struct epoll_event		ev, events[MAX_EVENTS];

	while((ch = getopt_long(argc, argv, "p:h", opts, NULL)) != -1)
	{
		switch(ch)
		{
			case 'p':
				port = atoi(optarg);
				break;
			
			case 'h':
				print_usage(argv[0]);
				break;
		}
	}

	getrlimit(RLIMIT_NOFILE, &limit);
	limit.rlim_cur = limit.rlim_max;
	setrlimit(RLIMIT_NOFILE, &limit);

	if(!debug)
	{
		daemon(0, 0);
	}

	signal(SIGTERM, sig_handler);
	signal(SIGSEGV, sig_handler);
	signal(SIGPIPE, sig_handler);
	signal(SIGINT, sig_handler);

	listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(listen_fd < 0)
	{
		log_error("创建listen_fd失败: %s", strerror(errno));
		return -1;
	}
	log_info("成功创建sock[%d]", listen_fd);

	setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if(bind(listen_fd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
	{
		log_error("绑定到端口: %d 失败，错误分析: %s", port, strerror(errno));
		return -2;
	}
	printf("socket[%d] bind on port[%d] successfully!\n", listen_fd, port);

	listen(listen_fd, BACKLOG);

	temporary_repo(db);
	log_info("数据库和数据表连接成功");
	
	epfd = epoll_create1(0);
	if(epfd < 0)
	{
		log_error("创建epoll进程失败: %s", strerror(errno));
		return -3;
	}
	
	ev.events = EPOLLIN;
	ev.data.fd = listen_fd;
	if(epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev) < 0)
	{
		log_error("添加监听事件失败: %s", strerror(errno));
		return -4;
	}

	while(!g_stop)
	{
		nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
		if(nfds <= 0)
		{
			log_error("无事件发生/发生错误: %s", strerror(errno));
			continue;
		}

		for(int i=0; i<nfds; i++)
		{
			fd = events[i].data.fd;
			if(fd == listen_fd)
			{
				client_fd = accept(listen_fd, (struct sockaddr *)&cli_addr, &addr_len);
				if(client_fd < 0)
				{
					log_error("获取新的client_fd 失败: %s", strerror(errno));
					continue;
				}
				log_info("连接上了新客户端[%s:%d], fd[%d]", 
						inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), client_fd);	
				
				ev.events = EPOLLIN | EPOLLRDHUP;
				ev.data.fd = client_fd;
				if(epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev) < 0)
				{
					log_error("添加新用户端失败");
					close(client_fd);
				}
			}
			
			else if(events[i].events & EPOLLIN)
			{	
					memset(buf, 0, sizeof(buf));
					rv = read(fd, buf, sizeof(buf));
					if(rv <= 0)
					{
						log_error("socket[%d] 断开连接", fd);
						epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
						close(fd);
						break;
					}
					if(temp_data_in(db, buf) == 0)
					{
						log_info("成功记录新数据");
					}
			}
		}
	}
	sqlite3_close(db);
	close(listen_fd);
	return 0;
}

