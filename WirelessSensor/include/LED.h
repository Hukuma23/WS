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


#define RED 	"\x20\x00\x00"
#define GREEN 	"\x00\x20\x00"
#define BLUE 	"\x00\x00\x20"
#define BLACK 	"\x00\x00\x00"
#define WHITE 	"\x20\x20\x20"

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
