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

#ifndef SOCKET_SOCK_H
#define SOCKET_SOCK_H

#ifdef __cplusplus
extern "C" {
#endif	

#include <stddef.h>

extern int socket_init();
extern int socket_bind(int fd, int port, struct sockaddr_in *serv_addr);
extern int domain_handle(char *domain_name, int port, char *serv_ip);

extern int socket_connect(int fd, char *serv_ip, int port, struct sockaddr_in *serv_addr);

extern int socket_reconnect(struct sockaddr_in *serv_addr, int cout);

extern int socket_epoll_init(int fd);

extern int socket_epoll(int epfd, int fd, struct epoll_event *events, size_t events_size, int *fds);

extern int socket_accept(int fd);

extern int socket_recv(int fd, char *buf, size_t buf_len);

extern int socket_send(int fd, const char *buf, size_t len);

extern int socket_close(int fd);

#ifdef __cplusplus
}
#endif

#endif
