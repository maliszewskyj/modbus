#ifndef ADAM_MAX_MSGLEN
#define ADAM_MAX_MSGLEN 1024
#endif

int CommonSendRead(int sockfd, 
		    int i_iAddr, 
		    int i_iFun,
		    int i_iStartIndex,
		    int i_iTotalPoint);
int CommonRecvRead(int sockfd, unsigned char o_szData[], int *o_iTotalByte);


void HexToRTU(char *i_szHexCmd, unsigned char *o_szRTUCmd);
void RTUToHex(unsigned char *i_szRTUCmd, int i_RTULen, char *o_szHexCmd);

// function : 01 (HEX 01)
// i_iAddr      : Slave address, decimal number
// i_iStartIndex : start from 1, not 0
// i_iTotal     : total bytes to read
int SendReadCoilStatus(int sockfd,
			int i_iAddr,
			int i_iStartIndex,
			int i_iTotalPoint);
int RecvReadCoilStatus(int sockfd, 
			unsigned char o_szData[], 
			int *o_iTotalByte);

// function : 02 (HEX 02)
int SendReadInputStatus(int sockfd,
			 int i_iAddr, 
			 int i_iStartIndex,
			 int i_iTotalPoint);
int RecvReadInputStatus(int sockfd,
			 unsigned char o_szData[], 
			 int *o_iTotalByte);

// function : 03 (HEX 03)
int SendReadHoldingRegs(int sockfd,
			 int i_iAddr, 
			 int i_iStartIndex,
			 int i_iTotalPoint);
int RecvReadHoldingRegs(int sockfd,
			 unsigned char o_szData[], 
			 int *o_iTotalByte);

// function : 04 (HEX 04)
int SendReadInputRegs( int sockfd,
			int i_iAddr, 
			int i_iStartIndex,
			int i_iTotalPoint);
int RecvReadInputRegs( int sockfd,
			unsigned char o_szData[], 
			int *o_iTotalByte);

// function : 05 (HEX 05)
int SendForceSingleCoil(int sockfd,
			 int i_iAddr,
			 int i_iCoilIndex,
			 int i_iData); // 0: OFF; otherwise: ON
int RecvForceSingleCoil(int sockfd);

// function : 06 (HEX 06)
int SendPresetSingleReg(int sockfd,
			 int i_iAddr, 
			 int i_iRegIndex,
			 int i_iData);
int RecvPresetSingleReg();

// function : 15 (HEX 0F)
int SendForceMultiCoils(int sockfd,
			 int i_iAddr, 
			 int i_iCoilIndex,
			 int i_iTotalPoint,
			 int i_iTotalByte,
			 unsigned char i_szData[]);
int RecvForceMultiCoils(int sockfd);

// function : 16 (HEX 10)
// i_iStartReg : start from 1
int SendPresetMultiRegs(int sockfd,
			 int i_iAddr, 
			 int i_iStartReg,
			 int i_iTotalReg,
			 int i_iTotalByte,
			 unsigned char i_szData[]);
int RecvPresetMultiRegs(int sockfd);

// extra
int ReadTransaction(int sockfd,
		     char *i_szTwoHexAddr,
		     int  i_regLen,
		     char *i_szCmd,
		     char *o_szRecv,
		     char *o_szSendHex, // if NULL, no value assign
		     char *o_szRecvHex); // if NULL, no value assign
