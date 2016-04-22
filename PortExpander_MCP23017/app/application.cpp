#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include <Libraries/MCP23017/MCP23017.h>

#define MCP_LED_PIN 	15
#define MCP_BTN_A 		0
#define MCP_BTN_B 		0

#define ESP_INT_PIN		12
#define ESP_OUT_PIN 	14

#define DEBOUNCE_TIME 	20
#define LONG_TIME 		500

MCP23017 mcp;
volatile boolean awakenByInterrupt = false;
byte mcpPinA = 0;

Timer timer;
Timer timerBtnHandle;

unsigned long intTime=0;

bool state = false;
uint8_t pin;

void interruptCallback();
void interruptHandler();
void init_mcp();
void longtimeHandler();


void flush() {
	if (state) {
		state = false;
		mcp.digitalWrite(MCP_LED_PIN, LOW);
		Serial.println("OFF");
	}
	else {
		state = true;
		mcp.digitalWrite(MCP_LED_PIN, HIGH);
		Serial.println("ON");
	}
	//attachInterrupt(ESP_INT_PIN, interruptCallback, FALLING);
	//pinMode(ESP_INT_PIN, OUTPUT);
	//delay(10);
	//pinMode(ESP_INT_PIN, INPUT);
	init_mcp();

}


void init_mcp() {
	uint8_t pin = mcp.getLastInterruptPin();
	uint8_t last_state = mcp.getLastInterruptPinValue();

	Serial.printf("interrupt pin= %d, state=%d", pin, last_state);
	Serial.println();

}

void interruptHandler() {
	//awakenByInterrupt = true;

	pin = mcp.getLastInterruptPin();
	uint8_t last_state = mcp.getLastInterruptPinValue();
	uint8_t act_state = mcp.digitalRead(pin);
	//Serial.printf("spent time=%d", (millis() - intTime));
	//Serial.println();

	Serial.printf("push pin=%d state=%d. ", pin, act_state);


	if (act_state == LOW)
		timerBtnHandle.initializeMs(LONG_TIME, longtimeHandler).startOnce();
	else
		Serial.println();

}

void longtimeHandler() {

	uint8_t act_state = mcp.digitalRead(pin);
	while (!(mcp.digitalRead(pin)));
	Serial.printf("push pin=%d state=%d. ", pin, act_state);


	if (act_state == LOW)
		Serial.println("*-long pressed-*");
	else
		Serial.println("*-pressed-*");


	attachInterrupt(ESP_INT_PIN, interruptCallback, FALLING);
}


void interruptCallback() {
	Serial.print("Interrupt Called! ");
	detachInterrupt(ESP_INT_PIN);
	timerBtnHandle.initializeMs(DEBOUNCE_TIME, interruptHandler).startOnce();
}

/*
void interruptCallback() {
	awakenByInterrupt = true;
	Serial.println("Interrupt Called" + String(intTime++));
	while (!(mcp.digitalRead(mcpPinA)));
}
*/

// python -m serial.tools.miniterm  --rts 0 --dtr 0 /dev/tty.usbserial-A50285BI 115200

void init() {

	Serial.begin(115200);

	Wire.pins(4, 5);

	mcp.begin(0);

	pinMode(ESP_INT_PIN, INPUT);

	/*
	for (uint8_t i = 0; i < 8; i++) {
		mcp.pinMode(i, INPUT);
		mcp.pullUp(i, HIGH);
		mcp.setupInterruptPin(i, FALLING);
	}
	*/

	/*
	mcp.pinMode(MCP_BTN_A, INPUT);
	mcp.pullUp(MCP_BTN_A, HIGH);
	mcp.setupInterruptPin(MCP_BTN_A, FALLING);

	mcp.pinMode(MCP_BTN_B, INPUT);
	mcp.pullUp(MCP_BTN_B, HIGH);
	mcp.setupInterruptPin(MCP_BTN_B, FALLING);
*/

	mcp.pinMode(MCP_LED_PIN, OUTPUT);
	mcp.digitalWrite(MCP_LED_PIN, LOW);




	//pullup(ESP_INT_PIN);

	mcp.setupInterrupts(false, false, LOW);

	mcp.pinMode(mcpPinA, INPUT);
	//mcp.pullUp(mcpPinA, HIGH);

	mcp.setupInterruptPin(mcpPinA, FALLING);

	attachInterrupt(ESP_INT_PIN, interruptCallback, FALLING);

	timer.initializeMs(30 * 1000, flush).start(); // every 25 seconds
	init_mcp();
}
