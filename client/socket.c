/*********************************************************************************
 *      Copyright:  (C) 2026 fly studio
 *                  All rights reserved.
 *
 *       Filename:  socket_cli.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(01/05/26)
 *         Author:  He Junfei <hejunfei2005@163.com>
 *      ChangeLog:  1, Release initial version on "01/05/26 15:38:00"
 *                 
 ********************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include "log.h"

int socket_init(socket_t *sock, char *host, int port)
{
    if( !sock || port <= 0 )
        return -1;

    memset(sock, 0, sizeof(*sock));
    sock->fd = -1;
    sock->port = port;
    if(host)
    {
        strncpy(sock->serv_host, host, 64);
    }

	return 0;
}

int socket_connect(socket_t *sock)
{
    int                 rs = -1;
    int                 sockfd = 0;
    char                port_buf[20];
    struct in_addr      inaddr;
    struct addrinfo     *result, hints;
    struct addrinfo     *p;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
        
    if(inet_aton(sock->serv_host, &inaddr))
    {
        hints.ai_flags |= AI_NUMERICHOST;
    }

    snprintf(port_buf, sizeof(port_buf), "%d", sock->port);
    if((rs = getaddrinfo(sock->serv_host, port_buf, &hints, &result)))
    {
        log_error("getaddrinfo() parser [%s:%s] failed: %s\n", sock->serv_host, port_buf, gai_strerror(rs));
        return -1;
    }

    for(p=result; p!=NULL; p=p->ai_next)
    {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if(sockfd < 0)
        {
            log_error("socket() create failed: %s\n", strerror(errno));
            rs = -1;
            continue;
        }

        rs = connect(sockfd, p->ai_addr, p->ai_addrlen);
        if(0 == rs)
        {
            sock->fd = sockfd;
            log_info("Connect to server[%s:%d] on fd[%d] successfully!\n", sock->serv_host, sock->port, sockfd);
            break;
        }

        else
        {
            close(sockfd);
            continue;
        }
    }
    
    freeaddrinfo(result);
    return rs;
}

int socket_check(socket_t *sock)
{
    struct tcp_info     info;
    int                 len = sizeof(info);
    
    if(!sock)
    {
        return -1;
    }

    if( sock->fd < 0)
    {
        return -1;
    }

    getsockopt(sock->fd, IPPROTO_TCP, TCP_INFO, &info, (socklen_t *)&len);
    if( TCP_ESTABLISHED==info.tcpi_state )
    {
        return 0;
    }

    else
    {
        return -1;
    }
}

int socket_send(socket_t *sock, const char *buf, size_t len)
{
    size_t  sent = 0;
    int     rc;

    if(!sock || sock->fd < 0 || !buf)
        return -1;

    while(sent < len)
    {
        rc = send(sock->fd, buf + sent, len - sent, 0);
        if(rc < 0)
        {
            log_error("发送数据失败: %s", strerror(errno));
            return -1;
        }
        sent += rc;
    }

    return sent;
}

int socket_recv(socket_t *sock, char *buf, size_t buf_len)
{
    int     rc;

    if(!sock || sock->fd < 0 || !buf)
        return -1;

    rc = recv(sock->fd, buf, buf_len - 1, 0);
    if(rc < 0)
    {
        log_error("接收数据失败: %s", strerror(errno));
        return -1;
    }

    if(rc > 0)
        buf[rc] = '\0';

    return rc;
}

int socket_close(socket_t *sock)
{
    if(!sock)
        return -1;

    if(sock->fd >= 0)
    {
        close(sock->fd);
        sock->fd = -1;
    }

    memset(sock->serv_host, 0, sizeof(sock->serv_host));
    sock->port = 0;
    return 0;
}
