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


class MQTT {
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

	void init(String broker_ip, int broker_port, unsigned int shift = DEFAULT_SHIFT, unsigned int interval = DEFAULT_INTERVAL);
	byte connect();
	void disconnect();

	void loop();
	void start();



public:
	MQTT(String broker_ip, int broker_port);
	MQTT(String broker_ip, int broker_port, unsigned int shift, unsigned int interval);
	MQTT(String broker_ip, int broker_port, unsigned int shift, unsigned int interval, String topicMain, String topicClient);
	virtual ~MQTT();


	void setShift(unsigned int shift);
	void setInterval(unsigned int interval);
	void setTimer(unsigned int shift, unsigned int interval);

	void startTimer();
	void stopTimer();


	void setTopic(String topicMain, String topicClient);


	bool publish(String fullTopic, String message);
	bool publish(String topic, MessageDirection direction, String message);
	bool publish(String topic, byte index, MessageDirection direction, String message);
	void onMessageReceived(String topic, String message); // Forward declaration for our callback

};


#endif /* INCLUDE_MQTT_H_ */
