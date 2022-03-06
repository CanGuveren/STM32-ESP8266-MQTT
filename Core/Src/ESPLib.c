/*
 * ESPLib.c
 *
 *  Created on: Feb 14, 2022
 *      Author: Ümit Can Güveren
 */


/*EKLENECEKLER*/
// mqtt connect yaparken password ve id ekleme, flag kontrolleri ve flag'e göre veri paketi oluşturma
// do-while döngülerinin içinde belli bir süre sonra işlemciye reset atma


#include <ESPLib.h>
#include "main.h"


char Buffer[BUFFERSIZE];
char rxBuffer[BUFFERSIZE];
char mqttBuffer[BUFFERSIZE];
char mqttPacket[100];
char temp_mqttBuffer[BUFFERSIZE];

extern UART_HandleTypeDef huart2;

uint8_t uartRxData;
uint8_t DataCounter;
uint8_t RxInterruptFlag;
uint8_t uartTimeCounter;
uint8_t uartPacketComplatedFlag;

/*Responses and Commands*/
char *OK = "OK\r\n";
char *WIFIDISCONNECT = "OK\r\nWIFI DISCONNECT\r\n"; //AT+CWQAP komutu karşılığı cihaz bir ağa bağlı ise verdiği cevap.
char *SEND_OK = "SEND OK\r\n";
char *ERROR_ = "ERROR\r\n";
char *CIPSEND_RESPONSE = "OK\r\n> ";


void sendData(char *cmd, uint8_t cmdSize, uint16_t timeout) //size'ı fonksiyonda al.
{
	HAL_StatusTypeDef uartCheck;

	memset(Buffer, 0, BUFFERSIZE);

	uartCheck = HAL_UART_Transmit_IT(&huart2, (uint8_t *)cmd, cmdSize); //AT ile ESP kontrolü yapılıyor.

	if(uartCheck == HAL_OK)
	{
		HAL_UART_Receive(&huart2, (uint8_t *)Buffer, cmdSize + 50, timeout); //Timeout fonksiyona gelecek.
	}

}

funcState_t ESP8266_Init(espMode_t Mode)
{
	funcState_t checkFunc;
	char cmd[100];
	uint8_t cmdSize;

	do{
	sendData("AT\r\n", strlen("AT\r\n"), 100);
	checkFunc = checkResponse(OK); //OK cevabı ,alınıyorsa ESP doğru şekilde çalışıyor
	}while(checkFunc != funcOk);

	do{
	memset(cmd, 0, 100);
	cmdSize = sprintf(cmd, "AT+CWMODE=%d\r\n", Mode); //Seçilen mode göre gönderilecek komut ayarlanır.
	sendData(cmd,cmdSize, 100);						//Komut gönderilir.
	checkFunc = checkResponse(OK); //OK cevabı alındıysa istenilen mode ayarlanmıştır.
	}while(checkFunc != funcOk);

	return funcOk;
}

funcState_t ESP8266_wifiConnect(char *SSID, char *PWD)
{
	funcState_t checkFunc;
	char cmd[100];
	uint8_t cmdSize;

	do{
	sendData("AT+CWQAP\r\n",strlen("AT+CWQAP\r\n"), 100);				//Cihaz herhangi bir ağa bağlı ise bağlantı kesilir.
	checkFunc = (checkResponse(OK) | checkResponse(WIFIDISCONNECT));	//Bağlantının kesildiği kontrol edilir  ve doğruysa döngüden çıkılır.
	}while(checkFunc != funcOk);

	do{
	memset(cmd, 0, 100);
	cmdSize = sprintf(cmd, "AT+CWJAP=\"%s\",\"%s\"\r\n", SSID, PWD);	//Ağ id ve şifresine göre gönderilecek komut ayarlanır.
	sendData(cmd, cmdSize, 15000);		   								//Gelen tüm metinleri almak için timeout süresi uzun tutulmuştur.
	checkFunc = checkResponse(OK); 										//OK cevabı alınıyorsa ağa bağlanılmıştır.
	}while(checkFunc != funcOk);

	return funcOk;
}

funcState_t ESP8266_portConnect(char *type, char *remoteIP, char *remotePort)
{
	funcState_t checkFunc;
	char cmd[100];
	uint8_t cmdSize;

	do{
	sendData("AT+CIPCLOSE\r\n", strlen("AT+CIPCLOSE\r\n") ,100);	//Cihaz herhangi bir ağa bağlı ise bağlantı kesilir.
	checkFunc = checkResponse(OK) | checkResponse(ERROR_);			//Bağlantının kesildiği kontrol edilir  ve doğruysa döngüden çıkılır.
	}while(checkFunc != funcOk);

	do{
	memset(cmd, 0, 100);
	cmdSize = sprintf(cmd, "AT+CIPSTART=\"%s\",\"%s\",%s\r\n", type, remoteIP, remotePort);			//Ağ id ve şifresine göre gönderilecek komut ayarlanır.
	sendData(cmd, cmdSize, 10000);		   //Gelen tüm metinleri almak için timeout süresi uzun tutulmuştur.
	checkFunc = checkResponse(OK); //OK cevabı alınıyorsa ağa bağlanılmıştır.
	}while(checkFunc != funcOk);

	return funcOk;
}

funcState_t ESP8266_sendMessage(char *msg, uint8_t msgSize)
{
	funcState_t checkFunc;
	char cmd[100];
	uint8_t cmdSize;


	do{
	memset(cmd, 0, 100);
    cmdSize = sprintf(cmd, "AT+CIPSEND=%d\r\n", msgSize);		//Ağ id ve şifresine göre gönderilecek komut ayarlanır.
    sendData(cmd, cmdSize, 100);
	checkFunc = checkResponse(CIPSEND_RESPONSE) | checkResponse(SEND_OK);
	}while(checkFunc != funcOk);

	if(checkFunc == funcOk)
	{
		sendData(msg, msgSize, 200); //500
	}

	/*
	cmdSize = sprintf(cmd, "AT+CIPSEND=%d\r\n", msgSize);
	HAL_UART_Transmit(&huart2, (uint8_t *)cmd, cmdSize, 100);
	HAL_Delay(100);
	HAL_UART_Transmit(&huart2, (uint8_t *)msg, msgSize, 100);
	 */
	return funcOk;
}


funcState_t checkResponse(char * response)
{
	uint8_t i;
	funcState_t checkState;
	uint8_t responseSize;
	uint8_t sizeBuffer;

	responseSize = strlen(response);

	sizeBuffer = strlen(Buffer);
	memcpy(rxBuffer, &Buffer[sizeBuffer - responseSize], responseSize);

	for(i = 0; i < responseSize; i++)
	{
		if(rxBuffer[i] != response[i])
		{
			checkState = funcErr;
			break;
		}
		else
		{
			checkState = funcOk;
		}

	}

	memset(rxBuffer, 0, BUFFERSIZE);
	return checkState;
}

/* MQTT Functions*/
void MQTT_connectBroker(uint8_t flag, uint16_t keepAlive, char *clientID)
{
	char *protocolName = "MQTT";

	uint16_t protocolLenght  = strlen(protocolName);
	uint16_t clientIDLenght = strlen(clientID);
	uint8_t remainingLenght = 8 + protocolLenght + clientIDLenght;
	uint8_t mqttLenght;

	memset(mqttPacket, 0, 100); // <- mqtt dizisini içeride tanımlamaı dene.

	mqttLenght = sprintf(mqttPacket, "%c%c%c%c%s%c%c%c%c%c%c%s", (char)MQTTconnect, (char)remainingLenght, (char)(protocolLenght >> 8), (char)(protocolLenght & 0x00FF),
						 protocolName,(char)MQTTlevel,(char)flag, (char)(keepAlive << 8), (char)keepAlive,(char)(clientIDLenght >> 8), (char)(clientIDLenght & 0x00FF),
						 clientID);

	ESP8266_sendMessage(mqttPacket, mqttLenght);

}

void MQTT_publishTopic(char *topic, char *msg)
{
	uint8_t mqttLenght;

	uint16_t topicLenght = strlen(topic);
	uint16_t msgLenght = strlen(msg);

	uint8_t remainingLenght = 2 + topicLenght + msgLenght;

	memset(mqttPacket, 0, 100);

	mqttLenght = sprintf(mqttPacket, "%c%c%c%c%s%s", (char)MQTTpublish, (char)remainingLenght, (char)topicLenght << 8, (char)topicLenght, topic, msg);

	HAL_UART_AbortReceive_IT(&huart2);

	ESP8266_sendMessage(mqttPacket, mqttLenght);   //burada değişiklikler yapıldı.

	HAL_UART_Receive_IT(&huart2, &uartRxData, 1);
}

void MQTT_subscribeTopic(char *topic, uint8_t QoS)
{
	uint8_t mqttLenght;

	uint16_t topicLenght = strlen(topic);

	uint8_t remainingLenght = 5 + topicLenght;

	uint16_t packetID = 0x01;
	memset(mqttPacket, 0, 100);

	mqttLenght = sprintf(mqttPacket, "%c%c%c%c%c%c%s%c", (char)MQTTsubscribe, (char)remainingLenght, (char)packetID << 8, (char)packetID, (char)topicLenght << 8, (char)topicLenght, topic, QoS);
	ESP8266_sendMessage(mqttPacket, mqttLenght);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{

	RxInterruptFlag = SET;
	mqttBuffer[DataCounter++] = uartRxData;
	if(DataCounter >= BUFFERSIZE)
	{
		DataCounter  = 0;
	}

	HAL_UART_Receive_IT(&huart2 , &uartRxData , 1);
	uartTimeCounter = 0;
}


void uartDataHandler(void)
{
	uint8_t tempCounter;

	if(uartPacketComplatedFlag == SET)     //Data receiving is finished
	{
		uartPacketComplatedFlag = RESET;
	    tempCounter = DataCounter;
		DataCounter = 0;
		memcpy(temp_mqttBuffer, mqttBuffer, tempCounter);
		memset(mqttBuffer, 0, BUFFERSIZE);
	}
}

/* This function should be called in systick timer */
void uartTimer(void)
{
	if(RxInterruptFlag == SET)
	{
		if(uartTimeCounter++ > 100)
		{

			RxInterruptFlag = RESET;
			uartTimeCounter = 0;
			uartPacketComplatedFlag = SET;
		}
	}
}

