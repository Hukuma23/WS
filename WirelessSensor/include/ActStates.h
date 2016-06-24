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
#define ACT_STATE_FILE "states.conf" // There is no leading point for security reasons :)

#define CONST_SW_CNT 		0
#define CONST_SSW_CNT		0
#define CONST_MSW_CNT 		0


// Singleton
class ActStates {

private:

	/* Singleton part
	// Конструкторы и оператор присваивания недоступны клиентам
	ActStates(AppSettings& appSettings);
	~ActStates();

	ActStates(ActStates const&) = delete;
	ActStates& operator= (ActStates const&) = delete;
	 */

	//byte msw_cnt = CONST_MSW_CNT;
	Timer saveTimer;
	AppSettings &appSettings;

public:
    /* Singleton part
	static ActStates& getInstance() {
        static ActStates instance( AppSettings::getInstance());
        return instance;
    }
     */

	ActStates(AppSettings &appSettings);
	~ActStates();

	bool needInit = false;
	bool* sw;
	bool* ssw;
	bool* msw;

	//byte sw_cnt = CONST_SW_CNT;
	//byte ssw_cnt = CONST_SSW_CNT;

	void load();
	void init();
	void save();
	void save2file();
	bool exist();
	bool check();
	String printf();
	String print();
	//String update(JsonObject& root);
	//void updateNsave(JsonObject& root);

	void setSw(byte num, bool state);
	bool getSw(byte num);

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
