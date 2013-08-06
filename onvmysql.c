/**
 * @file sql.c
 * @brief MySQL 라이브러리
 */

/*
 * MySQL 라이브러리
 *
 * REFERENCE:
 *
 * [winchild@STOC lib]$ splint -exportlocal -mustfreeonly -nullpass -branchstate -nullstate -retvalint -mustfreefresh -predboolint -compdef -unrecog -usereleased -warnposix -preproc -nestcomment -unqualifiedtrans -I/usr/include/mysql sql.c
 * Splint 3.1.1 --- 06 Jan 2007
 * 
 * sql.c: (in function db_connect)
 * sql.c:209:2: Buffer overflow possible with sprintf.  Recommend using snprintf
 *                 instead: sprintf
 *   Use of function that may lead to buffer overflow. (Use -bufferoverflowhigh to
 *   inhibit warning)
 * 
 * Finished checking --- 1 code warning
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <mysql.h>
#include <errmsg.h>
#include "onvmysql.h"
#include "misclib.h"
#include "log.h"

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * @brief 라이브러리 에러메세지 반환 함수 
 * @param sql - SQL 구조체
 * @return 에러메세지 포인터
 */
char *
db_errmsg(sql_t *sql)
{
    return sql->errmsg;
}


/**
 * @brief 마지막으로 insert한 row의 auto increment ID를 반환
 * @param sql - SQL 구조체
 * @return 마지막으로 insert한 row의 auto increment ID
 */
unsigned long long
db_insert_id(sql_t *sql)
{
    return (unsigned long long) mysql_insert_id(sql->mysql);
}

/**
 * SQL 서버와 재접속한다.
 *
 * @param sql - SQL 핸들러 포인터
 * @return 0; 에러 발생 시 -1
 */
static int
reconnect_db(sql_t *sql)
{
    char *host = NULL;
    char *user = NULL;
    char *passwd = NULL;
    char *db = NULL;
    unsigned int port;
    unsigned int timeout;
    int result;

    if (!sql)
    {
	errno = EINVAL;
	return -1;
    }

    host = sql->host ? strdup(sql->host) : NULL;
    user = sql->user ? strdup(sql->user) : NULL;
    passwd = sql->passwd ? strdup(sql->passwd) : NULL;
    db = sql->db ? strdup(sql->db) : NULL;
    port = sql->port;
    timeout = sql->timeout;

    db_close(sql);
    result = db_connect(sql, host, user, passwd, db, port, timeout);

    free(host);
    free(user);
    free(passwd);
    free(db);

    return result;
}

/**
 * @brief DB query 함수
 * @param sql - SQL 구조체
 * @param qry - 수행할 쿼리문
 * @param result - (OUT) SELECT 문인 경우 저장된 결과
 *  (호출자 사용 후 db_free_result())
 * @return
 *  성공 시 0,\n
 *  실패 시 -1
 */
int
db_query(sql_t *sql, const char *qry, sql_res_t **result)
{
    sql_res_t *res = NULL;

    if (mysql_ping(sql->mysql))
    {
	if (reconnect_db(sql))
	    return -1;
    }

    if (mysql_query(sql->mysql, qry) != 0)
    {
        snprintf(sql->errmsg, sizeof(sql->errmsg), "%s",
                mysql_error(sql->mysql));
        if (result)
            *result = NULL;
        return -1;
    }

    res = mysql_store_result(sql->mysql);
    if (!res)
    {
        if (mysql_field_count(sql->mysql) == 0)
        {
            if (result)
                *result = NULL;
            return 0;
        }
        else
        {
            snprintf(sql->errmsg, sizeof(sql->errmsg), "%s",
                    mysql_error(sql->mysql));
            if (result)
                *result = NULL;
            return -1;
        }
    }

    if (result)
        *result = res;
    else
        mysql_free_result(res);

    return 0;
}

/**
 * @brief 저장된 쿼리결과 변수를 free하는 함수
 * @param res - 쿼리결과 변수
 * @return 없음
 */
void
db_free_result(sql_res_t *res)
{
    mysql_free_result(res);
    res = NULL;
}


/**
 * @brief 쿼리 결과를 row 단위로 반환하는 함수
 * @param res - 쿼리결과 변수
 * @return 현재 row
 */
sql_row_t
db_fetch_row(sql_res_t *res)
{
    return mysql_fetch_row(res);
}

/**
 * @brief DB 접속 함수
 * @param sql - SQL 구조체
 * @param host - 접속할 DB HOST
 * @param user - DB USER
 * @param passwd - DB PASSWD
 * @param db - 접속할 DB NAME
 * @param port - 접속할 DB HOST's PORT
 * @param timeout - connection timeout
 * @return
 *  성공 시 0,\n
 *  실패 시 -1
 */
int
db_connect(sql_t *sql, const char *host, const char *user, const char *passwd, 
	       const char *db, unsigned int port, unsigned int timeout)
{
    int result;
    int ret_val = -1;
    MYSQL *status = NULL;
#ifdef MYSQL_OPT_RECONNECT
    my_bool my_true = 1;
#endif

    if ((result = pthread_mutex_lock(&mutex)))
	Log(FATAL, "Failed to lock mutex: %s", strerror(result));

    if ((sql->mysql = mysql_init(NULL)) == NULL)
    {
	sprintf(sql->errmsg, "MySQL 초기화 실패");
	goto cleanup;
    }

#ifdef MYSQL_OPT_RECONNECT
    /* MySQL 5.0.3 부터는 mysql_ping에서 연결이 끊어진걸로 나오더라도 재접속을
     * 자동으로 하지 않는다. 그래서 5.0.3 이상에서 컴파일 될 경우에는 재접속을
     * 자동으로 하도록 옵션을 지정해준다.
     */
    if (mysql_get_client_version() >= 50003UL)
        mysql_options(sql->mysql, MYSQL_OPT_RECONNECT, &my_true);
#endif

    if (timeout > 0)
	mysql_options(sql->mysql, MYSQL_OPT_CONNECT_TIMEOUT, (const char *) &timeout);

    status = mysql_real_connect(sql->mysql, host, user, passwd, db, port, NULL, 0);
    if (!status)
    {
	snprintf(sql->errmsg, sizeof(sql->errmsg), "%s", mysql_error(sql->mysql));
	goto cleanup;
    }

    sql->host = host ? strdup(host) : NULL;
    sql->user = user ? strdup(user) : NULL;
    sql->passwd = passwd ? strdup(passwd) : NULL;
    sql->db = db ? strdup(db) : NULL;
    sql->port = port;
    sql->timeout = timeout;

    if (db_set_character_set(sql, "euckr"))
    {
		db_close(sql);
		Log(FATAL, "Can not setting 'euckr' character set. : %s", (result));
		snprintf(sql->errmsg, sizeof(sql->errmsg), "%s", mysql_error(sql->mysql));
		goto cleanup;
    }

    /* success */
    ret_val = 0;

cleanup:
    if ((result = pthread_mutex_unlock(&mutex)))
	Log(FATAL, "Failed to unlock mutex: %s", strerror(result));

    return ret_val;
}

/**
 * @brief 현재 DB 연결의 캐릭터셋 설정
 * @param sql - SQL 핸들러
 * @param charset - 설정할 캐릭터셋
 * @return
 *  성공 시 0,\n
 *  실패 시 non-zero
 */
int
db_set_character_set(sql_t *sql, const char *charset)
{
    return mysql_set_character_set(sql->mysql, charset);
}


/**
 * @brief DB close 함수
 * @param sql - SQL 구조체
 * @return 없음
 */
void
db_close(sql_t *sql)
{
    mysql_close(sql->mysql);
    free(sql->host);
    free(sql->user);
    free(sql->passwd);
    free(sql->db);
}


/**
 * @brief 쿼리 결과의 갯수를 리턴
 * @param res - 쿼리 결과 포인터
 * @return 쿼리 결과의 갯수
 */
unsigned int
db_num_rows(sql_res_t *res)
{
    return (unsigned int) mysql_num_rows(res);
}

/**
 * @brief 쿼리 결과의 갯수를 리턴
 * @param res - 쿼리 결과 포인터
 * @return 쿼리 결과의 갯수
 */
unsigned int
db_num_fields(sql_res_t *res)
{
    return (unsigned int) mysql_num_fields(res);
}

/**
 * @brief SQL 쿼리 문자열 Escape String
 * @param sql - SQL 구조체
 * @param to - (OUT) Escape String된 문자열을 저장할 포인터 (호출자 사용 후 free)
 * @param from - Escape시킬 문자열
 * @param length - \a from 의 길이
 * @return
 *  성공 시 0,\n
 *  실패 시 -1
 */
int
db_escape_string(sql_t *sql, char **to, const char *from, unsigned long length)
{
    *to = malloc((size_t)(length * 2)+ 1);
    if (!*to)
	return -1;

    (void) mysql_real_escape_string(sql->mysql, *to, from, length);
    return 0;
}

/**
 * @brief 최근 쿼리에 영향을 받은 row의 개수를 리턴
 * @param sql - SQL 핸들러
 * @return 영향받은 row의 개수
 */
unsigned long long
db_affected_rows(sql_t *sql)
{
    return mysql_affected_rows(sql->mysql);
}


/**
 * @brief 충전기 갯수 리턴
 * @param sql - SQL 핸들러
 * @param err_msg - 오류메시지 버퍼
 * @return
 *  성공 시 새로운 요청의 갯수 (>= 0)\n
 *  실패 시 -1
 *
 */
int get_db_num (sql_t *sql, char *query)
{
    sql_row_t row;
    sql_res_t *res = NULL;
	int n;

	Log (DEBUG, "SQL=[%s]", query);
    if (db_query (sql, query, &res))
    {
		Log (ERROR, "%s: SQL=[%s]", db_errmsg(sql), query);
		return -1;
    }
    row = db_fetch_row(res);
    if (row == NULL)
	{
		Log (ERROR, "Row empty : SQL=[%s]", query);
		db_free_result(res);
		return 0;
	}
	n = atoi (row[0]);
	db_free_result(res);

	return n;
}

/**
 * @brief autocommit ON / OFF
 * @param sql - SQL 핸들러
 * @param flag - ON = 1 / OFF = 0
 * @return
 *  없음.
 *
 */
void set_autocommit (sql_t *sql, int flag)
{
	char query[64];

	snprintf (query, sizeof(query), "set autocommit=%d", (flag?1:0));
	Log (DEBUG, "SQL=[%s]", query);
    if (db_query (sql, query, NULL))
		Log (ERROR, "%s: SQL=[%s]", db_errmsg(sql), query);
	return;
}

/**
 * @brief rollback
 * @param sql - SQL 핸들러
 * @return
 *  없음.
 *
 */
void rollback (sql_t *sql)
{
	char query[32];

	snprintf (query, sizeof(query), "rollback");
	Log (WARN, "SQL=[%s]", query);
    if (db_query (sql, query, NULL))
		Log (ERROR, "%s: SQL=[%s]", db_errmsg(sql), query);
	return;
}

/**
 * @brief commit
 * @param sql - SQL 핸들러
 * @return
 *  없음.
 *
 */
void commit (sql_t *sql)
{
	char query[32];

	snprintf (query, sizeof(query), "commit");
	Log (DEBUG, "SQL=[%s]", query);
    if (db_query (sql, query, NULL))
		Log (ERROR, "%s: SQL=[%s]", db_errmsg(sql), query);
	return;
}

