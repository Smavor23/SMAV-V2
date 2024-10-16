#ifndef TRXGPS_TCP_H
#define TRXGPS_TCP_H
#ifdef __cplusplus
 extern "C" {
#endif

#include "main.h"
	
 	static char simSignal[] = "AT+CREG?";
    static char apnContext[] = "AT+CGSOCKCONT=1,\"IP\",\"internet.itelcel.com\"";
    static char apnAuth[]    = "AT+CSOCKAUTH=1,1,\"webgprs2002\",\"webgprs\"";
    static char apnProfile[] = "AT+CSOCKSETPN=1";
    static char tcpContext[] = "AT+CIPMODE=0";
    static char tcpTimeout[] = "AT+CIPTIMEOUT=120000,120000,120000";
    static char tcpOpen[]    = "AT+NETOPEN";
    static char ipAddress[]  = "AT+IPADDR";
    //static char tcpConnect[] = "AT+CIPOPEN=0,\"TCP\",\"t.trxgps.com\",700";
    static char tcpConnect[] = "AT+CIPOPEN=0,\"TCP\",\"52.41.58.18\",700";
    static char tcpLength[]  = "AT+CIPSEND=0,27";
    static char tcpLengthMulti[]  = "AT+CIPSEND=0,270";
    static char tcpMessage[] = "HELLO";
    static char tcpResponse[] = "AT+CIPSRIP=1";
    static char tcpDisconnect[] = "AT+CIPCLOSE=0";
    static char tcpClose[] = "AT+NETCLOSE";


#ifdef __cplusplus
}
#endif
#endif
