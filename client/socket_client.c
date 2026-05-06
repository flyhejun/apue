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


int main(int argc, char *argv[])
{
	/*socket var*/
	char					*servip = NULL;
	char					*dns = "www.123.com";
	int						port = 0;
	int						fd1 = -1;
	struct sockaddr_in 		serv_addr;
	char 					buf[512];	
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

	log_info("配置情况： IP:%s, Port:%d, 休眠时间:%d秒, 域名：%s",
			 servip ? servip : "(null)", port, sleep_t, strcmp(dns, "www.123.com") ? dns : "(未使用)");

	if( !servip || !port)
	{
		print_usage(argv[0]);
		return -1;
	}

	if(domain_handle(dns, port, servip) < 0)
	{
		return -2;	
	}

	fd1 = socket_init();
	while(fd1 < 0)
	{
		close(fd1);
		fd1 = socket_init();
	}

	if(socket_connect(fd1, servip, port, &serv_addr) < 0)
	{
		rc = -1;
		while(cout < 5 && rc < 0)
		{
			rc = socket_reconnect(&serv_addr, cout);
			cout++;
		}		
		
		if(rc < 0)
			return -3;
	}

	/* Initialize local database */
	temporary_repo(&db);
	while(1)
	{
		get_time(time, sizeof(time));
	
		if(read_temperature(&temp) < 0)
		{
			log_error("读取温度失败: %s", strerror(errno));
			return -5;
		}
		log_debug("当前时间: %s, 温度: %.2f", time, temp);
	
		if(date_packet(time, &temp, buf, sizeof(buf)) < 0)
		{
			log_error("数据打包失败(%s, 温度: %.2f)", time, temp);
			return -6;
		}
		log_trace("数据完成打包: %s", buf);

		rc = write(fd1, buf, strlen(buf));
		if(rc < 0)
		{
				close(fd1);
				temporary_repo(&db);
				log_warn("连接意外关闭，尝试重连(第%d次)", cout);
				if((fd1 = socket_reconnect(&serv_addr, cout)) < 0)
				{
					log_info("数据存入本地临时库(第%d批)", cout);	
					temp_data_in(db,buf);
				}
				
				else 
				{		
					log_info("重连(第%d次)成功,将本地数据传入服务器", cout);
					cout = 0;
				}
		}

		else
		{
				log_info("发送%d个字节数据成功", rc);
				stmt = data_exist(db);
				if(stmt == NULL)
				{
					sqlite3_close(db);
					continue;
					
				}

				rs = SQLITE_OK;
				log_debug("SQL准备就绪，开始遍历上传");
				while((rs = sqlite3_step(stmt)) == SQLITE_ROW && updata_count < 10)
				{
					tempo_data_in(stmt, buf, sizeof(buf));
					rc = write(fd1, buf, sizeof(buf));
					if(rc > 0)
					{
						log_info("data reupdata.");
						old_data_delete(db, "temp_recds");
						updata_count++;

						sqlite3_finalize(stmt);
						stmt = data_exist(db);  // 重新准备
						if(stmt == NULL)
						{
							log_error("重新准备SQL失败");
							break;
						}
						continue;
					}

					else if(rc <= 0)
					{
						log_error("失去连接，错误: %s", strerror(errno));
						break;
					}	
				}
				updata_count = 0;
		}

			sleep(sleep_t);
	}
	
	close(fd1);

	return 0;
}
