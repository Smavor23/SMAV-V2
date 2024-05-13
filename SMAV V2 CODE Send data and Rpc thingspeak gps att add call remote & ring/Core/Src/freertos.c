/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

// These are the custom libraries for the code 

#include "trx_sim.h"     //SIM800 lib
#include "spi.h"        //SPI lib
#include "usart.h"     //USART lib
#include "adc.h"      //ADC lib
#include "Lora.h"


/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define MEM_ADDR    0x00u
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

//========================================================================
//------------------------------BEGIN Variables---------------------------
//========================================================================

char *DEVEUI = "12-58-45-5M\n";        //DEVICE ID GLOBALE VARIABLE

//-----------------------------§VARIABLES FOR SENSORS§--------------------
char  float_buffer[32] = {0};
int   Tsense;                       		//variable to store Current sensor 2
int 	adc_index;  
uint32_t adc_buffer[4];        					//variable to store adc raw values
float Vsense_1;                        	//variable to store Voltage sensor 1
float Vsense_2;                       	//variable to store Voltage sensor 2
float VBattery;
float Isense;                       		//variable to store Current sensor 1
float Gas_lvl;                    			//variable to store Gas level
float Wind_stat;                 				//variable to store wind status
float wind_Kmh;
float temperature1 = 1;
float humidity1    = 1;
float temperature2 = 2;
float humidity2    = 2;
              					//variable to store index of ADC read
//--------------------------------------------------------------------------

//-----------------------------§VARIABLES FOR LORA§-------------------------
char 	*LORA_MODULE_STATUS = "";          //FOR CHECK LORA MODULE STATUS
char 	buffer[200];
char 	Module_Status[50];
int		RSSI;
uint8_t packet_size = 0;
uint8_t idx = 0;	
uint8_t error = 0;
uint8_t valid_data = 0;
uint8_t        read_data[128];
lora_sx1276    lora;
//--------------------------------------------------------------------------

//-----------------------------§VARIABLES FOR RPC§--------------------------
bool Smav_rpc_Ready;
//char response[100];
//uint8_t  sensor_buffer[_SMS_BUFFER_SIZE];  //String for SMS messages
//++++++++++++++++++++++++++++++++++trx_sim.c+++++++++++++++++++++++++++++++
extern char ARMED_STATUS[10];
extern bool relayState; 	
extern bool prev_relayState; 									// Initialize the relay state
extern bool rpc_status;
extern bool Previous_rpc_status;
extern bool fonction_control;
extern bool prev_fonction_control;
//--------------------------------------------------------------------------

//----------------------§BUFFER FOR SEND DATA TO SERVER§--------------------
//char response[100]; 												// Buffer to store the response
char buffer_X[512]; 													// Increase buffer size to accommodate larger JSON payload
char data[512];
//char charBuffer[100];												// not used
//uint8_t buffer_XX[100] = {0};								// not used
//--------------------------------------------------------------------------

//--------------------------------§URL SERVER§------------------------------
const char *url = "http://demo.thingsboard.io/api/v1/h0U2oV1SK60dSys7MxIC/telemetry";
const char *url_RPC = "https://demo.thingsboard.io/api/v1/h0U2oV1SK60dSys7MxIC/attributes?sharedKeys=setValue";
const char *url_ATTRIBUTE = "https://demo.thingsboard.io/api/v1/h0U2oV1SK60dSys7MxIC/attributes";
//--------------------------------------------------------------------------

//-----------------------§VARIABLES FOR DIAGNOSTIC DATA§--------------------
//char command[256];
char buffer_Diag[512]; 												// Increase buffer size to accommodate larger JSON payload
char buffer_Attribute[512]; 												// Increase buffer size to accommodate larger JSON payload
char data_Diag[256]; 													// Increase buffer size to accommodate larger JSON payload
char data_attribute[100]; 	
//+++++++++++++++++++++++++++++++++trx_sim.c++++++++++++++++++++++++++++++++
extern char rssiString[50];
extern char functionalityStatus[50];
extern char operatorName[50];
//--------------------------------------------------------------------------
//-------------------------------§GPS DATA§---------------------------------
extern char lat[20], lon[20];
//--------------------------------------------------------------------------
bool sendDataTaskRunning;
//==========================================================================
//------------------------------END Variables-------------------------------
//==========================================================================

/* USER CODE END Variables */
osThreadId DiagnosticTaskHandle;
osThreadId LoraTaskHandle;
osThreadId SendDataTaskHandle;
osThreadId RpcTaskHandle;
osSemaphoreId BinSemHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
//ADC is measured through interrupts
//this function will fill the adc_buffer with raw values (0 to 2^12).

/*void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* AdcHandle)
{
  if(__HAL_ADC_GET_FLAG(AdcHandle, ADC_FLAG_EOC)){
    //sprintf((char*)sensor_buffer,"ADC -> %d\n", (int)HAL_ADC_GetValue(&hadc1));
    //debugPrintln(sensor_buffer);
    adc_buffer[adc_index] = HAL_ADC_GetValue(&hadc1);
    adc_index++;
  }
  if(adc_index>=4)
            adc_index=0;
  	
}
*/
/* USER CODE END FunctionPrototypes */

void StartDiagnosticTask(void const * argument);
void StartLoraTask(void const * argument);
void StartSendDataTask(void const * argument);
void StartRpcTask(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* definition and creation of BinSem */
  osSemaphoreDef(BinSem);
  BinSemHandle = osSemaphoreCreate(osSemaphore(BinSem), 1);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of DiagnosticTask */
  osThreadDef(DiagnosticTask, StartDiagnosticTask, osPriorityAboveNormal, 0, 128);
  DiagnosticTaskHandle = osThreadCreate(osThread(DiagnosticTask), NULL);

  /* definition and creation of LoraTask */
  osThreadDef(LoraTask, StartLoraTask, osPriorityBelowNormal, 0, 128);
  LoraTaskHandle = osThreadCreate(osThread(LoraTask), NULL);

  /* definition and creation of SendDataTask */
  osThreadDef(SendDataTask, StartSendDataTask, osPriorityNormal, 0, 128);
  SendDataTaskHandle = osThreadCreate(osThread(SendDataTask), NULL);

  /* definition and creation of RpcTask */
  osThreadDef(RpcTask, StartRpcTask, osPriorityBelowNormal, 0, 128);
  RpcTaskHandle = osThreadCreate(osThread(RpcTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

}

void decode_values(void) {
    char buffer[200];
    char id;
    int temperature, humidity;

    // Find the positions of '*' and '#' in the read_data
    char *ptr_start = strchr((char*)read_data, '*');
    char *ptr_hash = strchr((char*)read_data, '#');

    // Check if the message has '*' and '#' in it, indicating valid data
    if (ptr_start != NULL && ptr_hash != NULL) {
			
				//debugPrint("+++++++++++++++++++++++++(ptr_start != NULL && ptr_hash != NULL)+++++++++++++++++++++++++++++++");
			
        // Extract the ID, temperature, and humidity using sscanf
        if (sscanf((char*)read_data, "*%c,%d,%d#", &id, &temperature, &humidity) == 3) {
					
						//debugPrint("++++++++++++++++++++++++Extract the ID, temperature, and humidity using sscanf++++++++++++++++++++++++++++++++");
            // Format and print the extracted values
            snprintf((char*)buffer, sizeof(buffer), "Temperature: %i\r\nHumidity: %i\r\n", temperature, humidity);
            //debugPrint((char*)buffer);

            // Match the ID and assign values to global variables
            if (id == 'A') {
								//debugPrint("++++++++++++++++++++++++Extract the ID = A++++++++++++++++++++++++++++++++");
                temperature1 = (float)temperature / 100.0;
                humidity1 = (float)humidity / 100.0;
                valid_data = 1;
            } else if (id == 'B') {
								//debugPrint("++++++++++++++++++++++++Extract the ID NO A++++++++++++++++++++++++++++++++");
                temperature2 = (float)temperature / 100.0;
                humidity2 = (float)humidity / 100.0;
                valid_data = 1;
            }
        }
    }
}

//----------------BEGIN DEBBUG Function to transmit a string over UART---------------
void uprintf(char *str) {
    HAL_UART_Transmit(&huart2, (uint8_t *)str, strlen(str), 100);
}
//----------------END BUBBUG Function to transmit a string over UART-----------------
void uint8ToChar(uint8_t *uintBuffer, char *charBuffer, uint32_t size) {
    for (uint32_t i = 0; i < size; i++) {
        charBuffer[i] = (char)uintBuffer[i];
    }
}

void RPC_FETCH_ATTRIBUTES(void) {
	
	SIM_SendAtCommand("AT+HTTPINIT\r\n",2000,"OK");
	memset(buffer_X,0,sizeof(buffer_X));
	snprintf(buffer_X, sizeof(buffer_X), "AT+HTTPPARA=\"URL\",\"%s\"\r\n", url_RPC);
	SIM_SendString(buffer_X);
	HAL_Delay(2000);
	SIM_SendAtCommand("AT+HTTPACTION=0\r\n",3000,"OK");
	SIM_SendAtCommand("AT+HTTPREAD=0,500\r\n",3000,"OK");
	SIM_SendAtCommand("AT+HTTPTERM=0\r\n",3000,"OK");
	HAL_Delay(3000);
	memset(buffer_X,NULL,sizeof(buffer_X));
	
}
//----------------------------BEGIN SIM7600_HTTPPost----------------------------------
void SIM7600_HTTPPost(const char *url, const char *data) {
    
    // Construct HTTP POST request
    snprintf(buffer_X, sizeof(buffer_X), "AT+HTTPINIT\r\n");
    SIM_SendString(buffer_X);
    HAL_Delay(2000);  	// DELAY 2sec
			
    snprintf(buffer_X, sizeof(buffer_X), "AT+HTTPPARA=\"URL\",\"%s\"\r\n", url);
    SIM_SendString(buffer_X);
    HAL_Delay(1000);		// DELAY 1sec	
    
    snprintf(buffer_X, sizeof(buffer_X), "AT+HTTPPARA=\"CONTENT\",\"application/json\"\r\n");
    SIM_SendString(buffer_X);
    HAL_Delay(1000);
    
    snprintf(buffer_X, sizeof(buffer_X), "AT+HTTPDATA=%d,10000\r\n", strlen(data));
    SIM_SendString(buffer_X);
    HAL_Delay(1000);
    
    snprintf(buffer_X, sizeof(buffer_X), "%s", data);
    SIM_SendString(buffer_X);
    HAL_Delay(1000);
    
    SIM_SendString("AT+HTTPACTION=1\r\n");
    HAL_Delay(3000); // Adjust delay based on expected response time
    
    SIM_SendString("AT+HTTPTERM\r\n");
    HAL_Delay(3000);
		memset(buffer_X,NULL,sizeof(buffer_X));
}
//--------------------------------END SIM7600_HTTPPost------------------------------------------------

//------------------------------------ATTRIBUTE-------------------------------------------------------
void SIM7600_ATTRIBUTE(const char *url_ATTRIBUTE, const char *data_attribute) {
    
    // Construct HTTP POST request
    snprintf(buffer_Attribute, sizeof(buffer_Attribute), "AT+HTTPINIT\r\n");
    SIM_SendString(buffer_Attribute);
    HAL_Delay(2000);
    
    snprintf(buffer_Attribute, sizeof(buffer_Attribute), "AT+HTTPPARA=\"URL\",\"%s\"\r\n", url_ATTRIBUTE);
    SIM_SendString(buffer_Attribute);
    HAL_Delay(1000);
    
    snprintf(buffer_Attribute, sizeof(buffer_Attribute), "AT+HTTPPARA=\"CONTENT\",\"application/json\"\r\n");
    SIM_SendString(buffer_Attribute);
    HAL_Delay(1000);
    
    snprintf(buffer_Attribute, sizeof(buffer_Attribute), "AT+HTTPDATA=%d,10000\r\n", strlen(data_attribute));
    SIM_SendString(buffer_Attribute);
    HAL_Delay(1000);
    
    snprintf(buffer_Attribute, sizeof(buffer_Attribute), "%s", data_attribute);
    SIM_SendString(buffer_Attribute);
    HAL_Delay(1000);
    
    SIM_SendString("AT+HTTPACTION=1\r\n");
    HAL_Delay(3000); // Adjust delay based on expected response time
    
    SIM_SendString("AT+HTTPTERM\r\n");
    HAL_Delay(3000);
		memset(buffer_Attribute,NULL,sizeof(buffer_Attribute));
}
void Send_Attribute_data(bool relayState) {
    
    // Format sensor data into JSON string
    snprintf(data_attribute, 100, "{\"relayState\": \"%d\", \"latitude\": \"%s\", \"longitude\": \"%s\"}", relayState, lat, lon);
    
    // Your existing code to send data to the server...
		SIM7600_ATTRIBUTE(url_ATTRIBUTE, data_attribute);
}
//----------------------------------------------------------------------------------------------------
//----------------------------BEGIN SIM7600_HTTPPost_Diagnostic_Data----------------------------------
void SIM7600_HTTPPost_Diagnostic_Data(const char *url, const char *data_Diag) {
    
    // Construct HTTP POST request
    snprintf(buffer_Diag, sizeof(buffer_Diag), "AT+HTTPINIT\r\n");
    SIM_SendString(buffer_Diag);
    HAL_Delay(2000);
    
    snprintf(buffer_Diag, sizeof(buffer_Diag), "AT+HTTPPARA=\"URL\",\"%s\"\r\n", url);
    SIM_SendString(buffer_Diag);
    HAL_Delay(1000);
    
    snprintf(buffer_Diag, sizeof(buffer_Diag), "AT+HTTPPARA=\"CONTENT\",\"application/json\"\r\n");
    SIM_SendString(buffer_Diag);
    HAL_Delay(1000);
    
    snprintf(buffer_Diag, sizeof(buffer_Diag), "AT+HTTPDATA=%d,10000\r\n", strlen(data_Diag));
    SIM_SendString(buffer_Diag);
    HAL_Delay(1000);
    
    snprintf(buffer_Diag, sizeof(buffer_Diag), "%s", data_Diag);
    SIM_SendString(buffer_Diag);
    HAL_Delay(1000);
    
    SIM_SendString("AT+HTTPACTION=1\r\n");
    HAL_Delay(3000); // Adjust delay based on expected response time
    
    SIM_SendString("AT+HTTPTERM\r\n");
    HAL_Delay(3000);
		memset(buffer_Diag,NULL,sizeof(buffer_Diag));
}
void Send_Diagnostic_data(char* rssiString, char* functionalityStatus, char* operatorName, char* Module_Status) {
    
    // Format sensor data into JSON string
    snprintf(data_Diag, 256, "{\"rssi\": \"%s\",\"functionalityStatus\": \"%s\",\"operatorName\": \"%s\",\"moduleStatus\": \"%s\"}", rssiString, functionalityStatus, operatorName, Module_Status);
    
    // Your existing code to send data to the server...
		SIM7600_HTTPPost_Diagnostic_Data(url, data_Diag);
}
//----------------------------END SIM7600_HTTPPost_Diagnostic_Data----------------------------------

//----------------------------BEGIN SIM7600_Init----------------------------------------------------
void SIM7600_Init(void) {
    // Basic initialization commands
		HAL_UART_Transmit(&huart2, (uint8_t *) "-->BGIN SIM7600_Init---------------\n", strlen ("-->BGIN SIM7600_Init---------------\n"), 100);
		SIM_Init(osPriorityNormal); //This functions starts another task in the trx_sim.c
		SIM_SendAtCommand("AT+CGPS=1\r\n",1000,"OK");
		SIM_SendAtCommand("AT+CPIN=0000\r\n",3000,"OK");
		HAL_Delay(1000);
    SIM_SendString("AT+CFUN=1\r\n");
    HAL_Delay(1000);
		SIM_SendAtCommand("AT+CCID\r\n",3000,"OK");
		SIM_SendAtCommand("AT+CREG?\r\n",3000,"OK");
		SIM_SendAtCommand("AT+CGATT=1\r\n",1000,"OK");
		SIM_SendAtCommand("AT+CGACT=1,1\r\n",1000,"OK");
		SIM_SendAtCommand("AT+CNMP=2\r\n",2000,"OK");
    SIM_SendString("AT+CGDCONT=1,\"IP\",\"www.inwi.ma\"\r\n");
    HAL_Delay(1000);
		SIM_SendAtCommand("AT+CNUM\r\n",10000,"OK");
		HAL_Delay(8000);
		HAL_UART_Transmit(&huart2, (uint8_t *) "-->END SIM7600_Init----------------\n", strlen ("-->END SIM7600_Init----------------\n"), 100);
}

void SIM7600_RESET(void) {
	//SIM_SendString("AT+CRESET=?\r\n");
	SIM_SendString("AT+CPOF\r\n");
	HAL_Delay(40000);
}
void GPS_DATA(void) {

	//SIM_SendAtCommand("AT+CGPS=1\r\n",1000,"OK");
	HAL_Delay(2000);
	SIM_SendAtCommand("AT+CGPSINFO\r\n",60000,"OK");
	HAL_Delay(3000);
	SIM_SendAtCommand("AT+CGPS=0\r\n",3000,"OK");
	
}
//----------------------------END SIM7600_Init----------------------------------
/* USER CODE BEGIN Header_StartDiagnosticTask */
/**
  * @brief  Function implementing the DiagnosticTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDiagnosticTask */
void StartDiagnosticTask(void const * argument)
{
		sendDataTaskRunning = true;
  /* USER CODE BEGIN StartDiagnosticTask */
		memset(buffer,NULL,sizeof(buffer));
		uint8_t LoRa_status = lora_init(&lora, &hspi1, NSS_GPIO_Port, RESET_GPIO_Port, NSS_Pin, RESET_Pin, LORA_BASE_FREQUENCY_EU);
		HAL_Delay(100);
		HAL_GPIO_WritePin(GPIOB, P4V_EN_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOA, SIM_EN_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(SMS_RST_GPIO_Port,SMS_RST_Pin,GPIO_PIN_RESET);
		HAL_Delay(20);
		HAL_GPIO_WritePin(SMS_RST_GPIO_Port,SMS_RST_Pin,GPIO_PIN_SET);
	
		HAL_GPIO_WritePin(GPO_B1_A_GPIO_Port, GPO_B1_A_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPO_B2_A_GPIO_Port, GPO_B2_A_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPO_B3_A_GPIO_Port, GPO_B3_A_Pin, GPIO_PIN_RESET);
	
		HAL_GPIO_WritePin(GPIOC, P9V_EN_Pin, GPIO_PIN_RESET );
		HAL_GPIO_WritePin(BATT_MON_EN_GPIO_Port, BATT_MON_EN_Pin,GPIO_PIN_RESET);
		HAL_Delay(1000);
		
  /* Infinite loop */
	
  for(;;)
  {
		//osSemaphoreWait(BinSemHandle, osWaitForever);
		
		HAL_UART_Transmit(&huart2, (uint8_t *) "DEVICE DEVUI : ", strlen ("DEVICE DEVUI : "), 100);
		HAL_UART_Transmit(&huart2, (uint8_t *) DEVEUI, strlen (DEVEUI), 100);
		
		SIM7600_RESET();
		SIM7600_Init();
		GPS_DATA();
		
		HAL_Delay(1000);
		
		char *str1 = "Entered StartDiagnosticTask\n";
		HAL_UART_Transmit(&huart2, (uint8_t *) str1, strlen (str1), 100);
		
		//--------------SENSORS DIAGNOSTIC-----------------------------------------------
		HAL_ADC_Start(&hadc1); 
		//Buffer to store the formatted string
		Vsense_1 =  (float)ADC_Read_VSense_1()/4095*16.5 + 5;
		Vsense_2 =  (float)ADC_Read_VSense_2()/4095*14.5+ 0.6;
		VBattery =  (float)ADC_Read_Vbattery();
		Wind_stat = (float)(ADC_Read_Wind()*(3.3)/(4095))*230; 
		Gas_lvl =   (float)ADC_Read_Gas()*83/4095;
		
		//Vsense_1 =  100.000000;
		//Vsense_2 =  100.000000;
		//VBattery =  100.000000;
		//Wind_stat = 100.000000; 
		//Gas_lvl =   100.000000;
		//temperature1 = 1;
		//humidity1 = 1;
		//HAL_Delay(250);
		
		HAL_UART_Transmit(&huart2, (uint8_t *) "-->BGIN SENSORS DATA VALUES----------\n", strlen ("-->BGIN SENSORS DATA VALUES----------\n"), 100);
		sprintf(float_buffer, "--> Vsense_1     :%f\n",Vsense_1);
		uprintf(float_buffer);
		sprintf(float_buffer, "--> Vsense_2     :%f\n",Vsense_2);
		uprintf(float_buffer);
		sprintf(float_buffer, "--> VBattery     :%f\n",VBattery);
		uprintf(float_buffer);
		sprintf(float_buffer, "--> Wind_stat    :%f\n",Wind_stat);
		uprintf(float_buffer);
		sprintf(float_buffer, "--> Gas_lvl      :%f\n",Gas_lvl);
		uprintf(float_buffer);
		sprintf(float_buffer, "--> temperature1 :%f\n",temperature1);
		uprintf(float_buffer);
		sprintf(float_buffer, "--> temperature2 :%f\n",temperature2);
		uprintf(float_buffer);
		sprintf(float_buffer, "--> humidity1		:%f\n",humidity1);
		uprintf(float_buffer);
		sprintf(float_buffer, "--> humidity2 		:%f\n",humidity2);
		uprintf(float_buffer);
		HAL_UART_Transmit(&huart2, (uint8_t *) "-->END SENSORS DATA VALUES----------\n", strlen ("-->END SENSORS DATA VALUES----------\n"), 100);
		HAL_Delay(3000);
		
		//--------------Begin LoRa DIAGNOSTIC---------------------------------------------
		HAL_UART_Transmit(&huart2, (uint8_t *) "-->BEGIN LORA MODULE DIAGNOSTIC----------\n", strlen ("-->BEGIN LORA MODULE DIAGNOSTIC----------\n"), 100);
		
		if (LoRa_status == LORA_OK)
			{
					LORA_MODULE_STATUS = "--> LoRa_Module_Status : MODULE OK\n";
					strcpy(Module_Status, "LORA MODULE OK");
			}else {
								LORA_MODULE_STATUS = "--> LoRa_Module_Status : MODULE ERROR\n";
								strcpy(Module_Status, "LORA MODULE ERROR");
						}
			
		//uprintf(LORA_MODULE_STATUS);
		HAL_UART_Transmit(&huart2, (uint8_t *) "-->END LORA MODULE DIAGNOSTIC----------\n", strlen ("-->END LORA MODULE DIAGNOSTIC----------\n"), 100);
		HAL_Delay(1000);
		//--------------End LoRa DIAGNOSTIC-----------------------------------------------
				
				
				
		//--------------Begin Send DIAGNOSTIC Data To Server---------------------------------------------
		Send_Diagnostic_data(rssiString, functionalityStatus, operatorName, Module_Status);
		//--------------End Send DIAGNOSTIC Data To Server---------------------------------------------
				
		
				
		//--------------Begin Send Sensors Data To Server---------------------------------------------
		// Example HTTP POST request with telemetry data
    snprintf(data, sizeof(data), "{\"Vsense_1\":%.2f,\"Vsense_2\":%.2f,\"VBattery\":%.2f,\"Wind_stat\":%.2f,\"Gas_lvl\":%.2f,\"Temp1\":%.2f,\"Temp2\":%.2f,\"Hum1\":%.2f,\"Hum2\":%.2f}",
             Vsense_1, Vsense_2, VBattery, Wind_stat, Gas_lvl, temperature1, temperature2, humidity1, humidity2);
    SIM7600_HTTPPost(url, data);
		//--------------End Send Sensors Data To Server---------------------------------------------
		
		
		
		
		
		char *str2 = "Leaving StartDiagnosticTask\n\n";
		HAL_UART_Transmit(&huart2, (uint8_t *) str2, strlen (str2), 100);
		// Set the flag to stop SendDataTask
    sendDataTaskRunning = false;
		//osSemaphoreRelease(BinSemHandle);
		osDelay(10 * 60 * 1000); // Delay in milliseconds
  }
  /* USER CODE END StartDiagnosticTask */
}

/* USER CODE BEGIN Header_StartLoraTask */
/**
* @brief Function implementing the LoraTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartLoraTask */
void StartLoraTask(void const * argument)
{
  /* USER CODE BEGIN StartLoraTask */
  /* Infinite loop */
  for(;;)
  {

		//char *str1 = "Entered StartLoraTask\n";
		//HAL_UART_Transmit(&huart2, (uint8_t *) str1, strlen (str1), 100);
		if(error == 0)
				{
					// RECEIVING DATA - - - - - - - - - - - - - - - - - - - - - - - -
					packet_size = lora_parsepacket(&lora);
					//debugPrintln("CHECK PACKET-SIZE");
					if (packet_size) {
						//debugPrintln("RECEIVING DATA - - - - - - - - - - - - - - - - - - - - - - - -");
						// read data until available
						idx = 0;
						while (lora_available(&lora)) read_data[idx++] = lora_read(&lora);
						// Receive ok
						read_data[idx] = '\0';
						RSSI = lora_packet_rssi(&lora);
						snprintf((char*)buffer, sizeof(buffer), "Rx Data:%s with RSSI %i db\r\n", read_data, RSSI);
						//debugPrintln("========================================================================================");
						//debugPrintln((char*)read_data);
						//debugPrintln("==================================decode_values=========================================");
						// process the incoming data
						decode_values();
						//debugPrintln("****************************************************************************************");
						//printf("receiveloradatacount: %d\r\n", receiveloradatacount);
						//debugPrintln("****************************************************************************************");
						//HAL_GPIO_WritePin(RLY_1_GPIO_Port, RLY_1_Pin, GPIO_PIN_RESET);
						memset(read_data,NULL,sizeof(read_data));	
						
					}
					else {
						//HAL_GPIO_WritePin(RLY_1_GPIO_Port, RLY_1_Pin, GPIO_PIN_SET);
						//debugPrintln("SORRY LORA PER TO PER DATA NOT VALIDE - - - - - - - - - - - - - - - - - - - - - - - -");
						HAL_Delay(100); 
					}
		//char *str2 = "Leaving StartLoraTask\n\n";
		//HAL_UART_Transmit(&huart2, (uint8_t *) str2, strlen (str2), 100);

		osDelay(1000); // Delay in milliseconds
					 }
  }
  /* USER CODE END StartLoraTask */
}

/* USER CODE BEGIN Header_StartSendDataTask */
/**
* @brief Function implementing the SendDataTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartSendDataTask */
void StartSendDataTask(void const * argument)
{
  /* USER CODE BEGIN StartSendDataTask */
  /* Infinite loop */
  for(;;)
  {
		/*
		// Check if SendDataTask should run
		
		char *str1 = "Entered StartRpcTask\n";
		HAL_UART_Transmit(&huart2, (uint8_t *) str1, strlen (str1), 100);
					    // Initialize HTTP session
    if (SIM_SendAtCommand("AT+HTTPINIT\r\n", 1000, "OK") != 1) {
        debugPrintln("HTTPINIT RESPONSE NOT OK");
        debugPrintln("Sorry we need to restard module");
        SIM_SendString("AT+CPOF\r\n");
        HAL_Delay(30000);
        return; // Sortir de la fonction si l'initialisation échoue
    }
		
    debugPrintln("HTTPINIT RESPONSE OK");
		
		SIM_SendString("AT+HTTPPARA=\"URL\",\"https://demo.thingsboard.io/api/v1/xMJwZXAY71oDofdBzdTC/rpc\"\r\n");
		SIM_SendAtCommand("AT+HTTPACTION=0\r\n",2000,"OK");
		
		while(SIM_SendAtCommand("AT+HTTPACTION=0\r\n",2000,"OK")!=1 || fonction_control==prev_fonction_control){
			//SIM_SendAtCommand("AT+HTTPACTION=0\r\n",2000,"OK");
			SIM_SendAtCommand("AT+HTTPREAD=0,500\r\n",5000,"OK");
					if(fonction_control!=prev_fonction_control){
							HAL_Delay(5000);
							SIM_SendString("AT+HTTPTERM\r\n");
							Send_Diagnostic_data(rssiString, functionalityStatus, operatorName, Module_Status,ARMED_STATUS);
							prev_fonction_control=fonction_control;
							SIM7600_RESET();
							SIM7600_Init();
							break;
					}
			HAL_Delay(1000);
			
		}
		

									SIM7600_RESET();
									SIM7600_Init();
		}
		//updateLed();
		//HAL_Delay(5000);
		char *str2 = "Leaving StartRpcTask\n\n";
		HAL_UART_Transmit(&huart2, (uint8_t *) str2, strlen (str2), 100);
		//osSemaphoreRelease(BinSemHandle);
		*/
		//osSemaphoreWait(BinSemHandle, osWaitForever);
		if(sendDataTaskRunning == false){
		RPC_FETCH_ATTRIBUTES();
		HAL_Delay(5000);
		
		if(prev_relayState!=relayState){
		prev_relayState=relayState;
		Send_Attribute_data(relayState);
		
		}
	}
		//osSemaphoreRelease(BinSemHandle);
		osDelay(40 * 1000); // Delay in milliseconds
  }
  /* USER CODE END StartSendDataTask */
}

/* USER CODE BEGIN Header_StartRpcTask */
/**
* @brief Function implementing the RpcTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartRpcTask */
void StartRpcTask(void const * argument)
{
  /* USER CODE BEGIN StartRpcTask */
  /* Infinite loop */
  for(;;)
  {
		
  }
  /* USER CODE END StartRpcTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
/* USER CODE END Application */

