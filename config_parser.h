/**
 * @file config_parser.h
 * @brief 설정파일 파싱 라이브러리 헤더
 */

/*
 * 설정파일 파싱 라이브러리 헤더
 *
 * AUTHOR:
 *
 * Copyright 1999 Haninet, Inc.  All rights reserved. (by 김장동 gumdong@haninet.co.kr)
 * Copyright 2007 Samjung Data Service, Inc.  All rights reserved. (by 김기태 superkkt@sds.co.kr)
 * Copyright 2010 OneNetView, Inc.  All rights reserved. (방창현 winchild@kldp.org)
 *
 */

#ifndef	CONFIG_PARSER_H
#define	CONFIG_PARSER_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

/*
 * 설정사항 리스트 구조체
 * 로딩된 전체 설정사항을 리스트로 반환해줄때 사용
 */
typedef struct
{
    char *parameter;
    char *value;
} config_list_t;

int config_set_parameter(const char *parameter, const char *value);
char *config_get_value(const char *parameter);
int config_check_parameter(const char *parameter);
int config_read(const char *filename, int (*check)(void **), void **err);
config_list_t *config_get_list(void);
void config_free_list(config_list_t *ptr);

#endif
