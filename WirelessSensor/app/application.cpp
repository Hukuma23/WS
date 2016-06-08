#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include <AppSettings.h>
#include <ActStates.h>
#include <Logger.h>
#include <LED.h>
#include <MQTT.h>
#include <Module.h>
#include <MCP.h>
#include <SerialConnector.h>

//FTPServer ftp;

// rBoot OTA object
rBootHttpUpdate* otaUpdater = 0;

//extern void wdt_feed (void);
void onMessageReceived(String topic, String message); // Forward declaration for our callback
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

Timer timerWIFI;

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

// MCP/SwIn object
//SwIn* mcp;

// MCP object
MCP* mcp;


//byte swState[3];

// INSW
unsigned long pushTime[10] = {0,0,0,0,0,0,0,0,0,0};
byte pushCount[10];
bool pushSwitched[10];


// MQTT client
MQTT* mqtt;

void IRAM_ATTR turnSw(byte num, bool state) {

	DEBUG4_PRINTF("num=%d; state=%d; ", num, state);

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


	for (byte num = 0; num < AppSettings.sw_cnt; num++) {
		DEBUG4_PRINTF("initSW. pin=%d ", AppSettings.sw[num]);

		pinMode(AppSettings.sw[num], OUTPUT);
		turnSw(num, ActStates.sw[num]);
		DEBUG4_PRINTLN("  done!");
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

	/*for (int i=0; i < sizeof swState; i++) {
		swState[i] = UNDEF;
	}
*/
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
		if (topic.equals(mqtt->getTopic(AppSettings.topSW, (i+1), IN))) {
			if (message.equals("ON")) {
				turnSw(i, HIGH);
			} else if (message.equals("OFF")) {
				turnSw(i, LOW);
			} else
				DEBUG4_PRINTF("Topic %s, message is UNKNOWN", (mqtt->getTopic(AppSettings.topSW, (i+1), IN)).c_str());
		}
	}

	// MSW
	if (mcp) {
		mcp->processCallback(topic, message);
	}
	// *** Serial block ***
	if (serialConnector)
		serialConnector->processCallback(topic, message);


	if (topic.equals(mqtt->getTopic(AppSettings.topConfig, IN))) {

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
		JsonArray& args = root["args"];
		//DEBUG4_PRINTLN("JSON 6");

		if (cmd.equals("sw_update")) {
			stopAllTimers();
			//mqtt.publish(topCfg_Out, "Will stop all timers and UPDATE on the air firmware only now");
			mqtt->publish(AppSettings.topConfig, OUT, "Will stop all timers and UPDATE on the air firmware only now");
			OtaUpdate(false); //OtaUpdateSW();
		}
		else if (cmd.equals("sw_update_all")) {
			//mqtt.publish(topCfg_Out, "Will stop all timers and UPDATE on the air firmware and spiffs now");
			mqtt->publish(AppSettings.topConfig, OUT, "Will stop all timers and UPDATE on the air firmware and spiffs now");
			stopAllTimers();
			OtaUpdate(true);
		}
		else if ((cmd.equals("version")) || (cmd.equals("ver"))) {
			//mqtt.publish(topCfg_Out, AppSettings.version);
			mqtt->publish(AppSettings.topConfig, OUT, AppSettings.version);
		}
		else if (cmd.equals("restart")) {
			//mqtt.publish(topCfg_Out, "Will restart now");
			mqtt->publish(AppSettings.topConfig, OUT, "Will restart now");
			System.restart();
		}
		else if (cmd.equals("conf_del")) {
			//mqtt.publish(topCfg_Out, "Delete config now");
			mqtt->publish(AppSettings.topConfig, OUT, "Will delete config now");
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
			mqtt->publish(AppSettings.topConfig, OUT, result);
			switchBootRom();
		}
		else if (cmd.equals("info")) {
			//mqtt.publish(topCfg_Out, ShowInfo());
			mqtt->publish(AppSettings.topConfig, OUT, ShowInfo());
		}
		else if (cmd.equals("uptime")) {

			String strUptime = mqtt->getUptime();
			DEBUG4_PRINTLN("Uptime is " + strUptime);
			//mqtt.publish(topCfg_Out, "Uptime is " + strUptime);
			mqtt->publish(AppSettings.topConfig, OUT, "Uptime is " + strUptime);
		}
		else if (cmd.equals("reboot")) {
			DEBUG4_PRINTLN("REBOOT stub routine");
			system_restart();
		}
		else if (cmd.equals("conf_httpload")) {
			String updList = "Will try to load Settings by http";
			AppSettings.loadHttp();
			//mqtt.publish(topCfg_Out, updList);
			mqtt->publish(AppSettings.topConfig, OUT, updList);

		}
		else if (cmd.equals("conf_save")) {
			AppSettings.save();
			//mqtt.publish(topCfg_Out, "Settings saved.");
			mqtt->publish(AppSettings.topConfig, OUT, "Settings saved.");
		}
		else if (cmd.equals("act_print")) {
			String strPrint = ActStates.print();
			DEBUG4_PRINTLN(strPrint);
			DEBUG4_PRINTF("Answer length is %d bytes\r\n", strPrint.length());
			//mqtt.publish(topCfg_Out, strPrint);
			mqtt->publish(AppSettings.topConfig, OUT, strPrint);
		}
		else if (cmd.equals("act_printf")) {
			String strPrintf = ActStates.printf();
			DEBUG4_PRINTLN(strPrintf);
			DEBUG4_PRINTF("Answer length is %d bytes\r\n", strPrintf.length());
			//mqtt.publish(topCfg_Out, strPrintf);
			mqtt->publish(AppSettings.topConfig, OUT, strPrintf);
		}
		else
			DEBUG4_PRINTLN("Topic matched, command is UNKNOWN");
	}
	else
		DEBUG4_PRINTLN("topic is UNKNOWN");
}

void mqtt_loop() {

	INFO_PRINTLN("_mqtt_loop");
	mqtt->publish(AppSettings.topLog+"_mem",OUT,String(system_get_free_heap_size()));
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
	if (AppSettings.is_wifi) {
		timerWIFI.initializeMs(AppSettings.interval_wifi, checkWifi).start();
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

	if (mqtt) {
		DEBUG4_PRINT("mqttTimer.. ");
		mqtt->startTimer(mqtt_loop); //timerMQTT.initializeMs(AppSettings.shift_mqtt, setMQTT).startOnce();
		delay(50);
		DEBUG4_PRINTLN("armed");
	}

	if (dsSensor) {
		DEBUG4_PRINT("dsTimer.. ");
		dsSensor->startTimer(); //timerDS.initializeMs(AppSettings.shift_ds, setReadOneWire).startOnce();
		delay(50);
		DEBUG4_PRINTLN("armed");
	}

	if (dhtSensor) {
		DEBUG4_PRINT("dhtTimer.. ");
		dhtSensor->startTimer();	//timerDHT.initializeMs(AppSettings.shift_dht, setReadDHT).startOnce();
		delay(50);
		DEBUG4_PRINTLN("armed");
	}

	if (bmpSensor) {
		DEBUG4_PRINT("bmpTimer.. ");
		bmpSensor->startTimer();	//timerBMP.initializeMs(AppSettings.shift_bmp, setReadBMP).startOnce();
		delay(50);
		DEBUG4_PRINTLN("armed");
	}

	if (mcp) {
		DEBUG4_PRINT("mcpTimer.. ");
		mcp->startTimer();
		delay(50);
		DEBUG4_PRINTLN("armed");
	}


	DEBUG4_PRINT("wifiTimer.. ");
	timerWIFI.initializeMs(AppSettings.shift_wifi, setCheckWifi).startOnce();
	delay(50);
	DEBUG4_PRINTLN("armed");

	DEBUG4_PRINT("MQTT.state=");
	DEBUG4_PRINTLN(mqtt->getConnectionState());
	DEBUG4_PRINTLN("Client name =\"" + mqtt->getName() + "\"");

	//startMqttClient();
	publishSwitches();

	if (serialConnector) {

		DEBUG4_PRINT("serialCollectorTimer.. ");
		serialConnector->startSerialCollector();	//timerSerialCollector.initializeMs(AppSettings.shift_collector, setSerialCollector).startOnce();
		delay(50);
		DEBUG4_PRINTLN("armed");

		DEBUG4_PRINT("serialReceiverTimer.. ");
		serialConnector->startSerialReceiver();	//timerSerialReceiver.initializeMs(AppSettings.shift_receiver, setSerialReceiver).startOnce();
		delay(50);
		DEBUG4_PRINTLN("armed");


		DEBUG4_PRINT("serialListenerTimer.. ");

		serialConnector->startListener();
		DEBUG4_PRINTLN("armed");

		DEBUG4_PRINTLN("Send sw init to Serial");
		uint8_t sw = ActStates.getSsw();
		DEBUG4_PRINT("Sw from ActStates = ");
		DEBUG4_PRINTLN(sw);
		serialConnector->sendSerialMessage(SerialCommand::SET_SW, ObjectType::SWITCH, ObjectId::ALL, sw);
		serialConnector->publishSerialSwitches();

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

	for (byte i = 0; i < AppSettings.sw_cnt; i++) {
		if (ActStates.getSw(i))
			mqtt->publish(AppSettings.topSW, (i+1), OUT, "ON");	//mqtt.publish(topSw_Out + String(i+1), "ON");
		else
			mqtt->publish(AppSettings.topSW, (i+1), OUT, "OFF");	//mqtt.publish(topSw_Out + String(i+1), "OFF");
	}

	for (byte i = 0; i < AppSettings.sw_cnt; i++) {
		DEBUG4_PRINTF("swState%d is ",i);
		DEBUG4_PRINTLN(ActStates.getSw(i));
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

	if (!isList)
		WifiStation.startScan(listNetworks);


	// If AP is enabled:
	DEBUG4_PRINTF("AP. ip: %s mac: %s", WifiAccessPoint.getIP().toString().c_str(), WifiAccessPoint.getMAC().c_str());
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
		rboot_set_current_rom(slot);
		System.restart();
	} else {
		// fail
		DEBUG4_PRINTLN("Firmware update failed!");
	}
}

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

/*
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

		if (AppSettings.is_serial) {
			serialConnector = new SerialConnector(&Serial, *mqtt);
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

		initSw();
		PRINT_MEM();
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
