/*
 * SerialGuaranteedDeliveryProtocol.h
 *
 *  Created on: 14 авг. 2015 г.
 *      Author: Nikita
 */

#ifndef INCLUDE_SERIALVERIFICATION_H_
#define INCLUDE_SERIALVERIFICATION_H_

#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include <HardwareSerial.h>
#include <SerialProtocol.h>

#define WAIT_BEFORE_REPEAT 20

typedef Delegate<void() > ProcessionDelegate;

struct SerialMessage {
    uint8_t cmd;
    uint8_t objType;
    uint8_t objId;

    uint8_t sw;
    //int8_t sw1;
    //int8_t sw2;
    //int8_t sw3;

    int16_t dht_temp;
    int16_t dht_hum;

    int16_t bmp_temp;
    int16_t bmp_press;

    int16_t ds1;
    int16_t ds2;
    //int16_t ds3;
    //int16_t ds4;
    //int16_t ds5;

    int16_t water_hot;
    int16_t water_cold;

    //int16_t ls1;
    //int16_t ls2;
    //int16_t ls3;
    //int16_t ls4;
    //int16_t ls5;

    //int16_t ms1;
    //int16_t ms2;
    //int16_t ms3;


};

class SerialGuaranteedDeliveryProtocol : public SerialProtocol {
public:

    SerialGuaranteedDeliveryProtocol(HardwareSerial*, uint8_t*, uint8_t);
    SerialGuaranteedDeliveryProtocol(HardwareSerial*);

    //uint8_t sendSerialMessage(uint8_t cmd, uint8_t objType, uint8_t objId);
    uint8_t sendSerialMessage(uint8_t cmd, uint8_t objType, uint8_t objId, uint8_t sw = 0);

    void flagListening(bool listening);
    bool flagListening();

    void startListening();
    void stopListening();
    //void stopListening();

    void clearPayload();

    void startListener(unsigned long interval_listener);
    void setSerial(HardwareSerial*);
    void setProcessing(ProcessionDelegate processMethod);
    void setBlink(ProcessionDelegate blink);


    uint8_t getPayloadCmd();
    uint8_t getPayloadObjType();
    uint8_t getPayloadObjId();
    uint8_t getPayloadSize();
    SerialMessage getPayload();

protected:
    bool serialAvailable();
    void sendData(uint8_t data);
    uint8_t readData();


private:
    void listener();
    SerialMessage oPayload,oLastPayload;
    HardwareSerial *serial;
    uint8_t* lastPayload;
    uint8_t receiveState;
    ProcessionDelegate processMessage;
    ProcessionDelegate blink;

    bool isListen = false;
    Timer timerListener;

    void setLastPayload();
    void getLastPayload();

    uint8_t repeatSerialMessage();
    void processSerialMessage();
    uint8_t requestRepeatSerialMessage();
    
};

#endif /* INCLUDE_SERIALVERIFICATION_H_ */
