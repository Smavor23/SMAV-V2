#ifndef	_DS18B20CONFIG_H
#define	_DS18B20CONFIG_H


//	Init timer on cube    1us per tick				example 72 MHz cpu >>> Prescaler=(72-1)      counter period=Max 
//###################################################################################
#define	_DS18B20_USE_FREERTOS		    				1
#define _DS18B20_MAX_SENSORS		    				1
#define	_DS18B20_GPIO												GPIOB
#define	_DS18B20_PIN												TEMP_2_Pin

#define	_DS18B20_CONVERT_TIMEOUT_MS					5000		
#if (_DS18B20_USE_FREERTOS==1)
#define	_DS18B20_UPDATE_INTERVAL_MS					5000					//  (((	if==0  >> Ds18b20_ManualConvert()  )))    ((( if>0  >>>> Auto refresh ))) 
#endif


#define	_DS18B20_TIMER											htim2						
//###################################################################################

#endif


