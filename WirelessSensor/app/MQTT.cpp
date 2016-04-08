/*
 * MQTT.cpp
 *
 *  Created on: 04 марта 2016 г.
 *      Author: Nikita
 */

#include <MQTT.h>

void MQTT::init(String broker_ip, int broker_port, unsigned int shift, unsigned int interval, MqttStringSubscriptionCallback delegate_callback) {
	this->timer_shift = shift;
	this->timer_interval = interval;

}

MQTT::MQTT(String broker_ip, int broker_port, MqttStringSubscriptionCallback delegate_callback):
		MqttClient(broker_ip, (broker_port==0?1883:broker_port), delegate_callback) {
	init(broker_ip, broker_port);
}

MQTT::MQTT(String broker_ip, int broker_port, unsigned int shift, unsigned int interval, MqttStringSubscriptionCallback delegate_callback):
		MqttClient(broker_ip, (broker_port==0?1883:broker_port), delegate_callback) {
	init(broker_ip, broker_port, shift, interval);
}

MQTT::MQTT(String broker_ip, int broker_port, unsigned int shift, unsigned int interval, String topicMain, String topicClient, MqttStringSubscriptionCallback delegate_callback):
		MqttClient(broker_ip, (broker_port==0?1883:broker_port), delegate_callback){
	init(broker_ip, broker_port, shift, interval);
	setTopic(topicMain, topicClient);
}

void MQTT::setTopic(String topicMain, String topicClient) {
	this->topicMain = topicMain;
	this->topicClient = topicClient;
	this->topicSubscr = this->topicMain + "/in/#";
	this->nameClient = topicClient + "esp8266-" + String(micros() & 0xffff, 16);
}

MQTT::~MQTT() {}

void MQTT::loop() {
	connect();
	delegate_loop();
}

void MQTT::startTimer(TimerDelegate delegate_loop) {
	this->delegate_loop = delegate_loop;
	timer.initializeMs(timer_shift, TimerDelegate(&MQTT::start, this)).startOnce();
	connect();	//!!!!!!!!
}

void MQTT::stopTimer() {
	timer.stop();
}

void MQTT::start() {
	timer.initializeMs(timer_interval, TimerDelegate(&MQTT::loop, this)).start();
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
	// Run MQTT client
	byte state = getConnectionState();
	DEBUG4_PRINT("mqtt.state=");
	DEBUG4_PRINTLN(state);

	if (state != eTCS_Connected) {
		MqttClient::connect(this->nameClient);
		DEBUG4_PRINTLN("Connecting to MQTT broker");
		MqttClient::subscribe(topicSubscr);
	}
	else if (!isConnected) {
		publish("start", OUT, "1");
		publish("log", OUT, nameClient);
		DEBUG4_PRINTLN("published mqtt START!");
		isConnected = true;
	}

	return state;
}


void MQTT::disconnect() {
	close();
}


bool MQTT::publish(String fullTopic, String message) {

	if (!isConnected)
		return false;

	return MqttClient::publish(fullTopic, message);
}

bool MQTT::publish(String topic, MessageDirection direction, String message) {
	if (!isConnected)
		return false;

	return MqttClient::publish(getTopic(topic, direction) , message);
}

bool MQTT::publish(String topic, byte index, MessageDirection direction, String message) {
	if (!isConnected)
		return false;

	return MqttClient::publish(getTopic(topic, index, direction), message);
}

String MQTT::getTopic(String topic, MessageDirection direction) {
	return (topicMain + (direction==IN?"/in/":"/out/") + topicClient + topic);
}

String MQTT::getTopic(String topic, byte index, MessageDirection direction) {
	return (topicMain + (direction==IN?"/in/":"/out/") + topicClient + topic + String(index));
}
