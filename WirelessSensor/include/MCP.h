#include <SmingCore/SmingCore.h>
#include <Libraries/MCP23017/MCP23017.h>
#include <Logger.h>

#ifndef INCLUDE_MCP_H_
#define INCLUDE_MCP_H_

//#define MCP_LED_PIN 	15
//#define MCP_BTN_A 		0
//#define MCP_BTN_B 		0

#define ESP_INT_PIN		12
//#define ESP_OUT_PIN 	14

#define DEBOUNCE_TIME 	20
#define LONG_TIME 		500

class MCP: protected MCP23017{

private:
	Timer timer;
	Timer timerBtnHandle;
	byte pin;
	//bool state = false;

	//byte mcpPinA = 0;
	byte sw_cnt;
	byte in_cnt;

	byte* sw;
	bool* sw_state;
	byte* in;

	void init(byte sw_cnt, byte in_cnt);
	void initArr(byte sw_cnt, byte in_cnt);
	void interruptCallback();
	void interruptHandler();
	void interruptReset();
	void longtimeHandler();
	void turnSw(byte num);
	void publish();

	byte getSwPinByNum(byte num);
	byte getInPinByNum(byte num);
	byte getInNumByPin(byte pin);

public:
	MCP(byte sw_cnt, byte in_cnt);
	~MCP();
	void startTimer();
	void stopTimer();



};
#endif /* INCLUDE_MCP_H_ */
