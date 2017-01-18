/* 
 * File:   WH2Storage.h
 * Author: Nikita
 *
 * Created on 17 January 2017 г., 14:40
 */

#ifndef WH2STORAGE_H
#define WH2STORAGE_H

#define UNDEF             127
#define SENSORS_MAX_CNT   3
//#define BUFFER_LENGTH 1 + 8 * SENSORS_MAX_CNT


// 8 bytes length
struct WH2Data {
  bool actual;
  uint8_t humidity;
  int16_t temperature;
  uint16_t sensorId;
  uint16_t secSinceGot;
};

class WH2Storage {
  public:
    WH2Storage() {
      clearAll();
    }

    byte bufferSize = 1 + dataSize * SENSORS_MAX_CNT;
    
    void add(uint16_t sensorId, uint16_t secSinceGot, int16_t temperature, byte humidity) {
      Serial.print("dataSize=");
      Serial.println(dataSize);
      
      Serial.print("bufferSize=");
      Serial.println(bufferSize);

      
      Serial.println("storage.add ");
      byte num = getKey(sensorId);
      if (num == UNDEF) return;

      Serial.print("storage.add: key=");
      Serial.println(num);
      
      WH2Data data;
      data.actual = true;
      data.sensorId = sensorId;
      data.secSinceGot = secSinceGot;
      data.temperature = temperature;
      data.humidity = humidity;
      this->wh2data[num] = data;
    }

    byte *getI2CBuffer() {
      Serial.println("getI2CBuffer.start");
      byte buffer[bufferSize];
      for (int i=0; i < bufferSize; i++) {
        buffer[i] = 0;
      }
      byte cnt = getActualCount();
      buffer[0] = cnt;
      printHex(buffer, bufferSize);
      
      resetRead();
      for (int i=0; i < cnt; i++) {
        byte *buf = getNextDataBuffer();

        Serial.print("buf=");
        printHex(buf, dataSize);
        
        for (int j=0; j < dataSize; j++) {
          buffer[1 + dataSize*i + j] = buf[j];
        }
        Serial.print(i);
        printHex(buffer, bufferSize);

      }

      clearAll();
      Serial.println("getI2CBuffer.end");
      return buffer;
      
    }

  private:
    byte dataSize = sizeof(WH2Data);
    
    
    byte sensorsCount = 0;
    WH2Data wh2data[SENSORS_MAX_CNT];
    uint16_t sensorKeys[SENSORS_MAX_CNT];
    byte currentNum = 0;

void printHex(byte *buffer, byte size) {
  for (int i=0; i < size; i++) {
    Serial.print(" 0x");
    Serial.print(buffer[i], HEX);
  }
  Serial.println();
}
    
    byte getKey(uint16_t key) {
      for (int i=0; i < sensorsCount; i++)  {
        if (key == sensorKeys[i]) return i;
      }

      if (sensorsCount < SENSORS_MAX_CNT) {
        sensorKeys[sensorsCount] = key;
        return sensorsCount++;
      } else {
        return UNDEF;
      }
    }

    void clearAll() {
      for (int i=0; i < SENSORS_MAX_CNT; i++) {
        wh2data[i].actual = false;
      }
    }

    byte getActualCount() {
      byte cnt = 0;
      for (int i=0; i < SENSORS_MAX_CNT; i++) {
        if (wh2data[i].actual) cnt += 1;
      }

      return cnt;
    }

    void resetRead() {
      currentNum = 0;
    }

    WH2Data getNextData() {
      for (int i = currentNum; i < sensorsCount; i++) {
        if (wh2data[i].actual) {
          currentNum = i + 1;
          return wh2data[i];
        }
      }
    }

    byte *getNextDataBuffer() {
      WH2Data data = getNextData();
      
      byte buf[sizeof(data)];
      memcpy(buf, &data, sizeof(data));
      return buf;
    }

  
};


#endif  /* WH2STORAGE */
