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

FTPServer ftp;

// rBoot OTA object
rBootHttpUpdate* otaUpdater = 0;

//extern void wdt_feed (void);
void onMessageReceived(String topic, String message); // Forward declaration for our callback
void readOneWire(void);
void readDHT(void);
void readBarometer(void);
void publishSwitches(void);
void checkWifi(void);
void OtaUpdateSW();
void OtaUpdateAll();
void Switch();
void ready();
void connectOk();
void connectFail();
void stopAllTimers();
String ShowInfo();

Timer timerMQTT;
Timer timerDHT;
Timer timerBMP;
Timer timerDS;
Timer timerWIFI;
Timer timerSerialListener;
Timer timerListener;
Timer timerSerialCollector;
Timer timerSerialReceiver;


// wifi vars block
bool isList = false;
long time1, time2;
String *wifilist;
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
//OneWire ds(DS_PIN);
OneWire ds(AppSettings.ds);

// DHT object
//DHT dht(DHT_PIN, DHT22);
DHT dht(AppSettings.dht, DHT22);
float dhtTemp;
float dhtHum;


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
MqttClient mqtt(AppSettings.broker_ip.toString(), AppSettings.broker_port==0?1883:AppSettings.broker_port, onMessageReceived);

String topSubscr;

String topSw_In;

String topCfg_In;
String topCfg_Out;

String topSw_Out;
String topSw2_Out;

String topTemp1_Out;
String topHum1_Out;

String topDSTemp_Out;

String topBMPTemp_Out;
String topBMPPress_Out;

String topVCC_Out;
String topLog_Out;
String topStart_Out;

// Serial
String sTopSw_In;

String sTopSw_Out;


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
	}
	else {
		DEBUG4_PRINTF(" set sw[%d] to RED;  ", num);
		digitalWrite(AppSettings.sw[num], LOW);
		AppSettings.led.red(num);
	}
}

void initSw() {
	for (byte num = 0; num < ActStates.sw_cnt; num++) {
		turnSw(num, ActStates.sw[num]);
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
		mqtt.publish(topCfg_Out, logStr.c_str());
		DEBUG4_PRINTLN(logStr);
		return;
	}
	else {
		pushCount[num]++;
	}

	if (((millis() - pushTime[num]) < AppSettings.debounce_time) && (pushCount[num] > 2) && (!pushSwitched[num])) {

		pushSwitched[num] = true;
		turnSw(num, !ActStates.sw[num]);
		String logStr = String(pushTime[num]) + "   PRESSED in[" + String(num) + "] " + pushCount[num] +" times, nowState = " + String(ActStates.sw[num]);
		mqtt.publish(topCfg_Out, logStr.c_str());

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

	topSw_In = topicMain + "/in/" + topicClient + "sw";
	topSw_Out = topicMain + "/out/" + topicClient + "sw";

	// * Serial start *
	sTopSw_In = topicMain + "/in/" + topicClient + "ssw";
	sTopSw_Out = topicMain + "/out/" + topicClient + "ssw";
	// * Serial end *

	topTemp1_Out = topicMain + "/out/" + topicClient + "Temperature";
	topHum1_Out = topicMain + "/out/" + topicClient + "Humidity";

	topDSTemp_Out = topicMain + "/out/" + topicClient + "DS_Temperature";
	topBMPTemp_Out = topicMain + "/out/" + topicClient + "BMP_Temperature";
	topBMPPress_Out = topicMain + "/out/" + topicClient + "BMP_Pressure";

	topCfg_In = topicMain + "/in/" + topicClient + "config";
	topCfg_Out = topicMain + "/out/" + topicClient + "config";

	topLog_Out = topicMain + "/out/" + topicClient + "log";
	topStart_Out = topicMain + "/out/" + topicClient + "start";

	topVCC_Out = topicMain + "/out/" + topicClient + "VCC";

	mqttClientName = "esp8266-" + topicClient;
	mqttClientName += String(micros() & 0xffff, 16);

	loopIndex = 0;

	bmpTemp = -255;
	bmpPress = -255;

	dhtTemp = -255;
	dhtHum = -255;


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
		if (topic.equals((topSw_In+String(i+1)))) {
			if (message.equals("ON")) {
				turnSw(i, HIGH);
			} else if (message.equals("OFF")) {
				turnSw(i, LOW);
			} else
				DEBUG4_PRINTF("Topic %s, message is UNKNOWN", (topSw_In+String(i+1)).c_str());
		}
	}

	// *** Serial block ***

	for (byte i = 0; i < AppSettings.ssw_cnt; i++) {
		if (topic.equals((sTopSw_In+String(i+1)))) {
			if (message.equals("ON")) {
				turnSsw(i, HIGH);
			} else if (message.equals("OFF")) {
				turnSsw(i, LOW);
			} else
				DEBUG4_PRINTF("Topic %s, message is UNKNOWN", (sTopSw_In+String(i+1)).c_str());
		}
	}
	// *** Serial block end ***
	if (topic.equals(topCfg_In)) {

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
			mqtt.publish(topCfg_Out, "Will stop all timers and UPDATE on the air firmware only now");
			OtaUpdateSW();
		}
		else if (cmd.equals("sw_update_all")) {
			mqtt.publish(topCfg_Out, "Will stop all timers and UPDATE on the air firmware and spiffs now");
			stopAllTimers();
			OtaUpdateAll();
		}
		else if ((cmd.equals("version")) || (cmd.equals("ver"))) {
			mqtt.publish(topCfg_Out, AppSettings.version);
		}
		else if (cmd.equals("restart")) {
			mqtt.publish(topCfg_Out, "Will restart now");
			System.restart();
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
			mqtt.publish(topCfg_Out, result);
			Switch();
		}
		else if (cmd.equals("info")) {
			mqtt.publish(topCfg_Out, ShowInfo());
		}
		else if (cmd.equals("uptime")) {
			mqtt.publish(topCfg_Out, "Loopindex = " + String(loopIndex));

			String strUptime = uptime();
			DEBUG4_PRINTLN("Uptime is " + strUptime);
			mqtt.publish(topCfg_Out, "Uptime is " + strUptime);
		}
		else if (cmd.equals("reboot")) {
			DEBUG4_PRINTLN("REBOOT stub routine");
			system_restart();
		}
		else if (cmd.equals("set_update")) {
			String updList = AppSettings.update(root);
			mqtt.publish(topCfg_Out, updList);
		}
		else if (cmd.equals("set_save")) {
			AppSettings.save();
			mqtt.publish(topCfg_Out, "Settings saved.");
		}
		else if (cmd.equals("act_print")) {
			String strPrint = ActStates.print();
			DEBUG4_PRINTLN(strPrint);
			DEBUG4_PRINTF("Answer length is %d bytes\r\n", strPrint.length());
			mqtt.publish(topCfg_Out, strPrint);
		}
		else if (cmd.equals("act_printf")) {
			String strPrintf = ActStates.printf();
			DEBUG4_PRINTLN(strPrintf);
			DEBUG4_PRINTF("Answer length is %d bytes\r\n", strPrintf.length());
			mqtt.publish(topCfg_Out, strPrintf);
		}
		/*
		else if (cmd.equals("help")) {
			String strHelp;
			strHelp = "HELP\r\n";
			strHelp += "Example: { \"cmd\" : \"command name\", \"args\" : { \"argument\" : \"option\" } }";
			strHelp += "Commands: help, restart, info, uptime";
			strHelp += "   sw_update - will update firmware only, sw_update_all, switch";
			strHelp += "   sw_update_all - will update firmware and spiff";
			strHelp += "   switch - between roms (ex. from rom0 to rom1)";
			strHelp += "   set_printf - print current configurations from memory";
			strHelp += "   set_print - print configuration file";
			strHelp += "   set_update - update configurations from command package";
			strHelp += "      ex. {\"cmd\":\"set_update\", \"serial_speed\":115200}";
			strHelp += "   set_save - save configurations from memory to file";

			DEBUG4_PRINTLN(strHelp);
			mqtt.publish(topCfg_Out, strHelp);
		}
		 */
		else
			DEBUG4_PRINTLN("Topic matched, command is UNKNOWN");
	}
	else
		DEBUG4_PRINTLN("topic is UNKNOWN");
}

void startMqttClient() {

	DEBUG4_PRINTLN("_mqttConnect");
	DEBUG4_PRINT("_mqtt_broker_ip=");
	DEBUG4_PRINTLN(mqtt.server);
	DEBUG4_PRINT("_mqtt_port=");
	DEBUG4_PRINTLN(mqtt.port);

	/*
	Serial.print("*** MQTT host = ");
	Serial.println(serverHost);
	Serial.print("*** MQTT port = ");
	Serial.println(serverPort);
	 *
	 */

	// Run MQTT client
	byte state = mqtt.getConnectionState();
	DEBUG4_PRINT("MQTT.state=");
	DEBUG4_PRINTLN(state);

	if (state != eTCS_Connected) {
		mqtt.connect(mqttClientName);
		DEBUG4_PRINTLN("Connecting to MQTT broker");
		mqtt.subscribe(topSubscr);
	}
}


void publishSerialSw() {

	for (byte i = 0; i < ActStates.ssw_cnt; i++) {
		if (ActStates.ssw[i])
			mqtt.publish(sTopSw_Out+String(i+1), "ON");
		else
			mqtt.publish(sTopSw_Out+String(i+1), "OFF");
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
		result = mqtt.publish(getOutTopic("sdht_t"), String(sDHTTemp));
		if (result)
			sDHTTemp = UNDEF;
	}

	if (sDHTHum != UNDEF) {
		result = mqtt.publish(getOutTopic("sdht_h"), String(sDHTHum));
		if (result)
			sDHTHum = UNDEF;
	}

	// Publish BMP
	if (sBMPTemp != UNDEF) {
		result = mqtt.publish(getOutTopic("sbmp_t"), String(sBMPTemp));
		if (result)
			sBMPTemp = UNDEF;
	}
	if (sBMPPress != UNDEF) {
		result = mqtt.publish(getOutTopic("sbmp_p"), String(sBMPPress));
		if (result)
			sBMPPress = UNDEF;
	}

	// Publish DS
	int dsSize = sizeof sDSTemp / sizeof sDSTemp[0];
	for (int i=0; i < dsSize; i++) {
		if (sDSTemp[i] != UNDEF) {
			result = mqtt.publish(getOutTopic("sds_t") + String(i), String(sDSTemp[i]));
			if (result)
				sDSTemp[i] = UNDEF;
		}
	}

	// Publish Water
	if ((sWaterCold > 0) && (sWaterCold != UNDEF)) {
		result = mqtt.publish(getOutTopic("s_wc"), String(sWaterCold));
		if (result)
			sWaterCold = UNDEF;
	}
	if ((sWaterHot > 0) && (sWaterHot != UNDEF)) {
		result = mqtt.publish(getOutTopic("s_wh"), String(sWaterHot));
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

	startMqttClient();

	// Published start message and mqtt client name
	if (!isPubStart) {
		if (mqtt.publish(topStart_Out, "1")) {
			mqtt.publish(topLog_Out, mqttClientName);
			DEBUG4_PRINTLN("publish mqtt START!");
			isPubStart = true;
		}
	}

	publishSwitches();

	system_soft_wdt_stop();
	// Publish DS Temperature
	DEBUG4_PRINTLN();
	if (dsTemp[0] > 0) {
		for (int i = 1; i <= dsTemp[0]; i++) {
			DEBUG4_PRINT("DS Temperature ");
			DEBUG4_PRINT(i);
			DEBUG4_PRINT(" ");

			if (dsTemp[i] > -255) {
				DEBUG4_PRINT(dsTemp[i]);
				DEBUG4_PRINT(" C... ");

				bool result = mqtt.publish(topDSTemp_Out + String(i),
						String(dsTemp[i]));

				DEBUG4_PRINT("Published: ");
				DEBUG4_PRINTLN(result);

				if (result == true)
					dsTemp[i] = -255;
			} else
				DEBUG4_PRINTLN("ignore");
		}
	} else
		DEBUG4_PRINTLN("DS ignore");

	system_soft_wdt_restart();

	// Publish DHT22 data
	DEBUG4_PRINTLN();
	DEBUG4_PRINT("DHT Temperature ");
	if (dhtTemp > -255) {
		bool result = mqtt.publish(topTemp1_Out, String(dhtTemp));
		DEBUG4_PRINT(" ");
		DEBUG4_PRINT(dhtTemp);
		DEBUG4_PRINT(" C... ");
		DEBUG4_PRINT("Published: ");
		DEBUG4_PRINTLN(result);

		if (result == true)
			dhtTemp = -255;
	} else
		DEBUG4_PRINTLN("ignore");

	DEBUG4_PRINT("DHT Humidity ");
	if (dhtHum > -255) {
		bool result = mqtt.publish(topHum1_Out, String(dhtHum));
		DEBUG4_PRINT(" ");
		DEBUG4_PRINT(dhtHum);
		DEBUG4_PRINT(" %... ");
		DEBUG4_PRINT("Published: ");
		DEBUG4_PRINTLN(result);

		if (result == true)
			dhtHum = -255;
	} else
		DEBUG4_PRINTLN("ignore");

	system_soft_wdt_stop();

	// Publish BMP180 data
	DEBUG4_PRINTLN();
	DEBUG4_PRINT("BMP Temperature ");
	if (bmpTemp > -255) {
		bool result = mqtt.publish(topBMPTemp_Out, String(bmpTemp));
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

		bool result = mqtt.publish(topBMPPress_Out, String(bmpPress));
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
	mqtt.publish(topLog_Out, "serialCollector() cmd = " + String(SerialCommand::COLLECT) + " objType=" + String(ObjectType::ALL) + " objId=" + String(ObjectId::ALL));
}

void serialReceiver() {
	protocol.sendSerialMessage(SerialCommand::GET, ObjectType::ALL, ObjectId::ALL);
	mqtt.publish(topLog_Out, "serialReceiver() cmd = " + String(SerialCommand::GET) + " objType=" + String(ObjectType::ALL) + " objId=" + String(ObjectId::ALL));
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

void setReadDHT() {
	if (AppSettings.is_dht) {
		timerDHT.initializeMs(AppSettings.interval_dht, readDHT).start();
		DEBUG4_PRINTLN("*** DHT timer done!");
	}
}

void setReadBMP() {
	if (AppSettings.is_bmp) {
		timerBMP.initializeMs(AppSettings.interval_bmp, readBarometer).start();
		DEBUG4_PRINTLN("*** BMP timer done!");
	}
}

void setReadOneWire() {
	if (AppSettings.is_ds) {
		timerDS.initializeMs(AppSettings.interval_ds, readOneWire).start();
		DEBUG4_PRINTLN("*** DS OneWire timer done!");
	}
}

void setMQTT() {
	DEBUG4_PRINTLN("*** MQTT timer done!");
	timerMQTT.initializeMs(AppSettings.interval_mqtt, mqtt_loop).start();
}

void stopAllTimers(void) {
	DEBUG4_PRINTLN("MQTT will close connection and ALL timers will STOPPED now!");

	//mqtt.close(); //Close MQTT connection with server

	timerMQTT.stop();
	timerDHT.stop();
	timerBMP.stop();
	timerDS.stop();
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

	mqtt.publish(topLog_Out, "processSerialMessage() cmd = " + String(cmd) + " objType=" + String(objType) + " objId=" + String(objId));

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

// Will be called when WiFi station was connected to AP
void connectOk() {
	DEBUG4_PRINTLN("I'm CONNECTED");

	saveDefaultWifi();

	var_init();

	DEBUG4_PRINTLN("Start timers...");

	//procTimer.initializeMs(30 * 1000, user_loop).start();

	DEBUG4_PRINT("mqttTimer.. ");
	timerMQTT.initializeMs(AppSettings.shift_mqtt, setMQTT).startOnce();
	delay(50);
	DEBUG4_PRINTLN("armed");

	DEBUG4_PRINT("dsTimer.. ");
	timerDS.initializeMs(AppSettings.shift_ds, setReadOneWire).startOnce();
	delay(50);
	DEBUG4_PRINTLN("armed");

	DEBUG4_PRINT("dhtTimer.. ");
	timerDHT.initializeMs(AppSettings.shift_dht, setReadDHT).startOnce();
	delay(50);
	DEBUG4_PRINTLN("armed");

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
	DEBUG4_PRINTLN(mqtt.getConnectionState());
	DEBUG4_PRINTLN("Client name =\"" + mqttClientName + "\"");
	startMqttClient();
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


	ready();
	//WifiStation.waitConnection(connectOk, 20, connectFail); // Repeat and check again
}


void readDHT(void) {

	DEBUG4_PRINTLN("_readDHT");
	float humidity = dht.readHumidity();
	float temperature = dht.readTemperature();

	// check if returns are valid, if they are NaN (not a number) then something went wrong!
	if (isnan(temperature) || isnan(humidity)) {
		DEBUG4_PRINTLN("Failed to read from DHT");
		dhtTemp = -255;
		dhtHum = -255;
		return;
	} else {
		DEBUG4_PRINT("Humidity: ");
		DEBUG4_PRINT(humidity);
		DEBUG4_PRINT(" %\t");
		DEBUG4_PRINT("Temperature: ");
		DEBUG4_PRINT(temperature);
		DEBUG4_PRINTLN(" *C");
		dhtTemp = temperature;
		dhtHum = humidity;

		return;
	}

}

void publishSwitches() {

	for (byte i = 0; i < ActStates.sw_cnt; i++) {
		if (ActStates.getSw(i))
			mqtt.publish(topSw_Out + String(i+1), "ON");
		else
			mqtt.publish(topSw_Out + String(i+1), "OFF");
	}


	for (byte i = 0; i < ActStates.sw_cnt; i++) {
		DEBUG4_PRINTF("swState%d is ",i);
		DEBUG4_PRINTLN(ActStates.getSw(i));
	}


}

float readDCByAddr(byte addr[]) {

	DEBUG4_PRINTLN("_readDCByAddr");
	byte i;
	byte cnt = 1;
	byte present = 0;
	byte type_s;
	byte data[12];
	float celsius, fahrenheit;

	DEBUG4_PRINT("Thermometer ROM =");
	for (i = 0; i < 8; i++) {
		DEBUG4_WRITE(' ');
		DEBUG4_PRINTHEX(addr[i]);
	}

	if (OneWire::crc8(addr, 7) != addr[7]) {
		DEBUG4_PRINTLN("CRC is not valid!");
		return -255;
	}
	DEBUG4_PRINTLN();

	// the first ROM byte indicates which chip
	switch (addr[0]) {
	case 0x10:
		DEBUG4_PRINTLN("  Chip = DS18S20"); // or old DS1820
		type_s = 1;
		break;
	case 0x28:
		DEBUG4_PRINTLN("  Chip = DS18B20");
		type_s = 0;
		break;
	case 0x22:
		DEBUG4_PRINTLN("  Chip = DS1822");
		type_s = 0;
		break;
	default:
		DEBUG4_PRINTLN("Device is not a DS18x20 family device.");
		return -255;
	}

	ds.reset();
	ds.select(addr);
	ds.write(0x44, 1); // start conversion, with parasite power on at the end

	delay(1000); // maybe 750ms is enough, maybe not
	// we might do a ds.depower() here, but the reset will take care of it.

	present = ds.reset();
	ds.select(addr);
	ds.write(0xBE); // Read Scratchpad

	DEBUG4_PRINT("  Data = ");
	DEBUG4_PRINTHEX(present);
	DEBUG4_PRINT(" ");
	for (i = 0; i < 9; i++) {
		// we need 9 bytes
		data[i] = ds.read();
		DEBUG4_PRINTHEX(data[i]);
		DEBUG4_PRINT(" ");
	}
	DEBUG4_PRINT(" CRC=");
	DEBUG4_PRINTHEX(OneWire::crc8(data, 8));
	DEBUG4_PRINTLN();

	// Convert the data to actual temperature
	// because the result is a 16 bit signed integer, it should
	// be stored to an "int16_t" type, which is always 16 bits
	// even when compiled on a 32 bit processor.
	int16_t raw = (data[1] << 8) | data[0];
	if (type_s) {
		raw = raw << 3; // 9 bit resolution default
		if (data[7] == 0x10) {
			// "count remain" gives full 12 bit resolution
			raw = (raw & 0xFFF0) + 12 - data[6];
		}
	} else {
		byte cfg = (data[4] & 0x60);
		// at lower res, the low bits are undefined, so let's zero them
		if (cfg == 0x00)
			raw = raw & ~7; // 9 bit resolution, 93.75 ms
		else if (cfg == 0x20)
			raw = raw & ~3; // 10 bit res, 187.5 ms
		else if (cfg == 0x40)
			raw = raw & ~1; // 11 bit res, 375 ms
		//// default is 12 bit resolution, 750 ms conversion time
	}

	celsius = (float) raw / 16.0;
	fahrenheit = celsius * 1.8 + 32.0;
	DEBUG4_PRINT("  Temperature = ");
	DEBUG4_PRINT(celsius);
	DEBUG4_PRINT(" Celsius, ");
	DEBUG4_PRINTLN();

	return celsius;

}

void readOneWire() {

	DEBUG4_PRINTLN("_readOneWire");

	byte addr[8];
	byte cnt = 1;
	float celsius, fahrenheit;
	system_soft_wdt_stop();
	while (ds.search(addr)) {

		celsius = readDCByAddr(addr);
		if (celsius > -100.0)
			dsTemp[cnt++] = celsius;
	}

	dsTemp[0] = cnt - 1;
	system_soft_wdt_restart();
	return;

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
	timerMQTT.stop();
	timerDHT.stop();
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

void rBootInit() {
	DEBUG4_PRINTLN("Main.rBootInit() running");
	// mount spiffs
	int slot = rboot_get_current_rom();
#ifndef DISABLE_SPIFFS
	if (slot == 0) {
#ifdef RBOOT_SPIFFS_0
		DEBUG1_PRINTF2("trying to mount spiffs at %x, length %d", RBOOT_SPIFFS_0 + 0x40200000, SPIFF_SIZE);
		spiffs_mount_manual(RBOOT_SPIFFS_0 + 0x40200000, SPIFF_SIZE);
#else
		DEBUG1_PRINTF2("trying to mount spiffs at %x, length %d", 0x40300000, SPIFF_SIZE);
		spiffs_mount_manual(0x40300000, SPIFF_SIZE);
#endif
	} else {
#ifdef RBOOT_SPIFFS_1
		DEBUG1_PRINTF2("trying to mount spiffs at %x, length %d", RBOOT_SPIFFS_1 + 0x40200000, SPIFF_SIZE);
		spiffs_mount_manual(RBOOT_SPIFFS_1 + 0x40200000, SPIFF_SIZE);
#else
		DEBUG1_PRINTF2("trying to mount spiffs at %x, length %d", 0x40500000, SPIFF_SIZE);
		spiffs_mount_manual(0x40500000, SPIFF_SIZE);
#endif
	}
#else
	INFO_PRINTLN("spiffs disabled");
#endif

	INFO_PRINTF("\r\nCurrently running rom %d.\r\n", slot);
	INFO_PRINTLN();
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
		otaUpdater->addItem(bootconf.roms[slot], AppSettings.rom1);
	}
#endif

	// request switch and reboot on success
	//otaUpdater->switchToRom(slot);
	// and/or set a callback (called on failure or success without switching requested)
	otaUpdater->setCallback(OtaUpdate_CallBack);

	// start update
	otaUpdater->start();
}

void OtaUpdateAll() {

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
		otaUpdater->addItem(bootconf.roms[slot], AppSettings.rom1);
	}
#endif

#ifndef DISABLE_SPIFFS
	// use user supplied values (defaults for 4mb flash in makefile)
	if (slot == 0) {
		otaUpdater->addItem(RBOOT_SPIFFS_0, AppSettings.spiffs);
		//otaUpdater->addItem(RBOOT_SPIFFS_0, SPIFFS_URL);
	} else {
		otaUpdater->addItem(RBOOT_SPIFFS_0, AppSettings.spiffs);
		//otaUpdater->addItem(RBOOT_SPIFFS_1, SPIFFS_URL);
	}
#endif

	// request switch and reboot on success
	//otaUpdater->switchToRom(slot);
	// and/or set a callback (called on failure or success without switching requested)
	otaUpdater->setCallback(OtaUpdate_CallBack);

	// start update
	otaUpdater->start();
}

void Switch() {
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

	result = "Info:";
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
	rBootInit();

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
<<<<<<< Updated upstream
=======
}
*/

void initModules() {
	if (AppSettings.exist()) {
		DEBUG1_PRINTLN("ActStates.init().start");
		DEBUG1_PRINT("ASt.needInit=");DEBUG1_PRINTLN(ActStates.needInit);
		DEBUG1_PRINT("AS.msw_cnt=");DEBUG1_PRINTLN(AppSettings.msw_cnt);
		ActStates.init();
		DEBUG1_PRINTLN("ActStates.init().end");

		mqtt = new MQTT(AppSettings.broker_ip, AppSettings.broker_port,AppSettings.shift_mqtt, AppSettings.interval_mqtt,AppSettings.main_topic, AppSettings.client_topic, onMessageReceived);

		DEBUG4_PRINTF("pmqtt=\"%p\"", mqtt);

		if (AppSettings.is_dht) {
			dhtSensor = new SensorDHT(*mqtt);
			//dhtSensor = new SensorDHT(AppSettings.dht, DHT22, *mqtt, AppSettings.shift_dht, AppSettings.interval_dht);
		}

		if (AppSettings.is_ds) {
			dsSensor = new SensorDSS(*mqtt);	//dsSensor = new SensorDS(*mqtt, 1);
			//dsSensor = new SensorDS(AppSettings.ds, 1, *mqtt, AppSettings.shift_ds, AppSettings.interval_ds);
		}

		if (AppSettings.is_bmp) { // I2C init
			bmpSensor = new SensorBMP(*mqtt);
			//bmpSensor = new SensorBMP(AppSettings.scl, AppSettings.sda, *mqtt, AppSettings.shift_bmp, AppSettings.interval_bmp);
		}

		if (AppSettings.is_mcp) { // I2C init
			mcp = new MCP(*mqtt);
			//mcp = new SwIn(*mqtt);
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
>>>>>>> Stashed changes

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

	INFO_PRINT("Firmware started. Version: ");
	INFO_PRINTLN(AppSettings.version);

	PRINT_MEM();
	//Serial.print("StartMem: ");
	//Serial.println(system_get_free_heap_size());

	if (AppSettings.exist()) {
		ActStates.init();

		//AppSettings.load();
		//DEBUG4_PRINTLN(AppSettings.print());
		//DEBUG1_PRINTLN("Config loaded...");
		//AppSettings.print();
		//AppSettings.load_debug();

		AppSettings.loadWifiList();

		//PRINT_MEM();

		for (int i=0; i < AppSettings.wifi_cnt; i++)
			DEBUG4_PRINTLN(AppSettings.wifiList[i]);


		WifiAccessPoint.enable(false);
		WifiStation.enable(true);

		//int result = AppSettings.loadNetwork();
		//DEBUG4_PRINT("result of AppSettings.loadNetwork = ");
		//DEBUG4_PRINTLN(result);

		//PRINT_MEM();
		//Serial.print("!-- Mem after loadNetwork: ");
		//Serial.println(system_get_free_heap_size());
		//if (result==0) {
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

		if (AppSettings.is_dht)
			dht.begin();

		if (AppSettings.is_ds)
			ds.begin(); // It's required for one-wire initialization!

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

		initSw();

		PRINT_MEM();



		//DEBUG1_PRINTLN("Program started");

	}
	else {
		ERROR_PRINTLN("Error: there is no configuration file!");
	}

	//Change CPU freq. to 160MHZ
	//System.setCpuFrequency(eCF_160MHz);
	//Serial.print("New CPU frequency is:");
	//Serial.println((int)System.getCpuFrequency());

	//DEBUG4_PRINTLN("_init.end");
}
