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
	loopIndex++;
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
	DEBUG4_PRINTLN("mqtt.publish3");

	if (!isConnected)
		return false;

	DEBUG4_PRINTLN("mqtt.publish3 will send");

	DEBUG4_PRINTLN("mqtt.p3: topic=\"" + topic +"\"");
	DEBUG4_PRINTF("mqtt.p3: index=%d, ", index);
	DEBUG4_PRINTLN("msg=\"" + message +"\"");

	return MqttClient::publish(getTopic(topic, index, direction), message);
}

String MQTT::getTopic(String topic, MessageDirection direction) {
	return (topicMain + (direction==IN?"/in/":"/out/") + topicClient + topic);
}

String MQTT::getTopic(String topic, byte index, MessageDirection direction) {
	return (topicMain + (direction==IN?"/in/":"/out/") + topicClient + topic + String(index));
}

String MQTT::getName() {
	return this->nameClient;
}

String MQTT::getUptime() {
	unsigned int uptime = loopIndex / (60000/timer_interval);
	int months = uptime / 43200;

	uptime %= 43200;
	int weeks = uptime / 10080;

	uptime %= 10080;
	int days = uptime / 1440;

	uptime %= 1440;
	int hours = uptime / 60;

	uptime %= 60;

	String result = "";

	if (months > 0) {
		result += String(months);
		if (months > 1)
			result += " months ";
		else
			result += " month ";
	}

	if (weeks > 0) {
		result += String(weeks);
		if (weeks > 1)
			result += " weeks ";
		else
			result += " week ";
	}

	if (days > 0) {
		result += String(days);
		if (days > 1)
			result += " days ";
		else
			result += " day ";
	}

	if (hours > 0) {
		result += String(hours);
		if (hours > 1)
			result += " hours ";
		else
			result += " hour ";
	}

	if (uptime > 0) {
		result += String(uptime);
		if (uptime > 1)
			result += " minutes ";
		else
			result += " minute ";
	}

	if ((months == 0) && (weeks == 0) && (days == 0) && (hours == 0) && (uptime == 0))
		result += "less than a minute";

	return result;
}
