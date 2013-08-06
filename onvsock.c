#include <stdio.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netdb.h>
#include <ctype.h>
#include "log.h"
#include "onvsock.h"
static int wait_packet(int fd,int nsec);
static int connect_nonb(int sockfd, const struct sockaddr *saptr, int salen, int nsec);
/**
 * @brief TCP 연결 함수 
 * @param ip ip주소 
 * @param port  포트 번호 
 * @return 성공: 소켓 번호 , 실패: -1
 */
int onvTCPconnect(char* ip , int port){
    return 0 ;
}
/**
 * @brief timeout 을 설정한 tcp connect 
 * @param hostname     호스트 이름 
 * @param service      포트 번호
 * @param nsec         타임아웃 시간 
 * @return 성공 시 소켓 구분자 
 * @return 실패 시 -1
 */
int onvTCPconnectNonBlock(const char *hostname, const char *service,int nsec)
{
    struct addrinfo hints, *res, *ressave;
    int  sock,n;

    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if( (n=getaddrinfo(hostname,service,&hints,&res)) != 0){
        Log(ERROR,"getaddrinfo function error");
        return -1;
    }
    ressave = res;
    do {
        struct  sockaddr_in *ts;
        sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if(sock < 0)
            continue;

        ts = (struct sockaddr_in *) res->ai_addr;

        if(connect_nonb(sock, (struct sockaddr *)res->ai_addr, res->ai_addrlen,nsec) == 0) 
            break;

        close(sock);
    }while( (res=res->ai_next) !=NULL);
    if( res == NULL){
        Log(ERROR,"connet_nonb function error");
        return -1;
    }
    freeaddrinfo(ressave);
    return sock;
}

/**
 * @brief timeout 을 설정한 tcp connect 
 * @param sockfd       소켓 구분자 
 * @param sapter       소켓 주소 구조체 
 * @param salen        소켓 주소 구조체  길이 
 * @param nsec         타임아웃 시간 
 * @return 성공 시 0 
 * @return 실패 시 -1
 */
static int connect_nonb(int sockfd, const struct sockaddr *saptr, int salen, int nsec)
{
    int             flags, n, error;
    socklen_t       len;
    fd_set          rset, wset;
    struct timeval  tval;

    flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    error = 0;
    if ( (n = connect(sockfd, (struct sockaddr *) saptr, salen)) < 0){
        if (errno != EINPROGRESS){
            Log(ERROR,"connet function error");
            return(-1);
        }
    }

    /* Do whatever we want while the connect is taking place. */

    if (n != 0){

        FD_ZERO(&rset);
        FD_SET(sockfd, &rset);
        wset = rset;
        tval.tv_sec = nsec;
        tval.tv_usec = 0;

        if ( (n = select(sockfd+1, &rset, &wset, NULL,
                         nsec ? &tval : NULL)) == 0) {
            close(sockfd);      /* timeout */
            errno = ETIMEDOUT;

            Log(ERROR,"connet timeout error");
            return(-1);
        }

        if (FD_ISSET(sockfd, &rset) || FD_ISSET(sockfd, &wset)) {
            len = sizeof(error);
            if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0){
                Log(ERROR,"getsockopt function error");
                return(-1);         /* Solaris pending error */
            }
        } else{
            Log(ERROR,"select error: sockfd not set\n");
        }
    }
    fcntl(sockfd, F_SETFL, flags);  /* restore file status flags */
    if (error) {
        close(sockfd);      /* just in case */
        errno = error;
        Log(ERROR,"fcntl function error");
        return(-1);
    }
    return(0);
}
/**
* wait_packet - 패킷 수신대기
*
* @param fd: 소켓 디스크립터
* @param nsec: 패킷 대기시간
* @return: 성공 시 0, 실패(Timeout or Error) 시 -1
*/
static int wait_packet(int fd,int nsec) {
    int result;
    fd_set rset;
    struct timeval tm;
    // 입력 시간이 잘못되었다면 실패
    if(nsec <0){
        Log(DEBUG,"timeout value error = [%d]",nsec);
        return -1;
    }
    FD_ZERO(&rset);
    FD_SET(fd, &rset);
    tm.tv_sec = nsec;
    tm.tv_usec = 0;
    result = select(fd + 1, &rset, NULL, NULL, &tm);
    if (result <= 0) {
        if (result == 0 || errno == EINTR){
            /* Timeout or Interrupt */
            return -1;
        } else {
            /* Error */
            Log(ERROR,"select() function failed: %s", strerror(errno));
            return -1;
        }
    }
    return 0;
}
/**
 * @brief 소켓에서 data를 읽어 반환하는 함수
 * @param sock 소켓
 * @param data 읽어드린 데이터를 저장하는 포인터
 * @param datalen 읽어야 할 데이터 길이 
 * @return 성공 시 read data length, 실패 시 -1 
 */
int onvRead(int sock,char * data,int datalen){
    size_t nleft;
    ssize_t nread;
    char *ptr = NULL;

    ptr = data;
    nleft = datalen;
    while(nleft > 0) {
        if((nread = read(sock, ptr, nleft)) < 0) {
            if(errno == EINTR) {
                nread = 0;
            }
            else {
                return -1;
            }
        }else if(nread== 0){
            break;
        }
        nleft -= nread;
        ptr += nread;
    }
    return (ssize_t) (datalen - nleft);
}

/**
 * @brief 소켓에서 data를 읽어 반환하는 함수 datalen 만큼
 * 소켓에서 읽지 못하면 실패한다.
 * @param sock 소켓
 * @param data 읽어드린 데이터를 저장하는 포인터
 * @param datalen 읽어야 할 데이터 길이 
 * @param timewait
 * @return 성공 시 read data length, 실패 시 -1 
 */
int onvReadNonBlock(int sock,char * data,int datalen,int timewait){
    size_t nleft;
    ssize_t nread;
    char *ptr = NULL;

    ptr = data;
    nleft = datalen;
    while(nleft > 0) {
        wait_packet(sock,timewait);
        if((nread = read(sock, ptr, nleft)) < 0) {
            if(errno == EINTR) {
                nread = 0;
            }
            else {
                return -1;
            }
        }
        nleft -= nread;
        ptr += nread;
    }
    return (ssize_t) (datalen - nleft);
}
int onvWrite(int sock, char * data,int datalen){
    int nwrite, twrite = 0, len = (int) datalen;
    const char *ptr = data;
    while(len > 0) {
        if((nwrite = write(sock, ptr, len)) < 0) {
            if(errno == EINTR) {
                continue;
            } else {
                return -1;
            }
        }
        len -= nwrite;
        ptr += nwrite;
        twrite += nwrite;
    }
    return (ssize_t) ((twrite == datalen) ? twrite : -1);
}
