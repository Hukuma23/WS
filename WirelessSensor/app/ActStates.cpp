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
	//load(ssw, appSettings.ssw_cnt, SSW_ACT_STATE_FILE);
	loadSSW();
	DEBUG1_PRINTF("msw_cnt = %d", appSettings.msw_cnt);
	DEBUG1_PRINTF("ssw_cnt = %d", appSettings.ssw_cnt);
	DEBUG1_PRINTF("sw_cnt = %d", appSettings.sw_cnt);
}

ActStates::~ActStates() {
	delete sw;
	delete ssw;
	delete msw;
}



void ActStates::load(bool arr[], byte cnt, String fileName) {
	DynamicJsonBuffer jsonBuffer;
	if (exist(fileName)) {
		int size = fileGetSize(fileName);
		char* jsonString = new char[size + 1];
		fileGetContent(fileName, jsonString, size + 1);
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


		byte count = root["cnt"];

		if (count > 0) {
			arr = new bool[count];
			for (byte i=0; i < count; i++)
				arr[i] = root[String(i)];
		}

		if (count != cnt) {
			ERROR_PRINTF("ERROR: ASt.count(%d) != AS.cnt(%d)", count, cnt);
			count = appSettings.msw_cnt;
		}

		DEBUG1_PRINTF("LED.load()\r\n");
		initLED();

		delete[] jsonString;
	}
	else {
		needInit = true;
	}
}

void ActStates::loadSSW() {
	DEBUG1_PRINTF("AS.loadSSW()");
	DynamicJsonBuffer jsonBuffer;
	if (exist(SSW_ACT_STATE_FILE)) {
		DEBUG1_PRINTF("AS.loadSSW().exist!");
		int size = fileGetSize(SSW_ACT_STATE_FILE);
		char* jsonString = new char[size + 1];
		fileGetContent(SSW_ACT_STATE_FILE, jsonString, size + 1);
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


		byte count = root["cnt"];

		if (count > 0) {
			ssw = new bool[count];
			for (byte i=0; i < count; i++)
				ssw[i] = root[String(i)];
		}

		if (count != appSettings.ssw_cnt) {
			ERROR_PRINTF("ERROR: ASt.count(%d) != AS.ssw_cnt(%d)", count, appSettings.ssw_cnt);
			count = appSettings.ssw_cnt;
		}

		DEBUG1_PRINTF("LED.load()\r\n");
		initLED();

		delete[] jsonString;
	}
	else {
		DEBUG1_PRINTF("AS.loadSSW().needInit");
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
			DEBUG4_PRINTLN("ASt.2.1");
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


	if (appSettings.sw_cnt > 0) {
		root["cnt"] = appSettings.sw_cnt;
		for (byte i=0; i < appSettings.sw_cnt; i++)
			root[String(i)] = sw[i];

		String str;
		root.printTo(str);

		fileSetContent(SW_ACT_STATE_FILE, str);

		DEBUG1_PRINTLN(str);
		DEBUG1_PRINTLN("SW states file was saved");
	}

	if (appSettings.ssw_cnt > 0) {
		root["cnt"] = appSettings.ssw_cnt;
		for (byte i=0; i < appSettings.ssw_cnt; i++)
			root[String(i)] = ssw[i];

		String str;
		root.printTo(str);

		fileSetContent(SSW_ACT_STATE_FILE, str);

		DEBUG1_PRINTLN(str);
		DEBUG1_PRINTLN("SSW states file was saved");
	}

	if (appSettings.msw_cnt > 0) {
		root["cnt"] = appSettings.msw_cnt;
		for (byte i=0; i < appSettings.msw_cnt; i++)
			root[String(i)] = msw[i];

		String str;
		root.printTo(str);

		fileSetContent(MCP_ACT_STATE_FILE, str);

		DEBUG1_PRINTLN(str);
		DEBUG1_PRINTLN("MCP states file was saved");
	}
}



bool ActStates::exist(String fileName) { return fileExist(fileName); }

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

String ActStates::print(String fileName) {
	if (exist(fileName))
	{
		int size = fileGetSize(fileName);
		char* jsonString = new char[size + 1];
		fileGetContent(fileName, jsonString, size + 1);
		return String(jsonString);
	}
	return "State file doesn't exist";
}
String ActStates::print() {
	if (exist(MCP_ACT_STATE_FILE)) {
		int size = fileGetSize(MCP_ACT_STATE_FILE);
		char* jsonString = new char[size + 1];
		fileGetContent(MCP_ACT_STATE_FILE, jsonString, size + 1);
		return "MCP: " + String(jsonString);
	} else if (exist(SSW_ACT_STATE_FILE)){
		int size = fileGetSize(SSW_ACT_STATE_FILE);
		char* jsonString = new char[size + 1];
		fileGetContent(SSW_ACT_STATE_FILE, jsonString, size + 1);
		return "SSW: " + String(jsonString);
	} else if (exist(SW_ACT_STATE_FILE)){
		int size = fileGetSize(SW_ACT_STATE_FILE);
		char* jsonString = new char[size + 1];
		fileGetContent(SW_ACT_STATE_FILE, jsonString, size + 1);
		return "SW: " + String(jsonString);
	}
	return "State files don't exist";
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

			DEBUG1_PRINTF("ASt.setSw()\r\n");
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
		ERROR_PRINTF("ERROR: setSw wrong number (%d) access. sw_cnt=%d", num, appSettings.sw_cnt);
	}
}

bool ActStates::switchSw(byte num) {
	bool state = !getSw(num);
	setSw(num, state);
	return state;
}

bool ActStates::getSw(byte num) {
	bool result = false;
	if ((num >= 0) && (appSettings.sw_cnt > num))
		result = this->sw[num];
	else
		ERROR_PRINT("ERROR: getSw wrong number access");
	return result;
}

String ActStates::getSwString(byte num) {
	if (getSw(num))
		return "ON";
	else
		return "OFF";
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
	DEBUG1_PRINTF("AS.getSsw()");
	uint8_t result = 0;
	if (appSettings.ssw_cnt > 0) {
		for (byte i = 0; i < appSettings.ssw_cnt; i++)
			result += ssw[i] << i;
	}
	//uint8_t sw = this->ssw1 + (this->ssw2 << 1) + (this->ssw3 << 2) + (this->ssw4 << 3) + (this->ssw5 << 4);
	DEBUG1_PRINTF("AS.getSsw().done");
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
