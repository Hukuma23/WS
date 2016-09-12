#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include <AppSettings.h>
#include <ActStates.h>
#include <Logger.h>
#include <MQTT.h>
#include <Module.h>
#include <MCP.h>
#include <SerialConnector.h>
#include <lcd1602.h>


#define DEBOUNCE_TTIME 	20
#define FIRMWARE "20160911"

//FTPServer ftp;

//extern void wdt_feed (void);
void onMessageReceived(String topic, String message); // Forward declaration for our callback
void publishSwitches(void);
void checkWifi(void);
void OtaUpdate(bool isSpiffs);
void OtaUpdate(bool isSpiffs, String rom0, String spiffs);
void switchBootRom();
void ready();
void connectOk();
void connectFail();
void stopAllTimers();
String ShowInfo();
void initModules();
void fwUpdate();

void  interruptHandlerInSw1();
void  interruptHandlerInSw2();
void  interruptHandlerInSw3();
void  interruptHandlerInSw4();
void  interruptHandlerInSw5();


// rBoot OTA object
rBootHttpUpdate* otaUpdater = 0;

/*
// AppSettings & ActStates Singletons part
AppSettings& appSettings = AppSettings::getInstance();
ActStates& actStates = ActStates::getInstance();
*/

Timer timerWIFI;
Timer timerBtnHandle;
Timer timerUpdate;

bool initHttp = false;

// wifi vars block
bool isList = false;
long time1, time2;
String* wifilist;
uint8_t list_count;
// end wifi vars block

int wifiCheckCount = 0;

//SerialGuaranteedDeliveryProtocol protocol(&Serial);
SerialConnector* serialConnector;


bool state = true;
//String mqttClientName;
float dsTemp[3];
int vcc;
bool isPubStart = false;

// BMP180 object
SensorBMP* bmpSensor;

// DS object
SensorDSS* dsSensor;

// DHT object
SensorDHT* dhtSensor;

// MH-Z19 object
SensorMHZ* mhz;

// MCP/SwIn object
//SwIn* mcp;

// MCP object
MCP* mcp;


//byte swState[3];

// INSW
unsigned long pushTime[10] = {0,0,0,0,0,0,0,0,0,0};
byte pushCount[10];
bool pushSwitched[10];
byte btnNum = -1;


// MQTT client
MQTT* mqtt;


// AppSettings & ActStates clients
AppSettings* appSettings;
ActStates* actStates;

// LCD init
LCD1602* lcd;


void fwUpdate() {
	String rom0 = appSettings->urlHttp[appSettings->urlIndex] + appSettings->mac_addr + "/" + appSettings->fileNameHttp[1];
	String spiffs = appSettings->urlHttp[appSettings->urlIndex] + appSettings->mac_addr + "/" + appSettings->fileNameHttp[2];

	OtaUpdate(true, rom0, spiffs);
}

bool turnSw(byte num, bool state) {

	DEBUG4_PRINTF("num=%d; state=%d; ", num, state);

	actStates->setSw(num, state);
	String msg;

	if (actStates->getSw(num)) {
		//DEBUG4_PRINTF(" set sw[%d] to GREEN;  ", num);
		digitalWrite(appSettings->sw[num], HIGH);
		//appSettings->led->green(num);
		//mqtt.publish(sTopSw_Out+String(num+1), "ON");
	}
	else {
		//DEBUG4_PRINTF(" set sw[%d] to RED;  ", num);
		digitalWrite(appSettings->sw[num], LOW);
		//appSettings->led->red(num);
		//mqtt.publish(sTopSw_Out+String(num+1), "OFF");
	}

	mqtt->publish(appSettings->topSW, (num+1), OUT, (state?"ON":"OFF"));
	DEBUG4_PRINTLN("turnSw. Done");
	return actStates->getSw(num);
}

bool turnSw(byte num) {
	if ((num == -1) || (num == 255)) {
		ERROR_PRINT("ERROR: IN.turnSw num = ");
		ERROR_PRINTLN(num);
		return false;
	}

	return turnSw(num, !(actStates->getSw(num)));

}

void publish(byte num, bool state, bool longPressed) {
	if (mqtt)
		if (longPressed)
			mqtt->publish(appSettings->topIN_L, num+1, OUT, (state?"ON":"OFF"));
		else
			mqtt->publish(appSettings->topIN, num+1, OUT, (state?"ON":"OFF"));

}

void attachInIntByNum (byte num) {
	if (num == 0)
		attachInterrupt(appSettings->in[0], interruptHandlerInSw1, FALLING);
	else if (num == 1)
		attachInterrupt(appSettings->in[1], interruptHandlerInSw2, FALLING);
	else if (num == 2)
		attachInterrupt(appSettings->in[2], interruptHandlerInSw3, FALLING);
	else if (num == 3)
		attachInterrupt(appSettings->in[3], interruptHandlerInSw4, FALLING);
	else if (num == 4)
		attachInterrupt(appSettings->in[4], interruptHandlerInSw5, FALLING);
}

void initSw() {


	for (byte num = 0; num < appSettings->sw_cnt; num++) {
		DEBUG4_PRINTF("initSW. pin=%d ", appSettings->sw[num]);

		pinMode(appSettings->sw[num], OUTPUT);
		turnSw(num, actStates->sw[num]);
		DEBUG4_PRINTLN("  done!");
	}
}

void longtimeHandler() {
	DEBUG4_PRINTLN("IN.ltH");
	byte pin = appSettings->in[btnNum];
	uint8_t act_state = digitalRead(pin);
	//while (!(MCP23017::digitalRead(pin)));
	DEBUG4_PRINTF("push pin=%d state=%d. ", pin, act_state);

	bool isLong = false;

	if (act_state == LOW) {
		//if (mqtt)
		//	mqtt->publish(appSettings.topMIN_L, num+1, OUT, (actStates.msw[num]?"ON":"OFF"));
		isLong = true;
		DEBUG1_PRINTLN("*-long-*");
	}
	else {
		isLong = false;
		DEBUG1_PRINTLN("*-short-*");
	}

	publish(btnNum, actStates->sw[btnNum], isLong);

	//attachInterrupt(appSettings.m_int, Delegate<void()>(&MCP::interruptCallback, this), FALLING);
	attachInIntByNum(btnNum);
}

void interruptHandler() {
	DEBUG4_PRINTLN("IN.intH");
	//awakenByInterrupt = true;

	byte pin = appSettings->in[btnNum];
	//uint8_t last_state = getLastInterruptPinValue();
	uint8_t act_state = digitalRead(pin);

	DEBUG4_PRINTF("push pin=%d state=%d. ", pin, act_state);

	String msg = "push pin= " + String(pin) + ", state= " + String(act_state);
	mqtt->publish("log_in_handler", OUT, msg);

	if (act_state == LOW) {
		timerBtnHandle.initializeMs(LONG_TIME, longtimeHandler).startOnce();
		turnSw(btnNum);

		//String strState = (turnSw(btnNum)?"ON":"OFF");
		//if (mqtt)
		//	mqtt->publish(appSettings.topMIN, appSettings.getMInNumByPin(pin)+1, OUT, strState);
	}
	else {

		attachInIntByNum(btnNum);

		DEBUG4_PRINTLN();
	}

}


void interruptCallback(byte num) {
	//detachInterrupt(appSettings->in[num]);
	DEBUG4_PRINT("IN.intCB   ");
	DEBUG4_PRINT(appSettings->in[num]);
	btnNum = num;
	DEBUG4_PRINT("  ..1 ");

	DEBUG4_PRINT("..2 ");
	timerBtnHandle.initializeMs(DEBOUNCE_TTIME, interruptHandler).startOnce();
	DEBUG4_PRINT("..3 ");
	DEBUG4_PRINTLN("...end");
}

void  interruptHandlerInSw1() {
	detachInterrupt(appSettings->in[0]);
	interruptCallback(0);
}

void  interruptHandlerInSw2() {
	detachInterrupt(appSettings->in[1]);
	interruptCallback(1);
}

void  interruptHandlerInSw3() {
	detachInterrupt(appSettings->in[2]);
	interruptCallback(2);
}

void  interruptHandlerInSw4() {
	detachInterrupt(appSettings->in[3]);
	interruptCallback(3);
}

void  interruptHandlerInSw5() {
	detachInterrupt(appSettings->in[4]);
	interruptCallback(4);
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
	for (byte i = 0; i < appSettings->sw_cnt; i++) {
		//if (topic.equals((topSw_In+String(i+1)))) {
		if (topic.equals(mqtt->getTopic(appSettings->topSW, (i+1), IN))) {
			if (message.equals("ON")) {
				turnSw(i, HIGH);
			} else if (message.equals("OFF")) {
				turnSw(i, LOW);
			} else
				DEBUG4_PRINTF("Topic %s, message is UNKNOWN", (mqtt->getTopic(appSettings->topSW, (i+1), IN)).c_str());
		}
	}

	// MSW
	if (mcp) {
		mcp->processCallback(topic, message);
	}
	// *** Serial block ***
	if (serialConnector) {
		if (serialConnector->processCallback(topic, message)) return;
	}

	if (topic.equals(mqtt->getTopic(appSettings->topConfig, IN))) {

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

		String cmd = root["cmd"];
		//DEBUG4_PRINTLN("JSON 4");
		DEBUG4_PRINTLN("cmd = " + cmd);
		//DEBUG4_PRINTLN("JSON 5");
		JsonArray& opts = root["opts"];
		//DEBUG4_PRINTLN("JSON 6");

		if (cmd.equals("update_noconf")) {
			stopAllTimers();
			//mqtt.publish(topCfg_Out, "Will stop all timers and UPDATE on the air firmware only now");
			mqtt->publish(appSettings->topConfig, OUT, "Will stop all timers and UPDATE on the air firmware only now");
			OtaUpdate(false); //OtaUpdateSW();
		}
		else if (cmd.equals("update_conf")) {
			//mqtt.publish(topCfg_Out, "Will stop all timers and UPDATE on the air firmware and spiffs now");
			mqtt->publish(appSettings->topConfig, OUT, "Will stop all timers and UPDATE on the air firmware and spiffs now");
			stopAllTimers();
			OtaUpdate(true);
		}
		else if (cmd.equals("update")) {
			//mqtt.publish(topCfg_Out, "Will stop all timers and UPDATE on the air firmware and spiffs now");
			mqtt->publish(appSettings->topConfig, OUT, "Will stop all timers and UPDATE on the air firmware and spiffs now");
			stopAllTimers();
			timerUpdate.initializeMs(UPDATE_TRY_PERIOD, fwUpdate).startOnce();
		}
		else if ((cmd.equals("mac")) || (cmd.equals("getMac"))) {
			mqtt->publish(appSettings->topConfig, OUT, appSettings->mac_addr);
		}
		else if ((cmd.equals("version")) || (cmd.equals("ver"))) {
			//mqtt.publish(topCfg_Out, appSettings->version);
			mqtt->publish(appSettings->topConfig, OUT, appSettings->version);
		}
		else if (cmd.equals("conf_del")) {
			//mqtt.publish(topCfg_Out, "Delete config now");
			mqtt->publish(appSettings->topConfig, OUT, "Will delete config now");
			appSettings->deleteConf();
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
			mqtt->publish(appSettings->topConfig, OUT, result);
			switchBootRom();
		}
		else if (cmd.equals("info")) {
			//mqtt.publish(topCfg_Out, ShowInfo());
			mqtt->publish(appSettings->topConfig, OUT, ShowInfo());
		}
		else if (cmd.equals("uptime")) {

			String strUptime = mqtt->getUptime();
			DEBUG4_PRINTLN("Uptime is " + strUptime);
			//mqtt.publish(topCfg_Out, "Uptime is " + strUptime);
			mqtt->publish(appSettings->topConfig, OUT, "Uptime is " + strUptime);
		}
		else if (cmd.equals("reboot")) {
			DEBUG4_PRINTLN("REBOOT stub routine");
			system_restart();
		}
		else if (cmd.equals("conf_httpload")) {
			String updList = "Will try to load Settings by http";
			appSettings->loadHttp();
			//mqtt.publish(topCfg_Out, updList);
			mqtt->publish(appSettings->topConfig, OUT, updList);

		}
		else if (cmd.equals("conf_save")) {
			appSettings->save();
			//mqtt.publish(topCfg_Out, "Settings saved.");
			mqtt->publish(appSettings->topConfig, OUT, "Settings saved.");
		}
		else if (cmd.equals("act_print")) {
			String strPrint = actStates->print();
			DEBUG4_PRINTLN(strPrint);
			DEBUG4_PRINTF("Answer length is %d bytes\r\n", strPrint.length());
			//mqtt.publish(topCfg_Out, strPrint);
			mqtt->publish(appSettings->topConfig, OUT, strPrint);
		}
		else if (cmd.equals("act_printf")) {
			String strPrintf = actStates->printf();
			DEBUG4_PRINTLN(strPrintf);
			DEBUG4_PRINTF("Answer length is %d bytes\r\n", strPrintf.length());
			//mqtt.publish(topCfg_Out, strPrintf);
			mqtt->publish(appSettings->topConfig, OUT, strPrintf);
		}
		else
			DEBUG4_PRINTLN("Topic matched, command is UNKNOWN");
	}
	else
		DEBUG4_PRINTLN("topic is UNKNOWN");
}

void mqtt_loop() {

	INFO_PRINTLN("_mqtt_loop");
	mqtt->publish(appSettings->topLog+"_mem",OUT,String(system_get_free_heap_size()));
	PRINT_MEM();

	publishSwitches();

	// Publish serial moved to SerialConnector::serialCollector (after serialSendMessage block)
	//publishSerial();

	// Publish VCC
	//vcc = readvdd33();
	//DEBUG4_PRINT("VCC is ");
	//DEBUG4_PRINT(vcc);
	//DEBUG4_PRINTLN(" mV");


	INFO_PRINTLN("_user_loop.end");
	// Go to deep sleep
	//system_deep_sleep_set_option(1);
	//system_deep_sleep(4000000);
}




void setCheckWifi() {
	if (appSettings->is_wifi) {
		timerWIFI.initializeMs(appSettings->interval_wifi, checkWifi).start();
		DEBUG4_PRINTLN("*** WiFi timer done!");
	}
}


void stopAllTimers(void) {
	DEBUG4_PRINTLN("MQTT will close connection and ALL timers will STOPPED now!");

	//mqtt.close(); //Close MQTT connection with server

	if (mqtt != NULL)
		mqtt->stopTimer();

	if (dsSensor != NULL)
		dsSensor->stopTimer();

	if (dhtSensor != NULL)
		dhtSensor->stopTimer();

	if (bmpSensor != NULL)
		bmpSensor->stopTimer();

	if (mcp != NULL)
		mcp->stopTimer();

	if (mhz != NULL)
		mhz->stopTimer();

	timerWIFI.stop();


	if (serialConnector) {
		serialConnector->stopTimers();

	}
}



/* перенесено в класс SerialGuaranteedDeliveryProtocol.listener
void listenSerialMessage() {
	protocol.listener();
}
*/

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
	for (uint8_t j=appSettings->network_ind; j < appSettings->wifi_cnt; j++) {
		for (int i = 0; i < list_count; i++) {

			//if (appSettings->wifiList[j].equalsIgnoreCase(wifilist[i])) {
			if (appSettings->wifiList[j].equals(wifilist[i])) {
				appSettings->ssid = wifilist[i];
				appSettings->network_ind = j+1;

				DEBUG4_PRINT("network=");
				DEBUG4_PRINTLN(appSettings->network_ind);

				DEBUG4_PRINT("ssid=");
				DEBUG4_PRINTLN(appSettings->ssid);

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

	DEBUG4_PRINT("appSettings->ssid = ");
	DEBUG4_PRINTLN(appSettings->ssid);
	DEBUG4_PRINT("appSettings->pass = ");
	DEBUG4_PRINTLN(appSettings->password);

	int result = appSettings->loadNetwork(WifiStation.getSSID());
	DEBUG4_PRINT("result of appSettings->loadNetwork = ");
	DEBUG4_PRINTLN(result);
	appSettings->saveLastWifi();

	//DEBUG4_PRINTLN(appSettings->printf());
}


void startTimers() {

	if (mqtt) {
		DEBUG4_PRINT("mqttTimer.. ");
		mqtt->startTimer(mqtt_loop); //timerMQTT.initializeMs(appSettings->shift_mqtt, setMQTT).startOnce();
		delay(50);
		DEBUG4_PRINTLN("armed");
	}

	if (dsSensor) {
		DEBUG4_PRINT("dsTimer.. ");
		dsSensor->startTimer(); //timerDS.initializeMs(appSettings->shift_ds, setReadOneWire).startOnce();
		delay(50);
		DEBUG4_PRINTLN("armed");
	}

	if (dhtSensor) {
		DEBUG4_PRINT("dhtTimer.. ");
		dhtSensor->startTimer();	//timerDHT.initializeMs(appSettings->shift_dht, setReadDHT).startOnce();
		delay(50);
		DEBUG4_PRINTLN("armed");
	}

	if (bmpSensor) {
		DEBUG4_PRINT("bmpTimer.. ");
		bmpSensor->startTimer();	//timerBMP.initializeMs(appSettings->shift_bmp, setReadBMP).startOnce();
		delay(50);
		DEBUG4_PRINTLN("armed");
	}

	if (mcp) {
		DEBUG4_PRINT("mcpTimer.. ");
		mcp->startTimer();
		delay(50);
		DEBUG4_PRINTLN("armed");
	}

	if (mhz) {
		DEBUG4_PRINT("mhzTimer.. ");
		mhz->startTimer();
		delay(50);
		DEBUG4_PRINTLN("armed");
	}

	DEBUG4_PRINT("wifiTimer.. ");
	timerWIFI.initializeMs(appSettings->shift_wifi, setCheckWifi).startOnce();
	delay(50);
	DEBUG4_PRINTLN("armed");

	DEBUG4_PRINT("MQTT.state=");
	DEBUG4_PRINTLN(mqtt->getConnectionState());
	DEBUG4_PRINTLN("Client name =\"" + mqtt->getName() + "\"");

	//startMqttClient();
	publishSwitches();

	if (serialConnector) {

		DEBUG4_PRINT("serialCollectorTimer.. ");
		serialConnector->startSerialCollector();	//timerSerialCollector.initializeMs(appSettings->shift_collector, setSerialCollector).startOnce();
		delay(50);
		DEBUG4_PRINTLN("armed");

		DEBUG4_PRINT("serialReceiverTimer.. ");
		serialConnector->startSerialReceiver();	//timerSerialReceiver.initializeMs(appSettings->shift_receiver, setSerialReceiver).startOnce();
		delay(50);
		DEBUG4_PRINTLN("armed");


		DEBUG4_PRINT("serialListenerTimer.. ");

		serialConnector->startListener();
		DEBUG4_PRINTLN("armed");

		DEBUG4_PRINTLN("Send sw init to Serial");
		uint8_t sw = actStates->getSsw();
		DEBUG4_PRINT("Sw from ActStates = ");
		DEBUG4_PRINTLN(sw);
		serialConnector->sendSerialMessage(SerialCommand::SET_SW, ObjectType::SWITCH, ObjectId::ALL, sw);
		serialConnector->publishSerialSwitches();
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

	DEBUG4_PRINTLN("Start timers...");
	startTimers();

}

void checkAppSettings() {
	if (appSettings->exist()) {
		DEBUG1_PRINTLN("Settings.conf successful downloaded. ESP8266 will be restarted soon");
		timerWIFI.stop();
		//System.restart();
		connectOk();
	}
}

void connectOkHttpLoad() {
	DEBUG4_PRINTLN("I'm CONNECTED needs Settings.conf load by http");

	appSettings->loadHttp();

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

	for (byte i = 0; i < appSettings->sw_cnt; i++) {
		if (actStates->getSw(i))
			mqtt->publish(appSettings->topSW, (i+1), OUT, "ON");	//mqtt.publish(topSw_Out + String(i+1), "ON");
		else
			mqtt->publish(appSettings->topSW, (i+1), OUT, "OFF");	//mqtt.publish(topSw_Out + String(i+1), "OFF");
	}

	for (byte i = 0; i < appSettings->sw_cnt; i++) {
		DEBUG4_PRINTF("swState%d is ",i);
		DEBUG4_PRINTLN(actStates->getSw(i));
	}
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


	//Stop timers
	DEBUG4_PRINT("All Timers are...");

	if (mqtt)
		mqtt->stopTimer();

	if (dsSensor)
		dsSensor->stopTimer();

	if (dhtSensor)
		dhtSensor->stopTimer();

	if (bmpSensor)
		bmpSensor->stopTimer();

	if (mcp)
		mcp->stopTimer();

	if (mhz)
		mhz->stopTimer();

	if (serialConnector)
		serialConnector->stopTimers();

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

	int8_t result = appSettings->loadNetwork();
	DEBUG4_PRINT("Result of appSettings->loadNetwork() = ");
	DEBUG4_PRINTLN(String(result));

	PRINT_MEM();

	DEBUG1_PRINT("ASet.ssid = ");
	DEBUG1_PRINTLN(appSettings->ssid);
	DEBUG1_PRINT("ASet.pass = ");
	DEBUG1_PRINTLN(appSettings->password);

	if (result == 0) {

		WifiStation.config(appSettings->ssid, appSettings->password);
		WifiStation.waitConnection(connectOk, 30, connectFail);
		time2 = millis();
	}
	else {
		DEBUG4_PRINT("ERROR: Can't load network settings from Configuration file");
		appSettings->deleteConf();
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

	if (!isList)
		WifiStation.startScan(listNetworks);


	// If AP is enabled:
	//DEBUG4_PRINTF("AP. ip: %s mac: %s", WifiAccessPoint.getIP().toString().c_str(), WifiAccessPoint.getMAC().c_str());
	timerWIFI.initializeMs(1000, connectWifi).start();
	PRINT_MEM();
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
		mqtt->publish(appSettings->topLog, OUT, "Firmware updated successful, will reboot now");
		rboot_set_current_rom(slot);
		System.restart();
	} else {
		// fail
		DEBUG4_PRINTLN("Firmware update failed!");
		appSettings->urlIndexChange();
		timerUpdate.initializeMs(UPDATE_TRY_PERIOD, fwUpdate).startOnce();
	}
}

void OtaUpdate(bool isSpiffs, String rom0, String spiffs) {

	uint8 slot;
	rboot_config bootconf;

	DEBUG4_PRINTLN("Updating...");

	if (isSpiffs)
		mqtt->publish(appSettings->topLog, OUT, "rom0: " + rom0 + "\r\nspiffs: " + spiffs);
	else
		mqtt->publish(appSettings->topLog, OUT, "rom0: " + rom0);

	// need a clean object, otherwise if run before and failed will not run again
	if (otaUpdater) delete otaUpdater;
	otaUpdater = new rBootHttpUpdate();

	// select rom slot to flash
	bootconf = rboot_get_config();
	slot = bootconf.current_rom;
	if (slot == 0) slot = 1; else slot = 0;

#ifndef RBOOT_TWO_ROMS
	// flash rom to position indicated in the rBoot config rom table
	otaUpdater->addItem(bootconf.roms[slot], rom0);
	//otaUpdater->addItem(bootconf.roms[slot], ROM_0_URL);
#else
	// flash appropriate rom
	if (slot == 0) {
		otaUpdater->addItem(bootconf.roms[slot], rom0);
	} else {
		otaUpdater->addItem(bootconf.roms[slot], rom0);
	}
#endif

#ifndef DISABLE_SPIFFS
	if (isSpiffs) {
		// use user supplied values (defaults for 4mb flash in makefile)
		DEBUG1_PRINTLN("Add Spiffs URL to update");
		if (slot == 0) {
			otaUpdater->addItem(RBOOT_SPIFFS_0, spiffs);
			//otaUpdater->addItem(RBOOT_SPIFFS_0, SPIFFS_URL);
		} else {
			otaUpdater->addItem(RBOOT_SPIFFS_1, spiffs);
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

void OtaUpdate(bool isSpiffs) {
	OtaUpdate(isSpiffs, appSettings->rom0, appSettings->spiffs);
}

void switchBootRom() {
	uint8 before, after;
	before = rboot_get_current_rom();
	if (before == 0) after = 1; else after = 0;
	DEBUG4_PRINTF("Swapping from rom %d to rom %d.\r\n", before, after);
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
	result += FIRMWARE;
	result += "\r\nConfig: ";
	result += appSettings->version;
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

/*
void configureNetwork() {
	DEBUG4_PRINTLN("There is no config...");
	DEBUG4_PRINTLN("Please write code to make config for firmware");
	DEBUG4_PRINTLN("*********************************************");

	//Initialization of rBoot OTA
	appSettings->rBootInit();

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
 */

void initModules() {
	if (appSettings->exist()) {
		DEBUG1_PRINTLN("actStates->init().start");
		DEBUG1_PRINT("ASt.needInit=");DEBUG1_PRINTLN(actStates->needInit);
		DEBUG1_PRINT("AS.msw_cnt=");DEBUG1_PRINTLN(appSettings->msw_cnt);
		actStates->init();
		DEBUG1_PRINTLN("actStates->init().end");

		mqtt = new MQTT(appSettings->broker_ip, appSettings->broker_port,appSettings->shift_mqtt, appSettings->interval_mqtt,appSettings->main_topic, appSettings->client_topic, onMessageReceived);

		DEBUG4_PRINTF("pmqtt=\"%p\"", mqtt);

		if (appSettings->is_mhz) { // Serial init
			mhz = new SensorMHZ(*mqtt, *appSettings, &Serial);
			lcd = new LCD1602(*appSettings);
		}

		if (appSettings->is_dht) {
			dhtSensor = new SensorDHT(*mqtt, *appSettings);
			//dhtSensor = new SensorDHT(appSettings->dht, DHT22, *mqtt, appSettings->shift_dht, appSettings->interval_dht);
		}

		if (appSettings->is_ds) {
			dsSensor = new SensorDSS(*mqtt, *appSettings);	//dsSensor = new SensorDS(*mqtt, 1);
			//dsSensor = new SensorDS(appSettings->ds, 1, *mqtt, appSettings->shift_ds, appSettings->interval_ds);
		}

		if (appSettings->is_bmp) { // I2C init
			bmpSensor = new SensorBMP(*mqtt, *appSettings);
			//bmpSensor = new SensorBMP(appSettings->scl, appSettings->sda, *mqtt, appSettings->shift_bmp, appSettings->interval_bmp);
		}

		if (appSettings->is_mcp) { // I2C init
			mcp = new MCP(*mqtt, *appSettings, *actStates);
			//mcp = new SwIn(*mqtt);
		}

		if (appSettings->is_serial) {
			serialConnector = new SerialConnector(&Serial, *mqtt, *appSettings, *actStates);
		}

		PRINT_MEM();

		if (appSettings->is_insw) {
			byte in_cnt = appSettings->in_cnt;
			for (byte i = 0; i < in_cnt; i++) {
				pinMode(appSettings->in[i], INPUT);
			}

			if (in_cnt >= 1)
				attachInterrupt(appSettings->in[0], interruptHandlerInSw1, FALLING);
			if (in_cnt >= 2)
				attachInterrupt(appSettings->in[1], interruptHandlerInSw2, FALLING);
			if (in_cnt >= 3)
				attachInterrupt(appSettings->in[2], interruptHandlerInSw3, FALLING);
			if (in_cnt >= 4)
				attachInterrupt(appSettings->in[3], interruptHandlerInSw4, FALLING);
			if (in_cnt >= 5)
				attachInterrupt(appSettings->in[4], interruptHandlerInSw5, FALLING);
		}

		initSw();
		PRINT_MEM();
	}
}

void init() {
	//ets_wdt_enable();
	//ets_wdt_disable();
	time1 = millis();
	Serial.begin(115200); // 115200 by default

	INFO_PRINT("Firmware started + Version: ");
	appSettings = new AppSettings();
	actStates = new ActStates(*appSettings);

#ifdef DEBUG1
	Serial.systemDebugOutput(true); // Allow debug print to serial
#else
	Serial.systemDebugOutput(false); // Won't allow debug print to serial
#endif




	INFO_PRINTLN(appSettings->version);

	PRINT_MEM();

	if (appSettings->exist()) {
		appSettings->loadWifiList();

		//PRINT_MEM();

		for (int i=0; i < appSettings->wifi_cnt; i++)
			DEBUG4_PRINTLN(appSettings->wifiList[i]);


		WifiAccessPoint.enable(false);
		WifiStation.enable(true);

		if (!(appSettings->ssid.equals("")) && (appSettings->ssid != null)) {
			WifiStation.config(appSettings->ssid, appSettings->password);
			WifiStation.waitConnection(connectOk, 30, connectFail); // We recommend 20+ seconds for connection timeout at start
			DEBUG1_PRINTF("ASet.ssid = %s  ", appSettings->ssid.c_str());
			DEBUG1_PRINTF("pass = %s", appSettings->password.c_str());
			DEBUG1_PRINTLN();

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
