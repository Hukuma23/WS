/*
 * Module.cpp
 *
 *  Created on: 01 марта 2016 г.
 *      Author: Nikita
 */

#include <Module.h>

// Sensor
Sensor::Sensor() : timer_shift(DEFAULT_SHIFT), timer_interval(DEFAULT_INTERVAL), mqtt(NULL) {}

Sensor::Sensor(unsigned int shift, unsigned int interval, MQTT* &mqtt) : timer_shift(shift), timer_interval(interval), mqtt(mqtt) {}

void Sensor::start() {
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
	timer.initializeMs(timer_shift, TimerDelegate(&Sensor::start, this)).startOnce();
}

void Sensor::stopTimer() {
	timer.stop();
}

void Sensor::setMqtt(MQTT* &mqtt) {
	this->mqtt = mqtt;
}

void Sensor::loop() {
	if (needCompute) {
		compute();
		needCompute = !needCompute;
	}
	else {
		publish();
		needCompute = !needCompute;
	}
}


// SensorDHT
SensorDHT::~SensorDHT() {}

void SensorDHT::init(byte pin, byte dhtType) {
	DHT:begin();
}

SensorDHT::SensorDHT(byte pin, byte dhtType) : DHT(pin, dhtType), Sensor(){
	init(pin, dhtType);
}

SensorDHT::SensorDHT(byte pin, byte dhtType, MQTT* &mqtt, unsigned int shift, unsigned int interval) : DHT(pin, dhtType), Sensor(shift, interval, mqtt) {
	init(pin, dhtType);
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
		result = mqtt->publish("dhtTemperature", OUT, String(temperature));

		if (result)
			temperature = undefined;
	}

	if (humidity != undefined) {
		result = mqtt->publish("dhtHumidity", OUT, String(humidity));

		if (result)
			humidity = undefined;
	}
}

// SensorBMP
SensorBMP::~SensorBMP() {}

void SensorBMP::init(byte scl, byte sda) {
	Wire.pins(scl, sda);
	Wire.begin();
}

SensorBMP::SensorBMP(byte scl, byte sda) : BMP180(), Sensor(){
	init(scl, sda);

};

SensorBMP::SensorBMP(byte scl, byte sda, MQTT* &mqtt, unsigned int shift, unsigned int interval) : BMP180(), Sensor(shift, interval, mqtt) {
	init(scl, sda);
};

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
		result = mqtt->publish("bmpTemperature", OUT, String(temperature));

		if (result)
			temperature = undefined;
	}

	if (pressure != undefined) {
		result = mqtt->publish("bmpPressure", OUT, String(pressure));

		if (result)
			pressure = undefined;
	}
}

// SensorDS
SensorDS::~SensorDS() {
	delete temperature;
}

void SensorDS::init(byte pin, byte count) {
	OneWire::begin();
	this->count = count;
	temperature = new float[this->count];
}

SensorDS::SensorDS(byte pin, byte count) : OneWire(pin), Sensor(){
	init(pin, count);
};

SensorDS::SensorDS(byte pin, byte count, MQTT* &matt, unsigned int shift, unsigned int interval) : OneWire(pin), Sensor(shift, interval, mqtt) {
	init(pin, count);
};

void SensorDS::compute() {

	DEBUG4_PRINTLN("_readOneWire");

	byte addr[8];
	byte num = 0;
	float celsius, fahrenheit;
	system_soft_wdt_stop();
	while (OneWire::search(addr)) {
		celsius = readDCByAddr(addr);

		if (num < (this->count-1)) {
			if (celsius > -100.0)
				temperature[num++] = celsius;
			else
				temperature[num++] = undefined;
		}
		else
			ERROR_PRINTLN("Error: can't save temperature value, enhanced max sensors count");
	}

	system_soft_wdt_restart();
	return;
}

void SensorDS::publish() {
	DEBUG4_PRINTLN("_publishDS");
	if (mqtt == NULL) {
		return;
	}

	bool result;

	for (byte i = 0; i < count; i++) {
		if (temperature[i] != undefined) {
			result = mqtt->publish("dsTemperature", i, OUT, String(temperature[i]));
			if (result)
				temperature[i] = undefined;
		}
	}
}

float SensorDS::readDCByAddr(byte addr[]) {

	DEBUG4_PRINTLN("_readDCByAddr");
	byte i;
	byte cnt = 1;
	byte present = 0;
	byte type_s;
	byte data[12];
	float celsius, fahrenheit;

	DEBUG4_PRINT("Thermometer ROM =");
	for (i = 0; i < 8; i++) {
		DEBUG4_WRITE(' ');
		DEBUG4_PRINTHEX(addr[i]);
	}

	if (OneWire::crc8(addr, 7) != addr[7]) {
		DEBUG4_PRINTLN("CRC is not valid!");
		return -255;
	}
	DEBUG4_PRINTLN();

	// the first ROM byte indicates which chip
	switch (addr[0]) {
	case 0x10:
		DEBUG4_PRINTLN("  Chip = DS18S20"); // or old DS1820
		type_s = 1;
		break;
	case 0x28:
		DEBUG4_PRINTLN("  Chip = DS18B20");
		type_s = 0;
		break;
	case 0x22:
		DEBUG4_PRINTLN("  Chip = DS1822");
		type_s = 0;
		break;
	default:
		DEBUG4_PRINTLN("Device is not a DS18x20 family device.");
		return -255;
	}

	OneWire::reset();
	OneWire::select(addr);
	OneWire::write(0x44, 1); // start conversion, with parasite power on at the end

	delay(1000); // maybe 750ms is enough, maybe not
	// we might do a ds.depower() here, but the reset will take care of it.

	present = OneWire::reset();
	OneWire::select(addr);
	OneWire::write(0xBE); // Read Scratchpad

	DEBUG4_PRINT("  Data = ");
	DEBUG4_PRINTHEX(present);
	DEBUG4_PRINT(" ");
	for (i = 0; i < 9; i++) {
		// we need 9 bytes
		data[i] = OneWire::read();
		DEBUG4_PRINTHEX(data[i]);
		DEBUG4_PRINT(" ");
	}
	DEBUG4_PRINT(" CRC=");
	DEBUG4_PRINTHEX(OneWire::crc8(data, 8));
	DEBUG4_PRINTLN();

	// Convert the data to actual temperature
	// because the result is a 16 bit signed integer, it should
	// be stored to an "int16_t" type, which is always 16 bits
	// even when compiled on a 32 bit processor.
	int16_t raw = (data[1] << 8) | data[0];
	if (type_s) {
		raw = raw << 3; // 9 bit resolution default
		if (data[7] == 0x10) {
			// "count remain" gives full 12 bit resolution
			raw = (raw & 0xFFF0) + 12 - data[6];
		}
	} else {
		byte cfg = (data[4] & 0x60);
		// at lower res, the low bits are undefined, so let's zero them
		if (cfg == 0x00)
			raw = raw & ~7; // 9 bit resolution, 93.75 ms
		else if (cfg == 0x20)
			raw = raw & ~3; // 10 bit res, 187.5 ms
		else if (cfg == 0x40)
			raw = raw & ~1; // 11 bit res, 375 ms
		//// default is 12 bit resolution, 750 ms conversion time
	}

	celsius = (float) raw / 16.0;
	fahrenheit = celsius * 1.8 + 32.0;
	DEBUG4_PRINT("  Temperature = ");
	DEBUG4_PRINT(celsius);
	DEBUG4_PRINT(" Celsius, ");
	DEBUG4_PRINTLN();

	return celsius;

}

byte SensorDS::getCount() {
	return (count-1);
}

float SensorDS::getTemperature(byte num) {
	if (num < (count))
		return temperature[num];
	else
		return undefined;
}