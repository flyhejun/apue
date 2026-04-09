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
			 servip ? servip : "(null)", port, sleep_t, (dns != "www.123.com") ? dns : "(未使用)");

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
			log_error("域名解析失败: %s:%p, 错误解析: %s", dns, port, gai_strerror(rc));
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
		log_error("创建socket失败: %s"，strerror(errno));
		return -3;
	}

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	inet_aton(servip, &serv_addr.sin_addr);
	if(connect(fd1, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		log_error("与服务器端 %s:%d 连接失败: %s", servip, port, strerror(errno));
		return -4;
	}
	log_info("成功链接服务器 %s:%d", servip, port);

	while(1)
	{
		get_time(time, sizeof(time));
	
		if(read_temperature(temp) < 0)
		{
			log_error("读取温度失败: %s", strerror(errno));
			return -5;
		}
		log_debug("当前时间: %s, 温度: %,2f", time, *temp);
	
		if(date_packet(time, temp, buf, sizeof(buf)) < 0)
		{
			log_error("数据打包失败(%s, 温度: %.2f)", time, *temp);
			return -6;
		}
		log_trace("数据完成打包: %s", buf);

		rc = write(fd1, buf, strlen(buf));
		if(rc < 0)
		{
				close(fd1);
				temporary_repo(db);
				log_warn("连接意外关闭，尝试重连(第%d次)", cout+1);
				fd1 = socket(AF_INET, SOCK_STREAM, 0);
				if(connect(fd1, (struct sockaddr *)&serv_addr, sizeof(serv_addr))==-1)
				{
					log_error("重连失败: %s", strerror(errno));
					cout+=1;
					log_info("数据存入本地临时库(第%d批)", cout);
					temp_data_in(db, buf);
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
				sql = "SELECT id, time, temperature FROM temp_recds";
				rs = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
				if(rs != SQLITE_OK)
				{
					log_error("SQL准备失败: %s", sqlite3_errmsg(db));
					sqlite3_close(db);
				}

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
					}
					else if(rc <= 0)
					{
						log_error("失去连接，错误: %s", strerror(errno));
						break;
					}
				}

		}

			sleep(sleep_t);
	}
	
	close(fd1);

	return 0;
}
