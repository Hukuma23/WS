/*
 * LED.h
 *
 *  Created on: 02 февраля 2016 г.
 *      Author: Nikita Litvinov
 *
 *      inside the Class we use GRB instead of RGB because GRB will be used by WS2812b natively
 */

#include <SmingCore/SmingCore.h>
#include <WS2812/WS2812.h>
#include <Logger.h>

#ifndef INCLUDE_LED_H_
#define INCLUDE_LED_H_

#define BRIGHT	0x10

class LED {

private:
	byte bright;
	byte pin;
	byte cnt;
	char* led; // !!!-- GRB will be used instead of RGB --!!!

	bool setColor(byte num, char* color);

public:
	LED();
	~LED();
	LED(byte _cnt);
	LED(byte pin, byte cnt, byte bright = BRIGHT);

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

	void showOn(byte num);
	void showOff(byte num);

	void print();
	void printId();

	void initRG(bool* arr, byte arr_cnt);


	void rgb (byte num, byte red, byte green, byte blue);

};
#endif /* INCLUDE_LED_H_ */
