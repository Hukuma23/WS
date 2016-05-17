/*
 *
 * Nikita Litvinov
 * 2016/05/17
 *
 * Развитие класса для взаимодействия по Serial протоколу между ESP8266 и Arduino
 *
 */

#include <SerialConnector.h>

SerialConnector::SerialConnector(HardwareSerial* serial, MQTT &mqtt) : SerialGuaranteedDeliveryProtocol(serial) {
	this->mqtt = &mqtt;

	setProcessing(ProcessionDelegate(&SerialConnector::processMessage, this));

	sDHTTemp = UNDEF;
	sDHTHum = UNDEF;
	sBMPTemp =  UNDEF;
	sBMPPress =  UNDEF;
	sWaterCold =  UNDEF;
	sWaterHot =  UNDEF;

	sDSTemp[0] = UNDEF;
	sDSTemp[1] = UNDEF;
	sDSTemp[2] = UNDEF;
}

void SerialConnector::startSerialCollector() {
	timerSerialCollector.initializeMs(AppSettings.shift_collector, TimerDelegate(&SerialConnector::setSerialCollector, this)).startOnce();
}

void SerialConnector::stopSerialCollector() {
	timerSerialCollector.stop();
}

void SerialConnector::startSerialReceiver() {
	timerSerialReceiver.initializeMs(AppSettings.shift_receiver, TimerDelegate(&SerialConnector::setSerialReceiver, this)).startOnce();
}

void SerialConnector::stopSerialReceiver() {
	timerSerialReceiver.stop();
}

void SerialConnector::stopListener() {
	stopListening();
}

void SerialConnector::startListener() {
	SerialGuaranteedDeliveryProtocol::startListener(AppSettings.interval_listener);
}

uint8_t SerialConnector::sendSerialMessage(uint8_t cmd, uint8_t objType, uint8_t objId, uint8_t sw) {
	SerialGuaranteedDeliveryProtocol::sendSerialMessage(cmd, objType, objId, sw);
}

void SerialConnector::serialCollector() {
	SerialGuaranteedDeliveryProtocol::sendSerialMessage(SerialCommand::COLLECT, ObjectType::ALL, ObjectId::ALL);
	mqtt->publish(AppSettings.topLog, OUT, "serialCollector() cmd = " + String(SerialCommand::COLLECT) + " objType=" + String(ObjectType::ALL) + " objId=" + String(ObjectId::ALL));
	publish();
}

void SerialConnector::serialReceiver() {
	SerialGuaranteedDeliveryProtocol::sendSerialMessage(SerialCommand::GET, ObjectType::ALL, ObjectId::ALL);
	mqtt->publish(AppSettings.topLog, OUT, "serialReceiver() cmd = " + String(SerialCommand::GET) + " objType=" + String(ObjectType::ALL) + " objId=" + String(ObjectId::ALL));
}


void SerialConnector::setSerialCollector() {
	if (AppSettings.is_serial) {
		timerSerialCollector.initializeMs(AppSettings.interval_collector, TimerDelegate(&SerialConnector::serialCollector, this)).start();
		DEBUG4_PRINTLN("*** Serial Collector timer done!");
	}
}

void SerialConnector::setSerialReceiver() {
	if (AppSettings.is_serial) {
		timerSerialReceiver.initializeMs(AppSettings.interval_receiver, TimerDelegate(&SerialConnector::serialReceiver, this)).start();
		DEBUG4_PRINTLN("*** Serial Receiver timer done!");
	}
}


void IRAM_ATTR SerialConnector::turnSsw(byte num, bool state) {
	if (state == HIGH) {
		ActStates.setSsw(num, HIGH);
		SerialGuaranteedDeliveryProtocol::sendSerialMessage(SerialCommand::SET_HIGH, ObjectType::SWITCH, num+1);
	} else {
		ActStates.setSsw(num, LOW);
		SerialGuaranteedDeliveryProtocol::sendSerialMessage(SerialCommand::SET_LOW, ObjectType::SWITCH, num+1);
	}
}

void SerialConnector::readSensors(SerialMessage payload) {
	sWaterCold = payload.water_cold;
	sWaterHot = payload.water_hot;

	sDHTTemp = (float)payload.dht_temp *0.01;
	sDHTHum = (float)payload.dht_hum *0.01;

	sDSTemp[0] = (float)payload.ds1 *0.01;
	sDSTemp[1] = (float)payload.ds2 *0.01;
	//dsTemp[2] = (float)payload.ds3 *0.01;
}

void SerialConnector::readSwitches(SerialMessage payload) {
	bool state;
	uint8_t sw = payload.sw;


	for (byte i = 0; i < AppSettings.ssw_cnt; i++) {
		state = ((sw & (int)powf(2, i)) == 1)?HIGH:LOW;

		DEBUG1_PRINTF("CHECK: readSwitches %d, state = ", i);
		DEBUG1_PRINTLN(state);

		if (state != ActStates.getSsw(i))
			ActStates.setSsw(i, state);
	}
	/* !!! Требует тщательной проверки !!!
	state =  ((sw & 1) == 1)?HIGH:LOW;
	if (state != ActStates.getSsw().ssw1)
		ActStates.setSsw1(state);

	state = (((sw & 2)  >> 1) == 1)?HIGH:LOW;
	if (state != ActStates.ssw2)
		ActStates.setSsw2(state);

	state = (((sw & 4)  >> 2) == 1)?HIGH:LOW;
	if (state != ActStates.ssw3)
		ActStates.setSsw3(state);

	state = (((sw & 8)  >> 3) == 1)?HIGH:LOW;
	if (state != ActStates.ssw4)
		ActStates.setSsw4(state);

	state = (((sw & 16) >> 4) == 1)?HIGH:LOW;
	if (state != ActStates.ssw5)
		ActStates.setSsw5(state);
	 */
}

void SerialConnector::processMessage() {
	// when message received correctly this method will run

	uint8_t cmd = getPayloadCmd();
	uint8_t objType = getPayloadObjType();
	uint8_t objId = getPayloadObjId();
	//blink(SWITCH_PIN2, 1, 10);

	mqtt->publish(AppSettings.topLog, OUT, "processMessage() cmd = " + String(cmd) + " objType=" + String(objType) + " objId=" + String(objId));

	/*
    DEBUG4_PRINTLN();
    DEBUG4_PRINT("cmd=");
    DEBUG4_PRINT(cmd);
    DEBUG4_PRINT(" type=");
    DEBUG4_PRINT(objType);
    DEBUG4_PRINT(" id=");
    DEBUG4_PRINTLN(objId);
	 */
	/*
	if (objType == ObjectType::SWITCH) {
		if ((objId == ObjectId::SWITCH_1) || (objId == ObjectId::ALL)) {
			if (cmd == SerialCommand::SET_LOW) {
				digitalWrite(AppSettings.sw1, LOW);
			} else if (cmd == SerialCommand::SET_HIGH) {
				digitalWrite(AppSettings.sw1, HIGH);
			}
		}
	}
	 */

	if (cmd == SerialCommand::RETURN) {
		SerialMessage pl = getPayload();
		if (objType == ObjectType::SENSORS) {
			readSensors(pl);
		}
		else if (objType == ObjectType::SWITCH) {
			readSwitches(pl);
		}
		else if (objType == ObjectType::ALL) {
			readSensors(pl);
			readSwitches(pl);
		}

	}
}

void SerialConnector::publishSerialSensors() {

	bool result;

	// Publish DHT
	if (sDHTTemp != UNDEF) {
		result = mqtt->publish("sdht_t", OUT, String(sDHTTemp));	//result = mqtt.publish(getOutTopic("sdht_t"), String(sDHTTemp));
		if (result)
			sDHTTemp = UNDEF;
	}

	if (sDHTHum != UNDEF) {
		result = mqtt->publish("sdht_h", OUT, String(sDHTHum));	//result = mqtt.publish(getOutTopic("sdht_h"), String(sDHTHum));
		if (result)
			sDHTHum = UNDEF;
	}

	// Publish BMP
	if (sBMPTemp != UNDEF) {
		result = mqtt->publish("sbmp_t", OUT, String(sBMPTemp));	//result = mqtt.publish(getOutTopic("sbmp_t"), String(sBMPTemp));
		if (result)
			sBMPTemp = UNDEF;
	}
	if (sBMPPress != UNDEF) {
		result = mqtt->publish("sbmp_p", OUT, String(sBMPPress));	//result = mqtt.publish(getOutTopic("sbmp_p"), String(sBMPPress));
		if (result)
			sBMPPress = UNDEF;
	}

	// Publish DS
	int dsSize = sizeof sDSTemp / sizeof sDSTemp[0];
	for (int i=0; i < dsSize; i++) {
		if (sDSTemp[i] != UNDEF) {
			result = mqtt->publish("sds_t", i, OUT, String(sDSTemp[i]));	//result = mqtt.publish(getOutTopic("sds_t") + String(i), String(sDSTemp[i]));
			if (result)
				sDSTemp[i] = UNDEF;
		}
	}

	// Publish Water
	if ((sWaterCold > 0) && (sWaterCold != UNDEF)) {
		result = mqtt->publish("s_wc", OUT, String(sWaterCold));	//result = mqtt.publish(getOutTopic("s_wc"), String(sWaterCold));
		if (result)
			sWaterCold = UNDEF;
	}
	if ((sWaterHot > 0) && (sWaterHot != UNDEF)) {
		result = mqtt->publish("s_wh", OUT, String(sWaterHot));	//result = mqtt.publish(getOutTopic("s_wh"), String(sWaterHot));
		if (result)
			sWaterHot = UNDEF;
	}

}


void SerialConnector::publishSerialSwitches() {

	for (byte i = 0; i < AppSettings.ssw_cnt; i++) {
		if (ActStates.ssw[i])
			mqtt->publish(AppSettings.topSSW, (i+1), OUT, "ON");	//mqtt.publish(sTopSw_Out+String(i+1), "ON");

		else
			mqtt->publish(AppSettings.topSSW, (i+1), OUT, "OFF");	//mqtt.publish(sTopSw_Out+String(i+1), "OFF");
	}

	for (byte i = 0; i < AppSettings.ssw_cnt; i++) {
		DEBUG4_PRINTF("swState%d is ", i);
		DEBUG4_PRINTLN(ActStates.ssw[i]);
	}
}

void SerialConnector::publish() {
	publishSerialSwitches();
	publishSerialSensors();
}
