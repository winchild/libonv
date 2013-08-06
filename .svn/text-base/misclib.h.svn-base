/**
 * @file misclib.h
 * @brief Support Functions Library Header
 */

/*
 * Support Functions Library Header
 *
 * AUTHOR:
 *
 * Copyright 1999 Haninet, Inc.  All rights reserved. (by 김장동 gumdong@haninet.co.kr)
 * Copyright 2007 Samjung Data Service, Inc.  All rights reserved. (by 김기태 superkkt@sds.co.kr)
 * Copyright 2010 OneNetView, Inc.  All rights reserved. (방창현 winchild@kldp.org)
 *
 */

#ifndef	MISCLIB_H
#define	MISCLIB_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include "log.h"

#define ARRAY_SIZE(x)	sizeof(x) / sizeof((x)[0])
#define	FREE(pointer)	do { free(pointer); (pointer) = NULL; } while(0)

#ifdef ENABLE_DEBUG
#define ASSERT(expression) \
    if(!(expression)) \
    { \
	Log(ERROR, "ASSERTION IN '%s': '%s' is NOT true\n", \
		__func__, #expression); \
	abort(); \
    } 
#else 
#define ASSERT(expression)  (void) 0
#endif

int Accept(int fd, struct sockaddr *sa, socklen_t *salenptr);
int daemonize(void);
int socket_listen(int port);
int tcp_Connect(const char *ip, const int port, int timeout);
ssize_t readn_timewait(int fd, void *vptr, size_t n,int msec);
ssize_t writen(int connfd, const char *buf, size_t len);
ssize_t readn(int fd, void *vptr, size_t n);
int udp_sendPacket(char * ip , int port , char *data, int datalen);
void dumpdata(char *filename, char* data,int datalen);
void printbyte(char * buf , int buflen);

time_t ConvertToSecSince1970(char *szYYYYMMDDHHMMSS);
int makeInt(const char *p, int size);
void copyFloatToByte(char * dest, float from);
void cut_CRLF (char *buf);

#define	L2A_MAX_ROW	128
int l2a (char *line_buff, char *arr_ptr[], const char del);
//int l2c (char *line_buff, const char del); -- 무한 LOOP 체크요.
void free_l2a (char *arr_ptr[]);
int count_DELIMITOR (char *line_buff, const char del);
char *only_digit (char *s);
int Exec(char *argv);

#endif
