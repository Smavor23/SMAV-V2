/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define P9V_EN_Pin GPIO_PIN_15
#define P9V_EN_GPIO_Port GPIOB
#define P3V3_OFF_Pin GPIO_PIN_15
#define P3V3_OFF_GPIO_Port GPIOA
#define RLY_1_Pin GPIO_PIN_13
#define RLY_1_GPIO_Port GPIOC
#define RLY_2_Pin GPIO_PIN_14
#define RLY_2_GPIO_Port GPIOC
#define RLY_3_Pin GPIO_PIN_15
#define RLY_3_GPIO_Port GPIOC
#define DEBUG_TX_Pin GPIO_PIN_2
#define DEBUG_TX_GPIO_Port GPIOA
#define DEBUG_RX_Pin GPIO_PIN_3
#define DEBUG_RX_GPIO_Port GPIOA
#define EEPROM_WP_Pin GPIO_PIN_7
#define EEPROM_WP_GPIO_Port GPIOB
#define ADC_1_Pin GPIO_PIN_5
#define ADC_1_GPIO_Port GPIOA
#define BULB_EN_Pin GPIO_PIN_7
#define BULB_EN_GPIO_Port GPIOA
#define ADC_2_Pin GPIO_PIN_0
#define ADC_2_GPIO_Port GPIOB
#define ADC_3_Pin GPIO_PIN_1
#define ADC_3_GPIO_Port GPIOB
#define GAS_SENSE_Pin GPIO_PIN_2
#define GAS_SENSE_GPIO_Port GPIOB
#define SMS_TX_Pin GPIO_PIN_10
#define SMS_TX_GPIO_Port GPIOB
#define SMS_RX_Pin GPIO_PIN_11
#define SMS_RX_GPIO_Port GPIOB
#define GPO_B1_A_Pin GPIO_PIN_12
#define GPO_B1_A_GPIO_Port GPIOB
#define GPO_B2_A_Pin GPIO_PIN_13
#define GPO_B2_A_GPIO_Port GPIOB
#define GPO_B3_A_Pin GPIO_PIN_15
#define GPO_B3_A_GPIO_Port GPIOB
#define WIND_SPEED_Pin GPIO_PIN_8
#define WIND_SPEED_GPIO_Port GPIOA
#define SMS_RST_Pin GPIO_PIN_9
#define SMS_RST_GPIO_Port GPIOA
#define SMS_RING_Pin GPIO_PIN_10
#define SMS_RING_GPIO_Port GPIOA
#define SMS_DTR_Pin GPIO_PIN_11
#define SMS_DTR_GPIO_Port GPIOA
#define BATT_MON_EN_Pin GPIO_PIN_12
#define BATT_MON_EN_GPIO_Port GPIOA
#define SIM_EN_Pin GPIO_PIN_9
#define SIM_EN_GPIO_Port GPIOA
#define SPI_CS_Pin GPIO_PIN_6
#define SPI_CS_GPIO_Port GPIOB
#define P4V_EN_Pin GPIO_PIN_12
#define P4V_EN_GPIO_Port GPIOA
#define SCL_Pin GPIO_PIN_8
#define SCL_GPIO_Port GPIOB
#define SDA_Pin GPIO_PIN_9
#define SDA_GPIO_Port GPIOB

/* HTTP SERVICE ENVIROMENT ex: "url/id" */ 
#define HTTP_URL "http://smav1-machines.herokuapp.com/postdata/"
#define ID_DEVICE "12345"
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
