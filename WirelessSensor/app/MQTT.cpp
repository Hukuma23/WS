/*
 * MQTT.cpp
 *
 *  Created on: 04 марта 2016 г.
 *      Author: Nikita
 */

#include <MQTT.h>


void MQTT::init(IPAddress broker_ip, int broker_port, unsigned int shift, unsigned int interval) {
	this->timer_shift = shift;
	this->timer_interval = interval;

	this->broker_ip = broker_ip.toString();
	this->broker_port = (broker_port==0?1883:broker_port);
	// MQTT client
	mqtt = new MqttClient(this->broker_ip, this->broker_port, MqttStringSubscriptionCallback(&MQTT::onMessageReceived, this));

}

MQTT::MQTT(IPAddress broker_ip, int broker_port) {
	init(broker_ip, broker_port);
}

MQTT::MQTT(IPAddress broker_ip, int broker_port, unsigned int shift, unsigned int interval) {
	init(broker_ip, broker_port, shift, interval);
}

MQTT::MQTT(IPAddress broker_ip, int broker_port, unsigned int shift, unsigned int interval, String topicMain, String topicClient) {
	init(broker_ip, broker_port, shift, interval);
	setTopic(topicMain, topicClient);

}

void MQTT::setTopic(String topicMain, String topicClient) {
	this->topicMain = topicMain;
	this->topicClient = topicClient;
	this->topicSubscr = this->topicMain + "/in/#";
	this->nameClient = topicClient + "esp8266-" + String(micros() & 0xffff, 16);
}

MQTT::~MQTT() {
	delete mqtt;
}

void MQTT::startTimer() {
	timer.initializeMs(timer_shift, TimerDelegate(&MQTT::start, this)).startOnce();
	connect();
}

void MQTT::stopTimer() {
	timer.stop();
}

void MQTT::start() {
	timer.initializeMs(timer_interval, TimerDelegate(&MQTT::loop,this)).start();
}

void MQTT::setShift(unsigned int shift) {
	timer_shift=shift;
}

void MQTT::setInterval(unsigned int interval) {
	timer_interval=interval;
}
void MQTT::setTimer(unsigned int shift, unsigned int interval) {
	timer_shift=shift;
	timer_interval=interval;
}

byte MQTT::connect() {
	DEBUG4_PRINTLN("_mqttConnect");
	//DEBUG4_PRINT("_mqtt_broker_ip=");
	//DEBUG4_PRINTLN(mqtt->server);
	//DEBUG4_PRINT("_mqtt_port=");
	//DEBUG4_PRINTLN(mqtt->port);

	// Run MQTT client
	byte state = mqtt->getConnectionState();
	DEBUG4_PRINT("MQTT.state=");
	DEBUG4_PRINTLN(state);

	if (state != eTCS_Connected) {
		mqtt->connect(this->nameClient);
		DEBUG4_PRINTLN("Connecting to MQTT broker");
		mqtt->subscribe(topicSubscr);
	}
	else if (isFirstTime) {
		publish("start", OUT, "1");
		publish("log", OUT, nameClient);
		DEBUG4_PRINTLN("published mqtt START!");
		isFirstTime = false;
	}

	return state;
}

void MQTT::disconnect() {
	DEBUG4_PRINTLN("_mqttDisconnect");
	mqtt->close();
}


bool MQTT::publish(String fullTopic, String message) {
	connect();
	return mqtt->publish(fullTopic, message);
}

bool MQTT::publish(String topic, MessageDirection direction, String message) {
	connect();
	String fullTopic = topicMain + (direction==IN?"/in/":"/out/") + topicClient + topic;
	return mqtt->publish(fullTopic, message);
}

bool MQTT::publish(String topic, byte index, MessageDirection direction, String message) {
	connect();
	String fullTopic = topicMain + (direction==IN?"/in/":"/out/") + topicClient + topic + String(index);
	return mqtt->publish(fullTopic, message);
}


