/*
 * ActStates.h
 *
 *  Created on: 21 сент. 2015 г.
 *      Author: Nikita
 *
 *      Actual State of switches and other information
 */

/*

 */


#include <SmingCore/SmingCore.h>
#include <Logger.h>


#ifndef INCLUDE_ACTSTATE_H_
#define INCLUDE_ACTSTATE_H_

//#define APP_SETTINGS_FILE ".settings.conf" // leading point for security reasons :)
#define ACT_STATE_FILE "states.conf" // There is no leading point for security reasons :)

struct ActualStateStorage {

	bool sw1 = LOW;
	bool sw2 = LOW;
	bool ssw1 = LOW;
	bool ssw2 = LOW;
	bool ssw3 = LOW;
	bool ssw4 = LOW;
	bool ssw5 = LOW;


	ActualStateStorage() {
		//Initialization of rBoot OTA
		rBootInit();
		//sw1 = LOW;
		//sw2 = LOW;
		load();
	}

	void load()
	{
		DynamicJsonBuffer jsonBuffer;
		if (exist())
		{
			int size = fileGetSize(ACT_STATE_FILE);
			char* jsonString = new char[size + 1];
			fileGetContent(ACT_STATE_FILE, jsonString, size + 1);
			JsonObject& root = jsonBuffer.parseObject(jsonString);

			JsonObject& switches = root["switches"];

			sw1 = switches["sw1"];
			sw2 = switches["sw2"];

			ssw1 = switches["ssw1"];
			ssw2 = switches["ssw2"];
			ssw3 = switches["ssw3"];
			ssw4 = switches["ssw4"];
			ssw5 = switches["ssw5"];

			delete[] jsonString;
		}
	}

	void save()
	{
		DynamicJsonBuffer jsonBuffer;
		JsonObject& root = jsonBuffer.createObject();

		JsonObject& switches = jsonBuffer.createObject();
		root["switches"] = switches;
		switches["sw1"] = sw1;
		switches["sw2"] = sw2;

		switches["ssw1"] = ssw1;
		switches["ssw2"] = ssw2;
		switches["ssw3"] = ssw3;
		switches["ssw4"] = ssw4;
		switches["ssw5"] = ssw5;

		//TODO: add direct file stream writing
		fileSetContent(ACT_STATE_FILE, root.toJsonString());
		DEBUG4_PRINTLN(root.toJsonString());
		DEBUG4_PRINTLN("States file was saved");
	}

	bool exist() { return fileExist(ACT_STATE_FILE); }

	void rBootInit() {
		// mount spiffs
		int slot = rboot_get_current_rom();
#ifndef DISABLE_SPIFFS
		if (slot == 0) {
#ifdef RBOOT_SPIFFS_0
			spiffs_mount_manual(RBOOT_SPIFFS_0 + 0x40200000, SPIFF_SIZE);
#else
			spiffs_mount_manual(0x40300000, SPIFF_SIZE);
#endif
		} else {
#ifdef RBOOT_SPIFFS_1
			//DEBUG4_PRINTF2("trying to mount spiffs at %x, length %d", RBOOT_SPIFFS_1 + 0x40200000, SPIFF_SIZE);
#else
			spiffs_mount_manual(0x40500000, SPIFF_SIZE);
#endif
		}
#else
#endif

	}

	String printf() {
		String result;

		result = "SWITCHES\r\n";
		result += "\tsw1=" + String(sw1) + "\r\n";
		result += "\tsw2=" + String(sw2) + "\r\n";
		result += "SERIAL\r\n";
		result += "\tssw1=" + String(ssw1) + "\r\n";
		result += "\tssw2=" + String(ssw2) + "\r\n";
		result += "\tssw3=" + String(ssw3) + "\r\n";
		result += "\tssw4=" + String(ssw4) + "\r\n";
		result += "\tssw5=" + String(ssw5) + "\r\n";
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

	String update(JsonObject& root) {

		String result;

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

	void setSw1(bool state) {
		this->sw1 = state;
		this->save();
	}
	void setSw2(bool state) {
		this->sw2 = state;
		this->save();
	}
	void setSsw1(bool state) {
		this->ssw1 = state;
		this->save();
	}
	void setSsw2(bool state) {
		this->ssw2 = state;
		this->save();
	}
	void setSsw3(bool state) {
		this->ssw3 = state;
		this->save();
	}
	void setSsw4(bool state) {
		this->ssw4 = state;
		this->save();
	}
	void setSsw5(bool state) {
		this->ssw5 = state;
		this->save();
	}

	uint8_t getSsw() {
		uint8_t sw = this->ssw1 + (this->ssw2 << 1) + (this->ssw3 << 2) + (this->ssw4 << 3) + (this->ssw5 << 4);
		return sw;
	}
	bool check() {
		//TODO: need to be coded check for mandatory fields
		return true;
	}

};

static ActualStateStorage ActStates;

#endif /* INCLUDE_ACTSTATE_H_ */
