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

#define CONST_SW_CNT 		0
#define CONST_SSW_CNT		0
#define CONST_MSW_CNT 		0

struct ActualStateStorage {

private:
	//byte msw_cnt = CONST_MSW_CNT;

public:
	bool needInit = false;
	bool* sw;
	bool* ssw;
	bool* msw;

	//byte sw_cnt = CONST_SW_CNT;
	//byte ssw_cnt = CONST_SSW_CNT;


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
			byte sw_cnt = jSW["cnt"];

			if (sw_cnt > 0) {
				sw = new bool[sw_cnt];
				for (byte i=0; i < sw_cnt; i++)
					sw[i] = jSW[String(i)];
			}

			if (sw_cnt != AppSettings.sw_cnt) {
				ERROR_PRINTF("ERROR: ASt.sw_cnt(%d) != AS.sw_cnt(%d)", sw_cnt, AppSettings.sw_cnt);
				sw_cnt = AppSettings.sw_cnt;
			}

			JsonObject& jSSW = root["ssw"];
			byte ssw_cnt = jSSW["cnt"];
			if (ssw_cnt > 0) {
				ssw = new bool[ssw_cnt];
				for (byte i=0; i < ssw_cnt; i++)
					ssw[i] = jSSW[String(i)];
			}

			if (ssw_cnt != AppSettings.ssw_cnt) {
				ERROR_PRINTF("ERROR: ASt.ssw_cnt(%d) != AS.ssw_cnt(%d)", ssw_cnt, AppSettings.ssw_cnt);
				ssw_cnt = AppSettings.ssw_cnt;
			}

			JsonObject& jMSW = root["msw"];
			byte msw_cnt = jMSW["cnt"];

			if (msw_cnt > 0) {
				msw = new bool[msw_cnt];
				for (byte i=0; i < msw_cnt; i++)
					msw[i] = jMSW[String(i)];
			}

			if (msw_cnt != AppSettings.msw_cnt) {
				ERROR_PRINTF("ERROR: ASt.msw_cnt(%d) != AS.msw_cnt(%d)", msw_cnt, AppSettings.msw_cnt);
				msw_cnt = AppSettings.msw_cnt;
			}

			delete[] jsonString;
		}
		else {
			needInit = true;
		}
	}

	void init() {
		DEBUG4_PRINTLN("ASt.init");
		if (needInit) {
			DEBUG4_PRINTLN("ASt.1");
			if (AppSettings.sw_cnt > 0) {
				sw = new bool[AppSettings.sw_cnt];
				for (byte i=0; i < AppSettings.sw_cnt; i++)
					sw[i] = false;
			}
			DEBUG4_PRINTLN("ASt.2");

			if (AppSettings.ssw_cnt > 0) {
				ssw = new bool[AppSettings.ssw_cnt];
				for (byte i=0; i < AppSettings.ssw_cnt; i++)
					ssw[i] = false;
			}

			if (AppSettings.msw_cnt > 0) {
				DEBUG4_PRINTLN("ASt.3.1");
				msw = new bool[AppSettings.msw_cnt];
				for (byte i=0; i < AppSettings.msw_cnt; i++)
					msw[i] = false;
			}

			needInit = false;
			this->save();
			DEBUG4_PRINTLN("ASt.5");
		}
	}

	void save()
	{
		DynamicJsonBuffer jsonBuffer;
		JsonObject& root = jsonBuffer.createObject();

		if (AppSettings.sw_cnt > 0) {
			JsonObject& jSW = jsonBuffer.createObject();
			root["sw"] = jSW;
			jSW["cnt"] = AppSettings.sw_cnt;
			for (byte i=0; i < AppSettings.sw_cnt; i++)
				jSW[String(i)] = sw[i];
		}

		if (AppSettings.ssw_cnt > 0) {
			JsonObject& jSSW = jsonBuffer.createObject();
			root["ssw"] = jSSW;
			jSSW["cnt"] = AppSettings.ssw_cnt;
			for (byte i=0; i < AppSettings.ssw_cnt; i++)
				jSSW[String(i)] = ssw[i];
		}

		if (AppSettings.msw_cnt > 0) {
			JsonObject& jMSW = jsonBuffer.createObject();
			root["msw"] = jMSW;
			jMSW["cnt"] = AppSettings.msw_cnt;
			for (byte i=0; i < AppSettings.msw_cnt; i++)
				jMSW[String(i)] = msw[i];
		}

		//TODO: add direct file stream writing
		fileSetContent(ACT_STATE_FILE, root.toJsonString());
		DEBUG1_PRINTLN(root.toJsonString());
		DEBUG1_PRINTLN("States file was saved");
	}

	bool exist() { return fileExist(ACT_STATE_FILE); }

	String printf() {
		String result;

		result = "SWITCHES[" + String(AppSettings.sw_cnt)+ "]\r\n";
		if (AppSettings.sw_cnt > 0) {
			for (byte i=0; i < AppSettings.sw_cnt; i++) {
				result += "\tsw" + String(i) + "=" + String(sw[i]) + "\r\n";
			}
		}
		result = "SERIAL[" + String(AppSettings.ssw_cnt)+ "]\r\n";
		if (AppSettings.ssw_cnt > 0) {
			for (byte i=0; i < AppSettings.ssw_cnt; i++) {
				result += "\tssw" + String(i) + "=" + String(ssw[i]) + "\r\n";
			}
		}
		result = "MCP[" + String(AppSettings.msw_cnt)+ "]\r\n";
		if (AppSettings.msw_cnt > 0) {
			for (byte i=0; i < AppSettings.msw_cnt; i++) {
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
		if ((num >= 0) && (AppSettings.sw_cnt > num)) {
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
		if ((num >= 0) && (AppSettings.sw_cnt > num))
			result = this->sw[num];
		else
			ERROR_PRINT("ERROR: getSw wrong number access");
		return result;
	}

	void setSsw(byte num, bool state) {
		if ((num >= 0) && (AppSettings.ssw_cnt > num)) {
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
		if ((num >= 0) && (AppSettings.ssw_cnt > num))
			result = this->ssw[num];

		return result;
	}


	uint8_t getSsw() {
		uint8_t result = 0;
		if (AppSettings.ssw_cnt > 0) {
			for (byte i = 0; i < AppSettings.ssw_cnt; i++)
				result += ssw[i] << i;
		}
		//uint8_t sw = this->ssw1 + (this->ssw2 << 1) + (this->ssw3 << 2) + (this->ssw4 << 3) + (this->ssw5 << 4);
		return result;
	}


	void setMsw(byte num, bool state) {
		if ((num >= 0) && (AppSettings.msw_cnt > num)) {
			if (msw[num] != state) {
				this->msw[num] = state;
				this->save();
			}
		}
		else {
			ERROR_PRINTF("ERROR: setMsw wrong number (%d) access. msw_cnt=%d", num, AppSettings.msw_cnt);
		}
	}

	bool switchMsw(byte num) {
		bool state = !getMsw(num);
		setMsw(num, state);
		return state;
	}

	bool getMsw(byte num) {
		bool result = false;
		if ((num >= 0) && (AppSettings.msw_cnt > num))
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
		if (AppSettings.msw_cnt > 0) {
			for (byte i = 0; i < AppSettings.msw_cnt; i++)
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
