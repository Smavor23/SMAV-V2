#ifndef TRXGPS_SIM5320_H
#define TRXGPS_SIM5320_H
#ifdef __cplusplus
 extern "C" {
#endif

#include "main.h"

 static char simAT[] = "AT";
 static char simBaud[]   = "AT+IPREX=4800";
 static char simResponse[] = "ATE0";
 static char simFunc[]   = "AT+CFUN=1,1";
 static char simFuncOff[]   = "AT+CFUN=7";
 static char simFuncRst[]   = "AT+CFUN=6";

#ifdef __cplusplus
}
#endif
#endif
