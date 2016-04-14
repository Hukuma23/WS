#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include <Libraries/DHT/DHT.h>
#include <Libraries/OneWire/OneWire.h>
#include <Libraries/BMP180/BMP180.h>
#include <HardwareSerial.h>
#include <SerialGuaranteedDeliveryProtocol.h>
#include <AppSettings.h>
#include <ActStates.h>
#include <Logger.h>
#include <LED.h>
#include <MQTT.h>
#include <Module.h>

FTPServer ftp;

// rBoot OTA object
rBootHttpUpdate* otaUpdater = 0;

//extern void wdt_feed (void);
void onMessageReceived(String topic, String message); // Forward declaration for our callback
void readBarometer(void);
void publishSwitches(void);
void checkWifi(void);
void OtaUpdate(bool isSpiffs);
void switchBootRom();
void ready();
void connectOk();
void connectFail();
void stopAllTimers();
String ShowInfo();
void initModules();

Timer timerBMP;
Timer timerWIFI;
Timer timerSerialListener;
Timer timerListener;
Timer timerSerialCollector;
Timer timerSerialReceiver;

bool initHttp = false;

// wifi vars block
bool isList = false;
long time1, time2;
String* wifilist;
uint8_t list_count;
// end wifi vars block

int wifiCheckCount = 0;

SerialGuaranteedDeliveryProtocol protocol(&Serial);

bool state = true;
String mqttClientName;
float dsTemp[3];
int vcc;
bool isPubStart = false;

// BMP180 object
BMP180 barometer;
float bmpTemp;
long bmpPress;

// DS object
SensorDS* dsSensor;

// DHT object
SensorDHT* dhtSensor;


// Serial
float sDSTemp[3];
float sDHTTemp, sDHTHum;
float sBMPTemp, sBMPPress;
int16_t sWaterCold, sWaterHot;
byte swState[3];

// INSW
unsigned long pushTime[10] = {0,0,0,0,0,0,0,0,0,0};
byte pushCount[10];
bool pushSwitched[10];


// MQTT client
MQTT* mqtt;

String topSubscr;

String topCfg;

String topSw;

String topDHTTemp;
String topDHTHum;

String topDSTemp;

String topBMPTemp;
String topBMPPress;

String topVCC;
String topLog;
String topStart;

// Serial
String sTopSw;

String topWaterCold;
String topWaterHot;



String topWaterCold_Out;
String topWaterHot_Out;

unsigned int loopIndex;

String getOutTopic(String name) {
	String result = AppSettings.main_topic + "/out/" + AppSettings.client_topic + name;
	return result;
}

void initSerialVars() {
	sDHTTemp = UNDEF;
	sDHTHum = UNDEF;
	sBMPTemp =  UNDEF;
	sBMPPress =  UNDEF;
	sWaterCold =  UNDEF;
	sWaterHot =  UNDEF;

	sDSTemp[0] = UNDEF;
	sDSTemp[1] = UNDEF;
	sDSTemp[2] = UNDEF;

	for (int i=0; i < sizeof swState; i++) {
		swState[i] = UNDEF;
	}
}

void IRAM_ATTR turnSw(byte num, bool state) {

	DEBUG4_PRINTF2("num=%d; state=%d; ", num, state);

	ActStates.setSw(num, state);

	if (ActStates.getSw(num)) {
		DEBUG4_PRINTF(" set sw[%d] to GREEN;  ", num);
		digitalWrite(AppSettings.sw[num], HIGH);
		AppSettings.led.green(num);
		//mqtt.publish(sTopSw_Out+String(num+1), "ON");
	}
	else {
		DEBUG4_PRINTF(" set sw[%d] to RED;  ", num);
		digitalWrite(AppSettings.sw[num], LOW);
		AppSettings.led.red(num);
		//mqtt.publish(sTopSw_Out+String(num+1), "OFF");
	}

	DEBUG4_PRINTLN("turnSw. Done");
}

void initSw() {


	for (byte num = 0; num < ActStates.sw_cnt; num++) {
		DEBUG4_PRINTF("initSW. pin=%d ", AppSettings.sw[num]);

		pinMode(AppSettings.sw[num], OUTPUT);
		turnSw(num, ActStates.sw[num]);
		DEBUG4_PRINTLN("  done!");
	}
}

void IRAM_ATTR turnSsw(byte num, bool state) {
	if (state == HIGH) {
		ActStates.setSsw(num, HIGH);
		protocol.sendSerialMessage(SerialCommand::SET_HIGH, ObjectType::SWITCH, num+1);
	} else {
		ActStates.setSsw(num, LOW);
		protocol.sendSerialMessage(SerialCommand::SET_LOW, ObjectType::SWITCH, num+1);
	}
}

void IRAM_ATTR interruptHandlerInSw(byte num) {
	if ((millis() - pushTime[num]) > AppSettings.debounce_time) {
		pushTime[num] = millis();
		pushSwitched[num] = false;
		pushCount[num] = 1;
		String logStr = String(pushTime[num]) + "   PRESSED in[" + String(num) + "] 1st time, nowState = " + String(ActStates.sw[num]);
		//mqtt.publish(topCfg_Out, logStr.c_str());
		DEBUG4_PRINTLN(logStr);
		return;
	}
	else {
		pushCount[num]++;
	}

	if (((millis() - pushTime[num]) < AppSettings.debounce_time) && (pushCount[num] > 4) && (!pushSwitched[num])) {

		pushSwitched[num] = true;
		turnSw(num, !ActStates.sw[num]);
		String logStr = String(pushTime[num]) + "   PRESSED in[" + String(num) + "] " + pushCount[num] +" times, nowState = " + String(ActStates.sw[num]);
		//mqtt.publish(topCfg_Out, logStr.c_str());

		DEBUG4_PRINT(pushTime[num]);
		DEBUG4_PRINTF( "   sw%d = ", num);
		DEBUG4_PRINT(ActStates.sw[num]);
		DEBUG4_PRINTLN();
	}
}

void IRAM_ATTR interruptHandlerInSw1() {
	interruptHandlerInSw(0);
}

void IRAM_ATTR interruptHandlerInSw2() {
	interruptHandlerInSw(1);
}

void IRAM_ATTR interruptHandlerInSw3() {
	interruptHandlerInSw(2);
}

void IRAM_ATTR interruptHandlerInSw4() {
	interruptHandlerInSw(3);
}

void IRAM_ATTR interruptHandlerInSw5() {
	interruptHandlerInSw(4);
}

void var_init() {
	DEBUG4_PRINTLN("_var_init");
	String topicMain = AppSettings.main_topic;
	String topicClient = AppSettings.client_topic;

	topSubscr = topicMain + "/in/#";

	topSw = "sw";

	// * Serial start *

	sTopSw = "ssw";
	// * Serial end *

	topDHTTemp = "dht_t";
	topDHTHum = "dht_h";

	topDSTemp = "ds_t";
	topBMPTemp = "bmp_t";
	topBMPPress = "bmp_p";


	topCfg = "config";

	topLog = "log";
	topStart = "start";
	topVCC = "VCC";

	mqttClientName = "esp8266-" + topicClient;
	mqttClientName += String(micros() & 0xffff, 16);

	loopIndex = 0;

	bmpTemp = -255;
	bmpPress = -255;

	//dhtTemp = -255;
	//dhtHum = -255;


	//Serial

	initSerialVars();

}
String uptime() {
	unsigned int uptime = loopIndex / 2;
	int months = uptime / 43200;

	uptime %= 43200;
	int weeks = uptime / 10080;

	uptime %= 10080;
	int days = uptime / 1440;

	uptime %= 1440;
	int hours = uptime / 60;

	uptime %= 60;

	String result = "";

	if (months > 0) {
		result += String(months);
		if (months > 1)
			result += " months ";
		else
			result += " month ";
	}

	if (weeks > 0) {
		result += String(weeks);
		if (weeks > 1)
			result += " weeks ";
		else
			result += " week ";
	}

	if (days > 0) {
		result += String(days);
		if (days > 1)
			result += " days ";
		else
			result += " day ";
	}

	if (hours > 0) {
		result += String(hours);
		if (hours > 1)
			result += " hours ";
		else
			result += " hour ";
	}

	if (uptime > 0) {
		result += String(uptime);
		if (uptime > 1)
			result += " minutes ";
		else
			result += " minute ";
	}

	if ((months == 0) && (weeks == 0) && (days == 0) && (hours == 0) && (uptime == 0))
		result += "less than a minute";

	return result;
}

// Callback for messages, arrived from MQTT server

void onMessageReceived(String topic, String message) {
	DEBUG4_PRINTLN("_onMessageReceived");
	DEBUG4_PRINT("MESSAGE RECEIVED: ");
	DEBUG4_PRINT(topic);
	DEBUG4_PRINT(": \""); // Prettify alignment for printing
	DEBUG4_PRINT(message);
	DEBUG4_PRINTLN("\"");

	// SW
	for (byte i = 0; i < AppSettings.sw_cnt; i++) {
		//if (topic.equals((topSw_In+String(i+1)))) {
		if (topic.equals(mqtt->getTopic(topSw, (i+1), IN))) {
			if (message.equals("ON")) {
				turnSw(i, HIGH);
			} else if (message.equals("OFF")) {
				turnSw(i, LOW);
			} else
				DEBUG4_PRINTF("Topic %s, message is UNKNOWN", (mqtt->getTopic(topSw, (i+1), IN)).c_str());
		}
	}

	// *** Serial block ***

	for (byte i = 0; i < AppSettings.ssw_cnt; i++) {
		if (topic.equals(mqtt->getTopic(sTopSw, (i+1), IN))) {
			if (message.equals("ON")) {
				turnSsw(i, HIGH);
			} else if (message.equals("OFF")) {
				turnSsw(i, LOW);
			} else
				DEBUG4_PRINTF("Topic %s, message is UNKNOWN", (mqtt->getTopic(sTopSw, (i+1), IN)).c_str());
		}
	}
	// *** Serial block end ***
	if (topic.equals(mqtt->getTopic(topCfg, IN))) {

		int msgLen = message.length() + 1;
		DEBUG4_PRINT("msgLen = ");
		DEBUG4_PRINTLN(msgLen);

		char msgBuff[msgLen];
		message.toCharArray(msgBuff, msgLen);
		DEBUG4_PRINT("msgBuff = ");
		String str2(msgBuff);
		DEBUG4_PRINTLN(str2);

		//DEBUG4_PRINTLN("JSON 1");

		//attachInterrupt(UPDATE_PIN, interruptHandler, CHANGE);
		DynamicJsonBuffer jsonBuffer;
		//DEBUG4_PRINTLN("JSON 2");
		JsonObject& root = jsonBuffer.parseObject(msgBuff);
		//DEBUG4_PRINTLN("JSON 3");

		String cmd = root["cmd"].toString();
		//DEBUG4_PRINTLN("JSON 4");
		DEBUG4_PRINTLN("cmd = " + cmd);
		//DEBUG4_PRINTLN("JSON 5");
		JsonArray& args = root["args"];
		//DEBUG4_PRINTLN("JSON 6");

		if (cmd.equals("sw_update")) {
			stopAllTimers();
			//mqtt.publish(topCfg_Out, "Will stop all timers and UPDATE on the air firmware only now");
			mqtt->publish(topCfg, OUT, "Will stop all timers and UPDATE on the air firmware only now");
			OtaUpdate(false); //OtaUpdateSW();
		}
		else if (cmd.equals("sw_update_all")) {
			//mqtt.publish(topCfg_Out, "Will stop all timers and UPDATE on the air firmware and spiffs now");
			mqtt->publish(topCfg, OUT, "Will stop all timers and UPDATE on the air firmware and spiffs now");
			stopAllTimers();
			OtaUpdate(true);
		}
		else if ((cmd.equals("version")) || (cmd.equals("ver"))) {
			//mqtt.publish(topCfg_Out, AppSettings.version);
			mqtt->publish(topCfg, OUT, AppSettings.version);
		}
		else if (cmd.equals("restart")) {
			//mqtt.publish(topCfg_Out, "Will restart now");
			mqtt->publish(topCfg, OUT, "Will restart now");
			System.restart();
		}
		else if (cmd.equals("conf_del")) {
			//mqtt.publish(topCfg_Out, "Delete config now");
			mqtt->publish(topCfg, OUT, "Will delete config now");
			AppSettings.deleteConf();
		}
		else if (cmd.equals("switch")) {
			uint8 before, after;
			before = rboot_get_current_rom();
			if (before == 0) after = 1; else after = 0;
			String result = "Swapping from rom ";
			result += String(before);
			result += " to rom ";
			result += String(after);
			result += ". Then will restart\r\n";
			//mqtt.publish(topCfg_Out, result);
			mqtt->publish(topCfg, OUT, result);
			switchBootRom();
		}
		else if (cmd.equals("info")) {
			//mqtt.publish(topCfg_Out, ShowInfo());
			mqtt->publish(topCfg, OUT, ShowInfo());
		}
		else if (cmd.equals("uptime")) {
			//mqtt.publish(topCfg_Out, "Loopindex = " + String(loopIndex));
			mqtt->publish(topCfg, OUT, "Loopindex = " + String(loopIndex));

			String strUptime = uptime();
			DEBUG4_PRINTLN("Uptime is " + strUptime);
			//mqtt.publish(topCfg_Out, "Uptime is " + strUptime);
			mqtt->publish(topCfg, OUT, "Uptime is " + strUptime);
		}
		else if (cmd.equals("reboot")) {
			DEBUG4_PRINTLN("REBOOT stub routine");
			system_restart();
		}
		else if (cmd.equals("conf_httpload")) {
			String updList = "Will try to load Settings by http";
			AppSettings.loadHttp();
			//mqtt.publish(topCfg_Out, updList);
			mqtt->publish(topCfg, OUT, updList);

		}
		else if (cmd.equals("conf_save")) {
			AppSettings.save();
			//mqtt.publish(topCfg_Out, "Settings saved.");
			mqtt->publish(topCfg, OUT, "Settings saved.");
		}
		else if (cmd.equals("act_print")) {
			String strPrint = ActStates.print();
			DEBUG4_PRINTLN(strPrint);
			DEBUG4_PRINTF("Answer length is %d bytes\r\n", strPrint.length());
			//mqtt.publish(topCfg_Out, strPrint);
			mqtt->publish(topCfg, OUT, strPrint);
		}
		else if (cmd.equals("act_printf")) {
			String strPrintf = ActStates.printf();
			DEBUG4_PRINTLN(strPrintf);
			DEBUG4_PRINTF("Answer length is %d bytes\r\n", strPrintf.length());
			//mqtt.publish(topCfg_Out, strPrintf);
			mqtt->publish(topCfg, OUT, strPrintf);
		}
		else
			DEBUG4_PRINTLN("Topic matched, command is UNKNOWN");
	}
	else
		DEBUG4_PRINTLN("topic is UNKNOWN");
}

void publishSerialSw() {

	for (byte i = 0; i < ActStates.ssw_cnt; i++) {
		if (ActStates.ssw[i])
			mqtt->publish(sTopSw, (i+1), OUT, "ON");	//mqtt.publish(sTopSw_Out+String(i+1), "ON");

		else
			mqtt->publish(sTopSw, (i+1), OUT, "OFF");	//mqtt.publish(sTopSw_Out+String(i+1), "OFF");
	}

	for (byte i = 0; i < ActStates.ssw_cnt; i++) {
		DEBUG4_PRINTF("swState%d is ", i);
		DEBUG4_PRINTLN(ActStates.ssw[i]);
	}
}

void publishSerial() {

	bool result;

	publishSerialSw();

	// Publish DHT
	if (sDHTTemp != UNDEF) {
		result = mqtt->publish("sdht_t", OUT, String(sDHTTemp));	//result = mqtt.publish(getOutTopic("sdht_t"), String(sDHTTemp));
		if (result)
			sDHTTemp = UNDEF;
	}

	if (sDHTHum != UNDEF) {
		result = mqtt->publish("sdht_h", OUT, String(sDHTHum));	//result = mqtt.publish(getOutTopic("sdht_h"), String(sDHTHum));
		if (result)
			sDHTHum = UNDEF;
	}

	// Publish BMP
	if (sBMPTemp != UNDEF) {
		result = mqtt->publish("sbmp_t", OUT, String(sBMPTemp));	//result = mqtt.publish(getOutTopic("sbmp_t"), String(sBMPTemp));
		if (result)
			sBMPTemp = UNDEF;
	}
	if (sBMPPress != UNDEF) {
		result = mqtt->publish("sbmp_p", OUT, String(sBMPPress));	//result = mqtt.publish(getOutTopic("sbmp_p"), String(sBMPPress));
		if (result)
			sBMPPress = UNDEF;
	}

	// Publish DS
	int dsSize = sizeof sDSTemp / sizeof sDSTemp[0];
	for (int i=0; i < dsSize; i++) {
		if (sDSTemp[i] != UNDEF) {
			result = mqtt->publish("sds_t", i, OUT, String(sDSTemp[i]));	//result = mqtt.publish(getOutTopic("sds_t") + String(i), String(sDSTemp[i]));
			if (result)
				sDSTemp[i] = UNDEF;
		}
	}

	// Publish Water
	if ((sWaterCold > 0) && (sWaterCold != UNDEF)) {
		result = mqtt->publish("s_wc", OUT, String(sWaterCold));	//result = mqtt.publish(getOutTopic("s_wc"), String(sWaterCold));
		if (result)
			sWaterCold = UNDEF;
	}
	if ((sWaterHot > 0) && (sWaterHot != UNDEF)) {
		result = mqtt->publish("s_wh", OUT, String(sWaterHot));	//result = mqtt.publish(getOutTopic("s_wh"), String(sWaterHot));
		if (result)
			sWaterHot = UNDEF;
	}

}


void mqtt_loop() {

	loopIndex++;
	INFO_PRINTLN("_mqtt_loop");
	PRINT_MEM();


	DEBUG4_PRINT("*** Index = ");
	DEBUG4_PRINT(loopIndex);
	DEBUG4_PRINTLN(" ***");

	publishSwitches();

	system_soft_wdt_stop();

	// Publish BMP180 data
	DEBUG4_PRINTLN();
	DEBUG4_PRINT("BMP Temperature ");
	if (bmpTemp > -255) {
		bool result = mqtt->publish(topBMPTemp, OUT, String(bmpTemp));	//bool result = mqtt.publish(topBMPTemp_Out, String(bmpTemp));
		DEBUG4_PRINT(" ");
		DEBUG4_PRINT(bmpTemp);
		DEBUG4_PRINT(" C... ");
		DEBUG4_PRINT("Published: ");
		DEBUG4_PRINTLN(result);

		if (result == true)
			bmpTemp = -255;
	} else
		DEBUG4_PRINTLN("ignore");

	DEBUG4_PRINT("BMP Pressure ");
	if (bmpPress > -255) {

		bool result = mqtt->publish(topBMPPress, OUT, String(bmpPress));	//bool result = mqtt.publish(topBMPPress_Out, String(bmpPress));
		DEBUG4_PRINT(" ");
		DEBUG4_PRINT(bmpPress);
		DEBUG4_PRINT(" Pa... ");
		DEBUG4_PRINT("Published: ");
		DEBUG4_PRINTLN(result);

		if (result == true)
			bmpPress = -255;
	} else
		DEBUG4_PRINTLN("ignore");

	system_soft_wdt_restart();


	// Publish serial
	publishSerial();

	// Publish VCC
	//vcc = readvdd33();
	//DEBUG4_PRINT("VCC is ");
	//DEBUG4_PRINT(vcc);
	//DEBUG4_PRINTLN(" mV");

	PRINT_MEM();
	INFO_PRINTLN("_user_loop.end");
	// Go to deep sleep
	//system_deep_sleep_set_option(1);
	//system_deep_sleep(4000000);
}


void serialCollector() {
	protocol.sendSerialMessage(SerialCommand::COLLECT, ObjectType::ALL, ObjectId::ALL);
	//mqtt.publish(topLog_Out, "serialCollector() cmd = " + String(SerialCommand::COLLECT) + " objType=" + String(ObjectType::ALL) + " objId=" + String(ObjectId::ALL));
	mqtt->publish(topLog, OUT, "serialCollector() cmd = " + String(SerialCommand::COLLECT) + " objType=" + String(ObjectType::ALL) + " objId=" + String(ObjectId::ALL));
}

void serialReceiver() {
	protocol.sendSerialMessage(SerialCommand::GET, ObjectType::ALL, ObjectId::ALL);
	//mqtt.publish(topLog_Out, "serialReceiver() cmd = " + String(SerialCommand::GET) + " objType=" + String(ObjectType::ALL) + " objId=" + String(ObjectId::ALL));
	mqtt->publish(topLog, OUT, "serialReceiver() cmd = " + String(SerialCommand::GET) + " objType=" + String(ObjectType::ALL) + " objId=" + String(ObjectId::ALL));
}


void setSerialCollector() {
	if (AppSettings.is_serial) {
		timerSerialCollector.initializeMs(AppSettings.interval_collector, serialCollector).start();
		DEBUG4_PRINTLN("*** Serial Collector timer done!");
	}
}

void setSerialReceiver() {
	if (AppSettings.is_serial) {
		timerSerialReceiver.initializeMs(AppSettings.interval_receiver, serialReceiver).start();
		DEBUG4_PRINTLN("*** Serial Receiver timer done!");
	}
}

void setCheckWifi() {
	if (AppSettings.is_wifi) {
		timerWIFI.initializeMs(AppSettings.interval_wifi, checkWifi).start();
		DEBUG4_PRINTLN("*** WiFi timer done!");
	}
}

void setReadBMP() {
	if (AppSettings.is_bmp) {
		timerBMP.initializeMs(AppSettings.interval_bmp, readBarometer).start();
		DEBUG4_PRINTLN("*** BMP timer done!");
	}
}

/*
void setMQTT() {
	DEBUG4_PRINTLN("*** MQTT timer done!");
	timerMQTT.initializeMs(AppSettings.interval_mqtt, mqtt_loop).start();
}
 */

void stopAllTimers(void) {
	DEBUG4_PRINTLN("MQTT will close connection and ALL timers will STOPPED now!");

	//mqtt.close(); //Close MQTT connection with server

	if (mqtt != NULL)
		mqtt->stopTimer();

	if (dsSensor != NULL)
		dsSensor->stopTimer();

	if (dhtSensor != NULL)
		dhtSensor->stopTimer();

	timerBMP.stop();

	timerWIFI.stop();
	timerSerialListener.stop();
	timerListener.stop();
	timerSerialCollector.stop();
	timerSerialReceiver.stop();
}

void readSensors(SerialMessage payload) {
	sWaterCold = payload.water_cold;
	sWaterHot = payload.water_hot;

	sDHTTemp = (float)payload.dht_temp *0.01;
	sDHTHum = (float)payload.dht_hum *0.01;

	sDSTemp[0] = (float)payload.ds1 *0.01;
	sDSTemp[1] = (float)payload.ds2 *0.01;
	//dsTemp[2] = (float)payload.ds3 *0.01;
}

void readSwitches(SerialMessage payload) {
	bool state;
	uint8_t sw = payload.sw;


	for (byte i = 0; i < ActStates.ssw_cnt; i++) {
		state = ((sw & (int)powf(2, i)) == 1)?HIGH:LOW;

		DEBUG1_PRINTF("CHECK: readSwitches %d, state = ", i);
		DEBUG1_PRINTLN(state);

		if (state != ActStates.getSsw(i))
			ActStates.setSsw(i, state);
	}
	/* !!! Требует тщательной проверки !!!
	state =  ((sw & 1) == 1)?HIGH:LOW;
	if (state != ActStates.getSsw().ssw1)
		ActStates.setSsw1(state);

	state = (((sw & 2)  >> 1) == 1)?HIGH:LOW;
	if (state != ActStates.ssw2)
		ActStates.setSsw2(state);

	state = (((sw & 4)  >> 2) == 1)?HIGH:LOW;
	if (state != ActStates.ssw3)
		ActStates.setSsw3(state);

	state = (((sw & 8)  >> 3) == 1)?HIGH:LOW;
	if (state != ActStates.ssw4)
		ActStates.setSsw4(state);

	state = (((sw & 16) >> 4) == 1)?HIGH:LOW;
	if (state != ActStates.ssw5)
		ActStates.setSsw5(state);
	 */
}

void processSerialMessage() {
	// when message received correctly this method will run

	uint8_t cmd = protocol.getPayloadCmd();
	uint8_t objType = protocol.getPayloadObjType();
	uint8_t objId = protocol.getPayloadObjId();
	//blink(SWITCH_PIN2, 1, 10);

	mqtt->publish(topLog, OUT, "processSerialMessage() cmd = " + String(cmd) + " objType=" + String(objType) + " objId=" + String(objId));

	/*
    DEBUG4_PRINTLN();
    DEBUG4_PRINT("cmd=");
    DEBUG4_PRINT(cmd);
    DEBUG4_PRINT(" type=");
    DEBUG4_PRINT(objType);
    DEBUG4_PRINT(" id=");
    DEBUG4_PRINTLN(objId);
	 */
	/*
	if (objType == ObjectType::SWITCH) {
		if ((objId == ObjectId::SWITCH_1) || (objId == ObjectId::ALL)) {
			if (cmd == SerialCommand::SET_LOW) {
				digitalWrite(AppSettings.sw1, LOW);
			} else if (cmd == SerialCommand::SET_HIGH) {
				digitalWrite(AppSettings.sw1, HIGH);
			}
		}
	}
	 */

	if (cmd == SerialCommand::RETURN) {
		SerialMessage pl = protocol.getPayload();
		if (objType == ObjectType::SENSORS) {
			readSensors(pl);
		}
		else if (objType == ObjectType::SWITCH) {
			readSwitches(pl);
		}
		else if (objType == ObjectType::ALL) {
			readSensors(pl);
			readSwitches(pl);
		}

	}

}

void listenSerialMessage() {
	protocol.listener();
}

// Will be called when WiFi station network scan was completed
void listNetworks(bool succeeded, BssList list) {


	DEBUG4_PRINTLN("listNetworks()");

	if (!succeeded)
	{
		DEBUG4_PRINTLN("Failed to scan networks");
		isList = true;
		list_count = 0;
		return;
	}
	list_count = list.count();
	wifilist = new String[list_count];
	for (int i = 0; i < list_count; i++) {

		DEBUG4_PRINT("\tWiFi: ");
		DEBUG4_PRINT(list[i].ssid);
		DEBUG4_PRINT(", ");
		DEBUG4_PRINT(list[i].getAuthorizationMethodName());
		//if (list[i].hidden) DEBUG4_PRINT(" (hidden)");
		DEBUG4_PRINTLN();
		wifilist[i] = list[i].ssid;

	}
	isList = true;

}

void findNetwork() {

	DEBUG4_PRINTLN("findNetwork()");
	for (uint8_t j=AppSettings.network_ind; j < AppSettings.wifi_cnt; j++) {
		for (int i = 0; i < list_count; i++) {

			//if (AppSettings.wifiList[j].equalsIgnoreCase(wifilist[i])) {
			if (AppSettings.wifiList[j].equals(wifilist[i])) {
				AppSettings.ssid = wifilist[i];
				AppSettings.network_ind = j+1;

				DEBUG4_PRINT("network=");
				DEBUG4_PRINTLN(AppSettings.network_ind);

				DEBUG4_PRINT("ssid=");
				DEBUG4_PRINTLN(AppSettings.ssid);

				return;
			}
		}
	}
	System.restart();

}

void saveDefaultWifi()
{
	timerWIFI.stop();
	DEBUG1_PRINTLN("CONNECTED");

	DEBUG4_PRINT("*** time1=");
	DEBUG4_PRINTLN(millis() - time1);
	DEBUG4_PRINT("*** time2=");
	DEBUG4_PRINTLN(millis() - time2);



	DEBUG4_PRINTLN(WifiStation.getIP().toString());
	DEBUG4_PRINTLN("Connected DONE!");

	DEBUG4_PRINT("WiFi.ssid = ");
	DEBUG4_PRINTLN(WifiStation.getSSID());

	DEBUG4_PRINT("AppSettings.ssid = ");
	DEBUG4_PRINTLN(AppSettings.ssid);
	DEBUG4_PRINT("AppSettings.pass = ");
	DEBUG4_PRINTLN(AppSettings.password);

	int result = AppSettings.loadNetwork(WifiStation.getSSID());
	DEBUG4_PRINT("result of AppSettings.loadNetwork = ");
	DEBUG4_PRINTLN(result);
	AppSettings.saveLastWifi();

	//DEBUG4_PRINTLN(AppSettings.printf());
}


void startTimers() {

	if (mqtt != NULL) {
		DEBUG4_PRINT("mqttTimer.. ");
		mqtt->startTimer(mqtt_loop); //timerMQTT.initializeMs(AppSettings.shift_mqtt, setMQTT).startOnce();
		delay(50);
		DEBUG4_PRINTLN("armed");
	}

	if (dsSensor != NULL) {
		DEBUG4_PRINT("dsTimer.. ");
		dsSensor->startTimer(); //timerDS.initializeMs(AppSettings.shift_ds, setReadOneWire).startOnce();
		delay(50);
		DEBUG4_PRINTLN("armed");
	}

	if (dhtSensor != NULL) {
		DEBUG4_PRINT("dhtTimer.. ");
		dhtSensor->startTimer();	//timerDHT.initializeMs(AppSettings.shift_dht, setReadDHT).startOnce();
		delay(50);
		DEBUG4_PRINTLN("armed");
	}

	DEBUG4_PRINT("bmpTimer.. ");
	timerBMP.initializeMs(AppSettings.shift_bmp, setReadBMP).startOnce();
	delay(50);
	DEBUG4_PRINTLN("armed");

	DEBUG4_PRINT("wifiTimer.. ");
	timerWIFI.initializeMs(AppSettings.shift_wifi, setCheckWifi).startOnce();
	delay(50);
	DEBUG4_PRINTLN("armed");

	DEBUG4_PRINT("serialCollectorTimer.. ");
	timerSerialCollector.initializeMs(AppSettings.shift_collector, setSerialCollector).startOnce();
	delay(50);
	DEBUG4_PRINTLN("armed");

	DEBUG4_PRINT("serialReceiverTimer.. ");
	timerSerialReceiver.initializeMs(AppSettings.shift_receiver, setSerialReceiver).startOnce();
	delay(50);
	DEBUG4_PRINTLN("armed");

	DEBUG4_PRINT("MQTT.state=");
	DEBUG4_PRINTLN(mqtt->getConnectionState());
	DEBUG4_PRINTLN("Client name =\"" + mqttClientName + "\"");

	//startMqttClient();
	publishSwitches();

	if (AppSettings.is_serial) {
		DEBUG4_PRINT("serialListenerTimer.. ");

		timerListener.initializeMs(AppSettings.interval_listener, listenSerialMessage);
		protocol.setProcessing(processSerialMessage);
		protocol.setTimerListener(&timerListener);
		DEBUG4_PRINTLN("armed");

		DEBUG4_PRINTLN("Send sw init to Serial");
		uint8_t sw = ActStates.getSsw();
		DEBUG4_PRINT("Sw from ActStates = ");
		DEBUG4_PRINTLN(sw);
		protocol.sendSerialMessage(SerialCommand::SET_SW, ObjectType::SWITCH, ObjectId::ALL, sw);
		publishSerialSw();

		DEBUG4_PRINT("Payload size = ");
		DEBUG4_PRINTLN(protocol.getPayloadSize());
	}
}

// Will be called when WiFi station was connected to AP
void connectOk() {
	if (!initHttp) {
		DEBUG4_PRINTLN("I'm CONNECTED");
		saveDefaultWifi();
	}
	else {
		initModules();
	}

	var_init();

	DEBUG4_PRINTLN("Start timers...");

	//procTimer.initializeMs(30 * 1000, user_loop).start();

	startTimers();

}

void checkAppSettings() {
	if (AppSettings.exist()) {
		DEBUG1_PRINTLN("Settings.conf successful downloaded. ESP8266 will be restarted soon");
		timerWIFI.stop();
		//System.restart();
		connectOk();
	}
}

void connectOkHttpLoad() {
	DEBUG4_PRINTLN("I'm CONNECTED needs Settings.conf load by http");

	AppSettings.loadHttp();

	timerWIFI.initializeMs(10000, checkAppSettings).start();

}

void reconnectOk() {

	DEBUG4_PRINTLN("RE_CONNECTED");

	wifiCheckCount = 0;
	connectOk();

}

// Will be called when WiFi station timeout was reached
void connectFail() {
	DEBUG1_PRINTLN("NOT CONNECTED!");
	DEBUG1_PRINT("&time1=");
	DEBUG1_PRINTLN(millis() - time1);
	DEBUG1_PRINT("&time2=");
	DEBUG1_PRINTLN(millis() - time2);

	if (initHttp)
		switchBootRom();


	ready();
	//WifiStation.waitConnection(connectOk, 20, connectFail); // Repeat and check again
}

void publishSwitches() {

	for (byte i = 0; i < ActStates.sw_cnt; i++) {
		if (ActStates.getSw(i))
			mqtt->publish(topSw, (i+1), OUT, "ON");	//mqtt.publish(topSw_Out + String(i+1), "ON");
		else
			mqtt->publish(topSw, (i+1), OUT, "OFF");	//mqtt.publish(topSw_Out + String(i+1), "OFF");
	}

	for (byte i = 0; i < ActStates.sw_cnt; i++) {
		DEBUG4_PRINTF("swState%d is ",i);
		DEBUG4_PRINTLN(ActStates.getSw(i));
	}
}

void readBarometer(void) {

	DEBUG4_PRINTLN("_readBarometer");

	if (!barometer.EnsureConnected()) {
		DEBUG4_PRINTLN("Could not connect to BMP180.");
		return;
	}

	// When we have connected, we reset the device to ensure a clean start.
	//barometer.SoftReset();
	// Now we initialize the sensor and pull the calibration data.
	barometer.Initialize();
	//barometer.PrintCalibrationData();

	DEBUG4_PRINT("Start reading");

	// Retrive the current pressure in Pascals.
	bmpPress = barometer.GetPressure();

	// Print out the Pressure.
	DEBUG4_PRINT("Pressure: ");
	DEBUG4_PRINT(bmpPress);
	DEBUG4_PRINT(" Pa");

	// Retrive the current temperature in degrees celcius.
	bmpTemp = barometer.GetTemperature();

	// Print out the Temperature
	DEBUG4_PRINT("\tTemperature: ");
	DEBUG4_PRINT(bmpTemp);
	DEBUG4_WRITE(176);
	DEBUG4_PRINT("C");

	DEBUG4_PRINTLN(); // Start a new line.
	return;

}

void checkWifi(void) {

	if (WifiStation.isConnected()) {

		DEBUG4_PRINTLN("WiFi is connected");
		return;

	}

	wifiCheckCount++;

	DEBUG4_PRINT("--- Error: WIFI status is ");
	DEBUG4_PRINTLN(WifiStation.getConnectionStatusName());

	DEBUG4_PRINT("WiFi isEnabled: ");
	DEBUG4_PRINTLN(WifiStation.isEnabled());

	DEBUG4_PRINT("WiFi isEnabledDHCP: ");
	DEBUG4_PRINTLN(WifiStation.isEnabledDHCP());

	DEBUG4_PRINT("WiFi isConnected: ");
	DEBUG4_PRINTLN(WifiStation.isConnected());

	DEBUG4_PRINT("WiFi isConnectionFailed: ");
	DEBUG4_PRINTLN(WifiStation.isConnectionFailed());

	DEBUG4_PRINT("WiFi MAC: ");
	DEBUG4_PRINTLN(WifiStation.getMAC());

	DEBUG4_PRINT("WiFi SSID: ");
	DEBUG4_PRINTLN(WifiStation.getSSID());

	DEBUG4_PRINT("WiFi Passwd: ");
	DEBUG4_PRINTLN(WifiStation.getPassword());

	//WifiStation.disconnect();
	//WifiStation.enable(false);
	//system_soft_wdt_stop();
	//os_delay_us(500000);
	//system_soft_wdt_restart();

	//Stop timers
	DEBUG4_PRINT("All Timers are...");

	if (mqtt != NULL)
		mqtt->stopTimer();

	if (dsSensor != NULL)
		dsSensor->stopTimer();

	if (dhtSensor != NULL)
		dhtSensor->stopTimer();


	timerBMP.stop();
	timerWIFI.stop();
	DEBUG4_PRINTLN(" stopped");

	if (wifiCheckCount > 15)
		system_restart();

	//WifiStation.enable(true);
	WifiStation.waitConnection(reconnectOk, 60, checkWifi);

}


void checkWifiConnection() {
	if (WifiStation.isConnected()) {

		connectOk();
	}
}

void connectWifi(void) {

	DEBUG1_PRINT("*CW");
	PRINT_MEM();

	if (!isList) {
		DEBUG4_PRINT("isList is ");
		DEBUG4_PRINT(isList);
		return;
	}

	findNetwork();

	PRINT_MEM();

	int8_t result = AppSettings.loadNetwork();
	DEBUG4_PRINT("Result of AppSettings.loadNetwork() = ");
	DEBUG4_PRINTLN(String(result));

	PRINT_MEM();

	DEBUG1_PRINT("ASet.ssid = ");
	DEBUG1_PRINTLN(AppSettings.ssid);
	DEBUG1_PRINT("ASet.pass = ");
	DEBUG1_PRINTLN(AppSettings.password);

	if (result == 0) {

		WifiStation.config(AppSettings.ssid, AppSettings.password);
		WifiStation.waitConnection(connectOk, 30, connectFail);
		time2 = millis();
	}
	else {
		DEBUG4_PRINT("ERROR: Can't load network settings from Configuration file");
	}
	timerWIFI.stop();
	timerWIFI.initializeMs(300, checkWifiConnection).start();
	PRINT_MEM();
}

// Will be called when WiFi hardware and software initialization was finished
// And system initialization was completed
void ready() {

	DEBUG1_PRINT("*READY");
	PRINT_MEM();
	//Serial.print("!---- ready.start.Mem: ");
	//Serial.println(system_get_free_heap_size());

	if (!isList)
		WifiStation.startScan(listNetworks);


	// If AP is enabled:
	DEBUG4_PRINTF2("AP. ip: %s mac: %s", WifiAccessPoint.getIP().toString().c_str(), WifiAccessPoint.getMAC().c_str());
	timerWIFI.initializeMs(1000, connectWifi).start();
	PRINT_MEM();
	//Serial.print("!---- ready.end.Mem: ");
	//Serial.println(system_get_free_heap_size());
}

void OtaUpdate_CallBack(bool result) {

	DEBUG4_PRINTLN("In callback...");
	if(result == true) {
		// success
		uint8 slot;
		slot = rboot_get_current_rom();

		//if (slot == 0) slot = 1; else slot = 0; // commented to avoid switch the rom after update

		// set to boot new rom and then reboot
		DEBUG4_PRINTF("Firmware updated, rebooting to rom %d...\r\n", slot);
		rboot_set_current_rom(slot);
		System.restart();
	} else {
		// fail
		DEBUG4_PRINTLN("Firmware update failed!");
	}
}

/*
void OtaUpdateSW() {

	uint8 slot;
	rboot_config bootconf;

	DEBUG4_PRINTLN("Updating...");

	// need a clean object, otherwise if run before and failed will not run again
	if (otaUpdater) delete otaUpdater;
	otaUpdater = new rBootHttpUpdate();

	// select rom slot to flash
	bootconf = rboot_get_config();
	slot = bootconf.current_rom;
	if (slot == 0) slot = 1; else slot = 0;

#ifndef RBOOT_TWO_ROMS
	// flash rom to position indicated in the rBoot config rom table
	otaUpdater->addItem(bootconf.roms[slot], AppSettings.rom0);
	//otaUpdater->addItem(bootconf.roms[slot], ROM_0_URL);
#else
	// flash appropriate rom
	if (slot == 0) {
		otaUpdater->addItem(bootconf.roms[slot], AppSettings.rom0);
	} else {
		otaUpdater->addItem(bootconf.roms[slot], AppSettings.rom0);
	}
#endif

	// request switch and reboot on success
	//otaUpdater->switchToRom(slot);
	// and/or set a callback (called on failure or success without switching requested)
	otaUpdater->setCallback(OtaUpdate_CallBack);

	// start update
	otaUpdater->start();
}
 */

void OtaUpdate(bool isSpiffs) {

	uint8 slot;
	rboot_config bootconf;

	DEBUG4_PRINTLN("Updating...");

	// need a clean object, otherwise if run before and failed will not run again
	if (otaUpdater) delete otaUpdater;
	otaUpdater = new rBootHttpUpdate();

	// select rom slot to flash
	bootconf = rboot_get_config();
	slot = bootconf.current_rom;
	if (slot == 0) slot = 1; else slot = 0;

#ifndef RBOOT_TWO_ROMS
	// flash rom to position indicated in the rBoot config rom table
	otaUpdater->addItem(bootconf.roms[slot], AppSettings.rom0);
	//otaUpdater->addItem(bootconf.roms[slot], ROM_0_URL);
#else
	// flash appropriate rom
	if (slot == 0) {
		otaUpdater->addItem(bootconf.roms[slot], AppSettings.rom0);
	} else {
		otaUpdater->addItem(bootconf.roms[slot], AppSettings.rom0);
	}
#endif

#ifndef DISABLE_SPIFFS
	if (isSpiffs) {
		// use user supplied values (defaults for 4mb flash in makefile)
		if (slot == 0) {
			otaUpdater->addItem(RBOOT_SPIFFS_0, AppSettings.spiffs);
			//otaUpdater->addItem(RBOOT_SPIFFS_0, SPIFFS_URL);
		} else {
			otaUpdater->addItem(RBOOT_SPIFFS_1, AppSettings.spiffs);
			//otaUpdater->addItem(RBOOT_SPIFFS_1, SPIFFS_URL);
		}
	}
#endif

	// request switch and reboot on success
	//otaUpdater->switchToRom(slot);
	// and/or set a callback (called on failure or success without switching requested)
	otaUpdater->setCallback(OtaUpdate_CallBack);

	// start update
	otaUpdater->start();
}

void switchBootRom() {
	uint8 before, after;
	before = rboot_get_current_rom();
	if (before == 0) after = 1; else after = 0;
	DEBUG4_PRINTF2("Swapping from rom %d to rom %d.\r\n", before, after);
	rboot_set_current_rom(after);
	DEBUG4_PRINTLN("Restarting...\r\n");
	System.restart();
}

String ShowInfo() {
	String result;
	String temp;

	rboot_config bootconf;
	bootconf = rboot_get_config();

	result = "Info:";
	result += "\r\nBoot slot: ";
	result += bootconf.current_rom;
	result += "\r\nROM: ";
	result += AppSettings.version;
	result += "\r\nSDK: ";
	result += String(system_get_sdk_version());
	result += "\r\nFree Heap: ";
	result += String(system_get_free_heap_size());
	result += "\r\nCPU Frequency: ";
	result += String(system_get_cpu_freq());
	result += "MHz\r\nSystem Chip ID: 0x";
	temp = String(system_get_chip_id(), HEX);
	temp.toUpperCase();
	result += temp;
	result += "\r\nSPI Flash ID: 0x";
	temp = String(spi_flash_get_id(),HEX);
	temp.toUpperCase();
	result += temp;
	result += "\r\n";

	DEBUG4_PRINTLN(result);
	return result;
}


void configureNetwork() {
	DEBUG4_PRINTLN("There is no config...");
	DEBUG4_PRINTLN("Please write code to make config for firmware");
	DEBUG4_PRINTLN("*********************************************");

	//Initialization of rBoot OTA
	AppSettings.rBootInit();

	String mac = WifiAccessPoint.getMAC();
	int mLen = mac.length();
	mac = mac.substring(mLen-5);
	String ssid = CONFIG_WIFI_SSID;
	//ssid += "_" + mac;

	DEBUG4_PRINTLN("*** Starting Access Point, ssid=\"" + ssid + "\", password=\"" + CONFIG_WIFI_PASS + "\" ***");

	WifiAccessPoint.enable(true);
	WifiStation.enable(false);
	WifiAccessPoint.config(ssid, CONFIG_WIFI_PASS, AUTH_WPA_PSK);


	if (!fileExist("index.html"))
		fileSetContent("index.html", "<h3>Please connect to FTP and upload configuration file</h3>");

	// Start FTP server
	ftp.listen(21);
	ftp.addUser(FTP_USER, FTP_PASS); // FTP account


	Vector<String> files = fileList();
	DEBUG4_PRINTF("filecount %d\r\n", files.count());
	for (unsigned int i = 0; i < files.count(); i++) {
		DEBUG4_PRINTLN(files[i]);
	}
	DEBUG4_PRINTLN("*********************************************");
	if (files.count() > 0) {
		DEBUG4_PRINTF("dumping file %s:\r\n", files[0].c_str());
		DEBUG4_PRINTLN(fileGetContent(files[0]));
	} else {
		DEBUG4_PRINTLN("Empty spiffs!");
	}
	DEBUG4_PRINTLN("*********************************************");

	//Serial.setCallback(serialCallBack);

}

void initModules() {
	if (AppSettings.exist()) {
		DEBUG4_PRINTLN("ActStates.init().start");
		ActStates.init();
		DEBUG4_PRINTLN("ActStates.init().end");

		mqtt = new MQTT(AppSettings.broker_ip, AppSettings.broker_port,AppSettings.shift_mqtt, AppSettings.interval_mqtt,AppSettings.main_topic, AppSettings.client_topic, onMessageReceived);

		DEBUG4_PRINTF("pmqtt=\"%p\"", mqtt);

		if (AppSettings.is_dht) {
			dhtSensor = new SensorDHT(AppSettings.dht, DHT22, *mqtt, AppSettings.shift_dht, AppSettings.interval_dht);
		}

		if (AppSettings.is_ds) {
			dsSensor = new SensorDS(AppSettings.ds, 1, *mqtt, AppSettings.shift_ds, AppSettings.interval_ds);
		}

		if (AppSettings.is_bmp) { // I2C init
			//Wire.pins(SCL_PIN, SDA_PIN);
			Wire.pins(AppSettings.scl, AppSettings.sda);
			Wire.begin();
		}

		PRINT_MEM();

		if (AppSettings.is_insw) {
			byte in_cnt = AppSettings.in_cnt;
			for (byte i = 0; i < in_cnt; i++) {
				pinMode(AppSettings.in[i], INPUT);
			}

			if (in_cnt >= 1)
				attachInterrupt(AppSettings.in[0], interruptHandlerInSw1, HIGH);
			if (in_cnt >= 2)
				attachInterrupt(AppSettings.in[1], interruptHandlerInSw2, HIGH);
			if (in_cnt >= 3)
				attachInterrupt(AppSettings.in[2], interruptHandlerInSw3, HIGH);
			if (in_cnt >= 4)
				attachInterrupt(AppSettings.in[3], interruptHandlerInSw4, HIGH);
			if (in_cnt >= 5)
				attachInterrupt(AppSettings.in[4], interruptHandlerInSw5, HIGH);

		}

		//PRINT_MEM();
		DEBUG4_PRINTLN("initSw.start");
		initSw();
		DEBUG4_PRINTLN("initSw.end");
		PRINT_MEM();



		//DEBUG1_PRINTLN("Program started");

	}
}

void init() {
	//ets_wdt_enable();
	//ets_wdt_disable();
	time1 = millis();
	Serial.begin(SERIAL_BAUD_RATE); // 115200 by default

#ifdef DEBUG1
	Serial.systemDebugOutput(true); // Allow debug print to serial
#else
	Serial.systemDebugOutput(false); // Won't allow debug print to serial
#endif

	INFO_PRINT("Firmware started + Version: ");
	INFO_PRINTLN(AppSettings.version);

	PRINT_MEM();
	//Serial.print("StartMem: ");
	//Serial.println(system_get_free_heap_size());

	if (AppSettings.exist()) {
		AppSettings.loadWifiList();

		//PRINT_MEM();

		for (int i=0; i < AppSettings.wifi_cnt; i++)
			DEBUG4_PRINTLN(AppSettings.wifiList[i]);


		WifiAccessPoint.enable(false);
		WifiStation.enable(true);

		if (!(AppSettings.ssid.equals("")) && (AppSettings.ssid != null)) {
			WifiStation.config(AppSettings.ssid, AppSettings.password);
			WifiStation.waitConnection(connectOk, 30, connectFail); // We recommend 20+ seconds for connection timeout at start
			DEBUG1_PRINTF("ASet.ssid = %s  ", AppSettings.ssid.c_str());
			DEBUG1_PRINTF("pass = %s", AppSettings.password.c_str());
			DEBUG1_PRINTLN();
			//delay(1000);

			PRINT_MEM();
		}
		else {
			// Set system ready callback method
			System.onReady(ready);
		}

		initModules();
	}
	else {
		initHttp = true;
		WifiStation.waitConnection(connectOkHttpLoad, 30, connectFail);
		ERROR_PRINTLN("Error: there is no configuration file!");
	}

	//Change CPU freq. to 160MHZ
	//System.setCpuFrequency(eCF_160MHz);
	//Serial.print("New CPU frequency is:");
	//Serial.println((int)System.getCpuFrequency());

	//DEBUG4_PRINTLN("_init.end");
}
