#include <MCP.h>

MCP::MCP() : MCP23017() {
	Serial.print("MCP.create");
	init();
}

MCP::~MCP() {
	Serial.println("MCP delete called");
}

/*
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
*/

void MCP::init() {
	DEBUG1_PRINTLN("MCP.init");
	Wire.pins(4, 5);
	//DEBUG4_PRINTLN("MCP.0");
	MCP23017::begin(0);
	//DEBUG4_PRINTLN("MCP.0.5");
	pinMode(AppSettings.m_int, INPUT);

	//DEBUG4_PRINTLN("MCP.1");

	//initArr(sw_cnt, in_cnt);

	/* SWout
		MCP23017::pinMode(MCP_LED_PIN, OUTPUT);
		MCP23017::digitalWrite(MCP_LED_PIN, LOW);
	 */
	for (byte i = 0; i < AppSettings.msw_cnt; i++) {
		//DEBUG4_PRINTLN("MCP.2");
		MCP23017::pinMode(AppSettings.msw[i], OUTPUT);
		MCP23017::digitalWrite(AppSettings.msw[i], ActStates.getMsw(i));
	}

	//DEBUG4_PRINTLN("MCP.3");
	pullup(AppSettings.m_int);

	MCP23017::setupInterrupts(false, false, LOW);

	//DEBUG4_PRINTLN("MCP.4");

	/* In
		MCP23017::pinMode(mcpPinA, INPUT);
		MCP23017::pullUp(mcpPinA, HIGH);
		MCP23017::setupInterruptPin(mcpPinA, FALLING);
	 */


	for (byte i = 0; i < AppSettings.min_cnt; i++) {
		//DEBUG4_PRINTLN("MCP.5");
		MCP23017::pinMode(AppSettings.min[i], INPUT);
		MCP23017::pullUp(AppSettings.min[i], HIGH);
		MCP23017::setupInterruptPin(AppSettings.min[i], FALLING);
	}

	//DEBUG4_PRINTLN("MCP.6");
	attachInterrupt(AppSettings.m_int, Delegate<void()>(&MCP::interruptCallback, this), FALLING);
	//DEBUG4_PRINTLN("MCP.7");
	timer.initializeMs(30 * 1000, TimerDelegate(&MCP::publish, this)).start(); // every 25 seconds
	//DEBUG4_PRINTLN("MCP.8");
	interruptReset();
	//DEBUG4_PRINTLN("MCP.9");
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
		turnSw(AppSettings.getMInNumByPin(pin));
	}
	else {
		attachInterrupt(AppSettings.m_int, Delegate<void()>(&MCP::interruptCallback, this), FALLING);
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

	attachInterrupt(AppSettings.m_int, Delegate<void()>(&MCP::interruptCallback, this), FALLING);
}


void MCP::interruptCallback() {
	Serial.print("MCP.intCB   ");
	detachInterrupt(AppSettings.m_int);
	timerBtnHandle.initializeMs(DEBOUNCE_TIME, TimerDelegate(&MCP::interruptHandler, this)).startOnce();
}

void MCP::publish() {
	Serial.print("MCP.publish");

	for (byte i = 0; i < AppSettings.msw_cnt; i++) {
		MCP23017::digitalWrite(AppSettings.msw[i], ActStates.getMsw(i));
		DEBUG4_PRINTF("sw[%d]=ON", i);
		if (ActStates.getMsw(i)) {
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


void MCP::turnSw(byte num) {
	if (num == -1) {
		ERROR_PRINTLN("ERROR: MCP.turnSw num = -1");
		return;
	}

	byte pin = AppSettings.getMSwPinByNum(num);

	bool state = ActStates.switchMsw(num);
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
