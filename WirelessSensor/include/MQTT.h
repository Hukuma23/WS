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


class MQTT: protected MqttClient{
private:
	Timer timer;
	unsigned int timer_shift;
	unsigned int timer_interval;
	TimerDelegate delegate_loop;

	String topicMain;
	String topicClient;
	String topicSubscr;
	String nameClient;

	bool isConnected = false;

	void init(String broker_ip, int broker_port, unsigned int shift = DEFAULT_SHIFT, unsigned int interval = DEFAULT_INTERVAL, MqttStringSubscriptionCallback delegate_callback = NULL);


	void loop();
	void start();

	byte connect();
	void disconnect();


public:

	MQTT(String broker_ip, int broker_port, MqttStringSubscriptionCallback delegate_callback = NULL);
	MQTT(String broker_ip, int broker_port, unsigned int shift, unsigned int interval, MqttStringSubscriptionCallback delegate_callback = NULL);
	MQTT(String broker_ip, int broker_port, unsigned int shift, unsigned int interval, String topicMain, String topicClient, MqttStringSubscriptionCallback delegate_callback = NULL);
	virtual ~MQTT();

	TcpClientState getConnectionState() { return MqttClient::getConnectionState(); }

	void setShift(unsigned int shift);
	void setInterval(unsigned int interval);
	void setTimer(unsigned int shift, unsigned int interval);

	void startTimer(TimerDelegate delegate_loop);
	void stopTimer();

	void setTopic(String topicMain, String topicClient);


	bool publish(String fullTopic, String message);
	bool publish(String topic, MessageDirection direction, String message);
	bool publish(String topic, byte index, MessageDirection direction, String message);

	String getTopic(String topic, MessageDirection direction);
	String getTopic(String topic, byte index, MessageDirection direction);

	String getName();

};


#endif /* INCLUDE_MQTT_H_ */
