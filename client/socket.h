/********************************************************************************
 *      Copyright:  (C) 2026 fly studio
 *                  All rights reserved.
 *
 *       Filename:  socket_cli.h
 *    Description:  This file 
 *
 *        Version:  1.0.0(02/05/26)
 *         Author:  He Junfei <hejunfei2005@163.com>
 *      ChangeLog:  1, Release initial version on "02/05/26 14:39:28"
 *                 
 ********************************************************************************/

#ifndef SOCKET_CLI_H
#define SOCKET_CLI_H

#ifdef __cplusplus
extern "C" {
#endif	

#include <stddef.h>

typedef struct socket_s
{   
    char        serv_host[64];          
    int         port;
    int         fd;
} socket_t;

extern int socket_init(socket_t *sock, char *host, int port);

extern int socket_connect(socket_t *sock);

extern int if_connected(socket_t *sock);

#ifdef __cplusplus
}
#endif

#endif
