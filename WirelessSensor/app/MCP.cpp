#include <MCP.h>

MCP::MCP(MQTT &mqtt, InterruptHandlerDelegate interruptHandler) : MCP23017(), Sensor(AppSettings.shift_mcp, AppSettings.interval_mcp, mqtt) {
	DEBUG1_PRINT("MCP.create");
	init(interruptHandler);
}

MCP::~MCP() {
	DEBUG1_PRINTLN("MCP delete called");
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
	//DEBUG4_PRINTLN("MCP.8");
	interruptReset();
	//DEBUG4_PRINTLN("MCP.9");
}

void MCP::interruptReset() {
	DEBUG1_PRINT("MCP.interruptReset");
	uint8_t pin = MCP23017::getLastInterruptPin();
	uint8_t last_state = MCP23017::getLastInterruptPinValue();

	//detachInterrupt(ESP_INT_PIN);
	DEBUG1_PRINTF("interrupt pin= %d, state=%d", pin, last_state);
	DEBUG1_PRINTLN();
}

void MCP::interruptHandler() {
	DEBUG1_PRINT("MCP.intH");
	pin = MCP23017::getLastInterruptPin();
	uint8_t last_state = MCP23017::getLastInterruptPinValue();
	uint8_t act_state = MCP23017::digitalRead(pin);

	DEBUG1_PRINTF("push pin=%d state=%d. ", pin, act_state);

	if (act_state == LOW) {
		timerBtn.initializeMs(AppSettings.long_time, TimerDelegate(&MCP::longtimeHandler, this)).startOnce();
		byte num = AppSettings.getMInNumByPin(pin);
		bool state = turnSw(num++);
		if (mqtt)
			mqtt->publish(AppSettings.topMIN,num, OUT, (state?"ON":"OFF"));
		//if (interruptHandlerExternal)
		//	interruptHandlerExternal(num, state);
	}
	else {
		attachInterrupt(AppSettings.m_int, Delegate<void()>(&MCP::interruptCallback, this), FALLING);
		DEBUG1_PRINTLN();
	}

}

void MCP::longtimeHandler() {
	DEBUG1_PRINT("MCP.ltH");
	uint8_t act_state = MCP23017::digitalRead(pin);
	while (!(MCP23017::digitalRead(pin)));
	DEBUG1_PRINTF("push pin=%d state=%d. ", pin, act_state);

	if (act_state == LOW) {
		DEBUG1_PRINTLN("*-long-*");
		byte num = AppSettings.getMInNumByPin(pin);
		if (mqtt)
			mqtt->publish(AppSettings.topMIN_L,(num+1), OUT, (ActStates.msw[num]?"ON":"OFF"));
	}
	else
		DEBUG1_PRINTLN("*-short-*");

	attachInterrupt(AppSettings.m_int, Delegate<void()>(&MCP::interruptCallback, this), FALLING);
}


void MCP::interruptCallback() {
	DEBUG1_PRINT("MCP.intCB   ");
	detachInterrupt(AppSettings.m_int);
	timerBtn.initializeMs(AppSettings.debounce_time, TimerDelegate(&MCP::interruptHandler, this)).startOnce();
}

void MCP::publish() {
	DEBUG1_PRINT("MCP.publish");

	for (byte i = 0; i < AppSettings.msw_cnt; i++) {
		MCP23017::digitalWrite(AppSettings.msw[i], ActStates.getMsw(i));
		String strState = (ActStates.getMsw(i)?"ON":"OFF");
		DEBUG4_PRINTF("sw[%d]=%s", i, strState);
		if (mqtt)
			mqtt->publish(AppSettings.topMSW, i+1, OUT, strState);
	}
	PRINT_MEM();
	DEBUG4_PRINTLN();
	//interruptReset();

}


bool MCP::turnSw(byte num) {
	if (num == -1) {
		ERROR_PRINTLN("ERROR: MCP.turnSw num = -1");
		return false;
	}

	byte pin = AppSettings.getMSwPinByNum(num);
	bool state = ActStates.switchMsw(num);
	MCP23017::digitalWrite(pin, state);
	DEBUG4_PRINTF("turnSw: sw[%d]=", num);
	DEBUG4_PRINTLN((state?"ON":"OFF"));

	return state;

}

void MCP::loop() {
	DEBUG1_PRINTLN("MCP::loop");
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
