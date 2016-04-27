/*
 * ActStates.h
 *
 *  Created on: 21 сент. 2015 г.
 *      Author: Nikita
 *
 *      Actual State of switches and other information
 */

#include <SmingCore/SmingCore.h>
#include <Logger.h>


#ifndef INCLUDE_ACTSTATE_H_
#define INCLUDE_ACTSTATE_H_

//#define APP_SETTINGS_FILE ".settings.conf" // leading point for security reasons :)
#define ACT_STATE_FILE "states.conf" // There is no leading point for security reasons :)

#define SW_CNT 		0
#define SSW_CNT		0
#define MSW_CNT 	0

struct ActualStateStorage {

	bool needInit = false;
	bool* sw;
	bool* ssw;
	bool* msw;

	byte sw_cnt = SW_CNT;
	byte ssw_cnt = SSW_CNT;
	byte msw_cnt = MSW_CNT;

	ActualStateStorage() {
		//Initialization of rBoot OTA
		AppSettings.rBootInit();
		//sw1 = LOW;
		//sw2 = LOW;
		load();
	}

	~ActualStateStorage() {
		delete sw;
		delete ssw;
		delete msw;
	}

	void load() {
		DynamicJsonBuffer jsonBuffer;
		if (exist()) {
			int size = fileGetSize(ACT_STATE_FILE);
			char* jsonString = new char[size + 1];
			fileGetContent(ACT_STATE_FILE, jsonString, size + 1);
			JsonObject& root = jsonBuffer.parseObject(jsonString);

			JsonObject& jSW = root["sw"];
			sw_cnt = jSW["cnt"];
			if (sw_cnt > 0) {
				sw = new bool[sw_cnt];
				for (byte i=0; i < sw_cnt; i++)
					sw[i] = jSW[String(i)];
			}

			JsonObject& jSSW = root["ssw"];
			ssw_cnt = jSSW["cnt"];
			if (ssw_cnt > 0) {
				ssw = new bool[ssw_cnt];
				for (byte i=0; i < ssw_cnt; i++)
					ssw[i] = jSSW[String(i)];
			}

			JsonObject& jMSW = root["msw"];
			msw_cnt = jMSW["cnt"];
			if (msw_cnt > 0) {
				msw = new bool[msw_cnt];
				for (byte i=0; i < msw_cnt; i++)
					msw[i] = jMSW[String(i)];
			}
			delete[] jsonString;
		}
		else {
			needInit = true;
		}
	}

	void init() {
		if (needInit) {
			sw_cnt = AppSettings.sw_cnt;
			if (sw_cnt > 0) {
				sw = new bool[sw_cnt];
				for (byte i=0; i < sw_cnt; i++)
					sw[i] = false;
			}

			ssw_cnt = AppSettings.ssw_cnt;
			if (ssw_cnt > 0) {
				ssw = new bool[ssw_cnt];
				for (byte i=0; i < ssw_cnt; i++)
					ssw[i] = false;
			}

			msw_cnt = AppSettings.msw_cnt;
			if (msw_cnt > 0) {
				msw = new bool[msw_cnt];
				for (byte i=0; i < msw_cnt; i++)
					msw[i] = false;
			}
			needInit = false;
		}
	}

	void save()
	{
		DynamicJsonBuffer jsonBuffer;
		JsonObject& root = jsonBuffer.createObject();

		if (sw_cnt > 0) {
			JsonObject& jSW = jsonBuffer.createObject();
			root["sw"] = jSW;
			jSW["cnt"] = sw_cnt;
			for (byte i=0; i < sw_cnt; i++)
				jSW[String(i)] = sw[i];
		}

		if (ssw_cnt > 0) {
			JsonObject& jSSW = jsonBuffer.createObject();
			root["ssw"] = jSSW;
			jSSW["cnt"] = ssw_cnt;
			for (byte i=0; i < ssw_cnt; i++)
				jSSW[String(i)] = ssw[i];
		}

		if (msw_cnt > 0) {
			JsonObject& jMSW = jsonBuffer.createObject();
			root["msw"] = jMSW;
			jMSW["cnt"] = msw_cnt;
			for (byte i=0; i < msw_cnt; i++)
				jMSW[String(i)] = msw[i];
		}

		//TODO: add direct file stream writing
		fileSetContent(ACT_STATE_FILE, root.toJsonString());
		DEBUG4_PRINTLN(root.toJsonString());
		DEBUG4_PRINTLN("States file was saved");
	}

	bool exist() { return fileExist(ACT_STATE_FILE); }

	String printf() {
		String result;

		result = "SWITCHES[" + String(sw_cnt)+ "]\r\n";
		if (sw_cnt > 0) {
			for (byte i=0; i < sw_cnt; i++) {
				result += "\tsw" + String(i) + "=" + String(sw[i]) + "\r\n";
			}
		}
		result = "SERIAL[" + String(ssw_cnt)+ "]\r\n";
		if (ssw_cnt > 0) {
			for (byte i=0; i < ssw_cnt; i++) {
				result += "\tssw" + String(i) + "=" + String(ssw[i]) + "\r\n";
			}
		}
		result = "MCP[" + String(msw_cnt)+ "]\r\n";
		if (msw_cnt > 0) {
			for (byte i=0; i < msw_cnt; i++) {
				result += "\tmsw" + String(i) + "=" + String(msw[i]) + "\r\n";
			}
		}

		return result;

	}

	String print() {
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
	String update(JsonObject& root) {

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


	void updateNsave(JsonObject& root) {
		this->update(root);
		this->save();
	}
	 */

	void setSw(byte num, bool state) {
		if ((num >= 0) && (sw_cnt > num)) {
			if (sw[num] != state) {
				this->sw[num] = state;
				this->save();
			}
		}
		else {
			ERROR_PRINT("ERROR: setSw wrong number access");
		}
	}

	bool getSw(byte num) {
		bool result = null;
		if ((num >= 0) && (sw_cnt > num))
			result = this->sw[num];
		else
			ERROR_PRINT("ERROR: getSw wrong number access");
		return result;
	}

	void setSsw(byte num, bool state) {
		if ((num >= 0) && (ssw_cnt > num)) {
			if (ssw[num] != state) {
				this->ssw[num] = state;
				this->save();
			}
		}
		else {
			ERROR_PRINT("ERROR: setSsw wrong number access");
		}
	}

	bool getSsw(byte num) {
		bool result = false;
		if ((num >= 0) && (ssw_cnt > num))
			result = this->ssw[num];

		return result;
	}


	uint8_t getSsw() {
		uint8_t result = 0;
		if (ssw_cnt > 0) {
			for (byte i = 0; i < ssw_cnt; i++)
				result += ssw[i] << i;
		}
		//uint8_t sw = this->ssw1 + (this->ssw2 << 1) + (this->ssw3 << 2) + (this->ssw4 << 3) + (this->ssw5 << 4);
		return result;
	}


	void setMsw(byte num, bool state) {
		if ((num >= 0) && (msw_cnt > num)) {
			if (msw[num] != state) {
				this->msw[num] = state;
				this->save();
			}
		}
		else {
			ERROR_PRINT("ERROR: setMsw wrong number access");
		}
	}

	bool switchMsw(byte num) {
		bool state = !getMsw(num);
		setMsw(num, state);
		return state;
	}

	bool getMsw(byte num) {
		bool result = false;
		if ((num >= 0) && (msw_cnt > num))
			result = this->msw[num];

		return result;
	}

	String getMswString(byte num) {
		if (getMsw(num))
			return "ON";
		else
			return "OFF";
	}


	uint8_t getMsw() {
		uint8_t result = 0;
		if (msw_cnt > 0) {
			for (byte i = 0; i < msw_cnt; i++)
				result += msw[i] << i;
		}
		//uint8_t sw = this->ssw1 + (this->ssw2 << 1) + (this->ssw3 << 2) + (this->ssw4 << 3) + (this->ssw5 << 4);
		return result;
	}


	bool check() {
		//TODO: need to be coded check for mandatory fields
		return true;
	}

};

static ActualStateStorage ActStates;

#endif /* INCLUDE_ACTSTATE_H_ */
