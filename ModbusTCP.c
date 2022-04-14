#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "TCP_util.h"
#include "ModbusTCP.h"

static char _szHexSend[ADAM_MAX_MSGLEN]; // for modbus functions
static char _szHexRecv[ADAM_MAX_MSGLEN]; // for modbus functions

int CommonSendRead(int sockfd,
		   int i_iAddr, 
		   int i_iFun,
		   int i_iStartIndex,
		   int i_iTotalPoint)
{
  int iHexLen, iRTULen;
  unsigned char szRTUCmd[ADAM_MAX_MSGLEN]={0};
  char szModCmd[ADAM_MAX_MSGLEN] = {0};

  memset(_szHexSend, 0, ADAM_MAX_MSGLEN);
  sprintf(szModCmd, "%02X%02X%04X%04X", i_iAddr, i_iFun, i_iStartIndex-1, i_iTotalPoint);
  sprintf(_szHexSend, "0000000000%02X%s", strlen(szModCmd)/2, szModCmd);
  iHexLen = strlen(_szHexSend);
  // form TCP-RTU command
  HexToRTU(_szHexSend, szRTUCmd);
  iRTULen = iHexLen/2;
  return SendTCP(sockfd, szRTUCmd, iRTULen);
}

int CommonRecvRead(int sockfd, unsigned char o_szData[], int *o_iTotalByte)
{
  unsigned char szRecv[ADAM_MAX_MSGLEN]={0};
  int iRTULen, iDatLen;

  // iRTULen must be greater or equal 9; 
  // 'header(12)', 'addr(2)', 'fun(2)', 'count(2)', 18 TCP-HEX bytes
  // 'header(6)', 'addr(1)', 'fun(1)', 'count(1)', 9 TCP-RTU bytes
  if (RecvTCP(sockfd, szRecv, &iRTULen) && iRTULen>9)
    {
      RTUToHex(szRecv, iRTULen, _szHexRecv);
      if (_szHexSend[14] == _szHexRecv[14] && 
	  _szHexSend[15] == _szHexRecv[15]) 
	// function code must be the same, otherwise, exception
	{
	  iDatLen = szRecv[8];
	  *o_iTotalByte = iDatLen;
	  memcpy(o_szData, szRecv+9, iDatLen);
	  return TRUE;
	}
    }
  return FALSE;
}

void HexToRTU(char *i_szHexCmd, unsigned char *o_szRTUCmd)
{
  int idx, iRTULen;
  char szTmp[8];
  unsigned int ich;

  iRTULen = strlen(i_szHexCmd)/2;
  for (idx=0; idx<iRTULen; idx++)
    {
      memset(szTmp, 0, 8);
      memcpy(szTmp, i_szHexCmd + idx*2, 2);
      sscanf(szTmp, "%X", &ich);
      *(o_szRTUCmd+idx) = (unsigned char) (ich & 0xFF);
    }
}

void RTUToHex(unsigned char *i_szRTUCmd, int i_RTULen, char *o_szHexCmd)
{
  int idx;
  char szTmp[8];
  unsigned char ch;

  o_szHexCmd[0] = '\0';
  for (idx=0; idx<i_RTULen; idx++)
    {
      memset(szTmp, 0, 8);
      ch = i_szRTUCmd[idx];
      sprintf(szTmp, "%02X", (unsigned int) ch);
      strcat(o_szHexCmd, szTmp);
    }
}

// function (01 HEX)
int SendReadCoilStatus(int sockfd,
		       int i_iAddr, 
		       int i_iStartIndex,
		       int i_iTotalPoint)
{
  return CommonSendRead(sockfd, i_iAddr, 1, i_iStartIndex, i_iTotalPoint);
}

// function (01 HEX)
int RecvReadCoilStatus(int sockfd,unsigned char o_szData[], int *o_iTotalByte)
{
  return CommonRecvRead(sockfd, o_szData, o_iTotalByte);
}

// function (02 HEX)
int SendReadInputStatus(int sockfd, 
			int i_iAddr, 
			int i_iStartIndex,
			int i_iTotalPoint)
{
  return CommonSendRead(sockfd, i_iAddr, 2, i_iStartIndex, i_iTotalPoint);
}

// function (02 HEX)
int RecvReadInputStatus(int sockfd, unsigned char o_szData[], int *o_iTotalByte)
{
  return CommonRecvRead(sockfd, o_szData, o_iTotalByte);
}

// function (03 HEX)
int SendReadHoldingRegs(int sockfd,
			int i_iAddr, 
			int i_iStartIndex,
			int i_iTotalPoint)
{
  return CommonSendRead(sockfd, i_iAddr, 3, i_iStartIndex, i_iTotalPoint);
}

// function (03 HEX)
int RecvReadHoldingRegs(int sockfd,unsigned char o_szData[], int *o_iTotalByte)
{
  return CommonRecvRead(sockfd,o_szData, o_iTotalByte);
}

// function (04 HEX)
int SendReadInputRegs( int sockfd,
		       int i_iAddr, 
		       int i_iStartIndex,
		       int i_iTotalPoint)
{
  return CommonSendRead(sockfd,i_iAddr, 4, i_iStartIndex, i_iTotalPoint);
}

// function (04 HEX)
int RecvReadInputRegs(int sockfd,unsigned char o_szData[], int *o_iTotalByte)
{
  return CommonRecvRead(sockfd,o_szData, o_iTotalByte);
}

// function (05 HEX)
int SendForceSingleCoil(int sockfd,
			int i_iAddr,
			int i_iCoilIndex,
			int i_iData)
{
  int iHexLen, iRTULen;
  unsigned char szRTUCmd[ADAM_MAX_MSGLEN]={0};
  char szModCmd[ADAM_MAX_MSGLEN] = {0};
  int coilData;

  if (i_iData == 0)
    coilData = 0x0000;
  else
    coilData = 0xFF00;
  memset(_szHexSend, 0, ADAM_MAX_MSGLEN);
  sprintf(szModCmd, "%02X%02X%04X%04X", i_iAddr, 5, i_iCoilIndex-1, coilData);
  sprintf(_szHexSend, "0000000000%02X%s", strlen(szModCmd)/2, szModCmd);
  iHexLen = strlen(_szHexSend);
  // form TCP-RTU command
  HexToRTU(_szHexSend, szRTUCmd);
  iRTULen = iHexLen/2;
  return SendTCP(sockfd,szRTUCmd, iRTULen);
}

// function (05 HEX)
int RecvForceSingleCoil(int sockfd)
{
  unsigned char szRecv[ADAM_MAX_MSGLEN];
  int iRTULen;

  // 'header(12)', 'addr(2)', 'fun(2)', 'Coil addr Hi-Lo(4)', 
  // 'Force data Hi-Lo(4)', 24 TCP-HEX bytes, 12 TCP_RTU bytes
  if (RecvTCP(sockfd,szRecv, &iRTULen) && iRTULen>=12)
    {
      RTUToHex(szRecv, iRTULen, _szHexRecv);
      if (_szHexSend[14] == _szHexRecv[14] && 
	  _szHexSend[15] == _szHexRecv[15]) 
	// function code must be the same, otherwise, exception
	return TRUE;
    }
  return FALSE;
}

// function (06 HEX)
int SendPresetSingleReg(int sockfd,
			int i_iAddr,
			int i_iRegIndex,
			int i_iData)
{
  int iHexLen, iRTULen;
  unsigned char szRTUCmd[ADAM_MAX_MSGLEN]={0};
  char szModCmd[ADAM_MAX_MSGLEN] = {0};

  memset(_szHexSend, 0, ADAM_MAX_MSGLEN);
  sprintf(szModCmd, "%02X%02X%04X%04X", i_iAddr, 6, i_iRegIndex-1, i_iData);
  sprintf(_szHexSend, "0000000000%02X%s", strlen(szModCmd)/2, szModCmd);
  iHexLen = strlen(_szHexSend);
  // form TCP-RTU command
  HexToRTU(_szHexSend, szRTUCmd);
  iRTULen = iHexLen/2;
  return SendTCP(sockfd,szRTUCmd, iRTULen);
}

// function (06 HEX)
int RecvPresetSingleReg(int sockfd)
{
  unsigned char szRecv[ADAM_MAX_MSGLEN];
  int iRTULen;

  // iRTULen must be equal 12;
  // 'header(12)', 'addr(2)', 'fun(2)', 'Coil addr Hi-Lo(4)',
  // 'Force data Hi-Lo(4)', 24 TCP-HEX bytes, 12 TCP-RTU bytes
  if (RecvTCP(sockfd,szRecv, &iRTULen) && iRTULen>=12)
    {
      RTUToHex(szRecv, iRTULen, _szHexRecv);
      if (_szHexSend[14] == _szHexRecv[14] && 
	  _szHexSend[15] == _szHexRecv[15]) 
	// function code must be the same, otherwise, exception
	return TRUE;
    }
  return FALSE;
}

// function (0F HEX)
int SendForceMultiCoils(int sockfd,
			int i_iAddr, 
			int i_iCoilIndex,
			int i_iTotalPoint,
			int i_iTotalByte,
			unsigned char i_szData[])
{
  int idx, iHexLen, iRTULen;
  unsigned char szRTUCmd[ADAM_MAX_MSGLEN]={0};
  char szTmp[8];
  char szModCmd[ADAM_MAX_MSGLEN] = {0};

  // form modbus command
  memset(_szHexSend, 0, ADAM_MAX_MSGLEN);
  sprintf(szModCmd, "%02X%02X%04X%04X%02X", i_iAddr, 0x0F, i_iCoilIndex-1,
	  i_iTotalPoint, i_iTotalByte);
  for (idx=0; idx<i_iTotalByte; idx++)
    {
      memset(szTmp, 0, 8);
      sprintf(szTmp, "%02X", i_szData[idx]);
      strcat(szModCmd, szTmp);
    }

  // form TCP-HEX command
  sprintf(_szHexSend, "0000000000%02X%s", strlen(szModCmd)/2, szModCmd);
  iHexLen = strlen(_szHexSend);
  // form TCP-RTU command
  HexToRTU(_szHexSend, szRTUCmd);
  iRTULen = iHexLen/2;
  return SendTCP(sockfd,szRTUCmd, iRTULen);
}

// function (0F HEX)
int RecvForceMultiCoils(int sockfd)
{
  unsigned char szRecv[ADAM_MAX_MSGLEN];
  int iRTULen;

  // iRTULen must be equal 12;
  // 'header(12)', 'addr(2)', 'fun(2)', 'addr Hi-Lo(4)',
  // 'quantity Hi-Lo(4)', 24 TCP-HEX bytes, 12 TCP-RTU bytes
  if (RecvTCP(sockfd,szRecv, &iRTULen) && iRTULen>=12)
    {
      RTUToHex(szRecv, iRTULen, _szHexRecv);
      if (_szHexSend[14] == _szHexRecv[14] && 
	  _szHexSend[15] == _szHexRecv[15]) 
	// function code must be the same, otherwise, exception
	return TRUE;
    }
  return FALSE;
}

// function (10 HEX)
int SendPresetMultiRegs(int sockfd,
			int i_iAddr, 
			int i_iStartReg,
			int i_iTotalReg,
			int i_iTotalByte,
			unsigned char i_szData[])
{
  int idx, iHexLen, iRTULen;
  unsigned char szRTUCmd[ADAM_MAX_MSGLEN]={0};
  char szTmp[8];
  char szModCmd[ADAM_MAX_MSGLEN] = {0};

  // form modbus command
  memset(_szHexSend, 0, ADAM_MAX_MSGLEN);
  sprintf(szModCmd, "%02X%02X%04X%04X%02X", i_iAddr, 0x10, i_iStartReg-1,
	  i_iTotalReg, i_iTotalByte);
  for (idx=0; idx<i_iTotalByte; idx++)
    {
      memset(szTmp, 0, 8);
      sprintf(szTmp, "%02X", i_szData[idx]);
      strcat(szModCmd, szTmp);
    }

  // form TCP-HEX command
  sprintf(_szHexSend, "0000000000%02X%s", strlen(szModCmd)/2, szModCmd);
  iHexLen = strlen(_szHexSend);
  // form TCP-RTU command
  HexToRTU(_szHexSend, szRTUCmd);
  iRTULen = iHexLen/2;
  return SendTCP(sockfd,szRTUCmd, iRTULen);
}

// function (10 HEX)
int RecvPresetMultiRegs(int sockfd)
{
  unsigned char szRecv[ADAM_MAX_MSGLEN];
  int iRTULen;

  // iRTULen must be equal 12;
  // 'header(12)', 'addr(2)', 'fun(2)', 'addr Hi-Lo(4)',
  // 'No. of reg Hi-Lo(4)', 24 TCP-HEX bytes, 12 TCP-RTU bytes
  if (RecvTCP(sockfd,szRecv, &iRTULen) && iRTULen>=12)
    {
      RTUToHex(szRecv, iRTULen, _szHexRecv);
      if (_szHexSend[14] == _szHexRecv[14] && 
	  _szHexSend[15] == _szHexRecv[15]) 
	// function code must be the same, otherwise, exception
	return TRUE;
    }
  return FALSE;
}

//=========================================================================
//=========================================================================
int ReadTransaction(int sockfd,
		    char *i_szTwoHexAddr,
		    int  i_regLen,
		    char *i_szCmd,
		    char *o_szRecv,
		    char *o_szSendHex, // if NULL, no value assign
		    char *o_szRecvHex) // if NULL, no value assign
{
  int iAddr, iLen, iReg, iTotal, idx, count, iHexLen;
  unsigned char szCmd[ADAM_MAX_MSGLEN] = {0}, szRecv[ADAM_MAX_MSGLEN] = {0};
  char szBuf[3]={0};

  // address
  sscanf(i_szTwoHexAddr, "%X", &iAddr);
  // copy command
  iLen = strlen(i_szCmd);
  memcpy(szCmd, i_szCmd, iLen);
  szCmd[iLen] = 0x0D;
  iLen++;
  // 2 bytew per reg
  if (iLen % 2) // odd number of bytes
    iLen++;
  iReg = iLen/2;
  // send read command
  if (SendPresetMultiRegs(sockfd,iAddr, 10000, iReg, iLen, szCmd) &&
      RecvPresetMultiRegs(sockfd))
    {
      if (o_szSendHex) // not NULL, copy send HEX to it
	strcpy(o_szSendHex, _szHexSend);
      count = 0;
      while(1)
	{
	  if (SendReadHoldingRegs(sockfd,iAddr, 10000, i_regLen) &&
	      RecvReadHoldingRegs(sockfd,szRecv, &iTotal)) // szRecv is data alreay
	    {
	      for (idx=0; idx<iTotal; idx++)
		{
		  if (szRecv[idx] == 0x0D)
		    {
		      o_szRecv[idx] = 0;
		      break;
		    }
		  o_szRecv[idx] = szRecv[idx];
		}
	      // Modbus/TCP (byte)
	      // header(6), add(1), fun(1), byte Count(1), data(idx+1)
	      if (o_szRecvHex) // not NULL, copy recv HEX to it
		{
		  iHexLen = 18+(idx+1)*2;
		  memcpy(o_szRecvHex, _szHexRecv, iHexLen);
		  o_szRecvHex[iHexLen] = '\0';
		  // modify header count
		  sprintf(szBuf, "%02X", idx+1+3); // data length(idx+1)+add(1)+fun(1)+byte count(1)
		  memcpy(o_szRecvHex+10, szBuf, 2);
		  // modify byte count
		  sprintf(szBuf, "%02X", idx+1); // data length(idx+1)
		  memcpy(o_szRecvHex+16, szBuf, 2);
		}
	      return TRUE;
	    }
	  if (++count >= 5)
	    return FALSE;
	  else
	    sleep(100);
	}
    }
  return FALSE;
}

