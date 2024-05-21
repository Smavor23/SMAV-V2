/* USER CODE BEGIN Includes */
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
#include <stdio.h>
/* USER CODE END Includes */

void NVIC_SystemReset(void); // Assuming this function exists for microcontroller reset

/* USER CODE BEGIN 0 */
const char apn[]  = "live.vodafone.com"; // Change this to your Provider details
const char server[] = "nizarmohideen.atwebpages.com"; // Change this to your domain
const int  port = 80;
const char resource[] = "/insert.php";
const uint32_t timeOut =10000;
char content[80];
char ATcommand[80];
uint8_t Gsm_buffer[100] = {0};
uint8_t ATisOK = 0;
uint8_t CGREGisOK = 0;
uint8_t CIPOPENisOK = 0;
uint8_t NETOPENisOK = 0;
uint32_t previousTick;
uint16_t distance;

void SIMTransmit(char *cmd)
{
  memset(Gsm_buffer,0,sizeof(Gsm_buffer));
  HAL_UART_Transmit(&huart3,(uint8_t *)cmd,strlen(cmd),1000);
  HAL_UART_Receive (&huart3, Gsm_buffer, 100, 1000);
}

void httpPost(void)
{
  ATisOK = 0;
  CGREGisOK = 0;
  NETOPENisOK = 0;
  CIPOPENisOK = 0;
  // Check for OK response for AT 
  previousTick =  HAL_GetTick();
  while(!ATisOK && previousTick  + timeOut >  HAL_GetTick())
  {
    SIMTransmit("AT\r\n");
    HAL_Delay(1000);
    if(strstr((char *)Gsm_buffer,"OK"))
    {
      ATisOK = 1;
    }
  }

  // Check for network registration. 
  if(ATisOK)
  {
    previousTick =  HAL_GetTick();
    while(!CGREGisOK  && previousTick  + timeOut >  HAL_GetTick())
    {
      SIMTransmit("AT+CGREG?\r\n");
      if(strstr((char *)Gsm_buffer,"+CGREG: 0,1"))
      {
        CGREGisOK = 1;
      }
    }
  }

  // Check for Internet IP Connection
  if(ATisOK && CGREGisOK)
  {
    previousTick =  HAL_GetTick();
    while(!NETOPENisOK  && previousTick  + timeOut >  HAL_GetTick())
    {
      SIMTransmit("AT+NETCLOSE\r\n");
      sprintf(ATcommand,"AT+CGDCONT=1,\"IP\",\"%s\",\"0.0.0.0\",0,0\r\n",apn);
      SIMTransmit(ATcommand);
      SIMTransmit("AT+CIPMODE=0\r\n");
      SIMTransmit("AT+CIPSENDMODE=0\r\n");
      SIMTransmit("AT+CIPCCFG=10,0,0,0,1,0,75000\r\n");
      SIMTransmit("AT+CIPTIMEOUT=75000,15000,15000\r\n");
      SIMTransmit("AT+NETOPEN\r\n");
      SIMTransmit("AT+NETOPEN?\r\n");
      if(strstr((char *)Gsm_buffer,"+NETOPEN: 1"))
      {
        NETOPENisOK = 1;
      }
    }
  }

  // Check for TCP connection
  if(ATisOK && CGREGisOK && NETOPENisOK)
  {
    SIMTransmit("AT+IPADDR\r\n");
    previousTick =  HAL_GetTick();
    while(!CIPOPENisOK  && previousTick  + timeOut >  HAL_GetTick())
    {
      SIMTransmit("AT+CIPCLOSE=0\r\n");
      sprintf(ATcommand,"AT+CIPOPEN=0,\"TCP\",\"%s\",%d\r\n",server,port);
      SIMTransmit(ATcommand);
      HAL_Delay(1000);
      if(strstr((char *)Gsm_buffer,"+CIPOPEN: 0,0"))
      {
        CIPOPENisOK = 1;
      }
    }
  }

  // If all Connection success (Wiring, Registration and TCP/IP)
  if(ATisOK && CGREGisOK && CIPOPENisOK && NETOPENisOK)
  {
    // Perform http request
    sprintf(ATcommand,"AT+CIPSEND=0,%d\r\n",strlen(resource)+16);
    SIMTransmit(ATcommand);
    if(strstr((char *)Gsm_buffer,">"))
    {
      sprintf(ATcommand,"POST %s HTTP/1.1\r\n",resource);
      SIMTransmit(ATcommand);
    }

    sprintf(ATcommand,"AT+CIPSEND=0,%d\r\n",strlen(server)+8);
    SIMTransmit(ATcommand);
    if(strstr((char *)Gsm_buffer,">"))
    {
      sprintf(ATcommand,"Host: %s\r\n",server);
      SIMTransmit(ATcommand);
    }

    SIMTransmit("AT+CIPSEND=0,19\r\n");
    if(strstr((char *)Gsm_buffer,">"))
    {
      SIMTransmit("Connection: close\r\n");
    }

    SIMTransmit("AT+CIPSEND=0,49\r\n");
    if(strstr((char *)Gsm_buffer,">"))
    {
      SIMTransmit("Content-Type: application/x-www-form-urlencoded\r\n");
    }

    SIMTransmit("AT+CIPSEND=0,16\r\n");
    if(strstr((char *)Gsm_buffer,">"))
    {
      SIMTransmit("Content-Length: ");
    }

    char sLength[5];
    snprintf(sLength, 5,"%d",strlen(content));
    sprintf(ATcommand,"AT+CIPSEND=0,%d\r\n",strlen(sLength));
    SIMTransmit(ATcommand);
    if(strstr((char *)Gsm_buffer,">"))
    {
      SIMTransmit(sLength);
    }

    SIMTransmit("AT+CIPSEND=0,2\r\n");
    if(strstr((char *)Gsm_buffer,">"))
    {
      SIMTransmit("\r\n");
    }

    SIMTransmit("AT+CIPSEND=0,2\r\n");
    if(strstr((char *)Gsm_buffer,">"))
    {
      SIMTransmit("\r\n");
    }

    sprintf(ATcommand,"AT+CIPSEND=0,%d\r\n",strlen(content));
    SIMTransmit(ATcommand);
    if(strstr((char *)Gsm_buffer,">"))
    {
      SIMTransmit(content);
    }

    SIMTransmit("AT+CIPSEND=0,2\r\n");
    if(strstr((char *)Gsm_buffer,">"))
    {
      SIMTransmit("\r\n");
    }
    HAL_Delay(2000);
    // Close connections
    SIMTransmit("AT+CIPCLOSE=0\r\n");
    SIMTransmit("AT++NETCLOSE\r\n");
  }
}
/* USER CODE END 0 */
