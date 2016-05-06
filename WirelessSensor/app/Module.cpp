/*
 * Module.cpp
 *
 *  Created on: 01 марта 2016 г.
 *      Author: Nikita
 */

#include <Module.h>

// Sensor
Sensor::Sensor() : timer_shift(DEFAULT_SHIFT), timer_interval(DEFAULT_INTERVAL) {
	this->mqtt = NULL;
}

Sensor::Sensor(unsigned int shift, unsigned int interval, MQTT &mqtt) : timer_shift(shift), timer_interval(interval) {
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


// SwIn

SwIn::~SwIn() {};

SwIn::SwIn(MQTT &mqtt) : MCP23017(), Sensor(AppSettings.shift_mcp, AppSettings.interval_mcp, mqtt){
	init(AppSettings.scl, AppSettings.sda);
}

void SwIn::init(byte scl, byte sda) {
	DEBUG4_PRINTLN("SwIn.init");
	Wire.pins(scl, sda);
	Wire.begin();

	MCP23017::begin(0);

	pinMode(AppSettings.m_int, INPUT);


	DEBUG4_PRINTLN("ASt1");
	DEBUG4_PRINTF("ASet.msw_cnt=%d   ", AppSettings.msw_cnt);
	DEBUG4_PRINTF("ASt.msw_cnt=%d   ", ActStates.getMswCnt());


	for (byte i=0; i < AppSettings.msw_cnt; i++) {
		DEBUG4_PRINTF2("ASet.msw[%d]=%d   ", i, AppSettings.msw[i]);
		DEBUG4_PRINTF2("ASt.msw[%d]=%d   ", i, ActStates.msw[i]);
		MCP23017::pinMode(AppSettings.msw[i], OUTPUT);
		MCP23017::digitalWrite(AppSettings.msw[i], ActStates.msw[i]);
	}

	DEBUG4_PRINTLN("ASt2");
	MCP23017::setupInterrupts(true, false, LOW);

	for (byte i=0; i < AppSettings.min_cnt; i++) {
		MCP23017::pinMode(AppSettings.min[i], INPUT);
		//MCP23017::pullUp(AppSettings.min[i], HIGH);
		MCP23017::setupInterruptPin(AppSettings.min[i], FALLING);
	}
	DEBUG4_PRINTLN("ASt3");

	//TimerDelegate(&Sensor::loop,this)
	attachInterrupt(AppSettings.m_int, Delegate<void()>(&SwIn::interruptCallback, this), FALLING);
	DEBUG4_PRINTLN("ASt4");
	interruptReset();
}

void SwIn::interruptReset() {
	DEBUG4_PRINT("swin.iReset:  ");
	uint8_t pin = MCP23017::getLastInterruptPin();
	uint8_t last_state = MCP23017::getLastInterruptPinValue();
	DEBUG4_PRINTF2("pin=%d state=%d. ", pin, last_state);
	DEBUG4_PRINTLN();
}

void SwIn::interruptCallback() {
	DEBUG4_PRINTLN("Interrupt Called! ");
	detachInterrupt(AppSettings.m_int);

	btnTimer.initializeMs(AppSettings.debounce_time, TimerDelegate(&SwIn::interruptHandler, this)).startOnce();
}

void SwIn::interruptHandler() {
	//awakenByInterrupt = true;

	pin = MCP23017::getLastInterruptPin();
	uint8_t last_state = MCP23017::getLastInterruptPinValue();
	uint8_t act_state = MCP23017::digitalRead(pin);
	//Serial.printf("spent time=%d", (millis() - intTime));
	//Serial.println();

	DEBUG4_PRINTF2("push pin=%d state=%d. ", pin, act_state);


	if (act_state == LOW)
		btnTimer.initializeMs(AppSettings.long_time, TimerDelegate(&SwIn::longtimeHandler, this)).startOnce();
	else {
		attachInterrupt(AppSettings.m_int, Delegate<void()>(&SwIn::interruptCallback, this), FALLING);
		DEBUG4_PRINTLN();
	}

}

void SwIn::longtimeHandler() {

	uint8_t act_state = MCP23017::digitalRead(pin);
	while (!(MCP23017::digitalRead(pin)));
	DEBUG4_PRINTF2("MCP push pin=%d state=%d. ", pin, act_state);

	byte num = AppSettings.getMINbyPin(pin);

	if (act_state == LOW) {
		DEBUG4_PRINTLN("*-long pressed-*");
		turnSw(num);
		if (mqtt != NULL)
			mqtt->publish(AppSettings.topMIN, num, OUT, String("LONG"));
	}
	else {
		DEBUG4_PRINTLN("*-pressed-*");
		turnSw(num);
		if (mqtt != NULL)
			mqtt->publish(AppSettings.topMIN, num, OUT, String("PRSD"));
	}


	attachInterrupt(AppSettings.m_int, Delegate<void()>(&SwIn::interruptCallback, this), FALLING);
}

void SwIn::compute() {
	interruptReset();
}

void SwIn::publish() {
	DEBUG4_PRINTLN("_publishMCP");

	if (mqtt == NULL) {
		return;
	}

	bool result;

	if (AppSettings.msw_cnt > 0) {
		for (byte i = 0; i < AppSettings.msw_cnt; i++) {
			result = mqtt->publish(AppSettings.topMSW, i, OUT, ActStates.getMswString(i));
			if (!result)
				ERROR_PRINTLN("Error: can't publish MSW states");
		}
	}
}


void SwIn::turnSw(byte num) {

	bool state = ActStates.switchMsw(num);
	DEBUG4_PRINTF2("num=%d; state=%d; ", num, state);

	if (state) {
		DEBUG4_PRINTF(" set msw[%d] to GREEN;  ", num);
		MCP23017::digitalWrite(AppSettings.msw[num], HIGH);
		AppSettings.led.green(num);
		//mqtt.publish(sTopSw_Out+String(num+1), "ON");
	}
	else {
		DEBUG4_PRINTF(" set msw[%d] to RED;  ", num);
		MCP23017::digitalWrite(AppSettings.msw[num], LOW);
		AppSettings.led.red(num);
		//mqtt.publish(sTopSw_Out+String(num+1), "OFF");
	}

	DEBUG4_PRINTLN("turnSw. Done");
}


// SensorDHT
SensorDHT::~SensorDHT() {}

void SensorDHT::init() {
	DHT:begin();
}

SensorDHT::SensorDHT(MQTT &mqtt, byte dhtType) : DHT(AppSettings.dht, dhtType), Sensor(AppSettings.shift_dht, AppSettings.interval_dht, mqtt){
	init();
}


SensorDHT::SensorDHT(byte pin, byte dhtType, MQTT &mqtt, unsigned int shift, unsigned int interval) : DHT(pin, dhtType), Sensor(shift, interval, mqtt) {
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
		result = mqtt->publish(AppSettings.topDHT_t, OUT, String(temperature));

		if (result)
			temperature = undefined;
	}

	if (humidity != undefined) {
		result = mqtt->publish(AppSettings.topDHT_h, OUT, String(humidity));

		if (result)
			humidity = undefined;
	}
}

// SensorBMP
SensorBMP::~SensorBMP() {}

void SensorBMP::init(byte scl, byte sda) {
	DEBUG4_PRINTF2("SensorBMP.init scl=%d, sda=%d", scl, sda); DEBUG4_PRINTLN();
	Wire.pins(scl, sda);
	Wire.begin();
}

SensorBMP::SensorBMP(MQTT &mqtt) : BMP180(), Sensor(AppSettings.shift_bmp, AppSettings.interval_bmp, mqtt){
	init(AppSettings.scl, AppSettings.sda);
}

SensorBMP::SensorBMP(byte scl, byte sda, MQTT &mqtt, unsigned int shift, unsigned int interval) : BMP180(), Sensor(shift, interval, mqtt) {
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
		result = mqtt->publish(AppSettings.topBMP_t, OUT, String(temperature));

		if (result)
			temperature = undefined;
	}

	if (pressure != undefined) {
		result = mqtt->publish(AppSettings.topBMP_p, OUT, String(pressure));

		if (result)
			pressure = undefined;
	}
}

// SensorDS
SensorDS::~SensorDS() {
	//	delete temperature;
}

void SensorDS::init(byte count) {
	OneWire::begin();
	this->count = count;
	this->temperature = new float[this->count];

	for (int i=0; i < this->count; i++)
		this->temperature[i] = undefined;
}

SensorDS::SensorDS(MQTT &mqtt, byte count) : OneWire(AppSettings.ds), Sensor(AppSettings.shift_ds, AppSettings.interval_ds, mqtt){
	init(count);
}

SensorDS::SensorDS(byte pin, byte count, MQTT &mqtt, unsigned int shift, unsigned int interval) : OneWire(pin), Sensor(shift, interval, mqtt) {
	init(count);
}

void SensorDS::compute() {

	DEBUG4_PRINTLN("_readOneWire");

	byte addr[8];
	byte num = 0;
	float celsius, fahrenheit;
	system_soft_wdt_stop();
	while (OneWire::search(addr)) {
		celsius = readDCByAddr(addr);

		if (num < (this->count)) {
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
	DEBUG4_PRINTLN("ds.publish");

	if (mqtt == NULL) {
		return;
	}

	DEBUG4_PRINTLN("ds.publish mqtt != null");
	bool result;


	for (byte i = 0; i < count; i++) {
		DEBUG4_PRINTF("ds.publish i=%d, temp=", i);
		DEBUG4_PRINTLN(temperature[i]);

		if (temperature[i] != undefined) {
			result = mqtt->publish(AppSettings.topDS_t, i, OUT, String(temperature[i]));
			DEBUG4_PRINTF("result = %d", result);
			DEBUG4_PRINTLN();
			if (result)
				temperature[i] = undefined;
		}
	}

	DEBUG4_PRINTLN();
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


void SensorDS::print() {
	DEBUG4_PRINTLN("ds.print");

	DEBUG4_PRINTF("ds.count=%d", this->count); DEBUG4_PRINTLN();
	DEBUG4_PRINTF("ds.pmqtt=\"%p\"", this->mqtt);DEBUG4_PRINTLN();
	//DEBUG4_PRINTLN("ds.mqtt.name=\"" + this->mqtt->getName() + "\"");

	for (int i=0; i < count; i++) {
		DEBUG4_PRINTF2("ds: i=%d, t=%4.2f", i, temperature[i]);
		DEBUG4_PRINTLN();
	}

	DEBUG4_PRINTLN("ds.print.done");

	return;
}



// SensorDSS
SensorDSS::~SensorDSS() {
	//	delete temperature;
}

void SensorDSS::init(byte pin) {
	DS18S20::Init(pin);
}

SensorDSS::SensorDSS(MQTT &mqtt) : DS18S20(), Sensor(AppSettings.shift_ds, AppSettings.interval_ds, mqtt){
	init(AppSettings.ds);
}

SensorDSS::SensorDSS(byte pin, MQTT &mqtt, unsigned int shift, unsigned int interval) : DS18S20(), Sensor(shift, interval, mqtt) {
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
				result = mqtt->publish(AppSettings.topDS_t, i, OUT, String(DS18S20::GetCelsius(i)));
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

