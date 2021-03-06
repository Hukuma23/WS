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
	//DEBUG4_PRINTF("LED()r\n", this);
	led = new char[cnt*3];
	printId();
}

LED::LED(byte _pin, byte _cnt, byte _bright) : pin(_pin), cnt(_cnt), bright(_bright)  {
	//DEBUG4_PRINTF("LED(pin, cnt, bright)r\n", this);
	led = new char[cnt*3];
	printId();
}

LED::LED(byte _cnt) : pin(0), cnt(_cnt), bright(BRIGHT)  {
	//DEBUG4_PRINTF("LED(cnt)r\n", this);
	led = new char[cnt*3];
	printId();
}

void LED::printId() {
	DEBUG4_PRINTF("led=%p\r\n", this);
}

void LED::setCount(byte cnt) {
	//DEBUG4_PRINTF("LED::setCount()\r\n");

	if ((this->cnt != cnt)  && (cnt > 0)){
		delete led;
		this->cnt = cnt;
		led = new char[cnt*3];

		//for (byte i=0; i < cnt*3; i++) led[i] = 0;
	}
}

void LED::initRG(bool* arr, byte arr_cnt) {
	print();
	byte cnt = (this->cnt<arr_cnt?this->cnt:arr_cnt);
	//DEBUG4_PRINTF("initRG.cnt = %d\r\n", cnt);
	for (byte i = 0; i < cnt; i++) {
		//DEBUG4_PRINTF("LED::initRG arr[%d] = %d, ", i, arr[i]);
		if (arr[i]) { 	//GREEN
			//DEBUG4_PRINT(" ..green");
			led[3*i] = bright;
			led[3*i + 1] = 0;
			led[3*i + 2] = 0;
			//DEBUG4_PRINTLN(" ..ok");
		}
		else {			// RED
			//DEBUG4_PRINT(" ..red");
			led[3*i] = 0;
			led[3*i + 1] = bright;
			led[3*i + 2] = 0;
			//DEBUG4_PRINTLN(" ..ok");
		}
	}
	show();
	print();

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
	//DEBUG4_PRINTF("LED::setColor()\r\n");
	print();

	if (num >= cnt)
		return false;

	byte nn = num * 3;
	for (byte i=0; i < 3; i++) {
		led[nn+i] = color[i];
	}
	print();
	return true;
}

void LED::show() {
	//DEBUG4_PRINTF("LED::show()\r\n");
	print();
	ws2812_writegrb(pin, led, (cnt*3));
	print();
}

void LED::red(byte num) {
	//DEBUG4_PRINTF("LED::red()\r\n");
	print();
	char color[3] = {0,bright,0};
	setColor(num, color);
	show();
}

void LED::green(byte num) {
	//DEBUG4_PRINTF("LED::green()\r\n");
	print();
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
	/*printId();
	DEBUG4_PRINTF("led[%d] (G R B): ", cnt);
	for (byte i = 0; i < cnt; i++){
		for (byte c=0; c < 3; c++) {
			DEBUG4_PRINTF("%03d ", (byte)led[(i*3)+c]);
		}
		DEBUG4_PRINTF(" ");
	}
	DEBUG4_PRINTF("\r\n");
	*/
}

void LED::showOn(byte num) {
	//DEBUG4_PRINTF("LED::showOn()\r\n");
	print();
	green(num);
}

void LED::showOff(byte num) {
	//DEBUG4_PRINTF("LED::showOff()\r\n");
	print();
	red(num);
}
