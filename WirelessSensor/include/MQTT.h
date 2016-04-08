/*
 * MQTT.h
 *
 *  Created on: 04 марта 2016 г.
 *      Author: Nikita
 */

#include <SmingCore/SmingCore.h>
#include <Logger.h>


#ifndef INCLUDE_MQTT_H_
#define INCLUDE_MQTT_H_

#define DEFAULT_SHIFT 		100
#define DEFAULT_INTERVAL 	60000

typedef enum {
	IN ,
	OUT
} MessageDirection;


struct MQTT {
private:
	String broker_ip;
	int broker_port;
	MqttClient* mqtt;

	Timer timer;
	unsigned int timer_shift;
	unsigned int timer_interval;

	String topicMain;
	String topicClient;
	String topicSubscr;
	String nameClient;

	bool isFirstTime = true;

	TimerDelegate delegate_loop = nullptr;
	MqttStringSubscriptionCallback delegate_callback = nullptr;

	MQTT(String broker_ip, int broker_port, MqttStringSubscriptionCallback delegateFunction = NULL) {
		init(broker_ip, broker_port, delegateFunction);
	}

	MQTT(String broker_ip, int broker_port, unsigned int shift, unsigned int interval, MqttStringSubscriptionCallback delegateFunction = NULL) {
		init(broker_ip, broker_port, shift, interval, delegateFunction);
	}

	MQTT(String broker_ip, int broker_port, unsigned int shift, unsigned int interval, String topicMain, String topicClient, MqttStringSubscriptionCallback delegateFunction = NULL) {
		init(broker_ip, broker_port, shift, interval, delegateFunction);
		setTopic(topicMain, topicClient);
	}


	byte connect() {
		if (!mqtt)
			return -1;

		DEBUG4_PRINTLN("mqtt.Connect");
		DEBUG4_PRINT("_mqtt_broker_ip=");
		DEBUG4_PRINTLN(mqtt->server);
		DEBUG4_PRINT("_mqtt_port=");
		DEBUG4_PRINTLN(mqtt->port);

		DEBUG4_PRINT("nameClient=");
		DEBUG4_PRINTLN(this->nameClient);

		DEBUG4_PRINT("topicSubscr=");
		DEBUG4_PRINTLN(this->topicSubscr);

		// Run MQTT client
		byte state = mqtt->getConnectionState();
		DEBUG4_PRINT("MQTT.state=");
		DEBUG4_PRINTLN(state);

		if (state != eTCS_Connected) {
			DEBUG4_PRINT("state != eTCS_Connected");
			mqtt->connect(this->nameClient);
			DEBUG4_PRINTLN("Connecting to MQTT broker");
			mqtt->subscribe(topicSubscr);
		}
		else if (isFirstTime) {
			DEBUG4_PRINT("isFirstTime");
			publish("start", OUT, "1");
			publish("log", OUT, nameClient);
			DEBUG4_PRINTLN("published mqtt START!");
			isFirstTime = false;
		}

		DEBUG4_PRINTLN("mqtt.Connect.done");
		return state;
	}


	bool isConnected() {
		if (connect() == eTCS_Connected)
			return true;
		return false;
	}

	void disconnect() {
		DEBUG4_PRINTLN("_mqttDisconnect");
		mqtt->close();
	}


	void start() {
		timer.initializeMs(timer_interval, TimerDelegate(&MQTT::loop, this)).start();
	}

	void loop() {
		//connect();
		delegate_loop();
	}




	void setCallback(TimerDelegate delegateFunction) {
		if (delegateFunction)
			this->delegate_loop = delegateFunction;
	}

	void setCallback(MqttStringSubscriptionCallback delegateFunction) {
		if (delegateFunction)
			this->delegate_callback = delegateFunction;
	}

public:

	MQTT() {};
	virtual ~MQTT() {
		delete mqtt;
	}

	void init(String broker_ip, int broker_port, unsigned int shift = DEFAULT_SHIFT, unsigned int interval = DEFAULT_INTERVAL, MqttStringSubscriptionCallback delegateFunction = NULL) {
		DEBUG4_PRINTLN("*** MQTT init ***");
		this->timer_shift = shift;
		this->timer_interval = interval;
		this->broker_ip = broker_ip;
		this->broker_port = (broker_port==0?1883:broker_port);

		setCallback(delegateFunction);

		// MQTT client
		mqtt = new MqttClient(this->broker_ip, this->broker_port, delegate_callback);
		DEBUG4_PRINTLN("*** MQTT init done ***");

	}

	void startTimer(TimerDelegate delegate_loop = NULL) {
		setCallback(delegate_loop);
		timer.initializeMs(timer_shift, TimerDelegate(&MQTT::start, this)).startOnce();
		connect();
	}

	void stopTimer() {
		timer.stop();
	}

	void setTimer(unsigned int shift, unsigned int interval) {
		timer_shift=shift;
		timer_interval=interval;
	}

	void setTopic(String topicMain, String topicClient) {
		this->topicMain = topicMain;
		this->topicClient = topicClient;
		this->topicSubscr = this->topicMain + "/in/#";
		this->nameClient = topicClient + "esp8266-" + String(micros() & 0xffff, 16);
	}

	bool publish(String fullTopic, String message) {
		if (isConnected())
			return mqtt->publish(fullTopic, message);
		return false;
	}

	bool publish(String topic, MessageDirection direction, String message) {
		if (isConnected()) {
			String fullTopic = getTopic(topic, direction);
			return mqtt->publish(fullTopic, message);
		}
		return false;
	}

	bool publish(String topic, byte index, MessageDirection direction, String message) {
		if (isConnected()) {
			String fullTopic = getTopic(topic, index, direction);
			return mqtt->publish(fullTopic, message);
		}
		return false;
	}

	String getTopic(String topic, byte index, MessageDirection direction) {
		return (topicMain + (direction==IN?"/in/":"/out/") + topicClient + topic + String(index));
	}

	String getTopic(String topic, MessageDirection direction) {
		return (topicMain + (direction==IN?"/in/":"/out/") + topicClient + topic);
	}



	byte getState() {
		return connect();
	}
};

static MQTT objMQTT;

#endif /* INCLUDE_MQTT_H_ */
