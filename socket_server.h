/********************************************************************************
 *      Copyright:  (C) 2026 He Junfei<hejunfei2005@163.com>
 *                  All rights reserved.
 *
 *       Filename:  socket_server.h
 *    Description:  This file 
 *
 *        Version:  1.0.0(2026年03月25日)
 *         Author:  He Junfei <hejunfei2005@163.com>
 *      ChangeLog:  1, Release initial version on "2026年03月25日 12时49分04秒"
 *                 
 ********************************************************************************/
#ifndef SOCKET_SERVER_H
#define SOCKET_SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <unistd.h>

extern int g_stop;

void print_usage(char *program);

void sig_handler(int signum);

int callback(void *NotUsed,int argc, char *argv[], char **azColName);
				
#endif
