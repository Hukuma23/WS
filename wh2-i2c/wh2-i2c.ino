/*
 * I2C message format:
 * First 1 byte - sensors count (byte type) from 0 to 5
 * Serial WH2Data packages, each length 8 bytes
 */

#include "WH2Sensor.h"
#include "WH2Storage.h"
#include <Wire.h>

WH2Data data2;
WH2Storage *storage = new WH2Storage();

void setup() {
  Serial.begin(9600);
  Wire.begin(23);                // join i2c bus with address #8
  Wire.onRequest(requestEvent); // register event
  wh2_setup();
  Serial.println("wh2-i2c started");
}


void requestEvent() {

  byte *buffer = storage->getI2CBuffer();
  printHex(buffer, storage->bufferSize);
  Wire.write(buffer, storage->bufferSize);
  
}

void printHex(byte *buffer, byte size) {
  for (int i=0; i < size; i++) {
    Serial.print(" 0x");
    Serial.print(buffer[i], HEX);
  }
  Serial.println();
}

/*
void requestEvent() {
  byte b[sizeof(data2)];
  memcpy(b, &data2, sizeof(data2));

  Serial.print("Sent ");
  Serial.print(sizeof(b));
  Serial.println(" bytes");

  //Wire.write(b); // respond with message of 6 bytes
  Wire.write(b, sizeof(b));
  clearData();
  // as expected by master
}

void clearData() {
  data2.actual = false;
}
*/

void loop() {
  // put your main code here, to run repeatedly:

  wh2_loop();
  //setData(wh2);
  /*
  if (isDataBacked()) {
    data2 = getData();
    Serial.println("data2 = getData()");
  }*/

}
