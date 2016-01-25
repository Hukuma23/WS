/*
 *
 * Nikita Litvinov
 * 2015/08/14
 *
 * Modification of the Serial to guaranty messages delivery
 *
 */

#include <SerialGuaranteedDeliveryProtocol.h>

SerialGuaranteedDeliveryProtocol::SerialGuaranteedDeliveryProtocol(HardwareSerial* serial, uint8_t* payload, uint8_t payloadSize) : SerialProtocol(payload, payloadSize) {
    processMessage = nullptr;
    blink = nullptr;
    this->lastPayload = (uint8_t*) & oLastPayload;
    setSerial(serial);

}

SerialGuaranteedDeliveryProtocol::SerialGuaranteedDeliveryProtocol(HardwareSerial* serial) : SerialProtocol(payload, payloadSize) {
    processMessage = nullptr;
    blink = nullptr;

    this->payload = (uint8_t*) & oPayload;
    this->lastPayload = (uint8_t*) & oLastPayload;
    this->payloadSize = sizeof (oPayload);

    inputBuffer = (uint8_t*) malloc(payloadSize);

    setSerial(serial);
}

void SerialGuaranteedDeliveryProtocol::setTimerListener(Timer* timer) {
    timerListener = timer;
    (*timerListener).start();
}

void SerialGuaranteedDeliveryProtocol::setSerial(HardwareSerial* serial) {
    this->serial = serial;
}

bool SerialGuaranteedDeliveryProtocol::serialAvailable() {
    return serial != NULL;
}

void SerialGuaranteedDeliveryProtocol::sendData(uint8_t data) {
    serial->write(data);
}

uint8_t SerialGuaranteedDeliveryProtocol::readData() {
    return serial->read();
}

void SerialGuaranteedDeliveryProtocol::setLastPayload() {
    memcpy(lastPayload, payload, payloadSize);
}

void SerialGuaranteedDeliveryProtocol::getLastPayload() {
    memcpy(payload, lastPayload, payloadSize);
}

/*
uint8_t SerialGuaranteedDeliveryProtocol::sendSerialMessage(uint8_t cmd, uint8_t objType, uint8_t objId) {

    if (isListen)
        return -1;

    stopListening();

    receiveState = receive();

    if (receiveState == ProtocolState::SUCCESS) {
        processSerialMessage();
    }

    oPayload.cmd = cmd;
    oPayload.objType = objType;
    oPayload.objId = objId;

    setLastPayload(); //lastPayload = payload;
    receiveState = send();
    startListening();

    return receiveState;
}*/

uint8_t SerialGuaranteedDeliveryProtocol::sendSerialMessage(uint8_t cmd, uint8_t objType, uint8_t objId, uint8_t sw /* = 0 */) {
    if (isListen)
        return -1;

    stopListening();

    receiveState = receive();

    if (receiveState == ProtocolState::SUCCESS) {
        processSerialMessage();
    }

    oPayload.cmd = cmd;
    oPayload.objType = objType;
    oPayload.objId = objId;
    oPayload.sw = sw;

    setLastPayload(); //lastPayload = payload;
    receiveState = send();
    startListening();

    return receiveState;
}

uint8_t SerialGuaranteedDeliveryProtocol::requestRepeatSerialMessage() {
    oPayload.cmd = SerialCommand::REPEAT;
    oPayload.objType = ObjectType::UNDEFINED;
    oPayload.objId = ObjectId::UNDEFINED;

    receiveState = send();

    return receiveState;
}

uint8_t SerialGuaranteedDeliveryProtocol::repeatSerialMessage() {

    if (blink)
        blink();
    /*receiveState = receive();

    if (receiveState == ProtocolState::SUCCESS) {
        processSerialMessage();
    }*/

    getLastPayload(); //payload = lastPayload;

    receiveState = send();

    return receiveState;
}

void SerialGuaranteedDeliveryProtocol::processSerialMessage() {

    // if needs to repeat send message
    if (oPayload.cmd == SerialCommand::REPEAT) {
        repeatSerialMessage();
        return;
    }

    // If all message is OK lets process it
    if (processMessage)
        processMessage();
}

void SerialGuaranteedDeliveryProtocol::setProcessing(processionDelegate processMethod) {

    if (processMessage) {
        SYSTEM_ERROR("WRONG CALL waitConnection method..");
        return;
    }

    processMessage = processMethod;
}

void SerialGuaranteedDeliveryProtocol::listener() {
    isListen = true;
    receiveState = receive();

    if ((receiveState == ProtocolState::INVALID_CHECKSUM) || (receiveState == ProtocolState::INVALID_SIZE)) {
        delay(WAIT_BEFORE_REPEAT);
        requestRepeatSerialMessage();
    } else if (receiveState == ProtocolState::SUCCESS) {
        processSerialMessage();
    }
    isListen = false;
}

void SerialGuaranteedDeliveryProtocol::startListening() {
    (*timerListener).start();
}

void SerialGuaranteedDeliveryProtocol::stopListening() {
    (*timerListener).stop();
}

uint8_t SerialGuaranteedDeliveryProtocol::getPayloadCmd() {
    return oPayload.cmd;
}

uint8_t SerialGuaranteedDeliveryProtocol::getPayloadObjType() {
    return oPayload.objType;
}

uint8_t SerialGuaranteedDeliveryProtocol::getPayloadObjId() {
    return oPayload.objId;
}

SerialMessage SerialGuaranteedDeliveryProtocol::getPayload() {
    return oPayload;
}
/*
void SerialGuaranteedDeliveryProtocol::clearPayload() {

    
     oPayload.dht_temp = UNDEF;
     oPayload.dht_hum = UNDEF;

     oPayload.bmp_temp = UNDEF;
     oPayload.bmp_press = UNDEF;

     oPayload.ds1 = UNDEF;
     oPayload.ds2 = UNDEF;
     oPayload.ds3 = UNDEF;
     oPayload.ds4 = UNDEF;
     oPayload.ds5 = UNDEF;

     oPayload.water_hot = UNDEF;
     oPayload.water_cold = UNDEF;

     oPayload.ls1 = UNDEF;
     oPayload.ls2 = UNDEF;
     oPayload.ls3 = UNDEF;
     oPayload.ls4 = UNDEF;
     oPayload.ls5 = UNDEF;

     oPayload.ms1 = UNDEF;
     oPayload.ms2 = UNDEF;
     oPayload.ms3 = UNDEF;
     
    oPayload.sw1 = UNDEF;
    oPayload.sw2 = UNDEF;
    oPayload.sw3 = UNDEF;
    oPayload.sw4 = UNDEF;
    oPayload.sw5 = UNDEF;
    oPayload.sw6 = UNDEF;
    oPayload.sw7 = UNDEF;
    oPayload.sw8 = UNDEF;
    oPayload.sw9 = UNDEF;
    oPayload.sw10 = UNDEF;

}
*/
void SerialGuaranteedDeliveryProtocol::flagListening(bool listening) {
    isListen = listening;
}

bool SerialGuaranteedDeliveryProtocol::flagListening() {
    return isListen;
}

void SerialGuaranteedDeliveryProtocol::setBlink(processionDelegate blink) {
    this->blink = blink;
}

uint8_t SerialGuaranteedDeliveryProtocol::getPayloadSize() {
    return payloadSize;
}
