#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <stdint.h>
#include "TCP_util.h"
#include "ModbusTCP.h"

#define DEFAULT_HOST "129.6.120.55"
#define DEFAULT_PORT 502

char host[256];
int  port;

unsigned char sndbuf[1024];
unsigned char rcvbuf[1024];

union CVT {
  float flt;
  unsigned int uint;
} cvt;

int main(int argc, char *argv[])
{
  char ch;
  unsigned char inbyte, byte1,byte2;
  unsigned short lower, upper,*pshort,us1,us2;
  unsigned int uint;
  float fval,fdata;
  int sockfd=-1;
  int coilno=0;
  int fltrno=0; // holding register with floating point number in it
  int readcoil=FALSE;
  int wrtcoil =FALSE;
  int readflt =FALSE;
  int wrtflt=FALSE;
  int boolval=FALSE;
  int bytesread;
  int i;
  int retn;

  memset(sndbuf,0,sizeof(sndbuf));
  memset(rcvbuf,0,sizeof(rcvbuf));
  fdata = 0;
  strcpy(host,DEFAULT_HOST);
  port = DEFAULT_PORT;
  while ((ch = getopt(argc, argv, "h:p:s:c:f:w:d:b:?")) != -1) {
    switch (ch) {
    case 'p':
      port = atoi(optarg);
      break;
    case 'h':
      strcpy(host,optarg);
      break;
    case 's':
      strcpy(sndbuf,optarg);
      break;
    case 'c':
      readcoil=TRUE;
      coilno = atoi(optarg);
      break;
    case 'b':
      wrtcoil=TRUE;
      boolval= (atoi(optarg)) ? TRUE : FALSE;
      break;
    case 'f':
      readflt = TRUE;
      fltrno  = atoi(optarg);
      break;
    case 'd':
      wrtflt = TRUE;
      fdata   = atof(optarg);
      break;
    case '?':
    default:
      fprintf(stderr,"Usage: message\n");
      fprintf(stderr,"       -h host       Connect to host\n");
      fprintf(stderr,"       -?            Print this message\n");
      exit(1);
    }
  }

  sockfd = OpenTCP(host, NULL, port);

  if (readcoil && wrtcoil) readcoil= FALSE;
  if (readflt && wrtflt) readflt=FALSE;

  if (wrtcoil) {
    printf("Writing Coil[%d] = %s : ",coilno,((boolval) ? "TRUE" : "FALSE"));
    SendForceSingleCoil(sockfd,1,coilno,boolval);
    if (RecvForceSingleCoil(sockfd)) {
      printf("SUCCESS\n");
    } else {
      printf("FAILURE\n");
    }
  }
  
  if (readcoil) {
    printf("Reading Coil[%d]:",coilno);
    SendReadCoilStatus(sockfd,1,coilno,1);
    if (RecvReadCoilStatus(sockfd,rcvbuf,&bytesread)) {
      for (i=0;i<bytesread;i++) {
	inbyte = rcvbuf[i];
	printf("%02x ",inbyte);
      }
      printf("\n");
    } else {
      printf(" BAD READ\n");
    }
  }

  if (wrtflt) {
    printf("Writing Float[%d]=%+9.4f :",fltrno,fdata);
    cvt.flt = fdata;
    uint = cvt.uint;
    lower = (unsigned short) (uint & 0xFFFF);
    upper = (unsigned short) ((uint & 0xFFFF0000) >> 16);
    byte2 = (unsigned char) (lower & 0xFF);
    byte1 = (unsigned char) ((lower & 0xFF00) >> 8);
    sndbuf[0] = byte1;
    sndbuf[1] = byte2;
    
    byte2 = (unsigned char) (upper & 0xFF);
    byte1 = (unsigned char) ((upper & 0xFF00) >> 8);
    sndbuf[2] = byte1;
    sndbuf[3] = byte2;

    SendPresetMultiRegs(sockfd,1,fltrno,2,4,sndbuf);
    if (RecvPresetMultiRegs(sockfd)) {
      printf("SUCCESS\n");
    } else {
      printf("FAILURE\n");
    }
  }

  if (readflt) {
    printf("Reading Float[%d]:",fltrno);
    SendReadHoldingRegs(sockfd,1,fltrno,2);
    if (RecvReadHoldingRegs(sockfd,rcvbuf,&bytesread)) {
      for (i=0;i<bytesread;i++) {
	inbyte = rcvbuf[i];
	printf("%02x ",inbyte);
      }

      byte1 = rcvbuf[0];
      byte2 = rcvbuf[1];
      lower = (((unsigned short) byte1) * 256 + (unsigned short) byte2);
      
      byte1 = rcvbuf[2];
      byte2 = rcvbuf[3];
      upper = (((unsigned short) byte1) * 256 + (unsigned short) byte2);

      uint = (upper*65536 + lower);
      cvt.uint = uint;
      printf(" UPPER = %d LOWER = %d UINT = %d FLOAT=%f",upper,lower,uint,cvt.flt);
      printf("\n");
    } else {
      printf(" BAD READ\n");
    }
  }

  return 0;
}
