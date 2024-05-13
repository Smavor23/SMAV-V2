#include "trx_sim.h"
#include "trx_simConf.h"
#include "cmsis_os.h"
#include "usart.h"
#include "labels.h"
#include <stdio.h> 
#include "sht2x_for_stm32_hal.h"
#include "at24cxx.h" 
#include "main.h"
#include <ctype.h>

void NVIC_SystemReset(void); // Assuming this function exists for microcontroller reset
// Declaration of SIM_Status_OK function
uint8_t SIM_Status_OK(char *response);
// Function declaration
uint8_t SIM_SendAtCommande(char *command, uint32_t timeout_ms, char *response, uint32_t response_size);
uint8_t slot = 0;
#define PREF_SMS_STORAGE "\"SM\""
char ATcommandd[80];
char mobileNumber[] = "+212771101564";  // Enter the Mobile Number you want to send to
uint8_t i=0;
#define SUCCESS 1
#define FAILURE 0
char temp_str[20]; 
char ARMED_STATUS[10];
bool relayState = false; // Initialize the relay state
bool prev_relayState; // Initialize the relay state
//Signal strength Variable
char signalString[10]; // Assuming maximum 10 characters for the signal value
int signalStrength;
char rssiString[50];
int functionality;
char functionalityStatus[50]; // Assuming maximum 50 characters for the functionality status
char operatorName[50]; // Assuming maximum 50 characters for the operator name
int rssiValue, berValue;

char *token;
char *saveptr; // Required for strtok_r
char lat[20], lon[20];

bool rpc_status=true;
bool Previous_rpc_status=true;
bool fonction_control=true;
bool prev_fonction_control=true;
SIM_t      		SIM;
osThreadId 		SIMBuffTaskHandle;
osThreadId		AlarmBuffTaskHandle;
bool            AlarmBuffQueue;
void 	        StartSIMBuffTask(void const * argument);
void            StartAlarmBuffTask(void const * argument);
uint8_t         LastCmdIdx = LAST_CMD_IDX;
uint8_t         Alert1Cmd = ALERT1_CMD;
uint8_t         Alert2Cmd = ALERT2_CMD;
static uint8_t  temp1_val;
static uint8_t  temp2_val;
static uint8_t  wind1_val;
uint8_t         MachineState = 0;
bool            WindyCond = false;
uint8_t         ledStatus = 0;



enum Machine_State{
	STARTED = 1,
	NOT_STARTED,
	STOPPED,
	MACHINE_ERROR,
};


/********************************************
 * ********   EEPROM Data MAP       *********
 * ******************************************
 * Adress           Values       bytes 		*
 * -----------------------------------------*
 * 0                Numset         2   		*
 * 2                Usr1Langset    1   		*
 * 3                Usr1Alarm      1   		*
 * 4                Usr1Temp       2        *    
 * 6                User1Number    12  		*
 * 18               Usr2Langset    1    	*          
 * 19               User2Alarm     1		*
 * 20               Usr2Temp       2        *    
 * 11               User2Num       12		*
 *                  ....					*
 *                  ....					*
 * 162              Admin1Langset  1		*
 * 											*
*/
#define         AT24_USR_NUM_CNT_ADDR       		0 /* At the start of AT24 ROM */ 
/* Address  = (user_number-12bytes * max users) + (num_set-2bytes + (lang-1byte * max_users) + admin-1byte) */
#define         NUM_SET_BYTES                       32
#define         LANG_SET_BYTES                      32
#define         USR_NUM_BYTES                       32 
#define         ALARM_BYTES                        32
#define         TEMP_BYTES                          32
#define         TOTAL_BYTES_PERUSER                 (USR_NUM_BYTES + LANG_SET_BYTES + ALARM_BYTES + TEMP_BYTES)                         
#define 		USER_LANG_IDX_EEPROM(user_idx) 		((TOTAL_BYTES_PERUSER * user_idx) + NUM_SET_BYTES)
#define         USER_ALARM_IDX_EEPROM(user_idx)     (USER_LANG_IDX_EEPROM(user_idx) + LANG_SET_BYTES) 
#define         USER_TEMP_IDX_EEPROM(user_idx)      (USER_ALARM_IDX_EEPROM(user_idx) + ALARM_BYTES)
#define 		USER_NUM_IDX_EEPROM(user_idx) 		(USER_TEMP_IDX_EEPROM(user_idx) + TEMP_BYTES)
#define 		ADMIN_LANG_IDX_EEPROM(admin_idx) 	((TOTAL_BYTES_PERUSER * MAX_USERS) + NUM_SET_BYTES + ((LANG_SET_BYTES + ALARM_BYTES + TEMP_BYTES) * admin_idx))
#define         ADMIN_ALARM_IDX_EEPROM(admin_idx)   (ADMIN_LANG_IDX_EEPROM(admin_idx ) + LANG_SET_BYTES)
#define         ADMIN_TEMP_IDX_EEPROM(admin_idx)    (ADMIN_ALARM_IDX_EEPROM(admin_idx) + ALARM_BYTES)

#define         VOLTAGE_PER_GAS_PERCENT             0.03183 /* on 55.5% gas voltage is 1.83 and on 80% gas voltage is 2.64 */

//void					AlarmsTask(void const * argument);
//********************************************************************************************************************//
//********************************************************************************************************************//
//********************************************************************************************************************//
char *mystristr(char *haystack, char *needle)
{
   if ( !*needle )
   {
      return haystack;
   }
   for ( ; *haystack; ++haystack )
   {
      if ( toupper(*haystack) == toupper(*needle) )
      {
         /*
          * Matched starting char -- loop through remaining chars.
          */
         const char *h, *n;
         for ( h = haystack, n = needle; *h && *n; ++h, ++n )
         {
            if ( toupper(*h) != toupper(*n) )
            {
               break;
            }
         }
         if ( !*n ) /* matched all of 'needle' to null termination */
         {
            return haystack; /* return the start of the match */
         }
      }
   }
   return 0;
}
//********************************************************************************************************************//
void	SIM_SendString(char *str)
{
  HAL_UART_Receive_IT(&_SIM_USART,&SIM.UsartRxTemp,1);	
  HAL_UART_Transmit(&_SIM_USART,(uint8_t*)str,strlen(str),HAL_MAX_DELAY);
  osDelay(10);
}
//********************************************************************************************************************//
void  SIM_SendRaw(uint8_t *Data,uint16_t len)
{
  HAL_UART_Receive_IT(&_SIM_USART,&SIM.UsartRxTemp,1);	
  HAL_UART_Transmit(&_SIM_USART,Data,len,100);
  osDelay(10);
}
//********************************************************************************************************************//
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
 if(huart->Instance==USART3)
 {
  if(SIM.UsartRxTemp!=0)
  {
    HAL_UART_Transmit(&huart2,&SIM.UsartRxTemp,1,0);
    SIM.UsartRxLastTime = HAL_GetTick();
    SIM.UsartRxBuffer[SIM.UsartRxIndex] = SIM.UsartRxTemp;
    if(SIM.UsartRxIndex < (_SIM_BUFFER_SIZE-1))
      SIM.UsartRxIndex++;
//		if (AlarmBuffTaskHandle)
//			osThreadSuspend(AlarmBuffTaskHandle);
//		osThreadResume(SIMBuffTaskHandle);
		HAL_UART_Receive_IT(&_SIM_USART,&SIM.UsartRxTemp,1);
  }
 }
 
 if(huart->Instance==USART2)
 {
    Debbug_RxCallBack();
 }
}

//********************************************************************************************************************// 
uint8_t SIM_SendAtCommande(char *command, uint32_t timeout_ms, char *response, uint32_t response_size) {
    // Clear the response buffer
    memset(response, 0, response_size);

    // Send the command
    HAL_UART_Transmit(&_SIM_USART, (uint8_t*)command, strlen(command), HAL_MAX_DELAY);

    // Wait for the response
    uint32_t start_time = HAL_GetTick();
    uint32_t response_index = 0;
    while ((HAL_GetTick() - start_time) < timeout_ms) {
        // Check if data is available to read
        if (HAL_UART_Receive(&_SIM_USART, (uint8_t*)(response + response_index), 1, 100) == HAL_OK) {
            // Move to the next position in the response buffer
            response_index++;
            // Check if the response buffer is full
            if (response_index >= response_size) {
                break;
            }
            // Check if the response ends with "\r\nOK\r\n"
            if (strstr(response, "\r\nOK\r\n") != NULL) {
                // Check SIM status if OK
                if (SIM_Status_OK(response)) {
                    // Print "SIM is OK"
                    HAL_UART_Transmit(&huart2, (uint8_t *)"SIM is OK\n", strlen("SIM is OK\n"), 100);
                    return SUCCESS; // Command successful and SIM status OK
                } else {
                    // Print "SIM is not OK"
                    HAL_UART_Transmit(&huart2, (uint8_t *)"SIM is not OK\n", strlen("SIM is not OK\n"), 100);
                    return FAILURE; // Command successful but SIM status not OK
                }
            }
        }
    }

    // Timeout or response buffer is full
    return FAILURE; // Command failed
}

// Function to check if SIM status is OK based on response
uint8_t SIM_Status_OK(char *response) {
    // Check if response contains "OK"
    if (strstr(response, "OK") != NULL) {
        return SUCCESS; // SIM status OK
    } else {
        return FAILURE; // SIM status not OK
    }
}
uint8_t     SIM_SendAtCommand(char *AtCommand,int32_t  MaxWaiting_ms,char *Answer)
{
  while(SIM.Status.Busy == 1)
  {
    osDelay(1000);
  }
  SIM.Status.Busy = 1;
  SIM.AtCommand.FindAnswer = 0;
  SIM.AtCommand.ReceiveAnswerExeTime=0;
  SIM.AtCommand.SendCommandStartTime = HAL_GetTick();
  SIM.AtCommand.ReceiveAnswerMaxWaiting = MaxWaiting_ms;
  memset(SIM.AtCommand.ReceiveAnswer,0,sizeof(SIM.AtCommand.ReceiveAnswer));
  strncpy(SIM.AtCommand.ReceiveAnswer,Answer,sizeof(SIM.AtCommand.ReceiveAnswer)); 
  strncpy(SIM.AtCommand.SendCommand,AtCommand,sizeof(SIM.AtCommand.SendCommand));            
  SIM_SendString(SIM.AtCommand.SendCommand); 
  while( MaxWaiting_ms > 0)
  {
    osDelay(10);
    if(SIM.AtCommand.FindAnswer > 0)
      return SIM.AtCommand.FindAnswer;    
    MaxWaiting_ms-=10;
  }
	debugPrintln(SIM.AtCommand.ReceiveAnswer);
  memset(SIM.AtCommand.ReceiveAnswer,0,sizeof(SIM.AtCommand.ReceiveAnswer));
  SIM.Status.Busy=0;
  return SIM.AtCommand.FindAnswer;
}
//********************************************************************************************************************//
//********************************************************************************************************************//
//********************************************************************************************************************//
void  SIM_InitValue(void)
{

	SIM_SendAtCommand("AT+CFUN?\r\n",4000,"OK");
	SIM_SendAtCommand("AT+CPIN?\r\n",4000,"OK");
	SIM_SendAtCommand("AT+CSQ\r\n",4000,"OK");
	SIM_SendAtCommand("AT+COPS?\r\n",4000,"OK");
	SIM_SendAtCommand("AT+CREG?\r\n",4000,"OK");
  debugPrintln("SIM7600 Module had turned on");
  SIM.Status.Ready = 1;
  SIM.Status.State = 0;
}

//********************************************************************************************************************//
void   SIM_SaveParameters(void)
{
  SIM_SendAtCommand("AT&W\r\n",1000,"AT&W\r\r\nOK\r\n");
} 

//********************************************************************************************************************//
void  SIM_SetPower(bool TurnOn)
{ 
  if(TurnOn==true)
  {  
    if(SIM_SendAtCommand("AT\r\n",200,"AT\r\r\nOK\r\n") == 1)
    {
      osDelay(100);
      SIM.Status.Power=1;
			SIM_InitValue();
    }
    else
    {     
      SIM.Status.Ready = 0;
      HAL_GPIO_WritePin(SMS_RST_GPIO_Port,SMS_RST_Pin,GPIO_PIN_RESET);
      osDelay(200);
      HAL_GPIO_WritePin(SMS_RST_GPIO_Port,SMS_RST_Pin,GPIO_PIN_SET);
      osDelay(5000);  
      if(SIM_SendAtCommand("AT\r\n",200,"AT\r\r\nOK\r\n") == 1)
      {
        SIM.Status.Power=1;
				SIM_InitValue();   
      }
      else
        SIM.Status.Power=0;
    }
  }
  else
  {
      SIM.Status.Power=0;
			SIM.Status.Ready = 0;
      HAL_GPIO_WritePin(SMS_RST_GPIO_Port,SMS_RST_Pin,GPIO_PIN_RESET);
      osDelay(200);
      HAL_GPIO_WritePin(SMS_RST_GPIO_Port,SMS_RST_Pin,GPIO_PIN_SET);
      osDelay(5000);  
      if(SIM_SendAtCommand("AT\r\n",200,"AT\r\r\nOK\r\n") == 1)
      {
        SIM.Status.Power=1;
				SIM_InitValue();   
      }
      else
        SIM.Status.Power=0; 
    
  }  
}
//********************************************************************************************************************//
void SIM_SetFactoryDefault(void)
{
  SIM_SendAtCommand("AT&F0\r\n",1000,"AT&F0\r\r\nOK\r\n");
  #if (_SIM_DEBUG==1)
  printf("\r\nSIM_SetFactoryDefault() ---> OK\r\n");
  #endif
}

int SIM_IsNumberAdmin(uint64_t compared_number)
{
	for (int i = 0; i < sizeof(SIM.Alert.Admin_Numbers); i++)
	{
		if (compared_number == SIM.Alert.Admin_Numbers[i].Number)
		{
			return i;
		}
	}

	return -1; /* Return -1 error as number not found */
}

int SIM_IsNumberAdded(uint64_t compared_number)
{
	//	for (int i = 0; i < 10; i++)
  for (int i = 0; i < SIM.Status.num_set; i++)
  {
    if (compared_number == SIM.Alert.User_Numbers[i].Number)
    {
      return i;
    }
  }

  return -1; /* Return -1 error as number not found */
}

void getUserNumbers(void)
{
	char tempChar[15];
	
	strcat((char*)SIM.SMSBuffer, "Users :\n");
	
	for (int i = 0; i < 10; i++)
	{
		sprintf(tempChar,  "%d %llu\n", (i+1), SIM.Alert.User_Numbers[i].Number);
		strcat((char*)SIM.SMSBuffer, tempChar);
		debugPrint(tempChar);
	}
}

//********************************************************************************************************************//
uint32_t integer;
int SIM_CMDParse (int command, float temp_1, float temp_2, float hum_1, float hum_2, float vsen1, float vsen2, int isen, int tsen, float gas_1, float wind_1)
{
  memset(SIM.SMSBuffer,0,sizeof SIM.SMSBuffer);
  memset(SIM.AuxBuffer,0,sizeof SIM.AuxBuffer);
  int admin_idx;
  int user_idx;
  uint8_t alert_stat;
  
  switch(command)
	{
	  case TEMP_CMD:	
			sprintf((char*)SIM.SMSBuffer,  "%s T2 = %d.%dC\n%s H2 = %d.%d%%\n%s T1 = %d.%dC\n%s H1 = %d.%d%% RH\n", 
			         temp_hum[SIM.Status.lang_set][0], SHT2x_GetInteger(temp_1), SHT2x_GetDecimal(temp_1, 1),
					 temp_hum[SIM.Status.lang_set][1], SHT2x_GetInteger(hum_1), SHT2x_GetDecimal(hum_1, 1),
					 temp_hum[SIM.Status.lang_set][0], SHT2x_GetInteger(temp_2), SHT2x_GetDecimal(temp_2, 1),
					 temp_hum[SIM.Status.lang_set][1], SHT2x_GetInteger(hum_2), SHT2x_GetDecimal(hum_2, 1));
			break; 
			
		case T1_CMD:
			sprintf((char*)(SIM.SMSBuffer), "%s T1 = %d.%dC\n", temp_hum[SIM.Status.lang_set][0], SHT2x_GetInteger(temp_2), SHT2x_GetDecimal(temp_2, 1));
			break;
		case T2_CMD:
			sprintf((char*)(SIM.SMSBuffer), "%s T2 = %d.%dC\n", temp_hum[SIM.Status.lang_set][0], SHT2x_GetInteger(temp_1), SHT2x_GetDecimal(temp_1, 1)); 
			break;
		case H1_CMD:
			sprintf((char*)(SIM.SMSBuffer), "%s H1 = %d.%d%% RH\n", temp_hum[SIM.Status.lang_set][1], SHT2x_GetInteger(hum_1), SHT2x_GetDecimal(hum_2, 1));
			break;
		case H2_CMD:
			sprintf((char*)(SIM.SMSBuffer), "%s H2 = %d.%d%% RH\n", temp_hum[SIM.Status.lang_set][1], SHT2x_GetInteger(hum_2), SHT2x_GetDecimal(hum_2, 1));
			break;
						
	  case GAS_CMD:
			
			sprintf((char*)SIM.SMSBuffer,  "%s: %.2f%%\n", ans_N[SIM.Status.lang_set][command], gas_1);
			break;


		
	  case STATE_CMD:
			if(vsen1 < 12)
			{
				sprintf((char*)SIM.SMSBuffer,  "%s\n", ans_N[SIM.Status.lang_set][command]);
			}
			else
			{
				sprintf((char*)SIM.SMSBuffer,  "%s\n", ans_P[SIM.Status.lang_set][command]);
			}
			break;
            
	  case BATTERY_CMD:
			sprintf((char*)SIM.SMSBuffer,  "%s %.2fV\n", ans_P[SIM.Status.lang_set][BATTERY_CMD], vsen2);
			break;
		
      case ALARM_CMD:
	  		admin_idx = SIM.Status.Active_Admin_Idx;
			user_idx  = SIM.Status.Active_User_Idx;

			/* Check if current sender is an Admin or a Normal user */
			if(admin_idx >= 0) 
			{ 
				alert_stat = SIM.Alert.Admin_Numbers[admin_idx].Alert_Stat;
			}
			else{ 
			    alert_stat = SIM.Alert.User_Numbers[user_idx].Alert_Stat;
			}

			sprintf((char*)SIM.SMSBuffer,  "%s\n", ((alert_stat == 1) ? (ans_P[SIM.Status.lang_set][ALARM_CMD]) : (ans_N[SIM.Status.lang_set][ALARM_CMD]) ));
			debugPrintln((char*)SIM.SMSBuffer);
			break;

	  case SET_ALARM_CMD:
			admin_idx = SIM.Status.Active_Admin_Idx;
			user_idx  = SIM.Status.Active_User_Idx;

			if(admin_idx >= 0) 
			{ 
				SIM.Alert.Admin_Numbers[admin_idx].Temp_Diff = temp1_val;
				sprintf((char*)SIM.AuxBuffer, "%d", temp1_val);
				at24_write(ADMIN_TEMP_IDX_EEPROM(admin_idx), (uint8_t*)SIM.AuxBuffer, 2, 1000); 
			}
			else{ 
			  SIM.Alert.User_Numbers[user_idx].Temp_Diff = temp1_val;
				sprintf((char*)SIM.AuxBuffer, "%d", temp1_val);
				/* Set User's Alarm */
				at24_write(USER_TEMP_IDX_EEPROM(user_idx), (uint8_t*)SIM.AuxBuffer, 2, 1000);	
			}
//			  char temp[USR_NUM_BYTES];

//			at24_read(ADMIN_TEMP_IDX_EEPROM(admin_idx), (uint8_t*)temp, 2, 1000);  
//			temp[2] = 0;
//			char *end;
//			SIM.Alert.Admin_Numbers[admin_idx].Temp_Diff = (uint8_t)(strtol((char*)temp, &end, 16));
//			integer = strtol((char*)temp, &end, 16);
			sprintf((char*)SIM.SMSBuffer,  "%s %dC\n", ans_P[SIM.Status.lang_set][SET_ALARM_CMD], temp1_val);
			debugPrintln((char*)SIM.SMSBuffer);
			break;

	  case ALARM_ON_CMD:
	  		
			  admin_idx = SIM.Status.Active_Admin_Idx;
	  		user_idx  = SIM.Status.Active_User_Idx;
	  		alert_stat = 1;

	  		/* Check if current sender is an Admin or a Normal user */
	  		if(admin_idx >= 0) 
	  		{ 
		  		SIM.Alert.Admin_Numbers[admin_idx].Alert_Stat = alert_stat;
					sprintf((char*)SIM.AuxBuffer, "%d", alert_stat);
		  		at24_write(ADMIN_ALARM_IDX_EEPROM(admin_idx), (uint8_t*)SIM.AuxBuffer, 1, 1000);
	  		}
	  		else{ 
		  		SIM.Alert.User_Numbers[user_idx].Alert_Stat = alert_stat;
					sprintf((char*)SIM.AuxBuffer, "%d", alert_stat);
		  		/* Set User's Alarm */
		  		at24_write(USER_ALARM_IDX_EEPROM(user_idx), (uint8_t*)SIM.AuxBuffer, 1, 1000);	
	  		}
	  		if (alert_stat == 1)
	  		{
		  		SIM.Alert.AlertCounter_1 = HAL_GetTick();
		  		SIM.Alert.AlertCounter_2 = HAL_GetTick();
	  		}
	  		sprintf((char*)SIM.SMSBuffer,  "%s \n", ans_P[SIM.Status.lang_set][ALARM_ON_CMD]);
	  		debugPrintln((char*)SIM.SMSBuffer);
	  		break;

		case ALARM_OFF_CMD:
	  		
			admin_idx = SIM.Status.Active_Admin_Idx;
	  		user_idx  = SIM.Status.Active_User_Idx;
	  		alert_stat = 0;

	  		/* Check if current sender is an Admin or a Normal user */
	  		if(admin_idx >= 0) 
	  		{ 
		  		SIM.Alert.Admin_Numbers[admin_idx].Alert_Stat = alert_stat;
					sprintf((char*)SIM.AuxBuffer, "%d", alert_stat);
		  		at24_write(ADMIN_ALARM_IDX_EEPROM(admin_idx), (uint8_t*)SIM.AuxBuffer, 1, 1000);
	  		}
	  		else{ 
		  		SIM.Alert.User_Numbers[user_idx].Alert_Stat = alert_stat;
					sprintf((char*)SIM.AuxBuffer, "%d", alert_stat);
		  		/* Set User's Alarm */
		  		at24_write(USER_ALARM_IDX_EEPROM(user_idx), (uint8_t*)SIM.AuxBuffer, 1, 1000);	
	  		}

	  		sprintf((char*)SIM.SMSBuffer,  "%s \n", ans_P[SIM.Status.lang_set][ALARM_OFF_CMD]);
	  		debugPrintln((char*)SIM.SMSBuffer);
	  		break;
						
		case SET_LANG_CMD:
	 	{
			admin_idx = SIM.Status.Active_Admin_Idx;
			user_idx  = SIM.Status.Active_User_Idx;
			int lang_idx  = SIM.Status.lang_set;
			sprintf((char*)SIM.SMSBuffer,  "%s set\n", lang[lang_idx]);
			debugPrintln("setting Language");
            
			/* Check if current sender is an Admin or a Normal user */
			if(admin_idx >= 0) 
			{ 
				SIM.Alert.Admin_Numbers[admin_idx].Lang = lang_idx;
				sprintf((char*)SIM.AuxBuffer, "%d", lang_idx);
				at24_write(ADMIN_LANG_IDX_EEPROM(admin_idx), (uint8_t*)SIM.AuxBuffer, 1, 1000);
			}
			else{ 
			    SIM.Alert.User_Numbers[user_idx].Lang = lang_idx;
				sprintf((char*)SIM.AuxBuffer, "%d", lang_idx);
				/* Set User's Language */
				at24_write(USER_LANG_IDX_EEPROM(user_idx), (uint8_t*)SIM.AuxBuffer, 1, 1000);	
			}
			
			break;
		}
          
		case WIND_CMD:
			
			sprintf((char*)SIM.SMSBuffer,  "%s: %.2f Km/hr\n", ans_N[SIM.Status.lang_set][WIND_CMD], wind_1);	//NO WIND
			
			break;
		
		case USRNUM_CMD:
			getUserNumbers();
			break;
		
		case ADD_CMD:	/* Add Number */
			if(SIM_IsNumberAdmin(SIM.Status.MSG_Number) >= 0)	//WORKS ONLY IF COMMAND SENT FROM ADMIN PHONE NUMBERS
			{
				
				if(SIM.Status.num_set < 10)
				{
					if((SIM.Alert.Alt_Number >= 1000000) && (SIM.Alert.Alt_Number < 1000000000000000))
					{
						if(!(SIM_IsNumberAdded(SIM.Alert.Alt_Number) >= 0))
						{
							/* Getting Number from SIM's AUX receiving buffer */
							SIM.Alert.User_Numbers[SIM.Status.num_set].Number = SIM.Alert.Alt_Number;

							sprintf((char*)SIM.AuxBuffer, "%llu", SIM.Alert.Alt_Number);
							/* 3 = '2 + 1'. 2 is because max number of user are 10 and so we need two character to represent max num_set which is 10 
							   1 is because we need to place lang set value at before user number 
							   Structure <num_set lang_set1 user_number1 lang_set2 user_number2 .. .. ..>
							   12 is because we have 12 digits in phone number and so need 12 characters */
							at24_write(USER_NUM_IDX_EEPROM(SIM.Status.num_set), (uint8_t*)SIM.AuxBuffer, strlen((const char*)SIM.AuxBuffer), 1000);
							SIM.Status.num_set++;
							sprintf((char*)SIM.AuxBuffer,  "%d", SIM.Status.num_set);
 							at24_write(AT24_USR_NUM_CNT_ADDR, (uint8_t*)SIM.AuxBuffer, strlen((const char*)SIM.AuxBuffer), 1000);

							sprintf((char*)SIM.SMSBuffer,  "%llu %s \n", SIM.Alert.Alt_Number, ans_P[SIM.Status.lang_set][ADD_CMD]);
							debugPrintln("Number successfully added..");
						}
						else
						{
							sprintf((char*)SIM.SMSBuffer, ans_N[SIM.Status.num_set][ADD_CMD]);
							debugPrintln("NUmber is already added");
						}
					}
					else {
						sprintf((char*)SIM.SMSBuffer, "Number should be between 7 to 15 digits");
						debugPrintln("NUmber is already added");
					}
				}
				else
				{
					sprintf((char*)SIM.SMSBuffer,"Max Users 10");
					debugPrintln("Mem full");
				}
			}
			else {
				sprintf((char*)SIM.SMSBuffer,"you don't have access,contact admin\n");
				debugPrintln("you don't have access,contact admin");
			}
			break;
						
		case DELETE_CMD:	/* Delete Number */
			if(SIM_IsNumberAdmin(SIM.Status.MSG_Number) >= 0)	/* WORKS ONLY IF COMMAND SENT FROM ADMIN PHONE NUMBERS */
			{
				SIM.Alert.numIndex = SIM_IsNumberAdded(SIM.Alert.Alt_Number);
				
				if(SIM.Alert.numIndex >= 0)
				{
					/* Check if number found is not stored at the end of the array then delete number and swap it with last number stored */
					if (SIM.Alert.numIndex != SIM.Status.num_set)
					{
						for(uint8_t i = SIM.Alert.numIndex; i < SIM.Status.num_set; i++){
							SIM.Alert.User_Numbers[i].Number = SIM.Alert.User_Numbers[i+1].Number;
						}
						SIM.Alert.User_Numbers[SIM.Status.num_set].Number = 0;
						debugPrintln("Deleting number");
						sprintf((char*)SIM.SMSBuffer, ans_P[SIM.Status.lang_set][DELETE_CMD]);
                        
                        /* Also swap number in EEprom */
						sprintf((char*)SIM.AuxBuffer, "%llu", SIM.Alert.User_Numbers[SIM.Alert.numIndex].Number);
						at24_write(USER_NUM_IDX_EEPROM(SIM.Alert.numIndex), (uint8_t*)0, 1, 1000);
						sprintf((char*)SIM.AuxBuffer,  "%d", --(SIM.Status.num_set));
						at24_write(AT24_USR_NUM_CNT_ADDR, (uint8_t*)SIM.AuxBuffer, strlen((const char*)SIM.AuxBuffer), 1000);
					}
					else
					{
						SIM.Alert.User_Numbers[SIM.Status.num_set].Number = 0;
						at24_write(USER_NUM_IDX_EEPROM(SIM.Alert.numIndex), (uint8_t*)0, 1, 1000);
						sprintf((char*)SIM.AuxBuffer,  "%d", --(SIM.Status.num_set));
						at24_write(AT24_USR_NUM_CNT_ADDR, (uint8_t*)SIM.AuxBuffer, strlen((const char*)SIM.AuxBuffer), 1000);
						debugPrintln("Deleting number e");
						sprintf((char*)SIM.SMSBuffer,  ans_P[SIM.Status.lang_set][DELETE_CMD]);
					}

				}
				else
				{
					sprintf((char*)SIM.SMSBuffer, ans_N[SIM.Status.num_set][DELETE_CMD]);
					debugPrintln("Invalid Number");
				}
			}
			break;
		
		case GET_LANG_CMD:
		    sprintf((char*)SIM.SMSBuffer,  "%s %s\n", ans_P[SIM.Status.lang_set][GET_LANG_CMD], lang[SIM.Status.lang_set]); /* Get current set language */
			debugPrintln("Currently Set Language is sent back to user");
			break;
		
		case ALERT1_CMD:	//ALERT 1
		    
			if(vsen1 >= 12)
			{
				MachineState = STARTED;
				strcat((char*)SIM.AlarmsBuffer, alarm[SIM.Status.lang_set][0]);
				strcat((char*)SIM.AlarmsBuffer, "\n");
				osDelay(10);//osDelay(1500);
			}
			else {
				if (MachineState == STARTED)
				{
					MachineState = STOPPED;
					strcat((char*)SIM.AlarmsBuffer, alarm[SIM.Status.lang_set][1]);
					strcat((char*)SIM.AlarmsBuffer, "\n");
				}
				else
				{
					MachineState = NOT_STARTED;
				}

				osDelay(10);//osDelay(1500);
			}

			if(gas_1 <= 40)
			{
				strcat((char*)SIM.AlarmsBuffer, alarm[SIM.Status.lang_set][2]);
				strcat((char*)SIM.AlarmsBuffer, "\n");
				osDelay(10);//osDelay(1500);
			}
			if(vsen2 < 11)
			{
				strcat((char*)SIM.AlarmsBuffer, alarm[SIM.Status.lang_set][8]);
				strcat((char*)SIM.AlarmsBuffer, "\n");
				debugPrintln((char*)SIM.AlarmsBuffer);
				osDelay(10);//osDelay(1500);
			}
            
			//temp1_val = temp_1;
			temp2_val = temp_2;
			wind1_val = wind_1;

			SIM_SendAlertSMS((char*)SIM.AlarmsBuffer);
//			debugPrintln("Resuming alarm task from alert1");
//			if (AlarmBuffTaskHandle)
//			{
//				AlarmBuffQueue = true;
//				osThreadResume(AlarmBuffTaskHandle);
//			}
//			osThreadSuspend(SIMBuffTaskHandle);
			memset(SIM.AlarmsBuffer,0,sizeof SIM.AlarmsBuffer);
			SIM.Alert.AlertCounter_1 = HAL_GetTick();
			//AlarmBuffQueue = true;
			break;
						
		case ALERT2_CMD:	//ALERT 2
			if(temp_1 <= 2.0)
			{
				strcat((char*)SIM.AlarmsBuffer, alarm[SIM.Status.lang_set][6]);
				strcat((char*)SIM.AlarmsBuffer, "\n");
				osDelay(10);//osDelay(1500);
			}
			if(SIM.Status.BatteryPercent <= 11)
			{
				strcat((char*)SIM.AlarmsBuffer, alarm[SIM.Status.lang_set][3]);
				strcat((char*)SIM.AlarmsBuffer, "\n");
				osDelay(10);//osDelay(1500);
			}
			if(gas_1 <= 50.0)
			{
				char temp_gas[] = "";
				sprintf(temp_gas,  "%s: %.2f%%\n", ans_N[SIM.Status.lang_set][command], gas_1);
				strcat((char*)SIM.AlarmsBuffer, temp_gas);
				strcat((char*)SIM.AlarmsBuffer, "\n");
				osDelay(10);//osDelay(1500);
			}
			if(wind_1 > 16.0)
			{
				char temp_wind[] = "";
				sprintf(temp_wind,  "%s: %.2fKm/hr\n", ans_N[SIM.Status.lang_set][command], wind_1);
				strcat((char*)SIM.AlarmsBuffer, temp_wind);
				strcat((char*)SIM.AlarmsBuffer, "\n");
				osDelay(10);//osDelay(1500);
			}
		
			/*if((temp_1 <= 40.0) || (temp_2 < 40.0))
			{
				strcat((char*)SIM.AlarmsBuffer, alarm[SIM.Status.lang_set][8]);
				strcat((char*)SIM.AlarmsBuffer, "\n");
				osDelay(10);//osDelay(1500);
			}*/

			SIM_SendAlertSMS((char*)SIM.AlarmsBuffer);
			//osThreadResume(AlarmBuffTaskHandle);
			memset(SIM.AlarmsBuffer,0,sizeof SIM.AlarmsBuffer);
			SIM.Alert.AlertCounter_2 = HAL_GetTick();
			break;
        
		case STATE_GNRL_CMD:
				{
			admin_idx = SIM.Status.Active_Admin_Idx;
			user_idx  = SIM.Status.Active_User_Idx;

		    sprintf((char*)SIM.SMSBuffer,   "%s T2 = %d.%dC\n%s H2 = %d.%d%%\n%s T1 = %d.%dC\n %s H1 = %d.%d%% RH\n", 
			     temp_hum[0][0], SHT2x_GetInteger(temp_1), SHT2x_GetDecimal(temp_1, 1),
					 temp_hum[0][1], SHT2x_GetInteger(hum_1), SHT2x_GetDecimal(hum_1, 1),
					 temp_hum[0][0], SHT2x_GetInteger(temp_2), SHT2x_GetDecimal(temp_2, 1),
					 temp_hum[0][1], SHT2x_GetInteger(hum_2), SHT2x_GetDecimal(hum_2, 1));
			
			/* StateWM */		
			if(vsen1 < 12)
			{
				strcat((char*)(SIM.SMSBuffer), ans_N[0][STATE_CMD]);
			}
			else
			{
				strcat((char*)(SIM.SMSBuffer), ans_P[0][STATE_CMD]);
			}

			strcat((char*)(SIM.SMSBuffer), "\n");
	
			
			char temp_wind[] = "";
			sprintf(temp_wind,  "%s: %.2f Km/hr", ans_N[0][WIND_CMD], wind_1);
			strcat((char*)(SIM.SMSBuffer), temp_wind);
			
			
			strcat((char*)(SIM.SMSBuffer), "\n");

			 /* GAS */
			char temp_gas[] = "";
			sprintf(temp_gas,  "%s: %.2f %%", ans_N[0][GAS_CMD], gas_1);
			strcat((char*)(SIM.SMSBuffer), temp_gas);
			strcat((char*)(SIM.SMSBuffer), "\n");

			/* Check if current sender is an Admin or a Normal user */
			if(admin_idx >= 0) 
			{ 
				if (SIM.Alert.Admin_Numbers[admin_idx].Alert_Stat  == 1)
				{
					strcat((char*)(SIM.SMSBuffer), ans_P[0][ALARM_CMD]);
				}
				else
				{
					strcat((char*)(SIM.SMSBuffer), ans_N[0][ALARM_CMD]);
				}
				strcat((char*)(SIM.SMSBuffer), "\n");
				sprintf(temp_str, "%s %dC", ans_P[0][SET_ALARM_CMD],SIM.Alert.Admin_Numbers[admin_idx].Temp_Diff);
				strcat((char*)(SIM.SMSBuffer), temp_str);
			}
			else{ 
				if (SIM.Alert.User_Numbers[user_idx].Alert_Stat  == 1)
				{
					strcat((char*)(SIM.SMSBuffer), ans_P[0][ALARM_CMD]);
				}
				else	
				{
					strcat((char*)(SIM.SMSBuffer), ans_N[0][ALARM_CMD]);
				}
			}
			strcat((char*)(SIM.SMSBuffer), "\n");
			
			/* battery */
			strcat((char*)(SIM.SMSBuffer), ans_P[0][BATTERY_CMD]);
			sprintf(temp_str, "%0.2fV\n", vsen2);
			debugPrintln(temp_str);
			strcat((char*)(SIM.SMSBuffer), temp_str);
			strcat((char*)(SIM.SMSBuffer), "\n");
			
			debugPrintln("Sending General State");
			break;
		}

		case RELAY1_ON_CMD:
			HAL_GPIO_WritePin(RLY_1_GPIO_Port, RLY_1_Pin, GPIO_PIN_SET);
			sprintf((char*)SIM.SMSBuffer,  "%s\n", ans_P[SIM.Status.lang_set][RELAY1_ON_CMD]);
			debugPrintln("Relay1 is set");
			break;
		case RELAY1_OFF_CMD:
			HAL_GPIO_WritePin(RLY_2_GPIO_Port, RLY_1_Pin, GPIO_PIN_RESET);
			sprintf((char*)SIM.SMSBuffer,  "%s\n", ans_N[SIM.Status.lang_set][RELAY1_OFF_CMD]);
			debugPrintln("Relay1 is Reset");
			break;
		case RELAY2_ON_CMD:
			HAL_GPIO_WritePin(RLY_1_GPIO_Port, RLY_2_Pin, GPIO_PIN_SET);
			sprintf((char*)SIM.SMSBuffer,  "%s\n", ans_P[SIM.Status.lang_set][RELAY2_ON_CMD]);
			debugPrintln("Relay2 is set");
			break;
		case RELAY2_OFF_CMD:
			HAL_GPIO_WritePin(RLY_2_GPIO_Port, RLY_2_Pin, GPIO_PIN_RESET);
			sprintf((char*)SIM.SMSBuffer,  "%s\n", ans_N[SIM.Status.lang_set][RELAY2_OFF_CMD]);
			debugPrintln("Relay2 is Reset");
			break;
		case RELAY1_CMD:	
		    {
				int gpio_pin_state = HAL_GPIO_ReadPin(RLY_1_GPIO_Port, RLY_1_Pin);
					sprintf((char*)SIM.SMSBuffer,  "%s\n", (gpio_pin_state == 0) ? 
					(ans_N[SIM.Status.lang_set][RELAY1_CMD]) : (ans_P[SIM.Status.lang_set][RELAY1_CMD]));
				debugPrintln("Sending state of Relay1");
			}
				break;
		case RELAY2_CMD:
			{
				int gpio_pin_state = HAL_GPIO_ReadPin(RLY_2_GPIO_Port, RLY_2_Pin);
				sprintf((char*)SIM.SMSBuffer,  "%s\n", (gpio_pin_state == 0) ? 
				(ans_N[SIM.Status.lang_set][RELAY2_CMD]) : (ans_P[SIM.Status.lang_set][RELAY2_CMD]));
				debugPrintln("Sending state of Relay2");
			}
			break;
		case HELP_CMD:
		default:
			sprintf((char*)(SIM.SMSBuffer),  "%s\n ", cmd[SIM.Status.lang_set][0]);
			for (int i = 1; i <= HELP_CMD; i++)
			{
				strcat((char*)(SIM.SMSBuffer), cmd[SIM.Status.lang_set][i]);
				strcat((char*)(SIM.SMSBuffer), "\n");
			}
			debugPrintln("Invalid Command");
			break;
    }
	
	if(SIM.Status.MSG_Valid == 0)
	{
		SIM.Status.MSG_CMD = LAST_CMD_IDX;
	}
	
  	if (command != ALERT1_CMD)
	{
		SIM_SendSMS((char*)SIM.SMSBuffer,1);
	}
  	//  SIM.Status.MSG_Valid = 0;
  	memset(SIM.SMSBuffer,0,sizeof SIM.SMSBuffer);
  	memset(SIM.AuxBuffer,0,sizeof SIM.AuxBuffer);
  	osDelay(500);
  	return 1;
}

//********************************************************************************************************************//
uint64_t number[3] = {923216969610, 923164933, 923216969610456};
	uint8_t num_set = 0;
	char AuxBuffer[16] = {0};
void    SIM_SetLang(void)
{
//  uint8_t *set_lang;
//  at24_read(0, set_lang, 2, 1000);
//  SIM.Status.lang_set = atoi((char*)set_lang);  

	at24_eraseChip();

	for (int i = 0; i < 3; i++)
	{
//		sprintf((char*)(AuxBuffer), "%llu",number[i]);
//		at24_write(USER_NUM_IDX_EEPROM(num_set), (uint8_t*)AuxBuffer, strlen((const char*)AuxBuffer), 1000);
//		num_set++;
//		sprintf((char*)AuxBuffer,  "%d", num_set);
		
//		at24_write(AT24_USR_NUM_CNT_ADDR, (uint8_t*)AuxBuffer, strlen((const char*)AuxBuffer), 1000);
//		sprintf((char*)AuxBuffer, "%d",i);
//		at24_write(USER_LANG_IDX_EEPROM(i), (uint8_t*)AuxBuffer, 1, 1000);
		
		sprintf((char*)AuxBuffer, "%d",i);
		at24_write(ADMIN_LANG_IDX_EEPROM(i), (uint8_t*)AuxBuffer, 1, 1000);
		sprintf((char*)AuxBuffer, "%d",i);
		at24_write(ADMIN_ALARM_IDX_EEPROM(i), (uint8_t*)AuxBuffer, 1, 1000);
		sprintf((char*)AuxBuffer, "%d",i);
		at24_write(ADMIN_TEMP_IDX_EEPROM(i), (uint8_t*)AuxBuffer, 1, 1000);
	}
}

//********************************************************************************************************************//
void    SIM_SetNum(void)
{
  char temp[USR_NUM_BYTES];
  char *end_ptr;
	SIM.Status.num_set = 0;
  at24_read(AT24_USR_NUM_CNT_ADDR, (uint8_t*)temp, 2, 1000);
  SIM.Status.num_set = atoi((char*)temp);
  for(int i = 0; i < SIM.Status.num_set; i++)
  {
	/* Get User Number */
    at24_read(USER_NUM_IDX_EEPROM(i), (uint8_t*)temp, 15, 1000);
	SIM.Alert.User_Numbers[i].Number = strtoull((char*)temp, &end_ptr, 10);
    
	/* Get User's Language */
	at24_read(USER_LANG_IDX_EEPROM(i), (uint8_t*)temp, 15, 1000);
	SIM.Alert.User_Numbers[i].Lang = atoi((char*)temp);
    
	/* Get User's Alarm value */
	at24_read(USER_ALARM_IDX_EEPROM(i), (uint8_t*)temp, 15, 1000);
	SIM.Alert.User_Numbers[i].Alert_Stat = atoi((char*)temp);

	/* Get User's Alarm value */
	at24_read(USER_TEMP_IDX_EEPROM(i), (uint8_t*)temp, 15, 1000);
	SIM.Alert.User_Numbers[i].Temp_Diff = atoi((char*)temp);
  }

  /* Get Admin's language settings */
  for (int i = 0; i < 8; i++)
  {
	/* Get admin language */
	at24_read(ADMIN_LANG_IDX_EEPROM(i), (uint8_t*)temp, 2, 1000);
	SIM.Alert.Admin_Numbers[i].Lang = atoi((char*)temp);

    /* Get Admin Alarm value */
	at24_read(ADMIN_ALARM_IDX_EEPROM(i), (uint8_t*)temp, 2, 1000);
	SIM.Alert.Admin_Numbers[i].Alert_Stat = atoi((char*)temp);

	/* Get User's Alarm value */
	at24_read(ADMIN_TEMP_IDX_EEPROM(i), (uint8_t*)temp, 2, 1000);
	SIM.Alert.Admin_Numbers[i].Temp_Diff = atoi((char*)temp);
  }
}
//********************************************************************************************************************//
void  SIM_SendAlertSMS(char *str)
{
	/* Send Alert SMS to all the Admin Numbers */
	for (int i = 0; i < sizeof(SIM.Alert.Admin_Numbers); i++)
	{
		SIM.Alert.Alt_Number = SIM.Alert.Admin_Numbers[i].Number;
		/* Check for null numbers */
		if((SIM.Alert.Alt_Number) && (SIM.Alert.Admin_Numbers[i].Alert_Stat == 1))
		{          
		  /* Check if use needs alert for temprature or not */
		  if(temp1_val <= (SIM.Alert.Admin_Numbers[i].Temp_Diff + 1))
		  {
			  if ((temp2_val - temp1_val) < 1)
			  {
		  		strcat((char*)SIM.AlarmsBuffer, alarm[SIM.Status.lang_set][4]);
		  		strcat((char*)SIM.AlarmsBuffer, "\n");
		  		osDelay(10);//osDelay(1500);
			  }

			  if ((wind1_val < 15) && (WindyCond == true))
			  {
				WindyCond = false;
				strcat((char*)SIM.AlarmsBuffer, alarm[SIM.Status.lang_set][6]);
		  		strcat((char*)SIM.AlarmsBuffer, "\n");
		  		osDelay(10);//osDelay(1500);
			  }
			  else if (wind1_val>= 15)
			  {
				WindyCond = true;
				strcat((char*)SIM.AlarmsBuffer, alarm[SIM.Status.lang_set][5]);
		  		strcat((char*)SIM.AlarmsBuffer, "\n");
		  		osDelay(10);//osDelay(1500);
			  }
		  }

		  if (temp1_val < SIM.Alert.Admin_Numbers[i].Temp_Diff)
		  {
			  if(MachineState == NOT_STARTED)
			  {
				strcat((char*)SIM.AlarmsBuffer, alarm[SIM.Status.lang_set][2]);
		  		strcat((char*)SIM.AlarmsBuffer, "\n");
				osDelay(10);//osDelay(1500);
			  }
			  else if (MachineState == STOPPED) 
			  {
				strcat((char*)SIM.AlarmsBuffer, alarm[SIM.Status.lang_set][3]);
		  		strcat((char*)SIM.AlarmsBuffer, "\n");
		  		osDelay(10);//osDelay(1500);
			  }
		  }

		  SIM_SendSMS(str, 0);    /*trim down string in case of temp_diff condition not met */
		}
	}

	/* Send Alert SMS to all the User Numbers */
	for (int i = 0; i < SIM.Status.num_set; i++)
	{
		/* Check for null numbers */
		if((SIM.Alert.Alt_Number) && (SIM.Alert.User_Numbers[i].Alert_Stat == 1))
		{
		  SIM.Alert.Alt_Number = SIM.Alert.User_Numbers[i].Number;
		  SIM_SendSMS(str, 0);
		}
	}
}



//**************************only******************************************************************************************//
void    SIM_SendSMS(char *str, int i)
{
	char aux[5];
	SIM_SendAtCommand("AT+CMGF=1\r\n",200,"OK");
	SIM_SendAtCommand("AT+CSCS=\"GSM\"\r\n",200,"OK");
	if(i == 0)
	{
		snprintf((char*)SIM.AuxBuffer, sizeof(SIM.AuxBuffer),"AT+CMGS=\"+%llu\"\r",SIM.Alert.Alt_Number);
		if(SIM_SendAtCommand((char*)SIM.AuxBuffer,5000,"> "))
		{
			osDelay(500);//1500
			
			SIM_SendString(str);
			
		}
	}
	else
	{
		snprintf((char*)SIM.AuxBuffer, sizeof(SIM.AuxBuffer),"AT+CMGS=\"+%llu\"\r",SIM.Status.MSG_Number);
		if(SIM_SendAtCommand((char*)SIM.AuxBuffer,5000,"> "))
		{
			osDelay(500);//1500
			SIM_SendString(str);
		}
	}
	osDelay(100);
	snprintf(aux, sizeof(aux),"%c%c",0X1A,0X1A);
	osDelay(3000);
	if(SIM_SendAtCommand(aux,5000,"\r\n+CMGS"))
	{
		debugPrintln("MESS SENT");
		
	}
}
//********************************************************************************************************************//
void	SIM_Init(osPriority Priority)
{

  SIM.Status.Ready = 0;
  memset(&SIM,0,sizeof(SIM));
  memset(SIM.UsartRxBuffer,0,_SIM_BUFFER_SIZE);
  HAL_UART_Receive_IT(&_SIM_USART,&SIM.UsartRxTemp,1);

  /* Set Current Admin and User Index to invalid as system is initializing */
  SIM.Status.Active_Admin_Idx = -1;
  SIM.Status.Active_User_Idx = -1;
  SIM.Status.lang_set = 0;
  

  osThreadDef(SIMBuffTask, StartSIMBuffTask, Priority, 0, 80);
  SIMBuffTaskHandle = osThreadCreate(osThread(SIMBuffTask), NULL);
	
	
    if(SIM_SendAtCommand("AT\r\n",2000,"OK") == 1)
		{
			debugPrintln("SIM RESPONSE OK");

		}else{
				debugPrintln("SIM RESPONSE NOT OK");
		}
    osDelay(200);
  
  SIM_SetPower(true);
}

//##################################################
//---       Buffer Process
//##################################################
void  SIM_BufferProcess(void)
{
	char      *strStart,*str1;
	//char *strStart, *str1, *field1_value;
  int32_t   tmp_int32_t;
  //char      tmp_str[16];
	
  strStart = (char*)&SIM.UsartRxBuffer[0];  
  //##################################################
  //+++       Buffer Process
  //################################################## OK
	//debugPrintln("-------------------------------------SIM_BufferProcessold-----------------------------------------");
	// here we will process the LED status
	// try to find the END of the json string
	// Find the start of the "field1" value
   // Find the start of the "feeds" array
    const char *feeds_pos = strstr(strStart, "\"feeds\":");
    if (feeds_pos != NULL) {
        // Move to the start of the array
        feeds_pos += strlen("\"feeds\":");

        // Find the start of the last entry
        const char *last_entry_pos = strstr(feeds_pos, "{");
        if (last_entry_pos != NULL) {
            // Initialize a variable to store the last field1 value
            int last_field1_value = -1; // Initialize with an invalid value

            // Loop through the entries in reverse order
            while (true) {
                // Find the start of the next entry
                const char *next_entry_pos = strstr(last_entry_pos + 1, "{");
                if (next_entry_pos == NULL) {
                    // Last entry reached
                    break;
                }

                // Find the end of the entry
                const char *end_pos = strstr(next_entry_pos, "}");
                if (end_pos == NULL) {
                    // Invalid entry format
                    break;
                }

                // Extract the value of "field1" from the entry
                const char *field1_pos = strstr(next_entry_pos, "\"field1\":\"");
                if (field1_pos != NULL && field1_pos < end_pos) {
                    field1_pos += strlen("\"field1\":\"");
                    const char *end_field1_pos = strchr(field1_pos, '"');
                    if (end_field1_pos != NULL && end_field1_pos < end_pos) {
                        int value_length = end_field1_pos - field1_pos;
                        char field1_value[value_length + 1];
                        strncpy(field1_value, field1_pos, value_length);
                        field1_value[value_length] = '\0';

                        // Convert field1_value to an integer
                        int value = atoi(field1_value);

                        // Update the last_field1_value with the value of "field1" from this entry
                        last_field1_value = value;
                    }
                }

                // Move to the next entry
                last_entry_pos = next_entry_pos;
            }

            // Process the last_field1_value
            if (last_field1_value != -1) {
                // Update LED status based on the last_field1_value
                if (last_field1_value == 0) {
                    // Turn on LED
                    debugPrintln("+++++++++++++++++++++++++++++++++++++++RELAY OFF+++++++++++++++++++++++++++++++++++++++++++++++++");
										ledStatus = 0;
										HAL_GPIO_WritePin(GPIOC, RLY_1_Pin, GPIO_PIN_RESET);
										HAL_GPIO_WritePin(GPIOC, RLY_2_Pin, GPIO_PIN_RESET);
										HAL_GPIO_WritePin(GPIOC, RLY_3_Pin, GPIO_PIN_RESET);

                } else  if (last_field1_value == 1) {
                    // Turn off LED
										debugPrintln("+++++++++++++++++++++++++++++++++++++++RELAY ON+++++++++++++++++++++++++++++++++++++++++++++++++");
										ledStatus = 1;
										HAL_GPIO_WritePin(GPIOC, RLY_1_Pin, GPIO_PIN_SET);
										HAL_GPIO_WritePin(GPIOC, RLY_2_Pin, GPIO_PIN_SET);
										HAL_GPIO_WritePin(GPIOC, RLY_3_Pin, GPIO_PIN_SET);

                }
            } else {
                debugPrintln("No valid entry found.");
            }
        }
    }
			str1 = strstr(strStart, "\r\n+HTTPACTION:");
	if (str1 != NULL)
	{
			// Move pointer to the colon ':' character
			str1 = strchr(str1, ':');
			str1 += 4;

			// Tokenize the string to extract the HTTP status code
			char *token = strtok(str1, ",");
			if (token != NULL)
			{
					tmp_int32_t = atoi(token);
					debugPrintln(token);
					if (tmp_int32_t == 200)
					{
							SIM_SendString("AT+HTTPREAD=0,500\r\n");
							//rpc_status = true;
					}
					else if(tmp_int32_t != 200 || tmp_int32_t == NULL)
					{
							//rpc_status = false;
					}
			}
	}
  //Thingsboard Section
	str1 = strstr(strStart, "\r\nRING");
if (str1 != NULL)
{
	
	//SIM_SendAtCommand("ATA\r\n",1000,"OK");
	SIM_SendAtCommand("ATH\r\n",1000,"OK");
	if(relayState==false)
	{
	HAL_GPIO_WritePin(GPIOC, RLY_1_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOC, RLY_2_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOC, RLY_3_Pin, GPIO_PIN_SET);
		relayState=true;
		SIM_SendAtCommand("AT+CMGF=1\r\n",1000,"OK");
		sprintf(ATcommandd,"AT+CMGS=\"%s\"\r\n",mobileNumber);
		SIM_SendString(ATcommandd);
		sprintf(ATcommandd,"MACHINE ARMED%c",0x1a);
		SIM_SendString(ATcommandd);
	}else if(relayState==true){
		HAL_GPIO_WritePin(GPIOC, RLY_1_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOC, RLY_2_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOC, RLY_3_Pin, GPIO_PIN_RESET);
		relayState=false;
		SIM_SendAtCommand("AT+CMGF=1\r\n",1000,"OK");
		sprintf(ATcommandd,"AT+CMGS=\"%s\"\r\n",mobileNumber);
		SIM_SendString(ATcommandd);
		sprintf(ATcommandd,"MACHINE DISARMED%c",0x1a);
		SIM_SendString(ATcommandd);
	}
}
str1 = strstr(strStart, "\"shared\"");
if (str1 != NULL)
{
	debugPrintln(str1);
    str1 = strstr(str1, "\"setValue\":");
    if (str1 != NULL)
    {
			debugPrintln(str1);
        str1 = strchr(str1, ':');
        if (str1 != NULL)
        {
						debugPrintln(str1);
            str1 += 1; // Move to the value after the colon
            // Skip whitespace and check for '{'
            //while (*str1 != '{' && *str1 != '\0')
            //{
              // str1++;
            //	}
           // if (*str1 == '{') // Found '{', move to next character
           // {
                //str1 += 1;
                // Read the boolean value at specified position
						debugPrintln(str1);
                if (*str1 == 't' && rpc_status == false) // Assuming 't' for true, 'f' for false
                {
                    debugPrintln("+++++++++++++++++++++++++++++++++++++++RELAY ON+++++++++++++++++++++++++++++++++++++++++++++++++");
                    ledStatus = 1;
                    HAL_GPIO_WritePin(GPIOC, RLY_1_Pin, GPIO_PIN_SET);
                    HAL_GPIO_WritePin(GPIOC, RLY_2_Pin, GPIO_PIN_SET);
                    HAL_GPIO_WritePin(GPIOC, RLY_3_Pin, GPIO_PIN_SET);
                    strcpy(ARMED_STATUS, "ARMED");
                    rpc_status = false;
                    fonction_control = false;
                    prev_fonction_control = true;
										relayState = true; // Update the relay state
                    //Previous_rpc_status=rpc_status;
                }
                else if (*str1 == 'f' && rpc_status == true)
                {
                    debugPrintln("+++++++++++++++++++++++++++++++++++++++RELAY OFF+++++++++++++++++++++++++++++++++++++++++++++++++");
                    ledStatus = 0;
                    HAL_GPIO_WritePin(GPIOC, RLY_1_Pin, GPIO_PIN_RESET);
                    HAL_GPIO_WritePin(GPIOC, RLY_2_Pin, GPIO_PIN_RESET);
                    HAL_GPIO_WritePin(GPIOC, RLY_3_Pin, GPIO_PIN_RESET);
                    strcpy(ARMED_STATUS, "DISARMED");
                    rpc_status = false;
                    fonction_control = false;
                    prev_fonction_control = true;
										relayState = false; // Update the relay state
                    //Previous_rpc_status=rpc_status;
                }
            //}
        }
    }
}
//get gps data--------------------------------------------------------------------
str1 = strstr(strStart, "+CGPSINFO:");
    if (str1 != NULL) {

        // Move to the latitude token
        token = strtok_r(str1, ",", &saveptr);
        if (token != NULL) {
            // Copy latitude
            strncpy(lat, token + 11, 9); // Skip "+CGPSINFO:", assuming latitude is 9 characters long
            lat[9] = '\0'; // Null-terminate the string
            // Move to the latitude direction token
            token = strtok_r(NULL, ",", &saveptr);
            if (token != NULL) {
                // Move to the longitude token
                token = strtok_r(NULL, ",", &saveptr);
                if (token != NULL) {
                    // Copy longitude
                    strncpy(lon, token + 1, 10); // Skip leading comma, assuming longitude is 10 characters long
                    lon[10] = '\0'; // Null-terminate the string

                    // Print latitude and longitude
                    debugPrint("Latitude: ");
                    debugPrintln(lat);
                    debugPrint("Longitude: ");
                    debugPrintln(lon);

                    return;
                }
            }
        }
        // Handle error: Unable to extract latitude and longitude
       // debugPrintln("Error: Unable to extract latitude and longitude");
    } else {
        // Handle error: CGPSINFO data not found
        //debugPrintln("Error: CGPSINFO data not found");
    }
	//End Thingsboard Section
  str1 = strstr(strStart,"\r\n+CREG:");
  if(str1!=NULL)
  {
    str1 = strchr(str1,',');
    str1++;
    if((atoi(str1)==1) ||(atoi(str1)==5))
		{
      SIM.Status.RegisterdToNetwork=1;
    }
    else
		{
      SIM.Status.RegisterdToNetwork=0;
      debugPrintln("Net ERR");
    }
  }
	//End Thingsboard Section

//****************************************************//BEGIN Vérifie l'état de la fonctionnalité du module.****************************************************************//
   // Find the first occurrence of "\r\n+CFUN:"
    str1 = strstr(strStart, "\r\n+CFUN:");
    if (str1 != NULL)
    {
        // Move pointer to the colon ':' character
        str1 = strchr(str1, ':');
        str1++;
        functionality = atoi(str1);

        // Determine the functionality status
        switch (functionality) {
            case 0:
                strcpy(functionalityStatus, "Minimum functionality");
                break;
            case 1:
                strcpy(functionalityStatus, "Full functionality, online mode");
                break;
            case 4:
                strcpy(functionalityStatus, "Disable phone both transmit and receive RF circuits");
                break;
            case 5:
                strcpy(functionalityStatus, "Factory Test Mode");
                break;
            case 6:
                strcpy(functionalityStatus, "Reset");
                break;
            case 7:
                strcpy(functionalityStatus, "Offline Mode");
                break;
            default:
                strcpy(functionalityStatus, "Unknown functionality");
        }

        // Transmit the functionality status over UART
        HAL_UART_Transmit(&huart2, (uint8_t *)functionalityStatus, strlen(functionalityStatus), 100);
    }
//****************************************************//END Vérifie l'état de la fonctionnalité du module.****************************************************************//

//****************************************************//BEGIN information of the operator****************************************************************//
str1 = strstr(strStart, "\r\n+COPS:");
if (str1 != NULL)
{
// Find the first occurrence of double quotes
    char* start = strchr(strStart, '\"');
    if (start != NULL) {
        // Find the next occurrence of double quotes
        char* end = strchr(start + 1, '\"');
        if (end != NULL) {
            // Copy the operator name between the double quotes
            strncpy(operatorName, start + 1, end - start - 1);
            operatorName[end - start - 1] = '\0'; // Null-terminate the string
        }
    }

    // Transmit the extracted operator name over UART
		HAL_UART_Transmit(&huart2, (uint8_t *) "operatorName is  :", strlen("operatorName is  :"), 100);
    HAL_UART_Transmit(&huart2, (uint8_t *)operatorName, strlen(operatorName), 100);
		HAL_UART_Transmit(&huart2, (uint8_t *) "\n", strlen("\n"), 100);
}
//****************************************************//BEGIN information of the operator****************************************************************//

//****************************************************//BEGIN Signal strength****************************************************************//
str1 = strstr(strStart, "\r\n+CSQ:");
if (str1 != NULL)
{
    // Move pointer to the colon ':' character
    str1 = strchr(str1, ':');
    if (str1 != NULL)
    {
        // Move pointer to the signal strength value
        str1++;

        // Extract the signal strength value
        sscanf(str1, "%d,%d", &rssiValue, &berValue);

        // Update SIM status with the extracted signal strength
        SIM.Status.Signal = rssiValue;

        // Transmit a debug message
        HAL_UART_Transmit(&huart2, (uint8_t *) "-->SIM.Status.Signal---------------\n", strlen("-->SIM.Status.Signal---------------\n"), 100);

        // Convert the signal strength to a string
        sprintf(signalString, "%d", SIM.Status.Signal);

        // Transmit the signal strength over UART
        //HAL_UART_Transmit(&huart2, (uint8_t *) signalString, strlen(signalString), 100);

        // Decode RSSI value to dBm
        if (rssiValue >= 2 && rssiValue <= 9)
        {
            sprintf(rssiString, "-%d dBm (Marginal)", abs((rssiValue * 2) - 113)); // Use absolute value to avoid double hyphen
        }
        else if (rssiValue >= 10 && rssiValue <= 14)
        {
            sprintf(rssiString, "-%d dBm (OK)", abs((rssiValue * 2) - 113)); // Use absolute value to avoid double hyphen
        }
        else if (rssiValue >= 15 && rssiValue <= 19)
        {
            sprintf(rssiString, "-%d dBm (Good)", abs((rssiValue * 2) - 113)); // Use absolute value to avoid double hyphen
        }
        else if (rssiValue >= 20 && rssiValue <= 30)
        {
            sprintf(rssiString, "-%d dBm (Excellent)", abs((rssiValue * 2) - 113)); // Use absolute value to avoid double hyphen
        }
        else
        {
            strcpy(rssiString, "Unknown RSSI value");
        }

        // Add newline characters
        strcat(rssiString, "\r\n");

        // Transmit the RSSI over UART
        HAL_UART_Transmit(&huart2, (uint8_t *) rssiString, strlen(rssiString), 100);
    }
}
//****************************************************//END Signal strength****************************************************************//
  str1 = strstr(strStart,"\r\n+CBC:");
  if(str1!=NULL)
  {
    str1 = strchr(str1,':');
    str1++;
    tmp_int32_t = atoi(str1);
    if(tmp_int32_t==0)
    {
      SIM.Status.BatteryCharging=0;
      SIM.Status.BatteryFull=0;
    }
    if(tmp_int32_t==1)
    {
      SIM.Status.BatteryCharging=1;
      SIM.Status.BatteryFull=0;
    }
    if(tmp_int32_t==2)
    {
      SIM.Status.BatteryCharging=0;
      SIM.Status.BatteryFull=1;
    }
    str1 = strchr(str1,',');
    str1++;
    SIM.Status.BatteryPercent = atoi(str1);
    str1 = strchr(str1,',');
    str1++;
    SIM.Status.BatteryVoltage = atof(str1);      
  }

//********************************************************************************************************************//
	str1 = strstr(strStart,"\r\n+CMT:");
  if(str1!=NULL)
  {
		str1 = strchr(str1,'\"');
					str1++;
					str1++;
					SIM.Status.MSG_Number = atoll(str1); // Read the cellphone number of who requested the information
		
		//CHECK PHONE NUMBER FIRST
		int admin_idx = SIM_IsNumberAdmin(SIM.Status.MSG_Number);
		int user_idx  = SIM_IsNumberAdded(SIM.Status.MSG_Number);
		if( (admin_idx >= 0) || (user_idx >= 0))
		{
			/* Set system language according to the user defined language */
			SIM.Status.lang_set = (admin_idx >= 0) ? (SIM.Alert.Admin_Numbers[admin_idx].Lang) : (SIM.Alert.User_Numbers[user_idx].Lang);

			/* Reset Index for next command */
			SIM.Status.Active_Admin_Idx = admin_idx;
			SIM.Status.Active_User_Idx  = user_idx;

			char **eng_cmd;
			/* Check if language is other than english the also accept english language as english is universal labguage in our case */
			if (SIM.Status.lang_set != 0)
			{
				/* get english command queue */
				eng_cmd = cmd[0];  	
			}

			/* Run loop for all commands currenlty registered within the system, as size of cmd_en depicts the size of command array */
			for(int w = 0; w < (sizeof(cmd_en) / sizeof(char*)) ; w++)
			{
				str1 = mystristr(strStart, cmd[SIM.Status.lang_set][w]);
				char *str2;
				/* check if eng_cmd array ptr is set */
				if (eng_cmd)
				{
					/* check incoming command if it's in english */
					str2 = mystristr(strStart, eng_cmd[w]);	
				}

				if((str1 != NULL) || (str2 != NULL))
				{
					SIM.Status.MSG_CMD = w;
					debugPrintln(cmd[SIM.Status.lang_set][w]);
					SIM.Status.MSG_Valid = 1;
					break;
				}
				SIM.Status.MSG_CMD = ERROR_CMD;
				SIM.Status.MSG_Valid = 1;
			}
						
			if(SIM.Status.MSG_CMD == SET_LANG_CMD)
			{
				for(int w = 0; w < 4; w++)
				{
					str1 = mystristr(strStart, lang[w]);
					if(str1!=NULL)
					{
						SIM.Status.lang_set = w;
						break;
					}
				}
			}
			/**/
			else if((SIM.Status.MSG_CMD == ADD_CMD) || (SIM.Status.MSG_CMD == DELETE_CMD))	/* ADD NUMBER || Delete Number Z */
			{
				debugPrintln(str1);
				char *pNumber;
				pNumber = strchr(str1,'+');
				str1++;
				
				SIM.Alert.Alt_Number = atoll(pNumber++);
				debugPrintln(str1);
				debugPrintln(pNumber);

			}
			/**/
			else if (SIM.Status.MSG_CMD == SET_ALARM_CMD )
			{
				str1 = mystristr(str1, "Alarm");
				str1 += 6;
				char *str_cop = mystristr(str1, "C"); /* Set Alarm x°C*/
				if (str_cop)
				{
					*str_cop = 0;
					str_cop--;
					*str_cop = 0;
				}
				temp1_val = atoi(str1);
			}
		}
		else
		{
			
			debugPrintln("Stop texting me please..!");
		}
  }
  
//********************************************************************************************************************//
  if(SIM.AtCommand.ReceiveAnswer[0]==0)
  {
    SIM.AtCommand.FindAnswer=0;
  }
  str1 = strstr(strStart,SIM.AtCommand.ReceiveAnswer);
  if(str1!=NULL)
  {
    SIM.AtCommand.FindAnswer = 1;
    SIM.AtCommand.ReceiveAnswerExeTime = HAL_GetTick()-SIM.AtCommand.SendCommandStartTime;
  }    
  SIM.UsartRxIndex=0;
  memset(SIM.UsartRxBuffer,0,_SIM_BUFFER_SIZE);    
  SIM.Status.Busy=0;

	
}

//********************************************************************************************************************//
void StartSIMBuffTask(void const * argument)
{ 
  //This task will check the buffer every 50ms and read serial data from SIM
  debugPrintln("Buffer start");
  while(1)
  {
    if( ((SIM.UsartRxIndex>4) && (HAL_GetTick()-SIM.UsartRxLastTime > 50)))
    {
      SIM.BufferStartTime = HAL_GetTick();      
      SIM_BufferProcess();      
      SIM.BufferExeTime = HAL_GetTick()-SIM.BufferStartTime;
    }
    osDelay(10);
  }    
}

void updateLed(void) {
    // Terminate any existing HTTP session
    //SIM_SendAtCommand("AT+HTTPTERM\r\n", 500, "OK");

    // Initialize HTTP session
		SIM_SendAtCommand("AT+HTTPTERM\r\n",1000,"OK");
    //SIM_SendAtCommand("AT+HTTPINIT\r\n", 2000, "OK");
			if(SIM_SendAtCommand("AT+HTTPINIT\r\n", 1000, "OK") == 1)
	{
		debugPrintln("HTTPINIT RESPONSE OK");

	}else{
			debugPrintln("HTTPINIT RESPONSE NOT OK");
			debugPrintln("Sorry we need to restard module");
			SIM_SendString("AT+CPOF\r\n");
			HAL_Delay(60000);
	}
    // Set the ThingSpeak URL
    SIM_SendString("AT+HTTPPARA=\"URL\",\"https://api.thingspeak.com/channels/2268171/fields/1.json?api_key=CGDRYLEUYF3P31MP&results=2\"\r\n");
    HAL_Delay(1000);
    // Perform HTTP GET request
    SIM_SendAtCommand("AT+HTTPACTION=0\r\n", 1000, "20");
    SIM_SendAtCommand("AT+HTTPREAD=0,500\r\n", 1000, "OK");
		SIM_SendAtCommand("AT+HTTPTERM\r\n",1000,"OK");
    HAL_Delay(5000);
}
void RPC_control(void) {
    // Terminate any existing HTTP session
    //SIM_SendString("AT+HTTPTERM\r\n");

   

    // Set the ThingSpeak URL
    SIM_SendString("AT+HTTPPARA=\"URL\",\"https://demo.thingsboard.io/api/v1/xMJwZXAY71oDofdBzdTC/rpc\"\r\n");
		SIM_SendAtCommand("AT+HTTPACTION=0\r\n",2000,"OK");
    // Perform HTTP GET request

		// Attendre jusqu'à ce que la réponse contienne la méthode "setValue"
    //while (rpc_status == Previous_rpc_status){
			//SIM_SendString("AT+HTTPPARA=\"URL\",\"https://demo.thingsboard.io/api/v1/xMJwZXAY71oDofdBzdTC/rpc\"\r\n");
			//SIM_SendAtCommand("AT+HTTPACTION=0\r\n",2000,"OK");
			//SIM_SendString("AT+HTTPREAD=0,500\r\n");
			//debugPrintln("ATT response with method setValue");
			//osDelay(1000);
		//}
		
		//rpc_status = true;
    // Une réponse contenant la méthode "setValue" a été trouvée
    //debugPrintln("Received response with method setValue");

    // Lire le reste de la réponse
    

    // Terminer la session HTTP
    //SIM_SendString("AT+HTTPTERM\r\n");
    //HAL_Delay(5000);
}






