/*
 * Module.h
 *
 *  Created on: 25 февр. 2016 г.
 *      Author: Nikita
 */

#include <SmingCore/SmingCore.h>
#include <Libraries/DHT/DHT.h>
#include <Libraries/OneWire/OneWire.h>
#include <Libraries/BMP180/BMP180.h>
#include <Logger.h>

#ifndef INCLUDE_MODULE_H_
#define INCLUDE_MODULE_H_

#define DEFAULT_SHIFT 		100
#define DEFAULT_INTERVAL 	60000

class Sensor {
protected:
	Timer timer;
	unsigned int timer_shift;
	unsigned int timer_interval;
	void start();
	Sensor();
	Sensor(unsigned int shift, unsigned int interval);

public:

	virtual ~Sensor();
	static const int undefined = -120;
	void setShift(unsigned int shift);
	void setInterval(unsigned int interval);
	void setTimer(unsigned int shift, unsigned int interval);

	virtual void compute();
	void startTimer();
	void stopTimer();
};

class SensorDHT:Sensor {

private:
	float temperature=undefined;
	float humidity=undefined;
	DHT *dht;
	void init(byte pin, byte dhtType);

public:
	SensorDHT(byte pin, byte dhtType);
	SensorDHT(unsigned int shift, unsigned int interval, byte pin, byte dhtType);
	~SensorDHT();
	void compute();
	float getTemperature();
	float getHumidity();
};



class SensorBMP:Sensor {

private:
	BMP180 barometer;
	float temperature = undefined;
	long pressure = undefined;
	void init(byte scl, byte sda);

public:
	SensorBMP(byte pin, byte count);
	SensorBMP(unsigned int shift, unsigned int interval, byte pin, byte count);
	~SensorBMP();
	void compute();
	float getTemperature();
	long getPressure();
};



class SensorDS:Sensor {

private:
	byte count = 0;
	float *temperature;
	OneWire *ds;
	float readDCByAddr(byte addr[]);
	void init(byte pin, byte count);

public:
	SensorDS(byte pin, byte count);
	SensorDS(unsigned int shift, unsigned int interval, byte pin, byte count);
	~SensorDS();
	void compute();
	byte getCount();
	float getTemperature(byte num);
};


#endif /* INCLUDE_MODULE_H_ */
