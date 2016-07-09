/*
 * ActStates.cpp
 *
 *  Created on: 23.06.2016 г.
 *      Author: Nikita
 *
 *      Actual State of switches and other information
 */

#include <actStates.h>


// Конструкторы и оператор присваивания недоступны клиентам

ActStates::ActStates(AppSettings& appSettings) : appSettings(appSettings) {
	//Initialization of rBoot OTA
	this->appSettings.rBootInit();
	//sw1 = LOW;
	//sw2 = LOW;
	load();
}

ActStates::~ActStates() {
	delete sw;
	delete ssw;
	delete msw;
}



void ActStates::load() {
	DynamicJsonBuffer jsonBuffer;
	if (exist()) {
		int size = fileGetSize(ACT_STATE_FILE);
		char* jsonString = new char[size + 1];
		fileGetContent(ACT_STATE_FILE, jsonString, size + 1);
		JsonObject& root = jsonBuffer.parseObject(jsonString);

		/*
			JsonObject& jSW = root["sw"];
			byte sw_cnt = jSW["cnt"];

			if (sw_cnt > 0) {
				sw = new bool[sw_cnt];
				for (byte i=0; i < sw_cnt; i++)
					sw[i] = jSW[String(i)];
			}

			if (sw_cnt != appSettings.sw_cnt) {
				ERROR_PRINTF("ERROR: ASt.sw_cnt(%d) != AS.sw_cnt(%d)", sw_cnt, appSettings.sw_cnt);
				sw_cnt = appSettings.sw_cnt;
			}

			JsonObject& jSSW = root["ssw"];
			byte ssw_cnt = jSSW["cnt"];
			if (ssw_cnt > 0) {
				ssw = new bool[ssw_cnt];
				for (byte i=0; i < ssw_cnt; i++)
					ssw[i] = jSSW[String(i)];
			}

			if (ssw_cnt != appSettings.ssw_cnt) {
				ERROR_PRINTF("ERROR: ASt.ssw_cnt(%d) != AS.ssw_cnt(%d)", ssw_cnt, appSettings.ssw_cnt);
				ssw_cnt = appSettings.ssw_cnt;
			}

			JsonObject& jMSW = root["msw"];
			byte msw_cnt = jMSW["cnt"];

			if (msw_cnt > 0) {
				msw = new bool[msw_cnt];
				for (byte i=0; i < msw_cnt; i++)
					msw[i] = jMSW[String(i)];
			}

			if (msw_cnt != appSettings.msw_cnt) {
				ERROR_PRINTF("ERROR: ASt.msw_cnt(%d) != AS.msw_cnt(%d)", msw_cnt, appSettings.msw_cnt);
				msw_cnt = appSettings.msw_cnt;
			}
		 */

		byte msw_cnt = root["cnt"];

		if (msw_cnt > 0) {
			msw = new bool[msw_cnt];
			for (byte i=0; i < msw_cnt; i++)
				msw[i] = root[String(i)];
		}

		if (msw_cnt != appSettings.msw_cnt) {
			ERROR_PRINTF("ERROR: ASt.msw_cnt(%d) != AS.msw_cnt(%d)", msw_cnt, appSettings.msw_cnt);
			msw_cnt = appSettings.msw_cnt;
		}

		DEBUG1_PRINTF("LED.load()\r\n");
		initLED();

		delete[] jsonString;
	}
	else {
		needInit = true;
	}
}

void ActStates::init() {
	DEBUG4_PRINTLN("ASt.init");
	if (needInit) {
		DEBUG4_PRINTLN("ASt.1");
		if (appSettings.sw_cnt > 0) {
			sw = new bool[appSettings.sw_cnt];
			for (byte i=0; i < appSettings.sw_cnt; i++)
				sw[i] = false;
		}
		DEBUG4_PRINTLN("ASt.2");

		if (appSettings.ssw_cnt > 0) {
			ssw = new bool[appSettings.ssw_cnt];
			for (byte i=0; i < appSettings.ssw_cnt; i++)
				ssw[i] = false;
		}

		if (appSettings.msw_cnt > 0) {
			DEBUG4_PRINTLN("ASt.3.1");
			msw = new bool[appSettings.msw_cnt];
			for (byte i=0; i < appSettings.msw_cnt; i++)
				msw[i] = false;
		}

		DEBUG1_PRINTF("LED.init()\r\n");
		initLED();

		this->save2file();
		needInit = false;
		DEBUG4_PRINTLN("ASt.5");
	}
}

void ActStates::save() {
	saveTimer.initializeMs(appSettings.shift_save, TimerDelegate(&ActStates::save2file, this)).startOnce();
}

void ActStates::save2file() {
	//StaticJsonBuffer<200> jsonBuffer;
	DynamicJsonBuffer jsonBuffer;
	JsonObject& root = jsonBuffer.createObject();

	/*
		JsonObject& jSW = root.createNestedObject("sw");	//JsonObject& jSW = jsonBuffer.createObject();
		if (appSettings.sw_cnt > 0) {
			jSW["cnt"] = appSettings.sw_cnt;
			for (byte i=0; i < appSettings.sw_cnt; i++)
				jSW[String(i)] = sw[i];
		}


		JsonObject& jSSW = root.createNestedObject("ssw");	//JsonObject& jSSW = jsonBuffer.createObject();
		//root["ssw"] = jSSW;
		if (appSettings.ssw_cnt > 0) {
			jSSW["cnt"] = appSettings.ssw_cnt;
			for (byte i=0; i < appSettings.ssw_cnt; i++)
				jSSW[String(i)] = ssw[i];
		}


		JsonObject& jMSW = root.createNestedObject("msw");	//JsonObject& jMSW = jsonBuffer.createObject();
		//root["msw"] = jMSW;
		if (appSettings.msw_cnt > 0) {
			jMSW["cnt"] = appSettings.msw_cnt;
			for (byte i=0; i < appSettings.msw_cnt; i++)
				jMSW[String(i)] = msw[i];
		}
	 */

	if (appSettings.msw_cnt > 0) {
		root["cnt"] = appSettings.msw_cnt;
		for (byte i=0; i < appSettings.msw_cnt; i++)
			root[String(i)] = msw[i];
	}

	String str;
	root.printTo(str);

	fileSetContent(ACT_STATE_FILE, str);
	DEBUG1_PRINTLN(str);
	DEBUG1_PRINTLN("States file was saved");
}



bool ActStates::exist() { return fileExist(ACT_STATE_FILE); }

String ActStates::printf() {
	String result;

	result = "SWITCHES[" + String(appSettings.sw_cnt)+ "]\r\n";
	if (appSettings.sw_cnt > 0) {
		for (byte i=0; i < appSettings.sw_cnt; i++) {
			result += "\tsw" + String(i) + "=" + String(sw[i]) + "\r\n";
		}
	}
	result = "SERIAL[" + String(appSettings.ssw_cnt)+ "]\r\n";
	if (appSettings.ssw_cnt > 0) {
		for (byte i=0; i < appSettings.ssw_cnt; i++) {
			result += "\tssw" + String(i) + "=" + String(ssw[i]) + "\r\n";
		}
	}
	result = "MCP[" + String(appSettings.msw_cnt)+ "]\r\n";
	if (appSettings.msw_cnt > 0) {
		for (byte i=0; i < appSettings.msw_cnt; i++) {
			result += "\tmsw" + String(i) + "=" + String(msw[i]) + "\r\n";
		}
	}

	return result;

}

String ActStates::print() {
	if (exist())
	{
		int size = fileGetSize(ACT_STATE_FILE);
		char* jsonString = new char[size + 1];
		fileGetContent(ACT_STATE_FILE, jsonString, size + 1);
		return String(jsonString);
	}
	return "State file doesn't exist";
}

/*
	String ActStates::update(JsonObject& root) {

		String result;

		if (root.containsKey("sw")) {
			JsonObject& switches = root["switches"];
			if (switches.containsKey("sw1")) {
				this->sw1 = switches["sw1"];
				result += "sw1, ";
			}

		if (root.containsKey("switches")) {
			JsonObject& switches = root["switches"];
			if (switches.containsKey("sw1")) {
				this->sw1 = switches["sw1"];
				result += "sw1, ";
			}
			if (switches.containsKey("sw2")) {
				this->sw2 = switches["sw2"];
				result += "sw2, ";
			}

			if (switches.containsKey("ssw1")) {
				this->ssw1 = switches["ssw1"];
				result += "ssw1, ";
			}
			if (switches.containsKey("ssw2")) {
				this->ssw2 = switches["ssw2"];
				result += "ssw2, ";
			}
			if (switches.containsKey("ssw3")) {
				this->ssw3 = switches["ssw3"];
				result += "ssw3, ";
			}
			if (switches.containsKey("ssw4")) {
				this->ssw4 = switches["ssw4"];
				result += "ssw4, ";
			}
			if (switches.containsKey("ssw5")) {
				this->ssw5 = switches["ssw5"];
				result += "ssw5, ";
			}
		}

		int len = result.length();
		if (len > 2) {
			result = "These states were updated: " + result.substring(0, len-2);
		}
		else
			result = "Nothing updated";

		return result;
	}


	void ActStates::updateNsave(JsonObject& root) {
		this->update(root);
		this->save();
	}
 */

void ActStates::setSw(byte num, bool state) {
	if ((num >= 0) && (appSettings.sw_cnt > num)) {
		if (sw[num] != state) {
			this->sw[num] = state;
			this->save();
		}
	}
	else {
		ERROR_PRINT("ERROR: setSw wrong number access");
	}
}

bool ActStates::getSw(byte num) {
	bool result = null;
	if ((num >= 0) && (appSettings.sw_cnt > num))
		result = this->sw[num];
	else
		ERROR_PRINT("ERROR: getSw wrong number access");
	return result;
}

void ActStates::setSsw(byte num, bool state) {
	if ((num >= 0) && (appSettings.ssw_cnt > num)) {
		if (ssw[num] != state) {
			this->ssw[num] = state;
			this->save();
		}
	}
	else {
		ERROR_PRINT("ERROR: setSsw wrong number access");
	}
}

bool ActStates::getSsw(byte num) {
	bool result = false;
	if ((num >= 0) && (appSettings.ssw_cnt > num))
		result = this->ssw[num];

	return result;
}


uint8_t ActStates::getSsw() {
	uint8_t result = 0;
	if (appSettings.ssw_cnt > 0) {
		for (byte i = 0; i < appSettings.ssw_cnt; i++)
			result += ssw[i] << i;
	}
	//uint8_t sw = this->ssw1 + (this->ssw2 << 1) + (this->ssw3 << 2) + (this->ssw4 << 3) + (this->ssw5 << 4);
	return result;
}


void ActStates::setMsw(byte num, bool state) {
	if ((num >= 0) && (appSettings.msw_cnt > num)) {
		if (msw[num] != state) {
			this->msw[num] = state;

			DEBUG1_PRINTF("ASt.setMsw()\r\n");
			appSettings.led->print();

			if (state) {
				appSettings.led->showOn(num);
			}
			else {
				appSettings.led->showOff(num);
			}

			this->save();
		}
	}
	else {
		ERROR_PRINTF("ERROR: setMsw wrong number (%d) access. msw_cnt=%d", num, appSettings.msw_cnt);
	}
}

void ActStates::initLED() {
	DEBUG1_PRINTF("InitLED()\r\n");
	if (appSettings.msw_cnt > 0)
		appSettings.led->initRG(msw, appSettings.msw_cnt);
	else if (appSettings.ssw_cnt > 0)
		appSettings.led->initRG(ssw, appSettings.ssw_cnt);
	else if (appSettings.sw_cnt > 0)
		appSettings.led->initRG(sw, appSettings.sw_cnt);
}

void ActStates::initLED(bool* arr, byte cnt) {
	for (byte i=0; i < cnt; i++) {
		if (arr[i])
			appSettings.led->showOn(i);
		else
			appSettings.led->showOff(i);
	}
}

bool ActStates::switchMsw(byte num) {
	bool state = !getMsw(num);
	setMsw(num, state);
	return state;
}

bool ActStates::getMsw(byte num) {
	bool result = false;
	if ((num >= 0) && (appSettings.msw_cnt > num))
		result = this->msw[num];

	return result;
}

String ActStates::getMswString(byte num) {
	if (getMsw(num))
		return "ON";
	else
		return "OFF";
}


uint8_t ActStates::getMsw() {
	uint8_t result = 0;
	if (appSettings.msw_cnt > 0) {
		for (byte i = 0; i < appSettings.msw_cnt; i++)
			result += msw[i] << i;
	}
	//uint8_t sw = this->ssw1 + (this->ssw2 << 1) + (this->ssw3 << 2) + (this->ssw4 << 3) + (this->ssw5 << 4);
	return result;
}


bool ActStates::check() {
	//TODO: need to be coded check for mandatory fields
	return true;
}