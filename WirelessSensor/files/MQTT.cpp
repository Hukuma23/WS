/*
 * MQTT.cpp
 *
 *  Created on: 04 марта 2016 г.
 *      Author: Nikita
 */

//#include <MQTT.h>

void MQTT::init(String broker_ip, int broker_port, unsigned int shift, unsigned int interval, MqttStringSubscriptionCallback delegateFunction) {
	this->timer_shift = shift;
	this->timer_interval = interval;
	this->broker_ip = broker_ip;
	this->broker_port = (broker_port==0?1883:broker_port);

	setCallback(delegateFunction);

	// MQTT client
	mqtt = new MqttClient(this->broker_ip, this->broker_port, delegate_callback);

}

/*

*/



MQTT::~MQTT() {
	delete mqtt;
}






