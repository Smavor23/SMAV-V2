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
//-----------------------------�VARIABLES FOR SENSORS from frertos.c�--------------------        					//variable to store adc raw values
extern float Vsense_1;                        	//variable to store Voltage sensor 1
extern float Vsense_2;                       	//variable to store Voltage sensor 2
extern float VBattery;
extern float Isense;                       		//variable to store Current sensor 1
extern float Gas_lvl;                    			//variable to store Gas level
extern float Wind_stat;                 				//variable to store wind status
extern float wind_Kmh;
extern float temperature1;
extern float humidity1;
extern float temperature2;
extern float humidity2;
extern bool sendDataTaskRunning;
extern char 	Module_Status[50];
//--------------------------------------------------------------------------
bool ACCESS;
char SMAV_NETWORK_STATUS[20];  // Enter the Mobile Number you want to send to
bool SMS_mode=false;
// Declaration of SIM_Status_OK function
uint8_t SIM_Status_OK(char *response);
// Function declaration
uint8_t SIM_SendAtCommande(char *command, uint32_t timeout_ms, char *response, uint32_t response_size);
uint8_t slot = 0;
#define PREF_SMS_STORAGE "\"SM\""
char ATcommandd[500];
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
				SIM7600_RESET();
		}
    osDelay(200);
  
  SIM_SetPower(true);
}

//##################################################
//---       Buffer Process
//##################################################
void  SIM_BufferProcess(void)
{
	
	if(Vsense_1<=5){
			strcpy(ARMED_STATUS, "DISARMED");
			}else if(Vsense_1>=10){
			strcpy(ARMED_STATUS, "ARMED");
			}
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
	//receive and send message
	 // if new message arrived, read the message

					 if( sscanf(strStart, "\n+CMTI: " PREF_SMS_STORAGE ",%hhd", &slot)==1)
					{
							SMS_mode = true;
							debugPrintln("find cmti");
							sprintf(ATcommandd,"AT+CMGRD=%d\r\n",slot);
							SIM_SendString(ATcommandd);
									 
						}
						else if (strstr(strStart,"hi smav"))
								{
										SIM_SendAtCommand("AT+CMGF=1\r\n",1000,"OK");
										sprintf(ATcommandd,"AT+CMGS=\"%s\"\r\n",mobileNumber);
										SIM_SendString(ATcommandd);
										sprintf(ATcommandd, "Hi,how can i help you?%c",0x1a);
										SIM_SendString(ATcommandd);
										memset(ATcommandd,NULL,sizeof(ATcommandd)); 
										ACCESS = false;
										SIM_SendString("AT+CPMS=\"SM\",\"SM\",\"SM\"\r\n");
										SIM_SendString("AT+CMGD=,4\r\n");
									//sendDataTaskRunning = false;	
								}
								else if (strstr(strStart,"\nARM"))
								{
										HAL_GPIO_WritePin(GPIOC, RLY_1_Pin, GPIO_PIN_SET);
										HAL_GPIO_WritePin(GPIOC, RLY_2_Pin, GPIO_PIN_SET);
										HAL_GPIO_WritePin(GPIOC, RLY_3_Pin, GPIO_PIN_SET);
										ACCESS = false;
										relayState=true;
										SIM_SendAtCommand("AT+CMGF=1\r\n",1000,"OK");
										sprintf(ATcommandd,"AT+CMGS=\"%s\"\r\n",mobileNumber);
										SIM_SendString(ATcommandd);
										sprintf(ATcommandd, "Machine is now armed%c",0x1a);
										SIM_SendString(ATcommandd);
										memset(ATcommandd,NULL,sizeof(ATcommandd)); 
										//SIM_SendString("AT+CPMS=\"SM\",\"SM\",\"SM\"\r\n");
										//SIM_SendString("AT+CMGD=,4\r\n");
									//sendDataTaskRunning = false;
								}
								else if (strstr(strStart,"\nDISARM"))
								{
										HAL_GPIO_WritePin(GPIOC, RLY_1_Pin, GPIO_PIN_RESET);
										HAL_GPIO_WritePin(GPIOC, RLY_2_Pin, GPIO_PIN_RESET);
										HAL_GPIO_WritePin(GPIOC, RLY_3_Pin, GPIO_PIN_RESET);
										ACCESS = false;
										relayState=false;
										SIM_SendAtCommand("AT+CMGF=1\r\n",1000,"OK");
										sprintf(ATcommandd,"AT+CMGS=\"%s\"\r\n",mobileNumber);
										SIM_SendString(ATcommandd);
										sprintf(ATcommandd, "Machine is now disarmed%c",0x1a);
										SIM_SendString(ATcommandd);
										memset(ATcommandd,NULL,sizeof(ATcommandd));
										//SIM_SendString("AT+CPMS=\"SM\",\"SM\",\"SM\"\r\n");
										//SIM_SendString("AT+CMGD=,4\r\n");
										//sendDataTaskRunning = false;
								}
								else if (strstr(strStart,"\ndiagnostic"))
								{
										SIM_SendAtCommand("AT+CMGF=1\r\n",1000,"OK");
										sprintf(ATcommandd,"AT+CMGS=\"%s\"\r\n",mobileNumber);
										SIM_SendString(ATcommandd);
										sprintf(ATcommandd, "Diagnostic:\n-Rssi=%s\n-Operator=%s\n-Lora module=%s\n%c", rssiString, operatorName, Module_Status,0x1a);
										SIM_SendString(ATcommandd);
										memset(ATcommandd,NULL,sizeof(ATcommandd)); 
										ACCESS = false;
										SIM_SendString("AT+CPMS=\"SM\",\"SM\",\"SM\"\r\n");
										SIM_SendString("AT+CMGD=,4\r\n");
										//sendDataTaskRunning = false;
								}
								else if (strstr(strStart,"\nweather high"))
								{
										SIM_SendAtCommand("AT+CMGF=1\r\n",1000,"OK");
										sprintf(ATcommandd,"AT+CMGS=\"%s\"\r\n",mobileNumber);
										SIM_SendString(ATcommandd);
										//sprintf(ATcommandd, "T1=%.2f�C\nT2=%.2f�C\nH1=%.2f\nH2=%.2f\nW_S=%.2f%c",temperature1, temperature2, humidity1, humidity2, Wind_stat,0x1a);
										sprintf(ATcommandd, "T1=%.2f�C\nH1=%.2f%%\n%c",temperature1,humidity1,0x1a);
										SIM_SendString(ATcommandd);
										memset(ATcommandd,NULL,sizeof(ATcommandd)); 
										ACCESS = false;
										SIM_SendString("AT+CPMS=\"SM\",\"SM\",\"SM\"\r\n");
										SIM_SendString("AT+CMGD=,4\r\n");
										//sendDataTaskRunning = false;
								}
								else if (strstr(strStart,"\nweather low"))
								{
										SIM_SendAtCommand("AT+CMGF=1\r\n",1000,"OK");
										sprintf(ATcommandd,"AT+CMGS=\"%s\"\r\n",mobileNumber);
										SIM_SendString(ATcommandd);
										//sprintf(ATcommandd, "T1=%.2f�C\nT2=%.2f�C\nH1=%.2f\nH2=%.2f\nW_S=%.2f%c",temperature1, temperature2, humidity1, humidity2, Wind_stat,0x1a);
										sprintf(ATcommandd, "T2=%.2f�C\nH2=%.2f%%\n%c",temperature2,humidity2,0x1a);
										SIM_SendString(ATcommandd);
										memset(ATcommandd,NULL,sizeof(ATcommandd)); 
										ACCESS = false;
										SIM_SendString("AT+CPMS=\"SM\",\"SM\",\"SM\"\r\n");
										SIM_SendString("AT+CMGD=,4\r\n");
									//sendDataTaskRunning = false;
								}
								else if (strstr(strStart,"\nwind speed"))
								{
										SIM_SendAtCommand("AT+CMGF=1\r\n",1000,"OK");
										sprintf(ATcommandd,"AT+CMGS=\"%s\"\r\n",mobileNumber);
										SIM_SendString(ATcommandd);
										//sprintf(ATcommandd, "T1=%.2f�C\nT2=%.2f�C\nH1=%.2f\nH2=%.2f\nW_S=%.2f%c",temperature1, temperature2, humidity1, humidity2, Wind_stat,0x1a);
										sprintf(ATcommandd, "W_S=%.2fKm/h%c",Wind_stat,0x1a);
										SIM_SendString(ATcommandd);
										memset(ATcommandd,NULL,sizeof(ATcommandd)); 
										ACCESS = false;
										SIM_SendString("AT+CPMS=\"SM\",\"SM\",\"SM\"\r\n");
										SIM_SendString("AT+CMGD=,4\r\n");
									//sendDataTaskRunning = false;
								}
								else if (strstr(strStart,"\nmachine"))
								{
										SIM_SendAtCommand("AT+CMGF=1\r\n",1000,"OK");
										sprintf(ATcommandd,"AT+CMGS=\"%s\"\r\n",mobileNumber);
										SIM_SendString(ATcommandd);
										sprintf(ATcommandd, "Machine_Status:%s%c",ARMED_STATUS,0x1a);
										SIM_SendString(ATcommandd);
										memset(ATcommandd,NULL,sizeof(ATcommandd)); 
										ACCESS = false;
										SIM_SendString("AT+CPMS=\"SM\",\"SM\",\"SM\"\r\n");
										SIM_SendString("AT+CMGD=,4\r\n"); 
									//sendDataTaskRunning = false;
								}else if (strstr(strStart,"\nbattery"))
								{
										SIM_SendAtCommand("AT+CMGF=1\r\n",1000,"OK");
										sprintf(ATcommandd,"AT+CMGS=\"%s\"\r\n",mobileNumber);
										SIM_SendString(ATcommandd);
										sprintf(ATcommandd, "Battery=%.2f%c",Vsense_2,0x1a);
										SIM_SendString(ATcommandd);
										memset(ATcommandd,NULL,sizeof(ATcommandd)); 
										ACCESS = false;
										SIM_SendString("AT+CPMS=\"SM\",\"SM\",\"SM\"\r\n");
										SIM_SendString("AT+CMGD=,4\r\n"); 
									//sendDataTaskRunning = false;
								}
								// This will delete all messages in the SIM card. (Is it ok for you?)
								else if (strstr(strStart,"RESET"))
								{
										SIM_SendAtCommand("AT+CMGF=1\r\n",1000,"OK");
										sprintf(ATcommandd,"AT+CMGS=\"%s\"\r\n",mobileNumber);
										SIM_SendString(ATcommandd);
										sprintf(ATcommandd, "OK,the device will restart now%c",0x1a);
										SIM_SendString(ATcommandd);
										memset(ATcommandd,NULL,sizeof(ATcommandd)); 
										osDelay(4000);
										NVIC_SystemReset();
								}else {
									
									//sendDataTaskRunning = false;

								}

		
					
					
		
  //Thingsboard Section
	str1 = strstr(strStart, "\r\nRING");
if (str1 != NULL)
{
	
	SIM_SendAtCommand("ATA\r\n",1000,"OK");
	SIM_SendAtCommand("ATH\r\n",2000,"OK");
	if(relayState==false)
	{
	HAL_GPIO_WritePin(GPIOC, RLY_1_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOC, RLY_2_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOC, RLY_3_Pin, GPIO_PIN_SET);
		// send feedback message to client
		SIM_SendAtCommand("AT+CMGF=1\r\n",1000,"OK");
		sprintf(ATcommandd,"AT+CMGS=\"%s\"\r\n",mobileNumber);
		SIM_SendString(ATcommandd);
		sprintf(ATcommandd, "Machine_Status:%s%c",ARMED_STATUS,0x1a);
		SIM_SendString(ATcommandd);
		memset(ATcommandd,NULL,sizeof(ATcommandd)); 
		ACCESS = false;
		SIM_SendString("AT+CPMS=\"SM\",\"SM\",\"SM\"\r\n");
		SIM_SendString("AT+CMGD=,4\r\n"); 
		relayState=true;
		SMS_mode = true;
		
	}else if(relayState==true){
		HAL_GPIO_WritePin(GPIOC, RLY_1_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOC, RLY_2_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOC, RLY_3_Pin, GPIO_PIN_RESET);
			// send feedback message to client
			SIM_SendAtCommand("AT+CMGF=1\r\n",1000,"OK");
			sprintf(ATcommandd,"AT+CMGS=\"%s\"\r\n",mobileNumber);
			SIM_SendString(ATcommandd);
			sprintf(ATcommandd, "Machine_Status:%s%c",ARMED_STATUS,0x1a);
			SIM_SendString(ATcommandd);
			memset(ATcommandd,NULL,sizeof(ATcommandd)); 
			ACCESS = false;
			SIM_SendString("AT+CPMS=\"SM\",\"SM\",\"SM\"\r\n");
			SIM_SendString("AT+CMGD=,4\r\n"); 		
		relayState=false;
		SMS_mode = true;
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
                if (*str1 == 't' ) // Assuming 't' for true, 'f' for false
                {
                    debugPrintln("+++++++++++++++++++++++++++++++++++++++RELAY ON+++++++++++++++++++++++++++++++++++++++++++++++++");
                    ledStatus = 1;
                    HAL_GPIO_WritePin(GPIOC, RLY_1_Pin, GPIO_PIN_SET);
                    HAL_GPIO_WritePin(GPIOC, RLY_2_Pin, GPIO_PIN_SET);
                    HAL_GPIO_WritePin(GPIOC, RLY_3_Pin, GPIO_PIN_SET);
                    rpc_status = false;
                    fonction_control = false;
                    prev_fonction_control = true;
										relayState = true; // Update the relay state
                    //Previous_rpc_status=rpc_status;
                }
                else if (*str1 == 'f' )
                {
                    debugPrintln("+++++++++++++++++++++++++++++++++++++++RELAY OFF+++++++++++++++++++++++++++++++++++++++++++++++++");
                    ledStatus = 0;
                    HAL_GPIO_WritePin(GPIOC, RLY_1_Pin, GPIO_PIN_RESET);
                    HAL_GPIO_WritePin(GPIOC, RLY_2_Pin, GPIO_PIN_RESET);
                    HAL_GPIO_WritePin(GPIOC, RLY_3_Pin, GPIO_PIN_RESET);
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

//****************************************************//BEGIN V�rifie l'�tat de la fonctionnalit� du module.****************************************************************//
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
//****************************************************//END V�rifie l'�tat de la fonctionnalit� du module.****************************************************************//

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

		// Attendre jusqu'� ce que la r�ponse contienne la m�thode "setValue"
    //while (rpc_status == Previous_rpc_status){
			//SIM_SendString("AT+HTTPPARA=\"URL\",\"https://demo.thingsboard.io/api/v1/xMJwZXAY71oDofdBzdTC/rpc\"\r\n");
			//SIM_SendAtCommand("AT+HTTPACTION=0\r\n",2000,"OK");
			//SIM_SendString("AT+HTTPREAD=0,500\r\n");
			//debugPrintln("ATT response with method setValue");
			//osDelay(1000);
		//}
		
		//rpc_status = true;
    // Une r�ponse contenant la m�thode "setValue" a �t� trouv�e
    //debugPrintln("Received response with method setValue");

    // Lire le reste de la r�ponse
    

    // Terminer la session HTTP
    //SIM_SendString("AT+HTTPTERM\r\n");
    //HAL_Delay(5000);
}






