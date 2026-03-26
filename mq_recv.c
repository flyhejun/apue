/*********************************************************************************
 *      Copyright:  (C) 2026 LINGYUN
 *                  All rights reserved.
 *
 *       Filename:  mq_recv.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(13/03/26)
 *         Author:  qyc <qyc@gmail.com>
 *      ChangeLog:  1, Release initial version on "13/03/26 10:28:40"
 *                 
 ********************************************************************************/
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>


#define FTOK_PATH "/dev/zero"
#define FTOK_PROJID 0x21

typedef struct msg_s
{
	long mtype;
	char mtext[512];
} msg_t;

int main(int argc, char *argv[])
{
	key_t 	key;
	msg_t 	msg;
	int 	msgid;
	int 	msgtype = 2;
	int		i;

	if((key = ftok(FTOK_PATH, FTOK_PROJID)) < 0)
	{
		printf("ftok() get IPC token failure: %s\n", strerror(errno));
		return -1;
	}

	msgid = msgget(key, IPC_CREAT|0666);
	if(msgid < 0)
	{
		printf("msgget() create new mq failure: %s\n", strerror(errno));
		return -2;
	}

	printf("key[%d] msgid[%d] msgtype[%d]\n", (int)key, msgid, msgtype);
	
	printf("Waiting for message...\n");
	while(1)
	{
		memset(&msg, 0, sizeof(msg));
		if(msgrcv(msgid, &msg, sizeof(msg.mtext), msgtype, 0) < 0)
		{
			printf("msgsnd() receive message failure: %s\n", strerror(errno));
			break;
		}

		printf("Receive: %s\n", msg.mtext);
		if(strncasecmp(msg.mtext, "exit", 4) == 0)
			break;
	}
	msgctl(msgid, IPC_RMID, NULL);
	return 0;
}
