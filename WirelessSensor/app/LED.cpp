/*
 * LED.cpp
 *
 *  Created on: 05 февр. 2016 г.
 *      Author: Nikita
 */

#include <LED.h>



LED::~LED() {
	delete led;
}

LED::LED() : pin(0), bright(BRIGHT), cnt(1) {
	led = new char[cnt*3];
}

LED::LED(byte _pin, byte _cnt, byte _bright) : pin(_pin), cnt(_cnt), bright(_bright)  {
	led = new char[cnt*3];
}

LED::LED(byte _cnt) : pin(0), cnt(_cnt), bright(BRIGHT)  {
	led = new char[cnt*3];
}



void LED::setCount(byte cnt) {
	if ((this->cnt != cnt)  && (cnt > 0)){
		delete led;
		this->cnt = cnt;
		led = new char[cnt*3];
	}
}

byte LED::getCount() {
	return cnt;
}

void LED::setPin(byte pin) {
	this->pin = pin;
}
void LED::setBright(byte bright) {
	this->bright = bright;
}

byte LED::getPin() {
	return pin;
}
byte LED::getBright() {
	return bright;
}

bool LED::setColor(byte num, char* color) {
	if (num >= cnt)
		return false;

	byte nn = num * 3;
	for (byte i=0; i < 3; i++) {
		led[nn+i] = color[i];
	}
	return true;
}

void LED::show() {
	ws2812_writegrb(pin, led, (cnt*3));
}

void LED::red(byte num) {
	char color[3] = {0,bright,0};
	setColor(num, color);
	show();
}

void LED::green(byte num) {
	char color[3] = {bright,0,0};
	setColor(num, color);
	show();
}
void LED::blue(byte num) {
	char color[3] = {0,0,bright};
	setColor(num, color);
	show();
}

void LED::black(byte num) {
	char color[3] = {0,0,0};
	setColor(num, color);
	show();
}

void LED::white(byte num) {
	char color[3] = {bright,bright,bright};
	setColor(num, color);
	show();
}

void LED::rgb (byte num, byte red, byte green, byte blue) {
	char color[3] = {green,red,blue};
	setColor(num, color);
	show();
}

void LED::print() {
	DEBUG4_PRINTF("led[%d] (G R B): ", cnt);
	for (byte i = 0; i < cnt; i++){
		for (byte c=0; c < 3; c++) {
			DEBUG4_PRINTF("%03d ", (byte)led[(i*3)+c]);
		}
		DEBUG4_PRINT("  ");
	}
}

void LED::showOn(byte num) {
	green(num);
}

void LED::showOff(byte num) {
	red(num);
}
