#ifndef ONV_SOCK_H
#define ONV_SOCK_H
int onvTCPconnect(char* ip , int port);
int onvTCPconnectNonBlock(const char *hostname, const char *service,int nsec);
int onvRead(int sock,char * data,int datalen);
int onvReadNonBlock(int sock,char * data,int datalen,int timewait);
int onvWrite(int sock, char * data,int datalen);
#endif

