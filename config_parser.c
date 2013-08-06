/**
 * @file config_parser.c
 * @brief 설정파일 파싱 라이브러리
 */

/*
 * 설정파일 파싱 라이브러리
 *
 * AUTHOR:
 *
 * Copyright 1999 Haninet, Inc.  All rights reserved. (by 김장동 gumdong@haninet.co.kr)
 * Copyright 2007 Samjung Data Service, Inc.  All rights reserved. (by 김기태 superkkt@sds.co.kr)
 * Copyright 2010 OneNetView, Inc.  All rights reserved. (방창현 winchild@kldp.org)
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "config_parser.h"
#include "misclib.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#include <limits.h>
#include <sys/stat.h>

#ifndef LINE_MAX
#define LINE_MAX	2048
#endif

/**
 * 설정파일 파라메터 링크드 리스트 구조체
 */
typedef struct config_node_t {
    char *parameter;
    char *value;
    struct config_node_t *next_node;
} config_node;

static config_node *config_value_start = NULL;
static config_node *config_value_end = NULL;

static int config_value_add(const char *parameter, const char *value);
static void config_value_destroy(config_node *start);
static int config_parse(FILE *fp);
static void trim_value(char **value);
static char *str_chr(const char *string);

/** 
 * @brief 파라메터 추가 함수 (config_value_add의 wrapping 함수)
 * @param parameter - 추가할 파라메터
 * @param value - 설정할 값
 * @return
 *  성공 시 0,\n
 *  실패 시 -1
 */
int
config_set_parameter(const char *parameter, const char *value)
{
    ASSERT(parameter != NULL && value != NULL);

    return(config_value_add(parameter, value));
}


/**
 * @brief 링크드 리스트 추가 함수 
 * @param parameter - 추가할 파라메터
 * @param value - 설정할 값
 * @return
 *  성공 시 0,\n
 *  실패 시 -1
 */
static int 
config_value_add(const char *parameter, const char *value)
{
    char *data = NULL;
    config_node *node = NULL;
    size_t size;

    /* +2, +1 = null character */
    if(value) {
	size = strlen(parameter) + strlen(value) + 2;
    }
    else {
	size = strlen(parameter) + 1;
    }

    data = (char *) malloc(size);
    node = (config_node *) malloc(sizeof(config_node));
    if(data == NULL || node == NULL) {
	FREE(data);
	FREE(node);
	return -1;
    }

    node->parameter = data;
    *node->parameter = '\0';
    strcat(node->parameter, parameter);

    if(value) {
	node->value = data + strlen(parameter) + 1;
	*node->value = '\0';
	strcat(node->value, value);
    }
    else {
	node->value = NULL;
    }

    node->next_node = NULL;

    /* 리스트가 존재할 경우 */
    if(config_value_start) {
	config_value_end->next_node = node;
	config_value_end = node;
    }
    else {
	config_value_start = node;
	config_value_end = node;
    }		

    return 0;
}


/**
 * @brief 링크드 리스트 제거 함수 
 * @param start - 링크드 리스트 시작점
 * @return 없음
 */
static void
config_value_destroy(config_node *start)
{
    config_node *next_node = NULL;
    config_node *cur_node = NULL;

    cur_node = start;
    while(cur_node) {
	next_node = cur_node->next_node;
	if(cur_node->parameter) {
	    free(cur_node->parameter);
	}
	free(cur_node);
	cur_node = next_node;
    }
}


/**
 * @brief 파라메터의 설정값 리턴 함수 
 * @param parameter - 검색할 파라메터 이름
 * @return 해당 파라메터 값의 포인터
 */
char *
config_get_value(const char *parameter)
{
    config_node *cur_node = NULL;

    ASSERT(parameter != NULL);

    cur_node = config_value_start;
    while(cur_node) {
	if(strcasecmp(cur_node->parameter, parameter) == 0) {
	    return cur_node->value;
	}
	cur_node = cur_node->next_node;	
    }

    return NULL;
}


/**
 * @brief 파라메터 존재여부 체크 함수 
 * @param parameter - 검색할 파라메터 이름
 * @return
 *  존재하면 1,\n
 *  없으면 0
 */
int
config_check_parameter(const char *parameter)
{
    config_node *cur_node = NULL;

    ASSERT(parameter != NULL);

    cur_node = config_value_start;
    while(cur_node) {
	if(strcasecmp(cur_node->parameter, parameter) == 0) {
	    return 1;
	}
	cur_node = cur_node->next_node;	
    }

    return 0;	
}


/** 
 * @brief 설정파일 처리 함수 
 * @param filename - 설정파일명
 * @param check - 설정파일 유효성 체크 함수 포인터
 * @param err - 에러 발생시 에러메세지 포인터 (호출자 사용 후 FREE)
 * @return
 *  성공시 0,\n
 *  실패시 -1
 */
int
config_read(const char *filename, int (*check)(void **), void **err)
{
    char msg[8192];
    FILE *fp = NULL;
    config_node *prev_config_start = NULL;

    ASSERT(filename != NULL);

    if((fp = fopen(filename, "r")) == NULL) {
	snprintf(msg, sizeof(msg), "%s 파일 열기 실패", filename);	
	*err = strdup(msg);
	return -1;
    }

    prev_config_start = config_value_start;
    config_value_start = NULL;

    /* parsing후 링크드 리스트 생성 */
    if(config_parse(fp) < 0) {
	goto finish;
    }

    if (check && (*check)(err) < 0)
	goto finish;

    /* 새로 설정파일을 읽어들이는데 성공하고나서
     * 기존 설정 리스트를 제거한다
     */
    if(prev_config_start) {
	config_value_destroy(prev_config_start);
    }

    fclose(fp);

    return 0;

finish:
    /* 새로 설정파일을 로드하는데 실패하면 기존
     * 설정 리스트를 복구한다.
     */
    config_value_destroy(config_value_start);
    config_value_start = prev_config_start;

    return -1;
}


/**
 * @brief 공백, 줄바꿈, ', "" 문자 제거 
 * @param value - 처리할 스트링
 * @return 없음
 *
 * value에서 문자열 뒤에 있는 공백과 줄바꿈 문자를 제거하고 value 앞, 뒤가 
 * ' 또는 "로 감싸져 있을때는 역시 제거한다.
 */
static void
trim_value(char **value)
{
    int cnt;

    if(*value == NULL) {
	return;
    }

    while(1) {
	cnt = strlen(*value) - 1;
	if((*value)[cnt] == '\r' || (*value)[cnt] == '\n' ||
		(*value)[cnt] == '\t' || (*value)[cnt] == ' ') {
	    (*value)[cnt] = '\0';
	}
	else {
	    break;
	}
    }

    cnt = strlen(*value) - 1;
    if(((*value)[0] == 0x22 && (*value)[cnt] == 0x22) ||
	    ((*value)[0] == 0x27 && (*value)[cnt] == 0x27)) {
	(*value)[cnt] = '\0';
	(*value)++;
    }
}


/**
 * @brief 문자열의 첫번째 문자의 위치를 반환 
 * @param string - 처리할 스트링
 * @return 첫문자의 포인터
 *
 * 문자열에서 \\r, \\n, \\t, ' ' 을 제외한 나머지 문자가 처음으로 나오는 
 * 위치를 반환하는 함수
 */
static char *
str_chr(const char *string)
{
    int i;

    if(!string) {
	return NULL;
    }

    for(i = 0; i < strlen(string); i++) {
	if(string[i] != '\t' &&    string[i] != ' ' &&
		string[i] != '\r' && string[i] != '\n') {
	    return (char *) &string[i];
	}
    }

    return NULL;
}


/**
 * @brief Config parser 
 * @param fp - 설정파일 포인터
 * @return 
 *  성공 시 0,\n
 *  실패 시 -1
 */
static int
config_parse(FILE *fp)
{
    int line;
    char buf[LINE_MAX];
    char *parameter = NULL;
    char *value = NULL;

    ASSERT(fp != NULL);

    line = 0;
    while(fgets(buf, LINE_MAX, fp) != NULL) {
	line++;
	parameter = str_chr(buf);
	if(parameter) {
	    /* 주석인 경우 */
	    if(*parameter == '#') {
		continue;
	    }

		value = strrchr (buf, '#');
		if (value != NULL) *value = '\0';	// comment delete.

	    /* 파라메터의 끝을 검색(\r, \n, \t, =, ' ') */
	    value = strpbrk(parameter, "\r\n\t= ");
	    if(value) {
		/* 파라메터가 '='으로 끝나면 여기까지가
		 * 파라메터이고 다음칸부터가 value
		 */
		if(*value == '=') {
		    *value++ = '\0';
		}
		/* 파라메터가 탭, 공백, 줄바꿈 문자로 끝나면
		 * 여기까지 파라메터로 인식하고 다음칸에서부터
		 * '=' 검색 시작
		 */
		else {
		    *value++ = '\0';
		    value = strchr(value, '=');
		    if(value) {
			value++;
		    }
		}

		/* 실제값 앞에 있는 공백 제거 */
		value = str_chr(value);
		/* 앞뒤로 감싸져있는 ", ' 제거 */
		trim_value(&value);
	    }

	    if(config_value_add(parameter, value) < 0) {
		return -1;
	    }
	}
    }	

    return 0;
}

/**
 * @brief 현재 로딩된 전체 설정을 배열로 반환
 * @param 없음
 * @return 
 *  성공 시 현재 설정의 배열을 저장하는 메모리 포인터,\n
 *  실패 시 NULL 포인터
 *
 * 이 함수를 통해 리턴된 데이터는 사용 후 config_free_list() 함수를 통해 할당된
 * 자원을 모두 해제하여야 함.
 */
config_list_t *
config_get_list(void)
{
    int i, cnt;
    config_list_t *buf = NULL;
    config_node *cur_node = NULL;

    /*
     * 현재 로딩된 설정 개수 확인
     */
    cnt = 0;
    cur_node = config_value_start;
    while (cur_node) 
    {
	cnt++;
	cur_node = cur_node->next_node;	
    }

    /* +1 is last NULL entry */
    buf = malloc(sizeof(config_list_t) * (cnt + 1));
    if (!buf)
	return NULL;

    for (i = 0, cur_node = config_value_start; i < cnt && cur_node; 
	    i++, cur_node = cur_node->next_node)
    {
	buf[i].parameter = cur_node->parameter ? strdup(cur_node->parameter) : NULL;
	buf[i].value = cur_node->value ? strdup(cur_node->value) : NULL;
    }
    buf[cnt].parameter = buf[cnt].value = NULL;

    return buf;
}

/**
 * @brief config_get_list() 통해 할당된 자원을 해제
 * @param ptr - config_get_list()를 통해 리턴된 데이터의 포인터
 * @return 없음
 */
void
config_free_list(config_list_t *ptr)
{
    int i;

    for (i = 0; ptr[i].parameter; i++)
    {
	free(ptr[i].parameter);
	free(ptr[i].value);
    }
    free(ptr);
}
