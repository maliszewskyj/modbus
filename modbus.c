static char cvsid[] = "$Id: modbus.c,v 1.0 2022/04/05 19:32:45 nickm Exp $";
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tcl.h>
#include "ModbusTCP.h"
#include "TCP_util.h"

/*
 * modbus - code for setting/clearing coils and reading/querying registers
            using Modbus/TCP

   modbus create <host ip>

   created command has the following syntax:

   mb0 coil <coiladdr>                Return coil state - true/false
   mb0 coil <coiladdr> <on/off>       Set coil state - true/false
   mb0 register <regaddr>             Read register value as a float
   mb0 register <regaddr> <value>     Set register value as a float




Transactions should be quick and each one should involve a separate 
open and close of the associated TCP ports. 

*/

static int mb_counter;

typedef struct {
  char host[80];
  int sockfd;
} modbusdev;

union CVT {
  float flt;
  unsigned int uint;
} cvt;

static unsigned char sndbuf[80];
static unsigned char rcvbuf[80];
int port=502;

int modbus_coil_read(ClientData clientdata, Tcl_Interp * interp,
		     int coilno)
{
  modbusdev * mptr;
  int boolval;
  int bytesread;

  mptr = clientdata;

  if ((mptr->sockfd = OpenTCP(mptr->host, NULL, port)) < 0) return TCL_ERROR;
  
  SendReadCoilStatus(mptr->sockfd,1,coilno,1);
  if (RecvReadCoilStatus(mptr->sockfd,rcvbuf,&bytesread)) {
    boolval = (int) rcvbuf[bytesread-1];
    Tcl_SetObjResult(interp,Tcl_NewBooleanObj(boolval));
  } else {
    Tcl_SetResult(interp,"modbus coil read failure",TCL_STATIC);
    CloseTCP(mptr->sockfd);
    mptr->sockfd=-1;
    return TCL_ERROR;
  }

  CloseTCP(mptr->sockfd);
  mptr->sockfd=-1;
  return TCL_OK;
}

int modbus_coil_write(ClientData clientdata, Tcl_Interp * interp,
		    int coilno, int state)
{
  modbusdev * mptr;

  mptr = clientdata;
  if ((mptr->sockfd = OpenTCP(mptr->host, NULL, port)) < 0) return TCL_ERROR;
  
  SendForceSingleCoil(mptr->sockfd,1,coilno,state);
  if (RecvForceSingleCoil(mptr->sockfd)) {
    Tcl_SetResult(interp, "modbus coil set success",TCL_STATIC);
  } else {
    Tcl_SetResult(interp, "modbus coil set failure",TCL_STATIC);
    CloseTCP(mptr->sockfd);
    return TCL_ERROR;
  }
  CloseTCP(mptr->sockfd);
  mptr->sockfd=-1;
  return TCL_OK;
}

int modbus_register_read(ClientData clientdata, Tcl_Interp * interp,
			 int regno)
{
  modbusdev * mptr;
  int bytesread;
  unsigned short lower, upper;
  double value;

  mptr = clientdata;

  memset(rcvbuf,0,sizeof(rcvbuf));
  if ((mptr->sockfd = OpenTCP(mptr->host, NULL, port)) < 0) return TCL_ERROR;
  
  SendReadHoldingRegs(mptr->sockfd,1,regno,2);
  if (RecvReadHoldingRegs(mptr->sockfd,rcvbuf,&bytesread)) {
    lower = (((unsigned short) rcvbuf[0]) * 256 + (unsigned short) rcvbuf[1]);
    upper = (((unsigned short) rcvbuf[2]) * 256 + (unsigned short) rcvbuf[3]);

    cvt.uint = (upper*65536 + lower);
    value = (double) cvt.flt;

    Tcl_SetObjResult(interp,Tcl_NewDoubleObj(value));
  }
  CloseTCP(mptr->sockfd);
  mptr->sockfd=-1;
  return TCL_OK;
}

int modbus_register_write(ClientData clientdata, Tcl_Interp * interp,
			  int regno, double value)
{
  modbusdev * mptr;
  int bytesread;
  unsigned short lower, upper;

  mptr = clientdata;

  memset(sndbuf,0,sizeof(sndbuf));
  if ((mptr->sockfd = OpenTCP(mptr->host, NULL, port)) < 0) return TCL_ERROR;
  
  cvt.flt = (float) value;
  lower = (unsigned short) (cvt.uint & 0xFFFF);
  upper = (unsigned short) ((cvt.uint & 0xFFFF0000) >> 16);
  sndbuf[0] = (unsigned char) ((lower & 0xFF00) >> 8);
  sndbuf[1] = (unsigned char) (lower & 0xFF);
  sndbuf[2] = (unsigned char) ((upper & 0xFF00) >> 8);
  sndbuf[3] = (unsigned char) (upper & 0xFF);

  SendPresetMultiRegs(mptr->sockfd,1,regno,2,4,sndbuf);
  if (RecvPresetMultiRegs(mptr->sockfd)) {
    Tcl_SetResult(interp,"modbus register write success",TCL_STATIC);  
  } else {
    Tcl_SetResult(interp,"modbus register write failure",TCL_STATIC);
    CloseTCP(mptr->sockfd);
    mptr->sockfd=-1;
    return TCL_ERROR;
  }
  CloseTCP(mptr->sockfd);
  mptr->sockfd=-1;
  return TCL_OK;
}

int modbus_command(ClientData clientdata, Tcl_Interp * interp,
		   int objc, Tcl_Obj * objv[])
{
  char * strval;
  int lstr,retn;
  double dval;
  float fval;
  int boolval;
  int coilno,regno;
  modbusdev * mptr;

  mptr = clientdata;
  coilno=0;
  regno=0;
  if (objc < 2) {
    Tcl_SetResult(interp,
		  "Options: coil register",
		  TCL_STATIC);
    return TCL_ERROR;
  }

  strval = Tcl_GetStringFromObj(objv[1],&lstr);
  
  if (!strncmp(strval,"coil",lstr)) {
    if (objc < 3) {
      Tcl_SetResult(interp,"specify coil number", TCL_STATIC);
      return TCL_ERROR;
    }
    
    Tcl_GetIntFromObj(interp,objv[2],&coilno);
    if (objc > 3) {
      Tcl_GetBooleanFromObj(interp,objv[3],&boolval);
      return modbus_coil_write(clientdata,interp,coilno,boolval);
    } else {
      return modbus_coil_read(clientdata,interp,coilno);
    }
  } else if (!strncmp(strval,"register",lstr)) {
    if (objc < 3) {
      Tcl_SetResult(interp,"specify register address",TCL_STATIC);
      return TCL_ERROR;
    }
    Tcl_GetIntFromObj(interp,objv[2],&regno);
    if (objc > 3) {
      Tcl_GetDoubleFromObj(interp,objv[3],&dval);
      return modbus_register_write(clientdata,interp,regno,dval);
    } else {
      return modbus_register_read(clientdata,interp,regno);
    }
  } else {
    Tcl_SetResult(interp,
		  "Options: coil register debug",
		  TCL_STATIC);
    return TCL_ERROR;
  }
  
  return TCL_OK;
}

/* Top level TCL wrapper */
int modbus(ClientData clientdata, Tcl_Interp * interp,
	   int objc, Tcl_Obj * objv[])
{
  char * strval, mbname[80], host[80];
  int lstr;
  modbusdev * mptr;
  if (objc < 3) {
    Tcl_SetResult(interp, "Usage: modbus create <hostname>",TCL_STATIC);
    return TCL_ERROR;
  }

  strval = Tcl_GetStringFromObj(objv[1],&lstr);
  if (strcmp("create",strval)) {
    Tcl_SetResult(interp, "Options: create",TCL_STATIC);
    return TCL_ERROR;
  }

  strval = Tcl_GetStringFromObj(objv[2],&lstr);
  mptr = (modbusdev *) ckalloc(sizeof(modbusdev));
  memset(mptr,0,sizeof(modbusdev));
  strncpy(mptr->host,strval,lstr);

  sprintf(mbname,"mb%d",mb_counter);
  mb_counter++;

  // Should probably connect to make sure this is a working modbus/TCP host
  
  Tcl_CreateObjCommand(interp,mbname,(Tcl_ObjCmdProc *) modbus_command,
		       (ClientData) mptr, NULL);
  Tcl_SetObjResult(interp,Tcl_NewStringObj(mbname,strlen(mbname)));
  return TCL_OK;
}

/*  Entry point for Tcl interface
 * 
 */
int Modbus_Init(Tcl_Interp *interp) {
  mb_counter = 0;
  Tcl_CreateObjCommand(interp,"modbus",(Tcl_ObjCmdProc *) modbus,
		 (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
  return TCL_OK;
}
