/**
 * @file sql.h
 * @brief MySQL 라이브러리 헤더
 */

/*
 * MySQL 라이브러리 헤더
 *
 * AUTHOR:
 *
 * Copyright 1999 Haninet, Inc.  All rights reserved. (by 김장동 gumdong@haninet.co.kr)
 * Copyright 2007 Samjung Data Service, Inc.  All rights reserved. (by 김기태 superkkt@sds.co.kr)
 * Copyright 2010 OneNetView, Inc.  All rights reserved. (방창현 winchild@kldp.org)
 *
 */

#ifndef SQL_H
#define SQL_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <mysql.h>

#define MAX_SQL_ERRMSG  8192

/**
 * SQL 핸들러 구조체
 */
typedef struct sql_s
{
    MYSQL *mysql;          /**< MySQL 핸들러 */
    char *host; /**< DB 호스트명 */
    char *user; /**< DB 사용자명 */
    char *passwd; /**< DB 패스워드 */
    char *db; /**< 접속할 데이터베이스명 */
    unsigned int port; /**< MySQL 포트 */
    unsigned int timeout; /**< 접속시도 제한 시간 */
    char errmsg[MAX_SQL_ERRMSG];    /**< 에러 메세지 버퍼 */
} sql_t;

typedef MYSQL_RES sql_res_t;    /**< SQL result 타입 */
typedef MYSQL_ROW sql_row_t;    /**< SQL row 타입 */

int db_query(sql_t *sql, const char *qry, sql_res_t **result);
void db_free_result(sql_res_t *res);
sql_row_t db_fetch_row(sql_res_t *res);
int db_connect(sql_t *sql, const char *host, const char *user, const char *passwd,
	const char *db, unsigned int port, unsigned int timeout);
void db_close(sql_t *sql);
unsigned int db_num_rows(sql_res_t *res);
char *db_errmsg(sql_t *sql);
unsigned long long db_insert_id(sql_t *sql);
int db_escape_string(sql_t *sql, char **to, const char *from, unsigned long length);
int db_set_character_set(sql_t *sql, const char *charset);
unsigned long long db_affected_rows(sql_t *sql);
unsigned int db_num_fields(sql_res_t *res);
int get_db_num (sql_t *sql, char *query);
void set_autocommit (sql_t *sql, int flag);
void rollback (sql_t *sql);
void commit (sql_t *sql);

#endif /* End of #ifdef SQL_H */
