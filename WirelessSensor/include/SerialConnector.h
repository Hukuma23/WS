/*
 * SerialConnector.h
 *
 *  Created on: 17 мая 2016 г.
 *      Author: Nikita
 */

#ifndef INCLUDE_SERIALCONNECTOR_H_
#define INCLUDE_SERIALCONNECTOR_H_

#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include <SerialGuaranteedDeliveryProtocol.h>
#include <AppSettings.h>
#include <ActStates.h>
#include <MQTT.h>

class SerialConnector: protected SerialGuaranteedDeliveryProtocol {

private:
	MQTT* mqtt;
	AppSettings& appSettings;
	ActStates& actStates;

	Timer timerSerialCollector;
	Timer timerSerialReceiver;

	// Serial
	float sDSTemp[3];
	float sDHTTemp, sDHTHum;
	float sBMPTemp, sBMPPress;
	int16_t sWaterCold, sWaterHot;

public:
	SerialConnector(HardwareSerial* serial, MQTT &mqtt);
	~SerialConnector();

	void stopSerialCollector();
	void stopSerialReceiver();
	void stopListener();
	void stopTimers();

	void startListener();
	uint8_t virtual sendSerialMessage(uint8_t cmd, uint8_t objType, uint8_t objId, uint8_t sw = 0);


	void IRAM_ATTR turnSsw(byte num, bool state);

	void serialCollector();
	void serialReceiver();

	void setSerialCollector();
	void setSerialReceiver();

	void startSerialCollector();
	void startSerialReceiver();

	void readSensors(SerialMessage payload);
	void readSwitches(SerialMessage payload);

	void processMessage();

	void publishSerialSensors();
	void publishSerialSwitches();

	void publish();

	bool processCallback(String topic, String message);





};


#endif /* INCLUDE_SERIALCONNECTOR_H_ */
