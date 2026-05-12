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
#include <signal.h>

#include "socket_client.h"
#include "ds18b20.h"
#include "packet.h"
#include "database.h"
#include "cJSON.h"
#include "log.h"
#include "socket.h"

static volatile sig_atomic_t g_running = 1;

static void sig_handler(int signum)
{
	g_running = 0;
}

static void print_usage(char *program)
{
	printf("%s usage: \n", program);
	printf("  -i(--ipaddr): specify server ip address.\n");
	printf("  -p(--port): specify server port.\n");
	printf("  -s(--sleeptime): sleep time setting\n");
	printf("  -d(--dnr): domain name resolution.\n");
	printf("  -h(--help): print this help information.\n");
	printf("if already have dnr, ipaddr is not necessary\n");
}

void get_time(char *time_str, size_t time_len)
{
    time_t t = time(NULL);
    strftime(time_str, time_len, "%Y-%m-%d %H:%M:%S", localtime(&t));
}

int wait_until(long sleep_time)
{
    static time_t   last = 0;
    time_t          now = time(NULL);

    if(difftime(now, last) >= sleep_time)
    {
        last = now;
        return 1;
    }
    return 0;
}

int main(int argc, char *argv[])
{
	/*socket var*/
    socket_t                sock;
	char                    host[64];
    int						port = 0;
	char 					buf[512];
    data_t                  data;

    /*temp var*/
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

    int                     sample_flag = 0;
	int 					rc = 0;
	sqlite3 				*db = NULL;

	memset(host, 0, sizeof(host));
	memset(&sock, 0, sizeof(sock));
	sock.fd = -1;

	while ((ch = getopt_long(argc, argv, "i:p:hs:d:", opts, NULL)) != -1)
	{
		switch(ch)
		{
			case 'i':
				strncpy(host, optarg, sizeof(host) - 1);
				break;

			case 'p':
				port = atoi(optarg);
				break;

			case 's':
				sleep_t = atoi(optarg);
				break;

			case 'd':
				strncpy(host, optarg, sizeof(host) - 1);
				break;

			case 'h':
				print_usage(argv[0]);
				return 0;

			default:
				print_usage(argv[0]);
				return -1;
		}
	}

	if( !host[0] || !port)
	{
		print_usage(argv[0]);
		return -1;
	}

	signal(SIGINT, sig_handler);
	signal(SIGTERM, sig_handler);

    socket_init(&sock, host, port);

	/* Initialize local database */
	if(db_open(&db) < 0)
    {
        log_error("初始化本地数据库失败");
        return -1;
    }

    while(g_running)
	{
        sample_flag = 0;
        
        if( wait_until(sleep_t))
        {
            get_devid(&data, 1);
		    get_time(data.time, sizeof(data.time));
		    if(read_temperature(&(data.temperature)) < 0)
		    {
		    	log_error("读取温度失败: %s", strerror(errno));
			    continue;
		    }

		    if(date_packet(&data, buf, sizeof(buf)) < 0)
		    {
		    	log_error("数据打包失败(%s, 温度: %.2f)", data.time, data.temperature);
		    	continue;
		    }
            sample_flag = 1;
        }

        if(socket_check(&sock) < 0)
        {
            socket_connect(&sock);
        }

        if(socket_check(&sock) < 0)
        {
            if(sample_flag == 1)
            {
                db_write(db, buf);
                continue;
            }
        }

        if(sample_flag == 1)
        {
		    rc = socket_send(&sock, buf, strlen(buf));
	    	if(rc < 0)
		    {
		    	log_info("数据存入本地临时库");
                db_write(db, buf);
                continue;
		    }
    	}

        db_read(db, buf, sizeof(buf), &sock);
	}

	log_info("收到退出信号，正在清理资源...");
	if(sock.fd >= 0)
    {	
        socket_close(&sock);
    }

    if(db)
    {
        db_close(db);
    }

    return 0;
}
