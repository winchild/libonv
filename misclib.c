/**
 * @file misclib.c
 * @brief Support Functions Library
 */

/*
 * Support Functions Library
 *
 * AUTHOR:
 *
 * Copyright 1999 Haninet, Inc.  All rights reserved. (by 김장동 gumdong@haninet.co.kr)
 * Copyright 2007 Samjung Data Service, Inc.  All rights reserved. (by 김기태 superkkt@sds.co.kr)
 * Copyright 2010 OneNetView, Inc.  All rights reserved. (방창현 winchild@kldp.org)
 *
 * REFERENCE:
 *   splint -noeffect +matchanyintegral -mustfreefresh -exportlocal -paramuse -usedef -compdef -retvalint -retvalother -nullpass -nestcomment -unrecog -preproc -warnposix misclib.c
 */

#include "misclib.h"
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>
#include <netdb.h>
#include <ctype.h>
#include <time.h>
static int wait_packet(int fd,int msec);

/**
 * @brief accept() wrapping 함수
 * @param fd - Socket descriptor
 * @param sa - 소켓 구조체
 * @param salenptr - \a sa 의 length
 * @return
 *  성공 시 socket descriptor\n
 *  실패 시 -1
 */
int
Accept(int fd, struct sockaddr *sa, socklen_t *salenptr)
{
    int n;

again:
    if((n = accept(fd, sa, salenptr)) < 0) {
#ifdef  EPROTO
	if(errno == EPROTO || 
		errno == ECONNABORTED || 
		errno == EINTR) {
#else
	    if(errno == ECONNABORTED || 
		    errno == EINTR) {
#endif
		goto again;
	    }
	    else {
		return -1;
	    }
	}

    return n;
}


/**
 * @brief 소켓 연결 
 * @param host - 호스트네임 또는 IP
 * @param port - 접속 포트
 * @param timeout - 최대 접속대기시간
 * @return
 *  성공 시 socket descriptor,\n
 *  실패 시 -1
 */
int tcp_Connect(const char *ip, const int port, int timeout)
{
    int fd;
    struct sockaddr_in servaddr;

/*
    struct hostent *hptr = NULL;

    // gethostbyname set h_errno if it failed 
    hptr = gethostbyname(host);
    if (!hptr) return -1;
*/
//    fd = socket(AF_INET, SOCK_STREAM, 0);
    fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    memset(&servaddr, 0, sizeof(servaddr));
//    servaddr.sin_family = AF_INET;
    servaddr.sin_family = PF_INET;
    servaddr.sin_port = htons(port);
    servaddr.sin_addr.s_addr = inet_addr(ip);
//    memcpy(&servaddr.sin_addr, *hptr->h_addr_list, sizeof(servaddr.sin_addr));

    if (connect(fd, (struct sockaddr *) &servaddr, (socklen_t) sizeof(servaddr)) < 0) return -1;

    return fd;
}
/* ******** timeout 을 설정한 예제 ********
int 
Connect(const char *host, const char *port, int timeout)
{
    int	sockfd, n;
    int result;
    struct addrinfo hints, *res, *ressave;

    ASSERT(host != NULL);

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if((n = getaddrinfo(host, port, &hints, &res)) != 0)
	return -1;
    ressave = res;

    do {
	sockfd = socket(res->ai_family, 
		res->ai_socktype, 
		res->ai_protocol);
	// ignore error
	if(sockfd < 0) {
	    continue;	
	}

	alarm(timeout);
	result = connect(sockfd, res->ai_addr, res->ai_addrlen);
	if(result == 0) {
	    alarm(0);
	    break;		
	}
	alarm(0);
	if(result < 0 && errno == EINTR) {
	    close(sockfd);
	    freeaddrinfo(ressave);
	    return -1;
	}

	close(sockfd);		
    } while((res = res->ai_next) != NULL);

    if(res == NULL) {
	freeaddrinfo(ressave);
	return -1;
    }

    freeaddrinfo(ressave);

    return sockfd;
}
*/



/**
 * @brief 데몬화 
 * @param 없음
 * @return
 *  성공 시 0,\n
 *  실패 시 -1
 */
int
daemonize(void)
{
    int fd;
    pid_t pid;

    /* 1차 fork */
    if((pid = fork()) < 0)
	return -1;
    else if(pid != 0) {
	exit(EXIT_SUCCESS);
    }

    /* 2차 fork */
    if((pid = fork()) < 0) {
	return -1;
    }
    else if(pid != 0) {
	exit(EXIT_SUCCESS);
    }

    usleep(1000);
    setsid();
    chdir("/");
    umask(0);
    close(0);
    close(1);
    close(2);
    fd = open("/dev/null", O_RDONLY);
    if (fd < 0)
    {
	return -1;
    }
    dup2(fd, 1);
    dup2(fd, 2);

    return 0;
}


/**
 * @brief 특정 포트로 소켓생성 후 리슨하는 함수 
 * @param port - 리슨할 포트 번호
 * @return
 *  성공 시 socket descriptor,\n
 *  실패 시 -1
 */
int
socket_listen(int port)
{
    int fd;
    struct sockaddr_in servaddr;
    const int on = 1;

    if((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	return -1;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons((uint16_t) port);

    if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, (socklen_t) sizeof(on)) < 0) {
	return -1;
    }
    if(bind(fd, (struct sockaddr *) &servaddr, (socklen_t) sizeof(servaddr)) < 0) {
	return -1;
    }
    if(listen(fd, 1024) < 0) {
	return -1;
    }

    return fd;
}


/**
 * @brief Socket에서 \a n 만큼 read
 * @param fd - Socket descriptor
 * @param vptr - 수신한 데이터를 저장할 버퍼
 * @param n - 수신할 데이터의 사이즈
 * @return
 *  성공 시 수신한 데이터 사이즈,\n
 *  실패 시 -1
 */
ssize_t
readn(int fd, void *vptr, size_t n)
{
    size_t nleft;
    ssize_t nread;
    char *ptr = NULL;

    ptr = vptr;
    nleft = n;
    while(nleft > 0) {
	if((nread = read(fd, ptr, nleft)) < 0) {
	    if(errno == EINTR) {
		nread = 0;
	    }
	    else {
		return -1;
	    }
	}
	else if(nread == 0) {
	    break;
	}

	nleft -= nread;
	ptr += nread;
    }

    return (ssize_t) (n - nleft);
}

/**
 * @brief Socket에서 \a 최대 n 만큼 read
 * @param fd - Socket descriptor
 * @param vptr - 수신한 데이터를 저장할 버퍼
 * @param n - 수신할 데이터의 최대 사이즈
 * @param msec 패킷간 최대 시간 
 * @return
 *  성공 시 수신한 데이터 사이즈,\n
 *  실패 시 -1
 */
ssize_t readn_timewait(int fd, void *vptr, size_t n,int msec)
{
    size_t nleft;
    ssize_t nread;
    char *ptr = NULL;
    int retcode = -1;

    ptr = vptr;
    nleft = n;
    while(nleft > 0) {
        retcode  =  wait_packet(fd,msec);
        // select time out or error
        if(retcode < 0 ){ 
            return (ssize_t) (n - nleft);
        }
	if((nread = read(fd, ptr, nleft)) < 0) {
	    if(errno == EINTR) {
		nread = 0;
	    }
	    else {
		return -1;
	    }
	} else if(nread == 0) {
	    break;
	}

	nleft -= nread;
	ptr += nread;
    }

    return (ssize_t) (n - nleft);
}


/**
 * @brief Socket에 \a len 만큼 write
 * @param connfd - Socket descriptor
 * @param buf - write할 데이터
 * @param len - \a buf 의 길이
 * @return
 *  성공 시 write한 바이트수,\n
 *  실패 시 -1
 */
ssize_t
writen(int connfd, const char *buf, size_t len)
{
    int nwrite, twrite = 0, olen = (int) len;
    const char *ptr = buf;

    ASSERT(buf != NULL);

    while(len > 0) {
again:
	if((nwrite = write(connfd, ptr, len)) < 0) {
	    if(errno == EINTR) {
		goto again;
	    }
	    else {
		return -1;
	    }
	}

	len -= nwrite;
	ptr += nwrite;
	twrite += nwrite;
    }

    return (ssize_t) ((twrite == olen) ? twrite : -1);
}

/**
 * @brief 스트림 의 딜리미터 카운트
 * @param line_buff - 오픈할 로그파일명
 * @return
 *  성공시 n - 배열의 갯수
 *  실패시 0, -1
 */
/**** 무한 LOOP 체크요 ***
int l2c (char *line_buff, const char del)
{
    int cnt;
	char *ptr;

	cnt = 0;
	ptr = line_buff;
	while (*ptr != '\0')
	{
		if (*ptr == del)
		{
			cnt++;
			if (cnt >= L2A_MAX_ROW) break;
			ptr++;	// delimitor skip
		}
	}

	if (cnt >= L2A_MAX_ROW) return -1;	// 필드 갯수가 너무 큼.

	if (*(ptr-1) != del) cnt++;

    return cnt;
}
****/

/**
 * @brief 스트림 to 배열
 * @param line_buff - 오픈할 로그파일명
 * @param arr_ptr - 수평배열
 * @return
 *  성공시 n - 배열의 갯수
 *  실패시 0, -1
 */
int l2a (char *line_buff, char *arr_ptr[], const char del)
{
    int cnt, n;
	char *ptr, buffer[256];

	cnt = 0;
	ptr = line_buff;
	n = 0;
	while (*ptr != '\0' && n < (int) sizeof(buffer)-1)
	{
		if (*ptr == del)
		{
			buffer[n] = '\0';
			arr_ptr[cnt] = strdup (buffer);
//printf ("arr[%d]=[%s], buffer=[%s]\n",cnt,arr_ptr[cnt],ptr);getchar();
			cnt++;
			if (cnt >= L2A_MAX_ROW) break;
			ptr++;	// delimitor skip
			n = 0;
		}
		else buffer[n++] = *ptr++;	// 데이터 복사
	}

	if (cnt >= L2A_MAX_ROW)
	{
		return -1;	// 필드 갯수가 너무 큼.
	}
	if (n >= (int) sizeof(buffer))
	{
		return -1;	// 필드 데이터 길이가 너무 큼.
	}

	if (*(ptr-1) != del)
	{
		buffer[n] = '\0';
		arr_ptr[cnt] = strdup (buffer);
//printf ("arr[%d]=[%s], buffer=[%s]\n",cnt,arr_ptr[cnt],ptr);getchar();
		cnt++;
	}
	arr_ptr[cnt] = NULL;

    return cnt;
}

/**
 * @brief 배열버퍼 free
 * @param arr_ptr - 수평배열
 * @return
 *  없음
 */
void free_l2a (char *arr_ptr[])
{
	int i = 0;

	while (arr_ptr[i] != NULL) free (arr_ptr[i++]);
}

/**
 * @brief 숫자이외의 값 필터링.
 * @param arr_ptr - 수평배열
 * @return
 *  없음
 */
char *only_digit (char *s)
{
	char *d, *p;

	d = malloc (strlen(s)+1);
	if (d == NULL) return NULL;
	p = d;
	while (*s != '\0')
	{
		if (isdigit(*s)) *p++ = *s;
		s++;
	}
	*p = '\0';

	return d;
}

/*
// 테스트용 코드...
int main (int argc, char *argv[])
{
	char *p[L2A_MAX_ROW];
	char *q;
	int i;

	l2a (argv[1], p, '|');

	i = 0;
	while (p[i] != NULL)
	{
		printf ("%02d=[%s]\n", i, p[i]);
		i++;
	}
	free_l2a (p);

	q = only_digit ("111-222-3333");
	printf ("q=%s\n", q);
//	free (q);
	

	exit (0);
}
*/

//extern int Forking;

/*	Execution child process.
*/
int Exec(char *argv)
{
	int Cpid;	/* child process id */

	int i=0, rc;
	char *arg_ptr[64];
	char run[128];

	while (*argv) {
		while (*argv == ' ' ||
			*argv == '\t') argv++; /* white space skip */
		if (*argv == '\'') {		 /* single quotation */
			arg_ptr[i] = ++argv;
			argv = strpbrk (argv, "'");
			 /* scan pare single quotation */
		}
		else	{
			arg_ptr[i] = argv;
			argv = strpbrk (argv, " ");
		}
		if (argv == (char *) NULL) { /* terminate */
			i++;
			break;
		}
		*argv++ = '\0';
		i++;
	} 
	arg_ptr[i] = NULL;	/* terminator */
	strcpy (run, arg_ptr[0]);	// 실행프로그램명

//	Forking = fork_type;	/* setting fork type */
	if ((Cpid = fork()) == 0) {
		execvp(run, arg_ptr);
		Log (ERROR, "EXECUTE_FAIL=[%s]", run);
		_exit(-1);
	}
	while ((i = wait(&rc)) != Cpid && i != -1) {
		/* wait for child process running */
	}
//	Forking = 0;	/* reset fork type */
	i = i >> 8;
	return i;	/* return code */
}

/**
 * wait_packet - 연결된 DCE connection에서 패킷 수신 대기
 *
 * @param fd: 소켓 디스크립터
 * @param msec: 소켓 디스크립터
 * @return: 성공 시 0, 실패(Timeout or Error) 시 -1
 */
static int wait_packet(int fd,int msec) {
    int result;
    fd_set rset;
    struct timeval tm;

    FD_ZERO(&rset);
    FD_SET(fd, &rset);

    /*
     * 소켓이 PING_PERIOD 시간동안 idle 상태이면 select 중지
     */
    tm.tv_sec = msec / 1000;
    tm.tv_usec = (msec % 1000) *1000;
    result = select(fd + 1, &rset, NULL, NULL, &tm);
    if (result <= 0) {
        if (result == 0 || errno == EINTR){ /* Timeout or Interrupt */
            return -1;
        }else {
            /* Error */
            Log(ERROR,"select() function failed: %s", strerror(errno));
            return -1;
        }
    }
    return 0;
}

/**
 * @brief UDP 패킷 전송 함수
 * @param ip ip주소
 * @param ip port
 * @param data 전송 데이터
 * @param datalen 전송 데이터 길이
 */
int udp_sendPacket(char * ip , int port , char *data, int datalen){
    int server_socket;
    struct sockaddr_in server_addr;

    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);

    server_socket = socket(PF_INET, SOCK_DGRAM,0);
    if(server_socket ==  -1){
        Log(ERROR,"server socket create error");
        return  -1;
    }
    // connect
    if(connect(server_socket,(struct sockaddr * ) &server_addr , sizeof(server_addr)) == -1){
        Log(ERROR,"server connect error");
        close(server_socket);
        return  -1;

    }
    // 패킷 전송
    if(write(server_socket,data,datalen) !=  datalen ) {
        Log(ERROR,"packet send error");
        close(server_socket);
        return -1;
    }
    // socket close
    close(server_socket);

    return 0;
}

/**
 * @brief binary 데이터 파일출력 함수
 * @param filename 출력 파일 이름
 * @param data 출력 데이터
 * @param datalen 출력 데이터 길이
 */
void dumpdata(char *filename, char* data,int datalen){
    FILE * fp=NULL;
    fp = fopen(filename, "wb");
    fwrite(data, datalen , 1, fp);
    fclose(fp);
}
/**
 * @brief binary 데이터 16진수 문자열 Log 출력 함수
 * @param buf 데이터 포인터 
 * @param buflen 데이터 길이 
 */
void printbyte(char * buf , int buflen){
    int i ;
    char line[256];
    char tempArray[10];
    line[0] = 0;

    for(i=0 ; i < buflen; i++){
        sprintf(tempArray, " %02X",buf[i] & 0xFF);
        strcat(line,tempArray);
        if(i % 16 == 15){
            Log(DEBUG,"%s",line);
            line[0] = 0;
        }
    }
    if(strlen(line) > 0  ){
        Log(DEBUG,"%s",line);
    }
}

/**
 * @brief 시간 문자열을 time_t 로 변환 함수 
 * @param szYYYYMMDDHHMMSS 시간 문자열 '\0'으로 문자열 끝을 나타냄
 * @return 계산된 time_t 값
 */
time_t ConvertToSecSince1970(char *szYYYYMMDDHHMMSS)
{
    struct tm    Tm;    
    char buf[256];
    buf[0]=0;

    memset(&Tm, 0, sizeof(Tm));
    Tm.tm_year = makeInt(szYYYYMMDDHHMMSS +  0, 4) - 1900;
    Tm.tm_mon  = makeInt(szYYYYMMDDHHMMSS +  4, 2) - 1;
    Tm.tm_mday = makeInt(szYYYYMMDDHHMMSS +  6, 2);
    Tm.tm_hour = makeInt(szYYYYMMDDHHMMSS +  8, 2);
    Tm.tm_min  = makeInt(szYYYYMMDDHHMMSS + 10, 2);
    Tm.tm_sec  = makeInt(szYYYYMMDDHHMMSS + 12, 2);

    /*
     * DEBUG용  출력 
    Log(DEBUG,"data time: %d%02d%02d%02d%02d%02d",
    Tm.tm_year +1900,
    Tm.tm_mon +1 ,
    Tm.tm_mday,
    Tm.tm_hour,
    Tm.tm_min ,
    Tm.tm_sec );

    strftime(buf,sizeof(buf),"%Y%m%d%H%M%S", &Tm);
    Log(DEBUG,"ConvertToSecSince1970 function [%s]",buf);
    */

    return mktime(&Tm);
}

/**
 * @brief 입력받은 문자열을 int로 변환하는 함수 
 * @param p 입력문자열 자연수만 입력받을 수 있다.
 * @param size 입력문자열 길이 
 * @return 변환 값
 */
int makeInt(const char *p, int size)
{
    const char *endp;
    int intval = 0;

    endp = p + size;
    while (p < endp)
    {
        intval = intval * 10 + *p - '0';
        p++;
    }
    return intval;
}
/**
 * @brief float 형 자료형을 byte array 로 변환하는 함수 
 * @param dest 출력 byte arry
 * @param from 입력 float 
 */
void copyFloatToByte(char * dest, float from){
    uint32_t * pint = (uint32_t*)dest;
    *pint =  htonl(*((uint32_t*)&(from )) );
}


void cut_CRLF (char *buf)
{
	char *p;

	p = strrchr (buf, '\n');
	if (p != NULL) *p = '\0';

	p = strrchr (buf, '\r');
	if (p != NULL) *p = '\0';
}
