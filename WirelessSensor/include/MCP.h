#include <SmingCore/SmingCore.h>
#include <Libraries/MCP23017/MCP23017.h>
#include <Module.h>
#include <Logger.h>

#ifndef INCLUDE_MCP_H_
#define INCLUDE_MCP_H_

//#define MCP_LED_PIN 	15
//#define MCP_BTN_A 		0
//#define MCP_BTN_B 		0

//#define ESP_INT_PIN		12
//#define ESP_OUT_PIN 	14

#define DEBOUNCE_TIME 	20
#define LONG_TIME 		500

class MCP: protected MCP23017 {

private:
	Timer timer;
	Timer timerBtnHandle;
	byte pin;
	MQTT *mqtt;

	void init(MQTT &mqtt);
	void interruptCallback();
	void interruptHandler();
	void interruptReset();
	void longtimeHandler();
	void publish();
	void start();

public:
	MCP(MQTT &mqtt);
	~MCP();
	void startTimer();
	void stopTimer();
	bool turnSw(byte num);
	bool turnSw(byte num, bool state);

	bool processCallback(String topic, String message);



};
#endif /* INCLUDE_MCP_H_ */
