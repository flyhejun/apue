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
#include "log.h"
#include "socket.h"
				
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

int wait_until(long sleep_time)
{
    static time_t   last = 0;
    time_t          now = time(NULL);

    if(last == 0 || difftime(now, last) >= sleep_time)
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
	struct sockaddr_in 		serv_addr;
	char 					buf[512];	
	
    data_t                  data;
    /*time var*/
	char					time[64];
 	
    /*temp var*/
	double					temp = 0.0;
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
	int						rs = 0;
	int						cout = 0;
	sqlite3_stmt			*stmt;
	sqlite3 				*db = NULL;
	int						updata_count = 0;

	while ((ch = getopt_long(argc, argv, "i:p:h:s:d", opts, NULL)) != -1)
	{
		switch(ch)
		{
			case 'i':
				host = optarg;
				break;

			case 'p':
				port = atoi(optarg);
				break;

			case 's':
				sleep_t = atoi(optarg);
				break;

			case 'd':
				host = optarg;
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

    socket_init(&sock, host, port);

	/* Initialize local database */
	if(temporary_repo(&db) < 0)
    {
        continue;
    }
    
    while(1)
	{

        if( wait_until(sleep_t))
        {     
            get_devid(&data, 001);  
		    get_time(&(data.time), sizeof(data.time));
		    if(read_temperature(&(data.temperature)) < 0)
		    {
		    	log_error("读取温度失败: %s", strerror(errno));
			    continue;
		    }
	
		    if(date_packet(&data, buf, sizeof(buf)) < 0)
		    {
		    	log_error("数据打包失败(%s, 温度: %.2f)", time, temp);
		    	continue;
		    }
            sample_flag = 1;
        }

        if(if_connected(&sock) < 0)
        {
            socket_connect(&sock);
        }

        if(if_connected(&sock) < 0)
        {
            if(sample_flag == 1)
            {
                temp_data_in(db, buf);
                sample_flag = 0;
                continue;
            }
        }
        
        if(sample_flag == 1)
        {
		    rc = write(fd1, buf, strlen(buf));
	    	if(rc < 0)
		    {
		    	log_info("数据存入本地临时库");	  
                temp_data_in(db,buf);
                sample_flag = 0;
                continue;
		    }   

		    else
		    {
	    		log_info("发送%d个字节数据成功", rc);  
                tempo_updata(db, buf, sizeof(buf), sock->fd);
	    	}

    	}
	
    	close(fd1);

    	return 0;
}
