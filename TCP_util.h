#ifndef _TCP_util_h
#define _TCP_util_h

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

int  OpenTCP(char * host, char * service, int port);
void CloseTCP(int sockfd);
int  SendTCP(int sockfd, unsigned char szRTUCmd[], int iRTULen);
int  RecvTCP(int sockfd, unsigned char szRecv[], int *iRTULen);

#endif
