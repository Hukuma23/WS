#include <MCP.h>

MCP::MCP(MQTT &mqtt, InterruptHandlerDelegate interruptHandler) : MCP23017(), Sensor(AppSettings.shift_mcp, AppSettings.interval_mcp, mqtt) {
	Serial.print("MCP.create");
	init(interruptHandler);
}

MCP::~MCP() {
	Serial.println("MCP delete called");
}


void MCP::init(InterruptHandlerDelegate interruptHandler) {
	DEBUG1_PRINTLN("MCP.init");
	Wire.pins(4, 5);
	//DEBUG4_PRINTLN("MCP.0");
	MCP23017::begin(0);
	//DEBUG4_PRINTLN("MCP.0.5");
	pinMode(AppSettings.m_int, INPUT);

	interruptHandlerExternal = interruptHandler;

	//DEBUG4_PRINTLN("MCP.1");

	for (byte i = 0; i < AppSettings.msw_cnt; i++) {
		//DEBUG4_PRINTLN("MCP.2");
		MCP23017::pinMode(AppSettings.msw[i], OUTPUT);
		MCP23017::digitalWrite(AppSettings.msw[i], ActStates.getMsw(i));
	}

	//DEBUG4_PRINTLN("MCP.3");
	pullup(AppSettings.m_int);

	MCP23017::setupInterrupts(false, false, LOW);

	//DEBUG4_PRINTLN("MCP.4");

	for (byte i = 0; i < AppSettings.min_cnt; i++) {
		//DEBUG4_PRINTLN("MCP.5");
		MCP23017::pinMode(AppSettings.min[i], INPUT);
		MCP23017::pullUp(AppSettings.min[i], HIGH);
		MCP23017::setupInterruptPin(AppSettings.min[i], FALLING);
	}

	//DEBUG4_PRINTLN("MCP.6");
	attachInterrupt(AppSettings.m_int, Delegate<void()>(&MCP::interruptCallback, this), FALLING);
	//DEBUG4_PRINTLN("MCP.7");
	//timer.initializeMs(30 * 1000, TimerDelegate(&MCP::publish, this)).start(); // every 25 seconds
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
		timerBtn.initializeMs(AppSettings.long_time, TimerDelegate(&MCP::longtimeHandler, this)).startOnce();
		byte num = AppSettings.getMInNumByPin(pin);
		bool state = turnSw(num++);
		mqtt->publish(AppSettings.topMIN,num, OUT, (state?"ON":"OFF"));
		if (interruptHandlerExternal)
			interruptHandlerExternal(num, state);
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

	if (act_state == LOW) {
		Serial.println("*-long-*");
		byte num = AppSettings.getMInNumByPin(pin);
		mqtt->publish(AppSettings.topMIN_L,(num+1), OUT, (ActStates.msw[num]?"ON":"OFF"));
	}
	else
		Serial.println("*-short-*");

	attachInterrupt(AppSettings.m_int, Delegate<void()>(&MCP::interruptCallback, this), FALLING);
}


void MCP::interruptCallback() {
	Serial.print("MCP.intCB   ");
	detachInterrupt(AppSettings.m_int);
	timerBtn.initializeMs(AppSettings.debounce_time, TimerDelegate(&MCP::interruptHandler, this)).startOnce();
}

void MCP::publish() {
	Serial.print("MCP.publish");

	for (byte i = 0; i < AppSettings.msw_cnt; i++) {
		MCP23017::digitalWrite(AppSettings.msw[i], ActStates.getMsw(i));
		String strState = (ActStates.getMsw(i)?"ON":"OFF");
		DEBUG4_PRINTF2("sw[%d]=%s", i, strState);
		mqtt->publish(AppSettings.topMSW, i+1, OUT, strState);
	}
	PRINT_MEM();
	DEBUG4_PRINTLN();
	//attachInterrupt(ESP_INT_PIN, interruptCallback, FALLING);
	//pinMode(ESP_INT_PIN, OUTPUT);
	//delay(10);
	//pinMode(ESP_INT_PIN, INPUT);
	//interruptReset();

}


bool MCP::turnSw(byte num) {
	if (num == -1) {
		ERROR_PRINTLN("ERROR: MCP.turnSw num = -1");
		return false;
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
	return state;

}

void MCP::loop() {
	DEBUG1_PRINTLN("MCP::loop");
	attachInterrupt(AppSettings.m_int, Delegate<void()>(&MCP::interruptCallback, this), FALLING);
	if (needCompute) {
		interruptReset();
		needCompute = !needCompute;
	}
	else {
		interruptReset();
		publish();
		needCompute = !needCompute;
	}
}
