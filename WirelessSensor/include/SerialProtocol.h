// Copyright Victor Hurdugaci (http://victorhurdugaci.com). All rights reserved.
// Licensed under the Apache License, Version 2.0. See LICENSE in the project root for license information.

#ifndef SERIALPROTOCOL_H
#define SERIALPROTOCOL_H

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define 	UNDEF	-127


struct SerialCommand
{
    enum Enum
    {

         // Set command
        SET_LOW = 0,

        // Set command
		SET_HIGH = 1,

        // Get command
        GET = 2,

		// Wait for information from slave
		WAIT = 3,

		// Collect data from sensors
		COLLECT = 4,

		// Return requested data from slave to master
		RETURN = 5,

		// Repeat last message please
		REPEAT = 6,

		// Confirmation of receipt message
		OK = 7,

		// Update Switches state
		SET_SW = 8,

		// Undefined
		UNDEFINED = 127,

    };
};


// Pointed to type of object
struct ObjectType
{
    enum Enum
    {
        // ALL
        ALL = 0,


		// All sensors
		SENSORS = 2,

		// DHT22 sensor: Temperature and Humidity
		DHT = 3,

		// BMP180 sensor: Temperature and Pressure
		BMP = 4,

		// DS Temperature sensors
		DS = 5,

		// Water counters
		WATER = 6,

		// Ambient Light Sensor BH1750FVI (16bit)
		LIGHT_SENSOR = 7,

		// Motion sensors
		MOTION = 8,

		// Switches / Relays
		SWITCH = 9,

		// Undefined
		UNDEFINED = 127,
    };
};

// Object Id in group
struct ObjectId {
    enum Enum {

		// Undefined
		UNDEFINED = 127,

    	// All sensors
		ALL = 0,

		// DHT22 sensor: Temperature and Humidity
		DHT_TEMP = 1,
		DHT_HUM = 2,

		// BMP180 sensor: Temperature and Pressure
		BMP_TEMP = 1,
		BMP_PRESS = 2,

		// DS Temperature sensors
		DS_1 = 1,
		DS_2 = 2,
		DS_3 = 3,
		DS_4 = 4,
		DS_5 = 5,
		DS_6 = 6,
		DS_7 = 7,
		DS_8 = 8,
		DS_9 = 9,
		DS_10 = 10,

		// Water counters
		WATER_HOT = 1,
		WATER_COLD = 2,

		// Ambient Light Sensor BH1750FVI (16bit)
		LIGHT_SENSOR_1 = 1,
		LIGHT_SENSOR_2 = 2,
		LIGHT_SENSOR_3 = 3,
		LIGHT_SENSOR_4 = 4,
		LIGHT_SENSOR_5 = 5,
		LIGHT_SENSOR_6 = 6,
		LIGHT_SENSOR_7 = 7,
		LIGHT_SENSOR_8 = 8,
		LIGHT_SENSOR_9 = 9,
		LIGHT_SENSOR_10 = 10,

		// Motion sensors
		MOTION_SENSOR_1 = 1,
		MOTION_SENSOR_2 = 2,
		MOTION_SENSOR_3 = 3,

		// Switches / Relays
		SWITCH_1 = 1,
		SWITCH_2 = 2,
		SWITCH_3 = 3,
		SWITCH_4 = 4,
		SWITCH_5 = 5,
		SWITCH_6 = 6,
		SWITCH_7 = 7,
		SWITCH_8 = 8,
    };
};


struct ProtocolState 
{
    enum Enum 
    {
        // The serial object was not set
        NO_SERIAL = 0,

         // The operation succeeded
        SUCCESS = 1,

        // There is not (enought) data to process
        NO_DATA = 2,
        
         // The object is being received but the buffer doesn't have all the data
        WAITING_FOR_DATA = 3,

        // The size of the received payload doesn't match the expected size
        INVALID_SIZE = 4,

        // The object was received but it is not the same as one sent
        INVALID_CHECKSUM = 5
    };
};

class SerialProtocol
{
public:
    SerialProtocol(uint8_t*, uint8_t);

    // Sends the current payload
    //
    // Returns a ProtocolState enum value
    uint8_t send();

    // Tries to receive the payload from the
    // current available data
    // Will replace the payload if the receive succeeds
    // 
    // Returns a ProtocolState enum value
    uint8_t receive();

protected:
    virtual bool serialAvailable() = 0;

    virtual void sendData(uint8_t data) = 0;
    virtual uint8_t readData() = 0;

    uint8_t* payload;
    uint8_t payloadSize;
    uint8_t* inputBuffer;

private:



    uint8_t bytesRead;

    uint8_t actualChecksum;
};

#endif
