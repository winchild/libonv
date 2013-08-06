/**
 * @file log.h
 * @brief 로그 라이브러리 헤더
 */

/*
 * 로그 라이브러리 헤더
 *
 * AUTHOR:
 *
 * Copyright 1999 Haninet, Inc.  All rights reserved. (by 김장동 gumdong@haninet.co.kr)
 * Copyright 2007 Samjung Data Service, Inc.  All rights reserved. (by 김기태 superkkt@sds.co.kr)
 * Copyright 2010 OneNetView, Inc.  All rights reserved. (방창현 winchild@kldp.org)
 *
 */

#ifndef	LOG_H
#define	LOG_H


/*
 * 로그 레벨
 */
#define FATAL	1
#define ERROR	2
#define WARN	3
#define INFO	4
#define DEBUG	5

#define Log(mode, format, ...) log_write(mode, __FILE__, __LINE__, format, ##__VA_ARGS__)
#include<stdio.h>

int log_init(const char *filename, int level);
void log_write(int mode, char *filename, int line, char *format, ...);
size_t get_time(char *buf, char *type, size_t size);

#define MAX_ERRMSG	8192

#endif
