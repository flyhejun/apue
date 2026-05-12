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
#include "socket.h"

#define MAX_EVENTS		1024
#define	BACKLOG			13			/*MAX LISTEN FDS  */

volatile sig_atomic_t g_stop = 0;

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
	printf("  -p(--port): specify listen port.\n");
	printf("  -d(--debug): run in foreground (debug mode).\n");
	printf("  -h(--help): print this help information.\n");
	return ;
}


int main (int argc, char **argv)
{
	/*SOCEKT var*/
	int						listen_fd = -1;
	int 					reuse = 1;
	struct sockaddr_in		serv_addr;
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
							{"debug", no_argument, NULL, 'd'},
							{"help", no_argument, NULL, 'h'},
							{NULL, 0, NULL, 0}
	};

	/*epoll var*/
	int						epfd;
	int						ep_fds[MAX_EVENTS];
	int 					i,j;
	struct epoll_event		events[MAX_EVENTS];

	while((ch = getopt_long(argc, argv, "p:dh", opts, NULL)) != -1)
	{
		switch(ch)
		{
			case 'p':
				port = atoi(optarg);
				break;

			case 'd':
				debug = 1;
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

	listen_fd = socket_init();
	if(listen_fd < 0)
	{
		return -1;
	}
	log_info("成功创建sock[%d]", listen_fd);

	setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

	if(socket_bind(listen_fd, port, &serv_addr) < 0)
	{
		socket_close(listen_fd);
		return -2;
	}
	printf("socket[%d] bind on port[%d] successfully!\n", listen_fd, port);

	listen(listen_fd, BACKLOG);

	if(db_open(&db) < 0)
	{
		log_error("数据库初始化失败");
		socket_close(listen_fd);
		return -4;
	}
	log_info("数据库和数据表连接成功");
	
	if((epfd = socket_epoll_init(listen_fd)) < 0)
	{
		socket_close(listen_fd);
		db_close(db);
		return -3;
	}

	while(!g_stop)
	{
		j = socket_epoll(epfd, listen_fd, events, MAX_EVENTS, ep_fds);

		for(i=0; i<j; i++)
		{
			memset(buf, 0, sizeof(buf));
			rv = socket_recv(ep_fds[i], buf, sizeof(buf));
			if(rv <= 0)
			{
				log_error("socket[%d] 断开连接", ep_fds[i]);
				epoll_ctl(epfd, EPOLL_CTL_DEL, ep_fds[i], NULL);
				socket_close(ep_fds[i]);
				continue;
			}                    
			if(db_write(db, buf) == 0)
			{                    
				log_info("成功记录新数据");
			}
		}
	}


	socket_close(epfd);
	db_close(db);
	socket_close(listen_fd);
	return 0;
}

