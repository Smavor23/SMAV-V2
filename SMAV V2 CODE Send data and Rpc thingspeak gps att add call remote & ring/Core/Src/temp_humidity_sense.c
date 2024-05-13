/*
 * temp_humidity_sense.c
 *
 *  Created on: 15 ene. 2022
 *      Author: LUIS
 */

#include "main.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "Replay_Builder.h"
#include "i2c.h"
//#include "string.h"

static const uint8_t TMP102_ADDR = 0x48 << 1; // Use 8-bit address
static const uint8_t REG_TEMP = 0x00;
static const uint8_t REG_HUMY = 0x01;

char replyBuffer[128];
uint8_t I2Cbuff[4];

uint16_t getSensorData(uint8_t add)
{
	int16_t val;

	I2Cbuff[0] = add;
	ret = HAL_I2C_Master_Transmit(&hi2c1, TMP102_ADDR, I2Cbuff, 1, 250);
	if ( ret != HAL_OK )
	{
		val = 0x00;
	}
	else
	{
		// Read 2 bytes from the temperature register
		ret = HAL_I2C_Master_Receive(&hi2c1, TMP102_ADDR, I2Cbuff, 2, 250);
		if ( ret != HAL_OK )
		{
			val = 0x00;
		} else {

		//Combine the bytes
		val = ((int16_t)I2Cbuff[0] << 4) | (I2Cbuff[1] >> 4);

		// Convert to 2's complement, since temperature can be negative
		if ( val > 0x7FF )
		{
		  val |= 0xF000;
		}
	}
	return val;
}

char* getTemperatureString()
{
	int16_t val;
	float temp_c;

	val = getSensorData(REG_TEMP);

	// Convert to float temperature value (Celsius)
	temp_c = val * 6.25;

	// Convert temperature to decimal format
	sprintf((char*)replyBuffer,
	  "%u.%u C\r\n",
	  ((unsigned int)temp_c / 100),
	  ((unsigned int)temp_c % 100));
	return replyBuffer;
}

char* getHumidityString()
{
	int16_t val;
	float humy_c;

	val = getSensorData(REG_HUMY);

	// Convert to float temperature value (Celsius)
	humy_c = val * 6.25;

	// Convert temperature to decimal format
	sprintf((char*)replyBuffer,
	  "%u.%u %\r\n",
	  ((unsigned int)humy_c / 100),
	  ((unsigned int)humy_c % 100));
	return replyBuffer;
}

char* getDewPointString()
{
	int16_t val1, val2;
	float dew_c;

	val1 = getSensorData(REG_TEMP);
	val2 = getSensorData(REG_HUMY);

	// Dew point formula
	dew_c = val1 * val2;

	sprintf((char*)replyBuffer,
	  "%u.%u %\r\n",
	  ((unsigned int)dew_c / 100),
	  ((unsigned int)dew_c % 100));

	return replyBuffer;

}
