static char cvsid[] = "$Id: tcp_io.c,v 1.3 2005/10/13 21:56:30 nickm Exp $";
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include "TCP_util.h"

extern int errno;

struct sockaddr_in tcp_srv_addr;  /* Server's socket address */
struct servent     tcp_serv_info; /* from getservbyname() */
struct hostent     tcp_host_info; /* from gethostbyname() */

void err_ret(char * s)
{
  perror(s);
  fflush(stdout);
  fflush(stderr);
  return;
}


int OpenTCP(char * host, char * service, int port) 
{
  int fd, resvport;
  unsigned long inaddr;
  char *host_err_str();
  struct servent *sp;
  struct hostent *hp;

  /* 
   * Initialize the server's Internet address structure.
   * We'll store the actual 4-byte Internet address and the
   * 2-byte port number below
   */
  bzero((char *) &tcp_srv_addr, sizeof(tcp_srv_addr));
  tcp_srv_addr.sin_family = AF_INET;

  if (service != NULL) {
    if ((sp = getservbyname(service,"tcp")) == NULL) {
      err_ret("tcp_open: unknown service");
      return -1;
    }
    tcp_serv_info = *sp;
    if (port > 0) tcp_srv_addr.sin_port = htons(port); /* Caller's value  */
    else          tcp_srv_addr.sin_port = sp->s_port;  /* Service's value */
  } else {
    if (port <= 0) {
      err_ret("tcp_open: must specify either service or port");
      return -1;
    }
    tcp_srv_addr.sin_port = htons(port);
  }

  /*
   * First try to convert the host hame as a dotted-decimal number.
   * If that fails we'll do gethostbyname().
   */
  if ((inaddr = inet_addr(host)) != INADDR_NONE) { /* It's dotted decimal */
    bcopy((char *) &inaddr, (char *) &tcp_srv_addr.sin_addr, sizeof(inaddr));
    tcp_host_info.h_name = NULL;
  } else {                                        /* It's a named address */
    if ((hp = gethostbyname(host)) == NULL) {
      err_ret("tcp_open: host name error");
      return -1;
    }
    tcp_host_info = *hp;
    bcopy(hp->h_addr, (char *) &tcp_srv_addr.sin_addr, hp->h_length);
  }

  if (port >= 0) {
    if ((fd = socket(AF_INET,SOCK_STREAM, 0)) < 0) {
      err_ret("tcp_open: can't create TCP socket");
      return -1;
    }
  } else {
    resvport = IPPORT_RESERVED - 1;
    if ((fd = rresvport(&resvport)) < 0) {
      err_ret("tcp_open: can't get a reserved TCP port");
      return -1;
    }
  }
   
  /*
   * Connect to the server.
   */
  if (connect(fd,(struct sockaddr *)&tcp_srv_addr,sizeof(tcp_srv_addr)) < 0) {
    err_ret("tcp_open: can't connect to server");
    close(fd);
    return -1;
  }

  return fd; /* OK */
}

void CloseTCP (int sockfd)
{
  int retn, try;
  retn = 0;
  try = 0;
  while ((retn = close (sockfd)) < 0) {
    fprintf(stderr,"Error closing socket, retrying\n");
    try++;
    usleep(100);
    if (try > 5) {
      fprintf(stderr,"Unable to close socket; giving up\n");
      break;
    }
  }
}


int SendTCP(int sockfd, unsigned char szRTUCmd[], int iRTULen)
{
  int retn;

  if ((retn = send(sockfd, szRTUCmd, iRTULen,  MSG_DONTWAIT)) < 0)
    return FALSE;
  return TRUE;

}

int RecvTCP(int sockfd, unsigned char szRecv[], int *iRTULen)
{
  fd_set fds;
  int retn,n;
  struct timeval timeout;
  unsigned char * buf;
  buf = &szRecv[0];

  FD_ZERO(&fds); FD_SET(sockfd,&fds);
  timeout.tv_sec = 3; timeout.tv_usec=0;
  n=select(sockfd+1,&fds,(fd_set*)NULL,(fd_set*)NULL,&timeout);
  *iRTULen = 0;
  if (n> 0) {
    if ((retn = recv(sockfd, buf, 256, MSG_DONTWAIT)) < 0) return FALSE;
  }  
  *iRTULen = retn;
  return TRUE;
}
