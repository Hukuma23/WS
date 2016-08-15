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
#include <Libraries/MCP23017/MCP23017.h>
#include <MQTT.h>
#include <Logger.h>
#include <AppSettings.h>
#include <HardwareSerial.h>
//#include <LCD1602.h>

#ifndef INCLUDE_MODULE_H_
#define INCLUDE_MODULE_H_

#define DEFAULT_SHIFT 		100
#define DEFAULT_INTERVAL 	60000
#define MAX_CRC_ERRORS		10

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
	AppSettings& appSettings;

	void start();
	Sensor(AppSettings &appSettings);
	Sensor(unsigned int shift, unsigned int interval, MQTT &mqtt, AppSettings &appSettings);

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

class SensorMHZ: public Sensor {

private:
	int co2 = undefined;
	byte crc_error_cnt = 0;
	byte cmd[9] = {0xFF,0x01,0x86,0x00,0x00,0x00,0x00,0x00,0x79};
	//unsigned char response[9];
	char response[9];

	HardwareSerial *serial;
	int readCO2();
	byte sendSerial(byte *buff, byte size);


public:
	SensorMHZ(MQTT &mqtt, AppSettings &appSettings, HardwareSerial *serial);
	//SensorMHZ19(HardwareSerial &serial, MQTT &mqtt, AppSettings &appSettings, unsigned int shift = DEFAULT_SHIFT, unsigned int interval = DEFAULT_INTERVAL);

	~SensorMHZ();
	void compute();
	void publish();
	int getCO2();
};

class SensorDHT: protected DHT, public Sensor {

private:
	float temperature = undefined;
	float humidity = undefined;
	void init();

public:
	SensorDHT(MQTT &mqtt, AppSettings &appSettings, byte dhtType = DHT22);
	SensorDHT(byte pin, byte dhtType, MQTT &mqtt, AppSettings &appSettings, unsigned int shift = DEFAULT_SHIFT, unsigned int interval = DEFAULT_INTERVAL);
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
	SensorBMP(MQTT &mqtt, AppSettings &appSettings);
	SensorBMP(byte scl, byte sda, MQTT &mqtt, AppSettings &appSettings, unsigned int shift = DEFAULT_SHIFT, unsigned int interval = DEFAULT_INTERVAL);
	~SensorBMP();
	void compute();
	void publish();
	float getTemperature();
	long getPressure();
};

class SensorDSS: protected DS18S20, public Sensor {

private:
	void init(byte pin);

public:
	SensorDSS(MQTT &mqtt, AppSettings &appSettings);
	SensorDSS(byte pin, MQTT &mqtt, AppSettings &appSettings, unsigned int shift = DEFAULT_SHIFT, unsigned int interval = DEFAULT_INTERVAL);

	~SensorDSS();
	void compute();
	void publish();
	byte getCount();
	float getTemperature(byte num);
	void print();
};

#endif /* INCLUDE_MODULE_H_ */
