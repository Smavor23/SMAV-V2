#ifndef	_TRX_SIM_H
#define	_TRX_SIM_H

#include "usart.h"
#include "trx_simConf.h"
#include "cmsis_os.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

//********************************************************************************************************************//
//********************************************************************************************************************//

#define         MAX_USERS                           10 /* Max 10 users */
#define         MAX_ADMINS                          8 /* Max 8 Admins */

typedef struct
{
  char        SendCommand[64];
  char        ReceiveAnswer[100];
  uint32_t    SendCommandStartTime;
  uint32_t    ReceiveAnswerExeTime;
  uint16_t    ReceiveAnswerMaxWaiting;  
  uint8_t     FindAnswer;
	
}SIMAtCommand_t;

//********************************************************************************************************************//
typedef struct
{
  uint8_t	    DataTransferMode:1;
  uint8_t     Busy:1;
  uint8_t     Ready:1;
  uint8_t     Power:1;
  uint8_t     RegisterdToNetwork:1;
  uint8_t     State;
  uint8_t     NetOpen:1;
  uint8_t     TCPOpen:1;
  uint8_t     IPAddress[4];
  uint8_t     ErrorCount;
  uint8_t     SendOK:1;
	
  uint32_t		GPS_Time;
  uint8_t			GPS_Hr;
  uint8_t			GPS_Min;
  uint8_t			GPS_Sec;
  uint32_t		GPS_PrevTime;
  uint32_t		GPS_Lat;
  uint32_t		GPS_Lng;
  uint32_t		GPS_Date;
  uint8_t			GPS_Day;
  uint8_t			GPS_Mon;
  uint8_t			GPS_Yr;
  char			GPS_NS;
  char  			GPS_EW;
  float			GPS_Spd;
  uint16_t		GPS_SpdKmH;
  float			GPS_COG;
  uint8_t           GPS_Valid:1;
  
  uint8_t     Alarm;
  uint16_t    Adj_Pause;
  
  uint8_t     BatteryCharging:1;
  uint8_t     BatteryFull:1;
  uint8_t     BatteryPercent;
  float       BatteryVoltage;
  
  uint64_t		MSG_Number;
  uint8_t			MSG_Valid:1;
  uint8_t			MSG_CMD;
  uint8_t			MSG_SubCMD;
  uint64_t		MSG_Value;

  int         Active_Admin_Idx;
  int         Active_User_Idx;
  
  uint8_t     Signal; 
  uint8_t	    SMSPending;
  uint32_t    lang_set;
  uint32_t    num_set;
  
  char        *cmd[6];
  char        *ans_P[7];
  char        *ans_N[7];
  char        *num[9];

  //char        *alert[70];
}SIMStatus_t;

//********************************************************************************************************************//
typedef struct
{
  uint64_t Number;
  uint8_t  Lang;
  uint8_t  Alert_Stat;
  uint8_t  Temp_Diff;   
} SIMNumber_t;

//********************************************************************************************************************//
typedef struct
{
  int8_t			numIndex;
  uint8_t      	Alert_stat:1;
  
  uint8_t      	Temp_flag:1;
  uint8_t      	Batt_flag:1;
  uint8_t      	Gas_flag:1;
  uint8_t      	LoB_flag:1;	
	
  uint32_t     	Alert_delay_1;
  uint32_t     	Alert_delay_2;

  
  uint32_t     	AlertCounter_1;
  uint32_t     	AlertCounter_2;
  
  uint32_t	  	Alert_Lat;
  uint32_t	     Alert_Lng;
	
  uint64_t		Alt_Number;
  
  SIMNumber_t		Admin_Numbers[MAX_ADMINS];  /* AT Max 8 Admins can be added to the system */  
  SIMNumber_t		User_Numbers[MAX_USERS];   /*  AT max 10 user numbers can be added        */
  
}AlertStatus_t;

//********************************************************************************************************************//
typedef struct
{
  uint32_t			BufferStartTime;
  uint8_t				BufferExeTime;
	
  uint16_t			UsartRxIndex;
  uint8_t				UsartRxTemp;
  uint8_t		          UsartRxBuffer[_SIM_BUFFER_SIZE];
  uint32_t	          UsartRxLastTime;
	
  uint8_t		          SMSBuffer[_SMS_BUFFER_SIZE];
  uint8_t		          AuxBuffer[_AUX_BUFFER_SIZE];
  uint8_t				AlarmsBuffer[_ALM_BUFFER_SIZE];
	
  SIMStatus_t            Status;
  AlertStatus_t          Alert;
	
  SIMAtCommand_t 	     AtCommand;
	
}SIM_t;

//********************************************************************************************************************//

extern SIM_t          SIM;
extern uint8_t        LastCmdIdx; 
extern uint8_t        Alert1Cmd; 
extern uint8_t        Alert2Cmd; 



//********************************************************************************************************************//
//********************************************************************************************************************//
//********************************************************************************************************************//

void						SIM_SendString(char *str);
void                          SIM_SendRaw(uint8_t *Data,uint16_t len);
uint8_t     		          SIM_SendAtCommand(char *AtCommand,int32_t  MaxWaiting_ms,char *Answer);

//********************************************************************************************************************//
void SIM7600_RESET(void);
void		      SIM_RxCallBack(void);
void		      SIM_Init(osPriority Priority);
void            SIM_SaveParameters(void);
void            SIM_SetPower(bool TurnOn);
void            SIM_SetFactoryDefault(void);
int             SIM_CMDParse(int command, float temp_1, float temp_2, float hum_1, float hum_2, float vsen1, float vsen2, int isen, int tsen, float gas_1, float wind_1);
void            SIM_SendSMS(char *str, int);
void            SIM_SetLang(void);
void            SIM_SetNum(void);
void            SIM_SendAlertSMS(char *str);
void            Aux_print(void);
void			 attachGPRS(void);
void RPC_control(void);
void 		 sendMessage( float sht30_t,float sht20_t,float sht30_h,float sht20_h,float Vsense_12,float Vsense_22,float Gas_l,float Wind_s);
void Send_Diagnostic_data(char* rssiString, char* functionalityStatus, char* operatorName, char* Module_Status);
void Send_Attribute_data(bool relayState,char* lat,char* lon,char* rssiString, char* operatorName, char* Module_Status);
void     set(int set_status);
void 		 fetchLED(void);
void RPC_FETCH_ATTRIBUTES(void);
void            updateLed(void);
void            sendHumiTemp(void);
void            sendLedState(void);

//********************************************************************************************************************//
#endif
