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

MQTT::~MQTT() {
	DEBUG4_PRINTLN("mqtt.~MQTT");
}

void MQTT::loop() {
	connect();
	delegate_loop();
}

void MQTT::startTimer(TimerDelegate delegate_loop) {
	DEBUG4_PRINTLN("mqtt.startTimer");
	this->delegate_loop = delegate_loop;
	timer.initializeMs(timer_shift, TimerDelegate(&MQTT::start, this)).startOnce();
	connect();	//!!!!!!!!
}

void MQTT::stopTimer() {
	DEBUG4_PRINTLN("mqtt.stopTimer");
	timer.stop();
}

void MQTT::start() {
	DEBUG4_PRINTLN("mqtt.start");
	timer.initializeMs(timer_interval, TimerDelegate(&MQTT::loop, this)).start();
}

void MQTT::setShift(unsigned int shift) {
	DEBUG4_PRINTLN("mqtt.setShift");
	timer_shift=shift;
}

void MQTT::setInterval(unsigned int interval) {
	DEBUG4_PRINTLN("mqtt.setInterval");
	timer_interval=interval;
}
void MQTT::setTimer(unsigned int shift, unsigned int interval) {
	DEBUG4_PRINTLN("mqtt.setTimer");
	timer_shift=shift;
	timer_interval=interval;
}

byte MQTT::connect() {
	DEBUG4_PRINTLN("mqtt.connect");
	//DEBUG4_PRINT("_mqtt_broker_ip=");
	//DEBUG4_PRINTLN(mqtt->server);
	//DEBUG4_PRINT("_mqtt_port=");
	//DEBUG4_PRINTLN(mqtt->port);

	// Run MQTT client
	byte state = getConnectionState();
	DEBUG4_PRINT("state=");
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
	DEBUG4_PRINTLN("mqtt.disconnect");
	close();
}

/*
bool MQTT::publish(String fullTopic, String message) {
	DEBUG4_PRINTLN("mqtt.publish1");

	if (!isConnected)
		return false;

	return MqttClient::publish(fullTopic, message);
}
*/
bool MQTT::publish(String topic, MessageDirection direction, String message) {
	DEBUG4_PRINTLN("mqtt.publish2");

	if (!isConnected)
		return false;

	String fullTopic = topicMain + (direction==IN?"/in/":"/out/") + topicClient + topic;
	return MqttClient::publish(fullTopic, message);
}

bool MQTT::publish(String topic, byte index, MessageDirection direction, String message) {
	DEBUG4_PRINTLN("mqtt.publish3");

	if (!isConnected)
		return false;

	String fullTopic = topicMain + (direction==IN?"/in/":"/out/") + topicClient + topic + String(index);
	return MqttClient::publish(fullTopic, message);
}

/*
bool MQTT::connect(String clientName) {
	DEBUG4_PRINTLN("mqttClient.connect");
	return MqttClient::connect(clientName);
}

bool MQTT::subscribe(String topic) {
	DEBUG4_PRINTLN("mqttClient.subscribe");
	return MqttClient::subscribe(topic);
}


String MQTT::getServer() {
	return server;
}

int MQTT::getPort() {
	return port;
}
*/
