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

class LED {

private:
	byte bright;
	byte pin;

public:
	LED();
	LED(byte pin, byte bright);

	void setPin(byte pin);
	void setBright(byte bright);

	byte getPin();
	byte getBright();


	void red();
	void green();
	void blue();
	void black();
	void white();


	void rgb (byte red, byte green, byte blue);

};
#endif /* INCLUDE_LED_H_ */
