/*
 * Module.h
 *
 *  Created on: 25 февр. 2016 г.
 *      Author: Nikita
 */

#include <SmingCore/SmingCore.h>
#include <Libraries/DHT/DHT.h>
#include <Libraries/OneWire/OneWire.h>
#include <Libraries/DS18S20/ds18s20.h>
#include <Libraries/BMP180/BMP180.h>
#include <MQTT.h>
#include <Logger.h>
#include <AppSettings.h>

#ifndef INCLUDE_MODULE_H_
#define INCLUDE_MODULE_H_

#define DEFAULT_SHIFT 		100
#define DEFAULT_INTERVAL 	60000

class Sensor;
class SensorDHT;
class SensorBMP;
class SensorDS;

class Sensor
{
protected:
	Timer timer;
	unsigned int timer_shift;
	unsigned int timer_interval;
	bool needCompute = true;

	MQTT* mqtt;

	void start();
	Sensor();
	Sensor(unsigned int shift, unsigned int interval, MQTT &mqtt);

public:

	virtual ~Sensor();
	static const int undefined = -120;
	void setShift(unsigned int shift);
	void setInterval(unsigned int interval);
	void setTimer(unsigned int shift, unsigned int interval);

	virtual void loop();
	virtual void publish();
	virtual void compute();

	void startTimer();
	void stopTimer();

	void setMqtt(MQTT &mqtt);
};

class SensorDHT: protected DHT, public Sensor {

private:
	float temperature = undefined;
	float humidity = undefined;
	void init();

public:
	SensorDHT(MQTT &mqtt, byte dhtType = DHT22);
	SensorDHT(byte pin, byte dhtType, MQTT &mqtt, unsigned int shift = DEFAULT_SHIFT, unsigned int interval = DEFAULT_INTERVAL);
	~SensorDHT();
	void compute();
	void publish();
	float getTemperature();
	float getHumidity();
};

class SensorBMP: protected BMP180, public Sensor {

private:
	float temperature = undefined;
	long pressure = undefined;
	void init(byte scl, byte sda);

public:
	SensorBMP(MQTT &mqtt);
	SensorBMP(byte scl, byte sda, MQTT &mqtt, unsigned int shift = DEFAULT_SHIFT, unsigned int interval = DEFAULT_INTERVAL);
	~SensorBMP();
	void compute();
	void publish();
	float getTemperature();
	long getPressure();
};

class SensorDS: protected OneWire, public Sensor {

private:
	byte count = 0;
	float* temperature;
	float readDCByAddr(byte addr[]);
	void init(byte count);

public:
	SensorDS(MQTT &mqtt, byte count = 1);
	SensorDS(byte pin, byte count, MQTT &mqtt, unsigned int shift = DEFAULT_SHIFT, unsigned int interval = DEFAULT_INTERVAL);

	~SensorDS();
	void compute();
	void publish();
	byte getCount();
	float getTemperature(byte num);
	void print();
};

class SensorDSS: protected DS18S20, public Sensor {

private:
	void init(byte pin);

public:
	SensorDSS(MQTT &mqtt);
	SensorDSS(byte pin, MQTT &mqtt, unsigned int shift = DEFAULT_SHIFT, unsigned int interval = DEFAULT_INTERVAL);

	~SensorDSS();
	void compute();
	void publish();
	byte getCount();
	float getTemperature(byte num);
	void print();
};

#endif /* INCLUDE_MODULE_H_ */
