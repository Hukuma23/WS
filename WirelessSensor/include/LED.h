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


void static ledRed(byte pin, byte bright = BRIGHT) {
	char color[3] = {bright,0,0};
	ws2812_writergb(pin, color, sizeof(color));
}

void static ledGreen(byte pin, byte bright = BRIGHT) {
	char color[3] = {0,bright,0};
	ws2812_writergb(pin, color, sizeof(color));
}
void static ledBlue(byte pin, byte bright = BRIGHT) {
	char color[3] = {0,0,bright};
	ws2812_writergb(pin, color, sizeof(color));
}

void static ledBlack(byte pin) {
	char color[3] = {0,0,0};
	ws2812_writergb(pin, color, sizeof(color));
}

void static ledWhite(byte pin, byte bright = BRIGHT) {
	char color[3] = {bright,bright,bright};
	ws2812_writergb(pin, color, sizeof(color));
}

#endif /* INCLUDE_LED_H_ */
