/*
 * LED.cpp
 *
 *  Created on: 05 февр. 2016 г.
 *      Author: Nikita
 */

#include <LED.h>

#define BRIGHT	0x10


LED::LED() : pin(0), bright(BRIGHT) {}

LED::LED(byte pin, byte bright) {
	this->pin = pin;
	this->bright = bright;
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

void LED::red() {
	char color[3] = {bright,0,0};
	ws2812_writergb(pin, color, sizeof(color));
}

void LED::green() {
	char color[3] = {0,bright,0};
	ws2812_writergb(pin, color, sizeof(color));
}
void LED::blue() {
	char color[3] = {0,0,bright};
	ws2812_writergb(pin, color, sizeof(color));
}

void LED::black() {
	char color[3] = {0,0,0};
	ws2812_writergb(pin, color, sizeof(color));
}

void LED::white() {
	char color[3] = {bright,bright,bright};
	ws2812_writergb(pin, color, sizeof(color));
}

void LED::rgb (byte red, byte green, byte blue) {
	char color[3] = {red,green,blue};
	ws2812_writergb(pin, color, sizeof(color));
}


