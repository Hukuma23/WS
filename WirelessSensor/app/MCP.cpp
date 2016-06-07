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
	delay(50);
	Wire.pins(4, 5);
	MCP23017::begin(0);

	pinMode(AppSettings.m_int, INPUT);

	this->mqtt = &mqtt;

	for (byte i = 0; i < AppSettings.msw_cnt; i++) {
		//DEBUG4_PRINTLN("MCP.2");
		MCP23017::pinMode(AppSettings.msw[i], OUTPUT);
		MCP23017::digitalWrite(AppSettings.msw[i], ActStates.msw[i]);
	}

	pullup(AppSettings.m_int);

	MCP23017::setupInterrupts(false, false, LOW);

	for (byte i = 0; i < AppSettings.min_cnt; i++) {
		MCP23017::pinMode(AppSettings.min[i], INPUT);
		MCP23017::pullUp(AppSettings.min[i], HIGH);
		MCP23017::setupInterruptPin(AppSettings.min[i], FALLING);
	}


	attachInterrupt(AppSettings.m_int, Delegate<void()>(&MCP::interruptCallback, this), FALLING);
	//timer.initializeMs(30 * 1000, TimerDelegate(&MCP::publish, this)).start(); // every 25 seconds
	interruptReset();
}

void MCP::interruptReset() {
	//DEBUG4_PRINT("MCP.interruptReset");
	uint8_t pin = MCP23017::getLastInterruptPin();
	uint8_t last_state = MCP23017::getLastInterruptPinValue();

	//DEBUG4_PRINTF("interrupt pin= %d, state=%d", pin, last_state);
	//DEBUG4_PRINTLN();
}

void MCP::interruptHandler() {
	DEBUG4_PRINTLN("MCP.intH");
	//awakenByInterrupt = true;

	pin = MCP23017::getLastInterruptPin();
	uint8_t last_state = MCP23017::getLastInterruptPinValue();
	uint8_t act_state = MCP23017::digitalRead(pin);

	DEBUG4_PRINTF("push pin=%d state=%d. ", pin, act_state);

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
	DEBUG4_PRINTF("push pin=%d state=%d. ", pin, act_state);

	byte num = AppSettings.getMInNumByPin(pin);
	bool isLong = false;

	if (act_state == LOW) {
		//if (mqtt)
		//	mqtt->publish(AppSettings.topMIN_L, num+1, OUT, (ActStates.msw[num]?"ON":"OFF"));
		isLong = true;
		DEBUG1_PRINTLN("*-long-*");
	}
	else {
		isLong = false;
		DEBUG1_PRINTLN("*-short-*");
	}

	publish(num, ActStates.msw[num], isLong);

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
		if (mqtt)
			mqtt->publish(AppSettings.topMSW,i+1, OUT, strState);

		DEBUG1_PRINTF("sw[%d]=", i);
		DEBUG1_PRINT(strState);
		DEBUG1_PRINTLN();
	}
	PRINT_MEM();
	DEBUG4_PRINTLN();

	interruptReset();

}

bool MCP::turnSw(byte num, bool state) {
	if ((num == -1) || (num == 255)) {
		ERROR_PRINT("ERROR: MCP.turnSw num = ");
		ERROR_PRINTLN(num);
		return false;
	}

	if (ActStates.msw[num] != state) {
		ActStates.setMsw(num, state);
		MCP23017::digitalWrite(AppSettings.msw[num], state);

/* Логирование перенесено во MCP::publish(byte num, bool state, bool longPressed)
		if (mqtt)
			if (longPressed)
				mqtt->publish(AppSettings.topMIN_L, num+1, OUT, (state?"ON":"OFF"));
			else
				mqtt->publish(AppSettings.topMIN, num+1, OUT, (state?"ON":"OFF"));
*/
		if (state) {
			AppSettings.led.showOn(num);
		}
		else {
			AppSettings.led.showOff(num);
		}
	}
	return state;
}

void MCP::publish(byte num, bool state, bool longPressed) {
	if (mqtt)
		if (longPressed)
			mqtt->publish(AppSettings.topMIN_L, num+1, OUT, (state?"ON":"OFF"));
		else
			mqtt->publish(AppSettings.topMIN, num+1, OUT, (state?"ON":"OFF"));

}

bool MCP::turnSw(byte num) {
	if ((num == -1) || (num == 255)) {
		ERROR_PRINT("ERROR: MCP.turnSw num = ");
		ERROR_PRINTLN(num);
		return false;
	}

	return turnSw(num, !ActStates.msw[num]);
}

void MCP::startTimer() {
	DEBUG4_PRINTLN("MCP::startTimer");
	timer.initializeMs(AppSettings.shift_mcp, TimerDelegate(&MCP::start, this)).startOnce();
};

void MCP::stopTimer() {
	DEBUG4_PRINTLN("MCP::stopTimer");
	timer.stop();
};

void MCP::start() {
	DEBUG4_PRINTLN("MCP::start");
	timer.initializeMs(AppSettings.interval_mcp, TimerDelegate(&MCP::publish,this)).start();
}

bool MCP::processCallback(String topic, String message) {
	for (byte i = 0; i < AppSettings.msw_cnt; i++) {
		if (topic.equals(mqtt->getTopic(AppSettings.topMSW, (i+1), IN))) {
			if (message.equals("ON")) {
				turnSw(i, HIGH);
				return true;
			} else if (message.equals("OFF")) {
				turnSw(i, LOW);
				return true;
			} else {
				DEBUG4_PRINTF("MCP:: Topic %s, message is UNKNOWN", (mqtt->getTopic(AppSettings.topMSW, (i+1), IN)).c_str());
				return false;
			}
		}
	}
	return false;


}
