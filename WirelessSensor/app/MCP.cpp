#include <MCP.h>

MCP::MCP(byte sw_cnt, byte in_cnt) : MCP23017() {
	Serial.print("MCP.create");
	init(sw_cnt, in_cnt);
}

MCP::~MCP() {
	Serial.println("MCP delete called");
}

void MCP::initArr(byte sw_cnt, byte in_cnt) {
	this->sw_cnt = sw_cnt;
	this->in_cnt = in_cnt;

	sw = new byte[this->sw_cnt];
	sw_state = new bool[this->sw_cnt];
	in = new byte[this->in_cnt];

	for (byte i = 0; i < in_cnt; i++) {
		in[i] = i;
	}

	for (byte i = 0; i < sw_cnt; i++) {
		sw[i] = 15-i;
	}

	for (byte i = 0; i < sw_cnt; i++) {
		sw_state[i] = LOW;
	}
}

void MCP::init(byte sw_cnt, byte in_cnt) {
	Serial.print("MCP.init");
	Wire.pins(4, 5);

	MCP23017::begin(0);

	pinMode(ESP_INT_PIN, INPUT);


	initArr(sw_cnt, in_cnt);

	/* SWout
		MCP23017::pinMode(MCP_LED_PIN, OUTPUT);
		MCP23017::digitalWrite(MCP_LED_PIN, LOW);
	 */
	for (byte i = 0; i < sw_cnt; i++) {
		MCP23017::pinMode(sw[i], OUTPUT);
		MCP23017::digitalWrite(sw[i], LOW);
	}

	pullup(ESP_INT_PIN);

	MCP23017::setupInterrupts(false, false, LOW);

	/* In
		MCP23017::pinMode(mcpPinA, INPUT);
		MCP23017::pullUp(mcpPinA, HIGH);
		MCP23017::setupInterruptPin(mcpPinA, FALLING);
	 */
	for (byte i = 0; i < in_cnt; i++) {
		MCP23017::pinMode(in[i], INPUT);
		MCP23017::pullUp(in[i], HIGH);
		MCP23017::setupInterruptPin(in[i], FALLING);
	}

	attachInterrupt(ESP_INT_PIN, Delegate<void()>(&MCP::interruptCallback, this), FALLING);

	timer.initializeMs(30 * 1000, TimerDelegate(&MCP::publish, this)).start(); // every 25 seconds
	interruptReset();
}

void MCP::interruptReset() {
	Serial.print("MCP.interruptReset");
	uint8_t pin = MCP23017::getLastInterruptPin();
	uint8_t last_state = MCP23017::getLastInterruptPinValue();

	//detachInterrupt(ESP_INT_PIN);
	Serial.printf("interrupt pin= %d, state=%d", pin, last_state);
	Serial.println();
	//attachInterrupt(ESP_INT_PIN, interruptCallback, FALLING);
}

void MCP::interruptHandler() {
	Serial.print("MCP.intH");
	//awakenByInterrupt = true;

	pin = MCP23017::getLastInterruptPin();
	uint8_t last_state = MCP23017::getLastInterruptPinValue();
	uint8_t act_state = MCP23017::digitalRead(pin);
	//Serial.printf("spent time=%d", (millis() - intTime));
	//Serial.println();

	Serial.printf("push pin=%d state=%d. ", pin, act_state);

	if (act_state == LOW) {
		timerBtnHandle.initializeMs(LONG_TIME, TimerDelegate(&MCP::longtimeHandler, this)).startOnce();
		turnSw(getInNumByPin(pin));
	}
	else {
		attachInterrupt(ESP_INT_PIN, Delegate<void()>(&MCP::interruptCallback, this), FALLING);
		Serial.println();
	}

}

void MCP::longtimeHandler() {
	Serial.print("MCP.ltH");
	uint8_t act_state = MCP23017::digitalRead(pin);
	while (!(MCP23017::digitalRead(pin)));
	Serial.printf("push pin=%d state=%d. ", pin, act_state);


	if (act_state == LOW)
		Serial.println("*-long-*");
	else
		Serial.println("*-short-*");


	attachInterrupt(ESP_INT_PIN, Delegate<void()>(&MCP::interruptCallback, this), FALLING);
}


void MCP::interruptCallback() {
	Serial.print("MCP.intCB   ");
	detachInterrupt(ESP_INT_PIN);
	timerBtnHandle.initializeMs(DEBOUNCE_TIME, TimerDelegate(&MCP::interruptHandler, this)).startOnce();
}

void MCP::publish() {
	Serial.print("MCP.publish");

	for (byte i = 0; i < sw_cnt; i++) {
		MCP23017::digitalWrite(sw[i], sw_state[i]);
		DEBUG4_PRINTF("sw[%d]=ON", i);
		if (sw_state[i]) {
			DEBUG4_PRINTLN("ON");
		}
		else {
			DEBUG4_PRINTLN("OFF");
		}
	}
	PRINT_MEM();
	DEBUG4_PRINTLN();
	//attachInterrupt(ESP_INT_PIN, interruptCallback, FALLING);
	//pinMode(ESP_INT_PIN, OUTPUT);
	//delay(10);
	//pinMode(ESP_INT_PIN, INPUT);
	interruptReset();

}

byte MCP::getSwPinByNum(byte num) {
	if ((num >= 0) && (num < sw_cnt))
		return sw[num];

	return -1;
}

byte MCP::getInPinByNum(byte num) {
	if ((num >= 0) && (num < in_cnt))
		return sw[num];

	return -1;
}

byte MCP::getInNumByPin(byte pin) {

	if ((pin >= 0) && (pin < 16)) {
		for (byte i = 0; i < in_cnt; i++) {
			if (in[i] == pin)
				return i;
		}
	}

	return -1;
}

void MCP::turnSw(byte num) {
	if (num == -1) {
		ERROR_PRINTLN("ERROR: MCP.turnSw num = -1");
		return;
	}

	byte pin = getSwPinByNum(num);
	sw_state[num] = !sw_state[num];
	bool state = sw_state[num];
	DEBUG4_PRINTF("turnSw: sw[%d]=", num);
	if (state) {
		DEBUG4_PRINTLN("ON");
		MCP23017::digitalWrite(pin, state);
	}
	else {
		DEBUG4_PRINTLN("OFF");
		MCP23017::digitalWrite(pin, state);
	}

}

void MCP::startTimer(){};

void MCP::stopTimer(){};
