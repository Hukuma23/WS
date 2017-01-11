/*
 * logger.h
 *
 *  Created on: 17 сент. 2015 г.
 *      Author: Nikita Litvinov
 */

#ifndef INCLUDE_LOGGER_H_
#define INCLUDE_LOGGER_H_


//#define DEBUG4
//#define MEMORY


//!---------------------------------------------------------------------------------------------------------
#ifdef DEBUG4
	#define ERROR
	#define INFO
	#define DEBUG1
#endif

#ifdef DEBUG1
	#define ERROR
	#define INFO
#endif

#ifdef INFO
	#define ERROR
#endif


#ifdef ERROR
    #define ERROR_PRINT(x)      		Serial.print(x)
    #define ERROR_PRINTLN(x)    		Serial.println(x)
	//#define ERROR_PRINTF(x,y)      		Serial.printf(x, y)
	//#define ERROR_PRINTF2(x,y,z)		Serial.printf(x,y,z)
	#define ERROR_PRINTF(fmt, ...) 		m_printf(fmt, ##__VA_ARGS__)
#else
    #define ERROR_PRINT(x)
    #define ERROR_PRINTLN(x)
	#define ERROR_PRINTF(fmt, ...)
#endif


#ifdef MEMORY
    #define PRINT_MEM()      			Serial.print("mem:");Serial.println(system_get_free_heap_size())
#else
	#define PRINT_MEM()
#endif



#ifdef INFO
    #define INFO_PRINT(x)      		Serial.print(x)
    #define INFO_PRINTLN(x)    		Serial.println(x)
    #define INFO_PRINTHEX(x)   		Serial.print(x, HEX)
	#define INFO_PRINTF(fmt, ...) 		m_printf(fmt, ##__VA_ARGS__)
	#define INFO_WRITE(x)			Serial.write(x)
#else
    #define INFO_PRINT(x)
    #define INFO_PRINTLN(x)
    #define INFO_PRINTHEX(x)
	#define INFO_PRINTF(fmt, ...)
	#define INFO_WRITE(x)
#endif


#ifdef DEBUG1
	#define DEBUG1_PRINT(x)      	Serial.print(x)
    #define DEBUG1_PRINTLN(x)    	Serial.println(x)
    #define DEBUG1_PRINTHEX(x)   	Serial.print(x, HEX)
	#define DEBUG1_PRINTF(fmt, ...) m_printf(fmt, ##__VA_ARGS__)
	#define DEBUG1_WRITE(x)			Serial.write(x)
#else
	#define DEBUG1_PRINT(x)
    #define DEBUG1_PRINTLN(x)
    #define DEBUG1_PRINTHEX(x)
	#define DEBUG1_PRINTF(fmt, ...)
	#define DEBUG1_WRITE(x)
#endif


#ifdef DEBUG4
    #define DEBUG4_PRINT(x)      	Serial.print(x)
    #define DEBUG4_PRINTLN(x)    	Serial.println(x)
    #define DEBUG4_PRINTHEX(x)   	Serial.print(x, HEX)
	#define DEBUG4_PRINTF(fmt, ...) m_printf(fmt, ##__VA_ARGS__)
	#define DEBUG4_WRITE(x)			Serial.write(x)

#else
    #define DEBUG4_PRINT(x)
    #define DEBUG4_PRINTLN(x)
    #define DEBUG4_PRINTHEX(x)
	#define DEBUG4_PRINTF(fmt, ...)
	#define DEBUG4_WRITE(x)
#endif




#endif /* INCLUDE_LOGGER_H_ */
