#include <AppSettings.h>
#include <Libraries/LiquidCrystal/LiquidCrystal_I2C.h>

#ifndef INCLUDE_LCD1602_
#define INCLUDE_LCD1602_


class LCD1602: public LiquidCrystal_I2C {

private:
	AppSettings& appSettings;
	int undef = -99;

	byte temp_cel[8] = {
		0b00111,
		0b00101,
		0b00111,
		0b00000,
		0b00000,
		0b00000,
		0b00000,
		0b00000
	};

	byte arrow_up[8] = {
		0b00000,
		0b00100,
		0b01110,
		0b10101,
		0b00100,
		0b00100,
		0b00100,
		0b00000
	};

	byte arrow_down[8] = {
		0b00000,
		0b00100,
		0b00100,
		0b00100,
		0b10101,
		0b01110,
		0b00100,
		0b00000
	};

	byte tilda[8] = {
		0b00000,
		0b00000,
		0b01000,
		0b10101,
		0b00010,
		0b00000,
		0b00000,
		0b00000
	};

	byte two[8] = {
		0b00000,
		0b00001,
		0b00000,
		0b11000,
		0b00100,
		0b01000,
		0b10001,
		0b11100
	};

	float temp = undef;
	float hum = undef;

	long press = undef;
	int co2 = undef;

public:
	LCD1602(AppSettings &appSettings);
	void begin();

	void setTemperature(float temp);
	void setHumidity(float hum);
	void setPressure(long press);
	void setCO2(int co2);

	void drawTemperature();
	void drawHumidity();
	void drawPressure();
	void drawCO2();
	void printMain();

};

#endif /* INCLUDE_LCD1602_ */
