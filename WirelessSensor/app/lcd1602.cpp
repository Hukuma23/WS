#include <lcd1602.h>

LCD1602::LCD1602(AppSettings &appSettings) : LiquidCrystal_I2C(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE), appSettings(appSettings) {

	Wire.pins(appSettings.scl, appSettings.sda);
	Wire.begin();

	// LCD

	LiquidCrystal_I2C::begin(16,2);
	LiquidCrystal_I2C::setCursor(0,0);

	LiquidCrystal_I2C::print(appSettings.version);

	LiquidCrystal_I2C::createChar(0, temp_cel);
	LiquidCrystal_I2C::createChar(1, arrow_up);
	LiquidCrystal_I2C::createChar(2, arrow_down);
	LiquidCrystal_I2C::createChar(3, tilda);
	LiquidCrystal_I2C::createChar(4, two);

	LiquidCrystal_I2C::clear();
}

void LCD1602::printMain() {

	drawCO2();
	drawPressure();
	drawTemperature();
	drawHumidity();

}

void LCD1602::drawTemperature() {
	String strDownT = "T:";
	if (temp > 0)
		strDownT += "+";
	else
		strDownT += "-";

	strDownT += String(temp) + " C";
	LiquidCrystal_I2C::setCursor(0,1);
	LiquidCrystal_I2C::print(strDownT);
	LiquidCrystal_I2C::setCursor(7,1);
	LiquidCrystal_I2C::print(char(0));

}

void LCD1602::drawHumidity() {
	String strDownH = "H:" + String((int)hum) + "%";
	LiquidCrystal_I2C::setCursor(11,1);
	LiquidCrystal_I2C::print(strDownH);
}

void LCD1602::drawPressure() {
	String strUpPress = "P:" + String ((int)((float)press/133.322));
	LiquidCrystal_I2C::setCursor(11,0);
	LiquidCrystal_I2C::print(strUpPress);
}

void LCD1602::drawCO2() {
	String strUpCO = "CO  " + String(co2);
	LiquidCrystal_I2C::setCursor(0,0);
	LiquidCrystal_I2C::print(strUpCO);
	LiquidCrystal_I2C::setCursor(2,0);
	LiquidCrystal_I2C::print(char(4));
}

void LCD1602::setTemperature(float temp) {
	this->temp = temp;
	drawTemperature();
}
void LCD1602::setHumidity(float hum) {
	this->hum = hum;
	drawHumidity();
}
void LCD1602::setPressure(long press) {
	this->press = press;
	drawPressure();
}
void LCD1602::setCO2(int co2) {
	this->co2 = co2;
	drawCO2();
}
