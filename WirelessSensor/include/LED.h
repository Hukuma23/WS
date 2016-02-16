/*
 * LED.h
 *
 *  Created on: 02 февраля 2016 г.
 *      Author: Nikita Litvinov
 */

#include <SmingCore/SmingCore.h>
#include <WS2812/WS2812.h>

#ifndef INCLUDE_LED_H_
#define INCLUDE_LED_H_

#define BRIGHT	0x10

struct RGB {
private:
	byte iBright;
	byte iRed;
	byte iGreen;
	byte iBlue;
public:
	RGB() : iBright(BRIGHT), iRed(0), iGreen(0), iBlue(0) {};
	RGB(byte bright) : iBright(bright), iRed(0), iGreen(0), iBlue(0) {};

};

class LED {

private:
	byte bright;
	byte pin;
	byte cnt;
	char* led;

	void setColor(byte num, char* color);

public:
	LED();
	~LED();
	LED(byte _cnt);
	LED(byte pin, byte cnt, byte bright);

	void setPin(byte pin);
	void setBright(byte bright);
	void show();
	void setCount(byte cnt);

	byte getPin();
	byte getCount();
	byte getBright();


	void red(byte num);
	void green(byte num);
	void blue(byte num);
	void black(byte num);
	void white(byte num);


	void rgb (byte num, byte red, byte green, byte blue);

};
#endif /* INCLUDE_LED_H_ */
