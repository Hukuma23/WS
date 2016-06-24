/*
 * Module.cpp
 *
 *  Created on: 01 марта 2016 г.
 *      Author: Nikita
 */

#include <Module.h>

// Sensor
Sensor::Sensor(AppSettings &appSettings) : timer_shift(DEFAULT_SHIFT), timer_interval(DEFAULT_INTERVAL), appSettings(appSettings) {
	this->mqtt = NULL;
}

Sensor::Sensor(unsigned int shift, unsigned int interval, MQTT &mqtt, AppSettings &appSettings) : timer_shift(shift), timer_interval(interval), appSettings(appSettings) {
	this->mqtt = &mqtt;
}

void Sensor::start() {
	DEBUG4_PRINTLN("Sensor::start!");
	timer.initializeMs(timer_interval, TimerDelegate(&Sensor::loop,this)).start();
}

void Sensor::setShift(unsigned int shift) {
	timer_shift=shift;
}

void Sensor::setInterval(unsigned int interval) {
	timer_interval=interval;
}
void Sensor::setTimer(unsigned int shift, unsigned int interval) {
	timer_shift=shift; timer_interval=interval;
}

void Sensor::startTimer() {
	DEBUG4_PRINTLN("Sensor::startTimer");
	timer.initializeMs(timer_shift, TimerDelegate(&Sensor::start, this)).startOnce();
}

void Sensor::stopTimer() {
	DEBUG4_PRINTLN("Sensor::stopTimer");
	timer.stop();
}

void Sensor::setMqtt(MQTT &mqtt) {
	this->mqtt = &mqtt;
}

void Sensor::loop() {
	DEBUG4_PRINTLN("Sensor::loop");
	if (needCompute) {
		compute();
		needCompute = !needCompute;
	}
	else {
		publish();
		needCompute = !needCompute;
	}
}

Sensor::~Sensor() {DEBUG4_PRINTLN("~Sensor");}
void Sensor::publish() {DEBUG4_PRINTLN("Sensor.publish()");};
void Sensor::compute() {DEBUG4_PRINTLN("Sensor.compute()");};


// SensorDHT
SensorDHT::~SensorDHT() {}

void SensorDHT::init() {
	DHT:begin();
}

SensorDHT::SensorDHT(MQTT &mqtt, AppSettings &appSettings, byte dhtType) : DHT(appSettings.dht, dhtType), Sensor(appSettings.shift_dht, appSettings.interval_dht, mqtt, appSettings){
	init();
}


SensorDHT::SensorDHT(byte pin, byte dhtType, MQTT &mqtt, AppSettings &appSettings, unsigned int shift, unsigned int interval) : DHT(pin, dhtType), Sensor(shift, interval, mqtt, appSettings) {
	init();
}

void SensorDHT::compute() {
	DEBUG4_PRINTLN("_readDHT");
	humidity = DHT::readHumidity();
	temperature = DHT::readTemperature();

	// check if returns are valid, if they are NaN (not a number) then something went wrong!
	if (isnan(temperature) || isnan(humidity)) {
		ERROR_PRINTLN("Error: Failed to read from DHT");
		temperature = undefined;
		humidity = undefined;
		return;
	} else {
		DEBUG4_PRINT("Humidity: ");
		DEBUG4_PRINT(humidity);
		DEBUG4_PRINT(" %\t");
		DEBUG4_PRINT("Temperature: ");
		DEBUG4_PRINT(temperature);
		DEBUG4_PRINTLN(" *C");
		return;
	}
}

float SensorDHT::getTemperature() {
	return temperature;
}

float SensorDHT::getHumidity() {
	return humidity;
}

void SensorDHT::publish() {
	DEBUG4_PRINTLN("_publishDHT");

	if (mqtt == NULL) {
		return;
	}

	bool result;

	if (temperature != undefined) {
		result = mqtt->publish(appSettings.topDHT_t, OUT, String(temperature));

		if (result)
			temperature = undefined;
	}

	if (humidity != undefined) {
		result = mqtt->publish(appSettings.topDHT_h, OUT, String(humidity));

		if (result)
			humidity = undefined;
	}
}

// SensorBMP
SensorBMP::~SensorBMP() {}

void SensorBMP::init(byte scl, byte sda) {
	DEBUG4_PRINTF("SensorBMP.init scl=%d, sda=%d", scl, sda); DEBUG4_PRINTLN();
	Wire.pins(scl, sda);
	Wire.begin();
}

SensorBMP::SensorBMP(MQTT &mqtt, AppSettings &appSettings) : BMP180(), Sensor(appSettings.shift_bmp, appSettings.interval_bmp, mqtt, appSettings){
	init(appSettings.scl, appSettings.sda);
}

SensorBMP::SensorBMP(byte scl, byte sda, MQTT &mqtt, AppSettings &appSettings, unsigned int shift, unsigned int interval) : BMP180(), Sensor(shift, interval, mqtt, appSettings) {
	init(scl, sda);
}

void SensorBMP::compute() {
	DEBUG4_PRINTLN("_readBarometer");

	if (!BMP180::EnsureConnected()) {
		DEBUG4_PRINTLN("Could not connect to BMP180.");
		return;
	}

	// When we have connected, we reset the device to ensure a clean start.
	//barometer.SoftReset();
	// Now we initialize the sensor and pull the calibration data.
	BMP180::Initialize();
	//BMP180::PrintCalibrationData();

	DEBUG4_PRINT("Start reading");

	// Retrive the current pressure in Pascals.
	pressure = BMP180::GetPressure();

	// Print out the Pressure.
	DEBUG4_PRINT("Pressure: ");
	DEBUG4_PRINT(pressure);
	DEBUG4_PRINT(" Pa");

	// Retrive the current temperature in degrees celcius.
	temperature = BMP180::GetTemperature();

	// Print out the Temperature
	DEBUG4_PRINT("\tTemperature: ");
	DEBUG4_PRINT(temperature);
	DEBUG4_WRITE(176);
	DEBUG4_PRINT("C");

	DEBUG4_PRINTLN(); // Start a new line.
	return;
}

float SensorBMP::getTemperature() {
	return temperature;
}

long SensorBMP::getPressure() {
	return pressure;
}

void SensorBMP::publish() {
	DEBUG4_PRINTLN("_publishBMP");

	if (mqtt == NULL) {
		return;
	}

	bool result;

	if (temperature != undefined) {
		result = mqtt->publish(appSettings.topBMP_t, OUT, String(temperature));

		if (result)
			temperature = undefined;
	}

	if (pressure != undefined) {
		result = mqtt->publish(appSettings.topBMP_p, OUT, String(pressure));

		if (result)
			pressure = undefined;
	}
}

// SensorDSS
SensorDSS::~SensorDSS() {
	//	delete temperature;
}

void SensorDSS::init(byte pin) {
	DS18S20::Init(pin);
}

SensorDSS::SensorDSS(MQTT &mqtt, AppSettings &appSettings) : DS18S20(), Sensor(appSettings.shift_ds, appSettings.interval_ds, mqtt, appSettings){
	init(appSettings.ds);
}

SensorDSS::SensorDSS(byte pin, MQTT &mqtt, AppSettings &appSettings, unsigned int shift, unsigned int interval) : DS18S20(), Sensor(shift, interval, mqtt, appSettings) {
	init(pin);
}

void SensorDSS::compute() {

	DEBUG4_PRINTLN("dss.compute");
	DS18S20::StartMeasure();
	return;
}

void SensorDSS::publish() {
	DEBUG4_PRINTLN("dss.publish");

	if (mqtt == NULL) {
		return;
	}

	DEBUG4_PRINTLN("ds.publish mqtt != null");
	bool result;
	byte count = DS18S20::GetSensorsCount();
	uint64_t ds_id;

	if ((!DS18S20::MeasureStatus()) && (count > 0)) {

		for (byte i = 0; i < count; i++) {
			DEBUG4_PRINTF("ds.publish i=%d, temp=", i);
			DEBUG4_PRINTLN(DS18S20::IsValidTemperature(i));
			ds_id = DS18S20::GetSensorID(i)>>32;
			DEBUG4_PRINTHEX((uint32_t)ds_id);

			if (DS18S20::IsValidTemperature(i)) {
				result = mqtt->publish(appSettings.topDS_t, i, OUT, String(DS18S20::GetCelsius(i)));
				DEBUG4_PRINTF("result = %d", result);
				DEBUG4_PRINTLN();

				//if (result) temperature[i] = undefined;
			}
		}
	}

	DEBUG4_PRINTLN();
}


byte SensorDSS::getCount() {
	return (DS18S20::GetSensorsCount());
}

