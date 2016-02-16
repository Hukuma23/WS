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

void LED::setColor(byte num, char* color) {
	byte nn = num * 3;
	for (byte i=0; i < 3; i++) {
		led[nn+i] = color[i];
	}
}

void LED::show() {
	ws2812_writergb(pin, led, sizeof(led));
}

void LED::red(byte num) {
	char color[3] = {bright,0,0};
	setColor(num, color);
	show();
}

void LED::green(byte num) {
	char color[3] = {0,bright,0};
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
	char color[3] = {red,green,blue};
	setColor(num, color);
	show();
}


