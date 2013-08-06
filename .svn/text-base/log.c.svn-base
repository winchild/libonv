/**
 * @file log.c
 * @brief 로그 라이브러리
 */

/*
 * $Id: log.c,v 1.17 2010/10/07 12:47:40 winchild Exp $
 *
 * 로그 라이브러리
 *
 * AUTHOR:
 *
 * Copyright 1999 Haninet, Inc.  All rights reserved. (by 김장동 gumdong@haninet.co.kr)
 * Copyright 2007 Samjung Data Service, Inc.  All rights reserved. (by 김기태 superkkt@sds.co.kr)
 * Copyright 2010 OneNetView, Inc.  All rights reserved. (방창현 winchild@kldp.org)
 *
 * REFERENCE:
 *   splint -exportlocal -paramuse -usedef -compdef -retvalint -retvalother -nullpass -nestcomment -unrecog -preproc -warnposix log.c
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <time.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include "log.h"


#if defined(OS_CYGWIN)
#define NEW_LINE    "\r\n"    /**< 줄바꿈 문자 */
#else
#define NEW_LINE    "\n"      /**< 줄바꿈 문자 */
#endif

static int log_level = DEBUG;
static char log_file[PATH_MAX + 1] = { (char) 0};

size_t get_time(char *buf, char *type, size_t size);
static int log_open(const char *filename);


/**
 * @brief 현재 시간을 지정된 형식으로 반환하는 함수 
 * @param buf - 변환된 시간을 저장할 메모리
 * @param type - 변환 형식(ex: "Y-M-D")
 * @param size - \a buf 의 길이
 * @return 변환된 시간 스트링의 바이트수
 */
size_t get_time(char *buf, char *type, size_t size)
{
    time_t tp;
    struct tm *tm;

    tp = time(NULL);
    tm = localtime(&tp);

    return strftime(buf, size, type, tm);
}


/**
 * @brief 로그파일 초기화
 * @param filename - 오픈할 로그파일명
 * @param level - 로그기록 레벨
 * @return
 *  성공시 0,\n
 *  실패시 -1
 */
int
log_init(const char *filename, int level)
{
	int fd;
    log_level = level;


    if (filename)
    {
		// 파일 오픈여부 확인.
		snprintf(log_file, sizeof(log_file), "%s", filename);
		if ((fd = log_open(log_file)) < 0) return -1;
		close(fd);

		// 리얼패스가 아니면, 재설정.
		if (filename[0] != '/')
		{
		    memset(log_file, 0, sizeof(log_file));
		    if (!realpath(filename, log_file)) return -1;
		}
    }

    return 0;
}


/**
 * @brief 로그파일 오픈 
 * @param filename - 오픈할 로그파일명
 * @return
 *  성공시 log file fd,\n
 *  실패시 -1
 */
static int
log_open(const char *filename)
{
    int fd;

    /* filename이 NULL이면 stderr에 로그 출력 */
    if(filename) {
	fd = open(filename, O_WRONLY | O_CREAT | 
		O_APPEND, S_IRUSR | S_IWUSR); 
    }
    else {
	fd = STDERR_FILENO;
    }

    return fd;
}


/**
 * @brief 로그기록 함수 (실제 코드에서는 Log라는 메크로를 사용)
 * @param mode - 로그레벨
 * @param filename - 이 함수를 실행하는 소스파일명
 * @param line - 이 함수를 실행하는 라인넘버
 * @param format - 로그 문자열(가변인자)
 * @return 없음
 */
void
log_write(int mode, char *filename, int line, char *format, ...)
{
	int fd;
    va_list ap;
    char tmp[MAX_ERRMSG];
    char buf[MAX_ERRMSG];
    char cur_time[26];

    /* 지정된 로그레벨 이상만 기록 */
    if(mode > log_level) {
		return;
    }

    va_start(ap, format);
    (void) vsnprintf(tmp, MAX_ERRMSG, format, ap);

    /* 현재시간 확인 */
//    get_time(cur_time, "[%b %d %H:%M:%S]", sizeof(cur_time));	
    (void) get_time(cur_time, "%m-%d %T", sizeof(cur_time));	

    switch(mode) {
#ifdef ENABLE_DEBUG
#ifdef NOCOMPILE
	/*
	 * DEBUG configure option이 주어지면 ERROR와 FATAL 기록시 호출자의
	 * 파일네임과 라인넘버를 같이 기록한다.
	 */
	case DEBUG:
	    snprintf(buf, MAX_ERRMSG, "%s [%d] [DEBUG] [%s:%d] %s" NEW_LINE, 
		    cur_time, (int) getpid(), filename, line, tmp);
	    break;
	case INFO:
	    snprintf(buf, MAX_ERRMSG, "%s [%d] [INFO] [%s:%d] %s" NEW_LINE, 
		    cur_time, (int) getpid(), filename, line, tmp);
	    break;
	case WARN:
	    snprintf(buf, MAX_ERRMSG, "%s [%d] [WARN] [%s:%d] %s" NEW_LINE, 
		    cur_time, (int) getpid(), filename, line, tmp);
	    break;
	case ERROR:
	    snprintf(buf, MAX_ERRMSG, "%s [%d] [ERROR] [%s:%d] %s" NEW_LINE, 
		    cur_time, (int) getpid(), filename, line, tmp);
	    break;
	case FATAL:
	    snprintf(buf, MAX_ERRMSG, "%s [%d] [FATAL] [%s:%d] %s" NEW_LINE, 
		    cur_time, (int) getpid(), filename, line, tmp);
	    break;
#endif	// ifdef NOCOMPILE
	case DEBUG:
	    snprintf(buf, MAX_ERRMSG, "%s [DEBUG] [%s:%d] %s" NEW_LINE, 
		    cur_time, filename, line, tmp);
	    break;
	case INFO:
	    snprintf(buf, MAX_ERRMSG, "%s [INFO] [%s:%d] %s" NEW_LINE, 
		    cur_time, filename, line, tmp);
	    break;
	case WARN:
	    snprintf(buf, MAX_ERRMSG, "%s [WARN] [%s:%d] %s" NEW_LINE, 
		    cur_time, filename, line, tmp);
	    break;
	case ERROR:
	    snprintf(buf, MAX_ERRMSG, "%s [ERROR] [%s:%d] %s" NEW_LINE, 
		    cur_time, filename, line, tmp);
	    break;
	case FATAL:
	    snprintf(buf, MAX_ERRMSG, "%s [FATAL] [%s:%d] %s" NEW_LINE, 
		    cur_time, filename, line, tmp);
	    break;
#else
	case DEBUG:
	    snprintf(buf, MAX_ERRMSG, "%s [%d] [DEBUG] %s" NEW_LINE, 
		    cur_time, (int) getpid(), tmp);
	    break;
	case INFO:
	    snprintf(buf, MAX_ERRMSG, "%s [%d] [INFO] %s" NEW_LINE, 
		    cur_time, (int) getpid(), tmp);
	    break;
	case WARN:
	    snprintf(buf, MAX_ERRMSG, "%s [%d] [WARN] %s" NEW_LINE, 
		    cur_time, (int) getpid(), tmp);
	    break;
	case ERROR:
	    snprintf(buf, MAX_ERRMSG, "%s [%d] [ERROR] %s" NEW_LINE, 
		    cur_time, (int) getpid(), tmp);
	    break;
	case FATAL:
	    snprintf(buf, MAX_ERRMSG, "%s [%d] [FATAL] %s" NEW_LINE, 
		    cur_time, (int) getpid(), tmp);
	    break;
#endif

    }

#ifdef ENABLE_DEBUG
	printf (buf);
#endif

    fd = log_open(log_file[0] != '\0' ? log_file : NULL);
	if (fd < 0) return;
    write(fd, buf, strlen(buf));
	close(fd);

    if (mode == FATAL) exit(EXIT_FAILURE);

    va_end(ap);

    /* 로그파일이 있을때만 close */

}

