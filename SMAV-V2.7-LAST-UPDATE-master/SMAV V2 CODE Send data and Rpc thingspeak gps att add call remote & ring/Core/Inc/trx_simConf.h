#ifndef	_TRX_SIMCONF_H
#define	_TRX_SIMCONF_H


//	0: No DEBUG				1:High Level Debug .Use printf		2:All RX Data.Use printf
#ifndef COMPANY
	#define COMPANY	1
#endif

#ifndef VERSION
	#define VERSION	4
#endif

#ifndef DEVICE
	#define DEVICE	1

#endif

#define	_SIM_DEBUG				        0					

#define	_SIM_USART				        huart3

#define	_SIM_BUFFER_SIZE			    1024

#define	_SMS_BUFFER_SIZE			    256
#define	_AUX_BUFFER_SIZE			    50
#define _ALM_BUFFER_SIZE					180

#define _SIM_DMA_TRANSMIT        		0


#endif
