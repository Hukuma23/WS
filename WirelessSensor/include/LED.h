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

#define RED 	"\x40\x00\x00"
#define GREEN 	"\x00\x40\x00"
#define BLUE 	"\x00\x00\x40"
#define BLACK 	"\x00\x00\x00"
#define WHITE 	"\x40\x40\x40"

void static ledRed(byte pin) {
	ws2812_writergb(pin, (char*)RED, 3);
}

void static ledGreen(byte pin) {
	ws2812_writergb(pin, (char*)GREEN, 3);
}
void static ledBlue(byte pin) {
	ws2812_writergb(pin, (char*)BLUE, 3);
}

void static ledBlack(byte pin) {
	ws2812_writergb(pin, (char*)BLACK, 3);
}

void static ledWhite(byte pin) {
	ws2812_writergb(pin, (char*)WHITE, 3);
}

#endif /* INCLUDE_LED_H_ */
