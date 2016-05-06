#include <MCP.h>

MCP::MCP(MQTT &mqtt) : MCP23017() {
	DEBUG4_PRINT("MCP.create");
	init(mqtt);
}

MCP::~MCP() {
	DEBUG4_PRINTLN("MCP delete called");
}


void MCP::init(MQTT &mqtt) {
	DEBUG1_PRINTLN("MCP.init");
	Wire.pins(4, 5);
	//DEBUG4_PRINTLN("MCP.0");
	MCP23017::begin(0);
	//DEBUG4_PRINTLN("MCP.0.5");
	pinMode(AppSettings.m_int, INPUT);

	//DEBUG4_PRINTLN("MCP.1");
	this->mqtt = &mqtt;
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
	DEBUG4_PRINT("MCP.interruptReset");
	uint8_t pin = MCP23017::getLastInterruptPin();
	uint8_t last_state = MCP23017::getLastInterruptPinValue();

	//detachInterrupt(ESP_INT_PIN);
	DEBUG4_PRINTF2("interrupt pin= %d, state=%d", pin, last_state);
	DEBUG4_PRINTLN();
	//attachInterrupt(ESP_INT_PIN, interruptCallback, FALLING);
}

void MCP::interruptHandler() {
	DEBUG4_PRINTLN("MCP.intH");
	//awakenByInterrupt = true;

	pin = MCP23017::getLastInterruptPin();
	uint8_t last_state = MCP23017::getLastInterruptPinValue();
	uint8_t act_state = MCP23017::digitalRead(pin);

	DEBUG4_PRINTF2("push pin=%d state=%d. ", pin, act_state);

	if (act_state == LOW) {
		timerBtnHandle.initializeMs(LONG_TIME, TimerDelegate(&MCP::longtimeHandler, this)).startOnce();
		String strState = (turnSw(AppSettings.getMInNumByPin(pin))?"ON":"OFF");
		//if (mqtt)
		//	mqtt->publish(AppSettings.topMIN, AppSettings.getMInNumByPin(pin)+1, OUT, strState);
	}
	else {
		attachInterrupt(AppSettings.m_int, Delegate<void()>(&MCP::interruptCallback, this), FALLING);
		DEBUG4_PRINTLN();
	}

}

void MCP::longtimeHandler() {
	DEBUG4_PRINTLN("MCP.ltH");
	uint8_t act_state = MCP23017::digitalRead(pin);
	while (!(MCP23017::digitalRead(pin)));
	DEBUG4_PRINTF2("push pin=%d state=%d. ", pin, act_state);

	byte num = AppSettings.getMInNumByPin(pin);

	if (act_state == LOW) {
		//if (mqtt)
		//	mqtt->publish(AppSettings.topMIN_L, num+1, OUT, (ActStates.msw[num]?"ON":"OFF"));
		DEBUG4_PRINTLN("*-long-*");
	}
	else
		DEBUG4_PRINTLN("*-short-*");

	attachInterrupt(AppSettings.m_int, Delegate<void()>(&MCP::interruptCallback, this), FALLING);
}


void MCP::interruptCallback() {
	DEBUG4_PRINT("MCP.intCB   ");
	detachInterrupt(AppSettings.m_int);
	timerBtnHandle.initializeMs(DEBOUNCE_TIME, TimerDelegate(&MCP::interruptHandler, this)).startOnce();
}

void MCP::publish() {
	DEBUG4_PRINT("MCP.publish");

	for (byte i = 0; i < AppSettings.msw_cnt; i++) {
		MCP23017::digitalWrite(AppSettings.msw[i], ActStates.getMsw(i));
		String strState = (ActStates.getMsw(i)?"ON":"OFF");
		//if (mqtt)
		//	mqtt->publish(AppSettings.topMSW,i+1, OUT, strState);

		DEBUG1_PRINTF("sw[%d]=", i);
		DEBUG1_PRINT(strState);
		DEBUG1_PRINTLN();
	}
	PRINT_MEM();
	DEBUG4_PRINTLN();

	interruptReset();

}


bool MCP::turnSw(byte num) {
	if (num == -1) {
		ERROR_PRINTLN("ERROR: MCP.turnSw num = -1");
		return false;
	}

	byte pin = AppSettings.getMSwPinByNum(num);

	bool state = ActStates.switchMsw(num);
	MCP23017::digitalWrite(pin, state);

	String strState = (state?"ON":"OFF");
	//if (mqtt)
	//	mqtt->publish(AppSettings.topMSW, num, OUT, strState);

	DEBUG1_PRINTFF("turnSw: sw[%d]=", num);
	DEBUG1_PRINT(strState);

	return state;
}

void MCP::startTimer(){};

void MCP::stopTimer(){};
