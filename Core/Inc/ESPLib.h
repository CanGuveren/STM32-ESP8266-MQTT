/*
 * ESPLib.h
 *
 *  Created on: Feb 14, 2022
 *      Author: DELL
 */

#ifndef INC_ESPLIB_H_
#define INC_ESPLIB_H_

#include "stdbool.h"
#include "stdio.h"
#include "stdint.h"
#include "string.h"
#include "stdbool.h"

#define BUFFERSIZE 100

extern uint8_t uartRxData;
extern uint8_t DataCounter;
extern uint8_t RxInterruptFlag;
extern uint8_t uartTimeCounter;
extern uint8_t uartPacketComplatedFlag;

typedef enum
{
	funcErr,
	funcOk,
}funcState_t;

typedef enum
{
	StationMode = 1,
	AccessPointMode,
	StationAP,
}espMode_t;

/* MQTT defines*/
#define MQTTlevel		0x04
#define MQTTconnect 	0x10
#define MQTTlength	 	0x04
#define MQTTsubscribe 	0x82
#define MQTTpublish		0x30

void sendData(char *cmd, uint8_t cmdSize, uint16_t timeout);

/* ESP8266 Function */
funcState_t ESP8266_Init(espMode_t mode);
funcState_t ESP8266_wifiConnect(char *SSID, char *PWD);
funcState_t ESP8266_portConnect(char *type, char *remoteIP, char *remotePort);
funcState_t checkResponse(char * response);
funcState_t ESP8266_sendMessage(char *msg, uint8_t msgSize);

/* MQTT Function */
void MQTT_connectBroker(uint8_t flag, uint16_t keepAlive, char *clientID);
void MQTT_publishTopic(char *topic, char *msg);
void MQTT_subscribeTopic(char *topic, uint8_t QoS);
/* Receiver functions */
void uartDataHandler(void);
void uartTimer(void);
void transmitDataMake(char *msg, uint8_t Lenght);


#endif /* INC_ESPLIB_H_ */
