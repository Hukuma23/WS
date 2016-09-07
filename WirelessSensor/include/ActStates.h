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
#include <AppSettings.h>


#ifndef INCLUDE_ACTSTATE_H_
#define INCLUDE_ACTSTATE_H_

//#define APP_SETTINGS_FILE ".settings.conf" // leading point for security reasons :)
#define SW_ACT_STATE_FILE "sw.states" // There is no leading point for security reasons :)
#define SSW_ACT_STATE_FILE "ssw.states" // There is no leading point for security reasons :)
#define MCP_ACT_STATE_FILE "mcp.states" // There is no leading point for security reasons :)

// Singleton
class ActStates {

private:
	Timer saveTimer;
	AppSettings &appSettings;

public:
	ActStates(AppSettings &appSettings);
	~ActStates();

	bool needInit = false;
	bool* sw;
	bool* ssw;
	bool* msw;


	bool load(bool arr[], byte cnt, String fileName);
	void loadStates();

	void init();
	void save();
	void save2file();
	bool exist(String fileName);
	bool check();
	String printf();
	String print(String fileName);
	String print();
	//String update(JsonObject& root);
	//void updateNsave(JsonObject& root);

	void setSw(byte num, bool state);
	bool getSw(byte num);
	bool switchSw(byte num);
	String getSwString(byte num);

	void setSsw(byte num, bool state);
	bool getSsw(byte num);
	uint8_t getSsw();

	void setMsw(byte num, bool state) ;
	bool getMsw(byte num);
	String getMswString(byte num);
	uint8_t getMsw();
	bool switchMsw(byte num);

	void initLED();
	void initLED(bool* arr, byte cnt);
};

#endif /* INCLUDE_ACTSTATE_H_ */
