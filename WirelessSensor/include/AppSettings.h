/*
 * AppSettings.h
 *
 *      Modified by Nikita Litvinov on 28.12.2015
 */

#include <SmingCore/SmingCore.h>
#include <Logger.h>
#include <LED.h>

#ifndef INCLUDE_APPSETTINGS_H_
#define INCLUDE_APPSETTINGS_H_

//#define APP_SETTINGS_FILE ".settings.conf" // leading point for security reasons :)
#define APP_SETTINGS_FILE "settings.conf" // There is no leading point for security reasons :)
#define HTTP_TRY_PERIOD 5000
#define MIN_FILE_SIZE	100

struct ApplicationSettingsStorage
{
	uint32_t serial_speed = 115200;
	String version = "Unknown";

	// NETWORK
	String wifiList[9];
	uint8_t wifi_cnt = 0;
	uint8_t network_ind = 0;
	String ssid;
	String ssid_default;
	String password;
	bool dhcp = true;
	IPAddress ip;
	IPAddress netmask;
	IPAddress gateway;

	// OTA
	String rom0;
	//String rom1;
	String spiffs;

	// MQTT
	String main_topic;
	String client_topic;
	String broker_ip;
	int broker_port;

	//MQTT topic names
	String topConfig;
	String topLog;
	String topStart;
	String topVCC;


	String topDHT_t;
	String topDHT_h;

	String topBMP_t;
	String topBMP_p;

	String topDS_t;

	String topSW;
	String topSSW;

	String topMSW;
	String topMIN;
	String topMIN_L;

	String topWater_h;
	String topWater_c;


	// PINS
	byte sda;
	byte scl;
	byte dht;
	byte ds;
	//byte sw1;
	//byte sw2;

	byte* sw;
	byte sw_cnt=0;

	byte* ssw;
	byte ssw_cnt=0;

	byte* in;
	byte in_cnt=0;
	LED led;

	byte* msw;
	byte msw_cnt;
	byte* min;
	byte min_cnt;
	byte m_int;

	// HTTP
	HttpClient httpClient;
	String urlFW[2] = {"http://10.0.1.22:8080/ota/settings.conf", "http://10.4.1.59:8080/Blink/settings.conf"};
	uint8_t urlIndex = 0;
	Timer timerHttp;


	// MODULES
	bool is_wifi = false;
	bool is_dht = false;
	bool is_bmp = false;
	bool is_ds = false;
	bool is_serial = false;
	bool is_insw = false;
	bool is_mcp = false;

	// TIMERS
	unsigned long shift_mqtt = 10000;
	unsigned long shift_dht = 2000;
	unsigned long shift_bmp = 0;
	unsigned long shift_ds = 6000;
	unsigned long shift_wifi = 25000;
	unsigned long shift_collector = 3000;
	unsigned long shift_receiver = 16000;
	unsigned long shift_mcp = 12000;
	unsigned long shift_save = 500;

	unsigned long interval_mqtt = 30000;
	unsigned long interval_dht = 60000;
	unsigned long interval_bmp = 60000;
	unsigned long interval_ds = 60000;
	unsigned long interval_wifi = 300000;
	unsigned long interval_listener = 200;
	unsigned long interval_collector = 30000;
	unsigned long interval_receiver = 30000;
	unsigned long interval_mcp = 30000;

	unsigned long debounce_time = 20;
	unsigned long long_time = 500;

	ApplicationSettingsStorage() {
		//Initialization of rBoot OTA
		//Serial.begin(115200);
		rBootInit();
		load();
		loadNetwork();

	}

	byte getMInNumByPin(byte pin) {
		if ((min_cnt > 0) && (pin >=0) && (pin < 16)){
			for (byte i = 0; i < min_cnt; i++) {
				if (min[i] == pin)
					return i;
			}
		}
		return -1;
	}

	/*
	byte getMSwNumByPin(byte pin) {
		if ((msw_cnt > 0) && (pin >=0) && (pin < 16))  {
			for (byte i = 0; i < msw_cnt; i++) {
				if (msw[i] == pin)
					return i;
			}
		}
		return -1;
	}
	 */

	byte getMSwPinByNum(byte num) {
		if ((num >= 0) && (num < msw_cnt))
			return msw[num];

		return -1;
	}

	byte getMInPinByNum(byte num) {
		if ((num >= 0) && (num < min_cnt))
			return min[num];

		return -1;
	}

	void loadWifiList() {

		DynamicJsonBuffer jsonBuffer;
		if (exist()) {
			int size = fileGetSize(APP_SETTINGS_FILE);
			char* jsonString = new char[size + 1];
			fileGetContent(APP_SETTINGS_FILE, jsonString, size + 1);
			JsonObject& root = jsonBuffer.parseObject(jsonString);

			JsonObject& config = root["config"];
			JsonObject& networks = config["networks"];
			JsonObject& list = networks["list"];

			uint8_t cnt = 1;
			while(list.containsKey(String(cnt).c_str()))
				cnt++;

			this->wifi_cnt = cnt - 1;

			for (uint8_t i=0; i < this->wifi_cnt; i++) {
				this->wifiList[i] = (const char*)list[String(i+1)];
			}


			delete[] jsonString;

		}
	}

	int8_t loadNetwork(String ssid = "") {

		bool zero = false;

		if ((ssid.equals("")) || (ssid == null))
			zero = true;

		if ((!zero) && (this->ssid.equalsIgnoreCase(ssid)))  {
			return 1; //Done, is not needs to update
		}

		DynamicJsonBuffer jsonBuffer;
		if (exist()) {
			int size = fileGetSize(APP_SETTINGS_FILE);
			char* jsonString = new char[size + 1];
			fileGetContent(APP_SETTINGS_FILE, jsonString, size + 1);
			JsonObject& root = jsonBuffer.parseObject(jsonString);
			delete[] jsonString;

			JsonObject& config = root["config"];
			JsonObject& networks = config["networks"];


			if (zero) {
				JsonObject& list = networks["list"];
				JsonObject& network = networks[(const char*)list[String(network_ind)]];
				ssid = (const char*)network["ssid"];

				if (network_ind == 0) {
					if (list.containsKey("0")) {
						this->ssid = ssid;
						this->ssid_default = ssid;
					}
					else {
						return -1;
					}
				}
			}

			JsonObject& network = networks[ssid];
			this->ssid = ssid;

			password = (const char*)network["password"];
			dhcp = network["dhcp"];
			//ip = network["ip"].toString();
			//netmask = network["netmask"].toString();
			//gateway = network["gateway"].toString();

			JsonObject& mqtt = network["mqtt"];
			broker_ip = (const char*)mqtt["broker_ip"];
			broker_port = mqtt["broker_port"];

			JsonObject& ota = network["ota"];
			rom0 = (const char*)ota["rom0"];
			//rom1 = ota["rom1"].toString();
			spiffs = (const char*)ota["spiffs"];

			return 0;
		}

		return -1;
	}

	void load(char* jsonString) {
		DynamicJsonBuffer jsonBuffer;
		JsonObject& root = jsonBuffer.parseObject(jsonString);

		JsonObject& config = root["config"];
		serial_speed = config["serial_speed"];
		version = (const char*)config["version"];

		JsonObject& mqtt = config["mqtt_topic"];
		main_topic = (const char*)mqtt["main_topic"];
		client_topic = (const char*)mqtt["client_topic"];

		topConfig = (const char*)mqtt["config"];
		topLog = (const char*)mqtt["log"];
		topStart = (const char*)mqtt["start"];
		topVCC = (const char*)mqtt["vcc"];


		topDHT_t = (const char*)mqtt["dht_t"];
		topDHT_h = (const char*)mqtt["dht_h"];

		topBMP_t = (const char*)mqtt["bmp_t"];
		topBMP_p = (const char*)mqtt["bmp_p"];

		topDS_t = (const char*)mqtt["ds_t"];

		topSW = (const char*)mqtt["sw"];
		topSSW = (const char*)mqtt["ssw"];

		topMSW = (const char*)mqtt["msw"];
		topMIN = (const char*)mqtt["min"];
		topMIN_L = (const char*)mqtt["min_l"];

		topWater_h = (const char*)mqtt["water_h"];
		topWater_c = (const char*)mqtt["water_c"];

		JsonObject& pins = config["pins"];
		sda = pins["sda"];
		scl = pins["scl"];
		dht = pins["dht"];
		ds = pins["ds"];
		m_int = pins["mint"];

		if (pins.containsKey("sw")) {
			JsonObject& jSw = pins["sw"];
			this->sw_cnt = (byte)jSw["cnt"];
			sw = new byte[sw_cnt];
			for (byte i = 0; i < sw_cnt; i++ ) {
				if (jSw.containsKey(String(i+1).c_str()))
					this->sw[i] = jSw[String(i+1)];
			}
		}

		if (pins.containsKey("ssw")) {
			JsonObject& jSsw = pins["ssw"];
			this->ssw_cnt = (byte)jSsw["cnt"];
			ssw = new byte[ssw_cnt];
			for (byte i = 0; i < ssw_cnt; i++ ) {
				if (jSsw.containsKey(String(i+1).c_str()))
					this->ssw[i] = jSsw[String(i+1)];
			}
		}

		if (pins.containsKey("mcp")) {
			JsonObject& jMCP = pins["mcp"];

			if (jMCP.containsKey("sw")) {
				JsonObject& jMSW = jMCP["sw"];
				this->msw_cnt = (byte)jMSW["cnt"];
				msw = new byte[msw_cnt];
				for (byte i = 0; i < msw_cnt; i++ ) {
					if (jMSW.containsKey(String(i+1).c_str()))
						this->msw[i] = jMSW[String(i+1)];
				}
			}

			if (jMCP.containsKey("in")) {
				JsonObject& jMIN = jMCP["in"];
				this->min_cnt = (byte)jMIN["cnt"];
				min = new byte[min_cnt];
				for (byte i = 0; i < min_cnt; i++ ) {
					if (jMIN.containsKey(String(i+1).c_str()))
						this->min[i] = jMIN[String(i+1)];
				}
			}
		}

		if (pins.containsKey("led")) {
			JsonObject& jLed = pins["led"];
			this->led.setCount((byte)jLed["cnt"]);
			this->led.setPin((byte)jLed["pin"]);
		}

		if (pins.containsKey("in")) {
			JsonObject& jIn = pins["in"];
			this->in_cnt = (byte)jIn["cnt"];

			if (in_cnt > 0) {
				in = new byte[in_cnt];
				for (byte i = 0; i < in_cnt; i++ ) {
					if (jIn.containsKey(String(i+1).c_str()))
						this->in[i] = jIn[String(i+1)];
				}
			}
		}

		JsonObject& modules = config["modules"];
		is_dht = modules["is_dht"];
		is_bmp = modules["is_bmp"];
		is_ds = modules["is_ds"];
		is_wifi = modules["is_wifi"];
		is_serial = modules["is_serial"];
		is_insw = modules["is_insw"];
		is_mcp = modules["is_mcp"];

		JsonObject& timers = config["timers"];
		shift_mqtt = timers["shift_mqtt"];
		shift_dht = timers["shift_dht"];
		shift_bmp = timers["shift_bmp"];
		shift_ds = timers["shift_ds"];
		shift_wifi = timers["shift_wifi"];
		shift_collector = timers["shift_collector"];
		shift_receiver = timers["shift_receiver"];
		shift_mcp = timers["shift_mcp"];
		shift_save = timers["shift_save"];

		interval_mqtt = timers["interval_mqtt"];
		interval_dht = timers["interval_dht"];
		interval_bmp = timers["interval_bmp"];
		interval_ds = timers["interval_ds"];
		interval_wifi = timers["interval_wifi"];
		interval_listener = timers["interval_listener"];
		interval_collector = timers["interval_collector"];
		interval_receiver = timers["interval_receiver"];
		interval_mcp = timers["interval_mcp"];

		debounce_time = timers["debounce_time"];
		long_time = timers["long_time"];

		delete[] jsonString;
	}


	void load() {
		if (exist()) {

			int size = fileGetSize(APP_SETTINGS_FILE);

			char* jsonString = new char[size + 1];
			fileGetContent(APP_SETTINGS_FILE, jsonString, size + 1);

			load(jsonString);
			//delete[] jsonString;
		}
	}


	void deleteConf() {
		if (exist()) {
			fileDelete(APP_SETTINGS_FILE);
			//DEBUG1_PRINT(APP_SETTINGS_FILE );
			//DEBUG1_PRINTLN(" successful deleted!");
		}
		else {
			//DEBUG1_PRINT(APP_SETTINGS_FILE );
			//DEBUG1_PRINTLN(" wasn't found. Didn't deleted!");
		}
	}

	//timer.initializeMs(timer_shift, TimerDelegate(&MQTT::start, this)).startOnce();

	uint8_t urlIndexChange() {
		urlIndex++;
		uint8_t urlMax = sizeof(urlFW)/ sizeof(urlFW[0]);
		if (urlIndex > (urlMax - 1))
			urlIndex = 0;

		return urlIndex;

	}

	void onComplete(HttpClient& client, bool successful)
	{
		if (successful) {
			//DEBUG1_PRINTLN("HttpLoad: Success sent");
		}
		else {
			//DEBUG1_PRINTLN("HttpLoad: Failed");
			urlIndexChange();
			timerHttp.initializeMs(HTTP_TRY_PERIOD, TimerDelegate(&ApplicationSettingsStorage::downloadSettings, this)).startOnce();
		}

		String response = client.getResponseString();
		//DEBUG4_PRINTLN("Server response: '" + response + "'");
		if (response.length() > 0) {
			fileSetContent(APP_SETTINGS_FILE, response);
			//DEBUG1_PRINTLN("Settings.conf downloaded and saved!");
			int size = response.length();
			//char* jsonString = new char[size + 1];
			char* jsonString = const_cast<char*>(response.c_str()); //!!! Check this
			load(jsonString);
		}
	}

	void downloadSettings() {
		httpClient.downloadString(urlFW[urlIndex], HttpClientCompletedDelegate(&ApplicationSettingsStorage::onComplete, this));
	}

	void loadHttp() {
		if (httpClient.isProcessing()) {
			//DEBUG4_PRINTLN("We need to wait while request processing was completed");
			return; // We need to wait while request processing was completed
		}

		timerHttp.initializeMs(HTTP_TRY_PERIOD, TimerDelegate(&ApplicationSettingsStorage::downloadSettings, this)).startOnce();

	}

	void saveLastWifi() {

		DEBUG1_PRINT("*SLWF");
		PRINT_MEM();

		if (this->ssid.equalsIgnoreCase(this->ssid_default)) {
			INFO_PRINT("Settings: doesn't need to be updated. default ssid is ");
			INFO_PRINTLN(ssid_default);
			return;
		}

		DynamicJsonBuffer jsonBuffer;
		if (exist()) {
			int size = fileGetSize(APP_SETTINGS_FILE);
			char* jsonString = new char[size + 1];
			fileGetContent(APP_SETTINGS_FILE, jsonString, size + 1);
			JsonObject& root = jsonBuffer.parseObject(jsonString);

			PRINT_MEM();

			JsonObject& config = root["config"];
			JsonObject& networks = config["networks"];
			JsonObject& list = networks["list"];
			list["0"] = ssid.c_str();

			DEBUG4_PRINTLN("root.toJsonString");

			String appSettings;
			root.printTo(appSettings);
			PRINT_MEM();
			DEBUG4_PRINTLN(appSettings);
			PRINT_MEM();
			fileSetContent(APP_SETTINGS_FILE, appSettings);
			//DEBUG4_PRINTLN(root.toJsonString());
			INFO_PRINTLN("Settings: last wifi was updated, ESP8266 will restart now!");

			PRINT_MEM();
			System.restart();
		}
	}

	void save() {
		DynamicJsonBuffer jsonBuffer;
		char* jsonString;
		bool isExist = exist();
		if (isExist) {
			int size = fileGetSize(APP_SETTINGS_FILE);
			jsonString = new char[size + 1];
			fileGetContent(APP_SETTINGS_FILE, jsonString, size + 1);
		}

		JsonObject& root = (isExist?jsonBuffer.parseObject(jsonString):jsonBuffer.createObject());

		JsonObject& config = (isExist?root["config"]:jsonBuffer.createObject());
		JsonObject& networks = (isExist?config["networks"]:jsonBuffer.createObject());
		JsonObject& list = (isExist?networks["list"]:jsonBuffer.createObject());

		config["serial_speed"] = serial_speed;
		config["version"] = version.c_str();

		int8_t num = -1;
		for (int i=0; i < wifi_cnt; i++) {
			if (ssid.equalsIgnoreCase(this->wifiList[i]))
				num = i+1;
		}

		// Check: Is there same ssid in configuration
		if (num <= 0) {
			wifi_cnt++;
			list[String(wifi_cnt)] = ssid.c_str();
			JsonObject& network = jsonBuffer.createObject();
			network["ssid"] = ssid;
			network["password"] = password;
			network["dhcp"] = dhcp;
			network["ip"] = ip.toString();
			network["netmask"] = netmask.toString();
			network["gateway"] = gateway.toString();

			JsonObject& ota = jsonBuffer.createObject();
			ota["rom0"] = rom0.c_str();
			//ota["rom1"] = rom1.c_str();
			ota["spiffs"] = spiffs.c_str();

			JsonObject& mqtt = jsonBuffer.createObject();
			mqtt["broker_ip"] = broker_ip.c_str();
			mqtt["broker_port"] = broker_port;

			network["ota"] = ota;
			network["mqtt"] = mqtt;
			networks[ssid] = network;
			networks["list"] = list;

		}
		else {
			JsonObject& network = (isExist?networks[ssid]:jsonBuffer.createObject());
			network["ssid"] = ssid.c_str();
			network["password"] = password.c_str();
			network["dhcp"] = dhcp;
			//network.addCopy("ip", ip.toString());
			//network.addCopy("netmask", netmask.toString());
			//network.addCopy("gateway", gateway.toString());

			JsonObject& ota = (isExist?network["ota"]:jsonBuffer.createObject());
			ota["rom0"] = rom0.c_str();
			//ota["rom1"] = rom1.c_str();
			ota["spiffs"] = spiffs.c_str();

			JsonObject& mqtt = (isExist?network["mqtt"]:jsonBuffer.createObject());
			mqtt["broker_ip"] = broker_ip.c_str();
			mqtt["broker_port"] = broker_port;

			if (isExist) {
				network["ota"] = ota;
				network["mqtt"] = mqtt;
				networks[ssid] = network;
				networks["list"] = list;
			}
		}

		JsonObject& mqtt_topic = (isExist?config["mqtt_topic"]:jsonBuffer.createObject());
		mqtt_topic["main_topic"] = main_topic.c_str();
		mqtt_topic["client_topic"] = client_topic.c_str();

		mqtt_topic["config"] = topConfig.c_str();
		mqtt_topic["log"] = topLog.c_str();
		mqtt_topic["start"] = topStart.c_str();
		mqtt_topic["vcc"] = topVCC.c_str();

		mqtt_topic["dht_t"] = topDHT_t.c_str();
		mqtt_topic["dht_h"] = topDHT_h.c_str();

		mqtt_topic["bmp_t"] = topBMP_t.c_str();
		mqtt_topic["bmp_p"] = topBMP_p.c_str();

		mqtt_topic["ds_t"] = topDS_t.c_str();

		mqtt_topic["sw"] = topSW.c_str();
		mqtt_topic["ssw"] = topSSW.c_str();

		mqtt_topic["msw"] = topMSW.c_str();
		mqtt_topic["min"] = topMIN.c_str();
		mqtt_topic["min_l"] = topMIN_L.c_str();

		mqtt_topic["water_h"] = topWater_h.c_str();
		mqtt_topic["water_c"] = topWater_c.c_str();


		JsonObject& pins = (isExist?config["pins"]:jsonBuffer.createObject());
		pins["sda"] = sda;
		pins["scl"] = scl;
		pins["dht"] = dht;
		pins["ds"] = ds;
		pins["mint"] = m_int;

		JsonObject& jSw = (isExist?pins["sw"]:jsonBuffer.createObject());
		jSw["cnt"] = this->sw_cnt;
		for (byte i = 0; i < sw_cnt; i++)
			jSw[String(i+1)] = sw[i];

		JsonObject& jSsw = (isExist?pins["ssw"]:jsonBuffer.createObject());
		jSsw["cnt"] = this->ssw_cnt;
		for (byte i = 0; i < ssw_cnt; i++)
			jSsw[String(i+1)] = ssw[i];

		JsonObject& jIn = (isExist?pins["in"]:jsonBuffer.createObject());
		jIn["cnt"] = this->in_cnt;
		for (byte i = 0; i < in_cnt; i++)
			jIn[String(i+1)] = in[i];

		JsonObject& jLed = (isExist?pins["led"]:jsonBuffer.createObject());
		jLed["cnt"] = this->led.getCount();
		jLed["pin"] = this->led.getPin();

		JsonObject& modules = (isExist?config["modules"]:jsonBuffer.createObject());
		modules["is_dht"] = is_dht;
		modules["is_bmp"] = is_bmp;
		modules["is_ds"] = is_ds;
		modules["is_wifi"] = is_wifi;
		modules["is_serial"] = is_serial;
		modules["is_insw"] = is_insw;
		modules["is_mcp"] = is_mcp;

		JsonObject& timers = (isExist?config["timers"]:jsonBuffer.createObject());
		timers["shift_mqtt"] = shift_mqtt;
		timers["shift_dht"] = shift_dht;
		timers["shift_bmp"] = shift_bmp;
		timers["shift_ds"] = shift_ds;
		timers["shift_wifi"] = shift_wifi;
		timers["shift_collector"] = shift_collector;
		timers["shift_receiver"] = shift_receiver;
		timers["shift_mcp"] = shift_mcp;
		timers["shift_save"] = shift_save;

		timers["interval_mqtt"] = interval_mqtt;
		timers["interval_dht"] = interval_dht;
		timers["interval_bmp"] = interval_bmp;
		timers["interval_ds"] = interval_ds;
		timers["interval_wifi"] = interval_wifi;
		timers["interval_listener"] = interval_listener;
		timers["interval_collector"] = interval_collector;
		timers["interval_receiver"] = interval_receiver;
		timers["interval_mcp"] = interval_mcp;

		timers["debounce_time"] = debounce_time;
		timers["long_time"] = long_time;

		if (isExist) {
			root["config"] = config;

			pins["sw"] = jSw;
			pins["ssw"] = jSsw;
			pins["led"] = jLed;
			pins["in"] = jIn;

			config["networks"] = networks;
			config["modules"] = modules;
			config["mqtt-topic"] = mqtt_topic;
			config["pins"] = pins;
			config["timers"] = timers;



		}
		String str;
		root.printTo(str);
		fileSetContent(APP_SETTINGS_FILE, str);
		DEBUG4_PRINTLN(root.toJsonString());
		INFO_PRINTLN("Settings file was saved");

		delete[] jsonString;

	}

	/*
	void save_new() {

		DynamicJsonBuffer jsonBuffer;
		JsonObject& root = jsonBuffer.createObject();

		JsonObject& config = jsonBuffer.createObject();
		root["config"] = config;
		config["serial_speed"] = serial_speed;
		config["version"] = version.c_str();

		JsonObject& networks = jsonBuffer.createObject();

		JsonObject& network = jsonBuffer.createObject();
		network["ssid"] = ssid.c_str();
		network["password"] = password.c_str();
		//network["dhcp"] = dhcp;
		// Make copy by value for temporary string objects
		//network.addCopy("ip", ip.toString());
		//network.addCopy("netmask", netmask.toString());
		//network.addCopy("gateway", gateway.toString());

		JsonObject& list = jsonBuffer.createObject();
		list["1"] = ssid.c_str();
		list["0"] = ssid.c_str();


		JsonObject& ota = jsonBuffer.createObject();
		ota["rom0"] = rom0.c_str();
		//ota["rom1"] = rom1.c_str();
		ota["spiffs"] = spiffs.c_str();

		JsonObject& mqtt = jsonBuffer.createObject();
		mqtt["broker_ip"] = broker_ip.c_str();
		mqtt["broker_port"] = broker_port;

		network["ota"] = ota;
		network["mqtt"] = mqtt;
		networks[ssid] = network;
		networks["list"] = list;

		JsonObject& mqtt_topic = jsonBuffer.createObject();
		mqtt["main_topic"] = main_topic.c_str();
		mqtt["client_topic"] = client_topic.c_str();
		mqtt["config"] = topConfig.c_str();
		mqtt["log"] = topLog.c_str();
		mqtt["start"] = topStart.c_str();
		mqtt["vcc"] = topVCC.c_str();

		mqtt["dht_t"] = topDHT_t.c_str();
		mqtt["dht_h"] = topDHT_h.c_str();

		mqtt["bmp_t"] = topBMP_t.c_str();
		mqtt["bmp_p"] = topBMP_p.c_str();

		mqtt["ds_t"] = topDS_t.c_str();

		mqtt["sw"] = topSW.c_str();
		mqtt["ssw"] = topSSW.c_str();

		mqtt["msw"] = topMSW.c_str();
		mqtt["min"] = topMIN.c_str();
		mqtt["min_l"] = topMIN_L.c_str();

		mqtt["water_h"] = topWater_h.c_str();
		mqtt["water_c"] = topWater_c.c_str();

		JsonObject& pins = jsonBuffer.createObject();
		pins["sda"] = sda;
		pins["scl"] = scl;
		pins["dht"] = dht;
		pins["ds"] = ds;
		pins["mint"] = m_int;

		JsonObject& jSw = jsonBuffer.createObject();
		jSw["cnt"] = this->sw_cnt;
		for (byte i = 0; i < sw_cnt; i++)
			jSw[String(i+1)] = sw[i];

		JsonObject& jSsw = jsonBuffer.createObject();
		jSsw["cnt"] = this->ssw_cnt;
		for (byte i = 0; i < ssw_cnt; i++)
			jSsw[String(i+1)] = ssw[i];

		JsonObject& jIn = jsonBuffer.createObject();
		jIn["cnt"] = this->in_cnt;
		for (byte i = 0; i < in_cnt; i++)
			jIn[String(i+1)] = in[i];

		JsonObject& jLed = jsonBuffer.createObject();
		jLed["cnt"] = this->led.getCount();
		jLed["pin"] = this->led.getPin();

		pins["sw"] = jSw;
		pins["ssw"] = jSsw;
		pins["led"] = jLed;
		pins["in"] = jIn;

		JsonObject& modules = jsonBuffer.createObject();
		modules["is_dht"] = is_dht;
		modules["is_bmp"] = is_bmp;
		modules["is_ds"] = is_ds;
		modules["is_wifi"] = is_wifi;
		modules["is_serial"] = is_serial;
		modules["is_insw"] = is_insw;
		modules["is_mcp"] = is_mcp;

		JsonObject& timers = jsonBuffer.createObject();
		timers["shift_mqtt"] = shift_mqtt;
		timers["shift_dht"] = shift_dht;
		timers["shift_bmp"] = shift_bmp;
		timers["shift_ds"] = shift_ds;
		timers["shift_wifi"] = shift_wifi;
		timers["shift_collector"] = shift_collector;
		timers["shift_receiver"] = shift_receiver;
		timers["shift_mcp"] = shift_mcp;
		timers["shift_save"] = shift_save;

		timers["interval_mqtt"] = interval_mqtt;
		timers["interval_dht"] = interval_dht;
		timers["interval_bmp"] = interval_bmp;
		timers["interval_ds"] = interval_ds;
		timers["interval_wifi"] = interval_wifi;
		timers["interval_listener"] = interval_listener;
		timers["interval_collector"] = interval_collector;
		timers["interval_receiver"] = interval_receiver;
		timers["interval_mcp"] = interval_mcp;

		timers["debounce_time"] = debounce_time;
		timers["long_time"] = long_time;

		config["networks"] = networks;
		config["modules"] = modules;
		config["mqtt-topic"] = mqtt_topic;
		config["pins"] = pins;
		config["timers"] = timers;

		fileSetContent(APP_SETTINGS_FILE, root.toJsonString());
		DEBUG4_PRINTLN(root.toJsonString());
		DEBUG4_PRINTLN("Settings file was saved");
	}
	 */

	bool exist() {
		if (fileExist(APP_SETTINGS_FILE))
			return (fileGetSize(APP_SETTINGS_FILE)>100);

		return false;
	}

	void rBootInit() {
		// mount spiffs
		int slot = rboot_get_current_rom();
#ifndef DISABLE_SPIFFS
		if (slot == 0) {
#ifdef RBOOT_SPIFFS_0
			spiffs_mount_manual(RBOOT_SPIFFS_0 + 0x40200000, SPIFF_SIZE);
#else
			spiffs_mount_manual(0x40300000, SPIFF_SIZE);
#endif
		} else {
#ifdef RBOOT_SPIFFS_1
			spiffs_mount_manual(RBOOT_SPIFFS_1 + 0x40200000, SPIFF_SIZE);
#else
			spiffs_mount_manual(0x40500000, SPIFF_SIZE);
#endif
		}
#else
		//DEBUG4_PRINTLN("spiffs disabled");
#endif

		//DEBUG4_PRINTF("\r\nCurrently running rom %d.\r\n", slot);
		//DEBUG4_PRINTLN();
	}

	/*
	 * WRONG. Before use NEEDS to be changed to new format (several wifi networks) application settings file

	String update(JsonObject& root) {

		String result;

		if (root.containsKey("config")) {
			JsonObject& config = root["config"];
			if (config.containsKey("serial_speed")) {
				this->serial_speed = config["serial_speed"];
				result += "serial_speed, ";
			}
			if (config.containsKey("version")) {
				this->version = config["version"].toString();
				result += "version, ";
			}

			if (config.containsKey("network")) {
				JsonObject& network = config["network"];
				if (network.containsKey("ssid")) {
					this->ssid = network["ssid"].toString();
					result += "ssid, ";
				}
				if (network.containsKey("password")) {
					this->password = network["password"].toString();
					result += "password, ";
				}
				if (network.containsKey("dhcp")) {
					this->dhcp = network["dhcp"];
					result += "dhcp, ";
				}
				if (network.containsKey("ip")) {
					this->ip = network["ip"].toString();
					result += "ip, ";
				}
				if (network.containsKey("netmask")) {
					this->netmask = network["netmask"].toString();
					result += "netmask, ";
				}
				if (network.containsKey("gateway")) {
					this->gateway = network["gateway"].toString();
					result += "gateway, ";
				}
			}

			if (config.containsKey("ota")) {
				JsonObject& ota = config["ota"];
				if (ota.containsKey("rom0")) {
					this->rom0 = ota["rom0"].toString();
					result += "rom0, ";
				}
				if (ota.containsKey("rom1")) {
					this->rom1 = ota["rom1"].toString();
					result += "rom1, ";
				}
				if (ota.containsKey("spiffs")) {
					this->spiffs = ota["spiffs"].toString();
					result += "spiffs, ";
				}
			}

			if (config.containsKey("mqtt")) {
				JsonObject& mqtt = config["mqtt"];
				if (mqtt.containsKey("main_topic")) {
					this->main_topic = mqtt["main_topic"].toString();
					result += "main_topic, ";
				}
				if (mqtt.containsKey("client_topic")) {
					this->client_topic = mqtt["client_topic"].toString();
					result += "client_topic, ";
				}
				if (mqtt.containsKey("broker_ip")) {
					this->broker_ip = mqtt["broker_ip"].toString();
					result += "broker_ip, ";
				}
				if (mqtt.containsKey("broker_port")) {
					this->broker_port = mqtt["broker_port"];
					result += "broker_port, ";
				}
			}

			if (config.containsKey("pins")) {
				JsonObject& pins = config["pins"];
				if (pins.containsKey("sda")) {
					this->sda = pins["sda"];
					result += "sda, ";
				}
				if (pins.containsKey("scl")) {
					this->scl = pins["scl"];
					result += "scl, ";
				}
				if (pins.containsKey("dht")) {
					this->dht = pins["dht"];
					result += "dht, ";
				}
				if (pins.containsKey("ds")) {
					this->ds = pins["ds"];
					result += "ds, ";
				}
				if (pins.containsKey("sw")) {
					JsonObject& jSw = pins["sw"];
					this->sw_cnt = jSw["cnt"];
					for (byte i = 0; i < sw_cnt; i++) {
						if (jSw.containsKey(String(i+1).c_str()))
							sw[i] = jSw[String(i+1)];
					}
					result += "sw, ";
				}
				if (pins.containsKey("ssw")) {
					JsonObject& jSsw = pins["ssw"];
					this->ssw_cnt = jSsw["cnt"];
					for (byte i = 0; i < ssw_cnt; i++) {
						if (jSsw.containsKey(String(i+1).c_str()))
							ssw[i] = jSsw[String(i+1)];
					}
					result += "ssw, ";
				}
				if (pins.containsKey("in")) {
					JsonObject& jIn = pins["in"];
					this->in_cnt = jIn["cnt"];
					for (byte i = 0; i < in_cnt; i++) {
						if (jIn.containsKey(String(i+1).c_str()))
							in[i] = jIn[String(i+1)];
					}
					result += "in, ";
				}
				if (pins.containsKey("led")) {
					JsonObject& jLed = pins["led"];
					this->led.setCount((byte)jLed["cnt"]);
					this->led.setPin((byte)jLed["pin"]);
					result += "led, ";
				}
			}

			if (config.containsKey("modules")) {
				JsonObject& modules = config["modules"];
				if (modules.containsKey("is_dht")) {
					this->is_dht = modules["is_dht"];
					result += "is_dht, ";
				}
				if (modules.containsKey("is_bmp")) {
					this->is_bmp = modules["is_bmp"];
					result += "is_bmp, ";
				}
				if (modules.containsKey("is_ds")) {
					this->is_ds = modules["is_ds"];
					result += "is_ds, ";
				}
				if (modules.containsKey("is_wifi")) {
					this->is_wifi = modules["is_wifi"];
					result += "is_wifi, ";
				}
				if (modules.containsKey("is_serial")) {
					this->is_serial = modules["is_serial"];
					result += "is_serial, ";
				}
				if (modules.containsKey("is_insw")) {
					this->is_insw = modules["is_insw"];
					result += "is_insw, ";
				}
				if (modules.containsKey("is_mcp")) {
					this->is_mcp = modules["is_mcp"];
					result += "is_mcp, ";
				}
			}

			if (config.containsKey("timers")) {
				JsonObject& timers = config["timers"];
				if (timers.containsKey("shift_mqtt")) {
					this->shift_mqtt = timers["shift_mqtt"];
					result += "shift_mqtt, ";
				}
				if (timers.containsKey("shift_dht")) {
					this->shift_dht = timers["shift_dht"];
					result += "shift_dht, ";
				}
				if (timers.containsKey("shift_bmp")) {
					this->shift_bmp = timers["shift_bmp"];
					result += "shift_bmp, ";
				}
				if (timers.containsKey("shift_ds")) {
					this->shift_ds = timers["shift_ds"];
					result += "shift_ds, ";
				}
				if (timers.containsKey("shift_wifi")) {
					this->shift_wifi = timers["shift_wifi"];
					result += "shift_wifi, ";
				}
				if (timers.containsKey("shift_collector")) {
					this->shift_collector = timers["shift_collector"];
					result += "shift_collector, ";
				}
				if (timers.containsKey("shift_receiver")) {
					this->shift_receiver = timers["shift_receiver"];
					result += "shift_receiver, ";
				}

				if (timers.containsKey("interval_mqtt")) {
					this->interval_mqtt = timers["interval_mqtt"];
					result += "interval_mqtt, ";
				}
				if (timers.containsKey("interval_dht")) {
					this->interval_dht = timers["interval_dht"];
					result += "interval_dht, ";
				}
				if (timers.containsKey("interval_bmp")) {
					this->interval_bmp = timers["interval_bmp"];
					result += "interval_bmp, ";
				}
				if (timers.containsKey("interval_ds")) {
					this->interval_ds = timers["interval_ds"];
					result += "interval_ds, ";
				}
				if (timers.containsKey("interval_wifi")) {
					this->interval_wifi = timers["interval_wifi"];
					result += "interval_wifi, ";
				}
				if (timers.containsKey("interval_listener")) {
					this->interval_listener = timers["interval_listener"];
					result += "interval_listener, ";
				}
				if (timers.containsKey("interval_collector")) {
					this->interval_collector = timers["interval_collector"];
					result += "interval_collector, ";
				}
				if (timers.containsKey("interval_receiver")) {
					this->interval_receiver = timers["interval_receiver"];
					result += "interval_receiver, ";
				}
				if (timers.containsKey("debounce_time")) {
					this->debounce_time = timers["debounce_time"];
					result += "debounce_time, ";
				}
				if (timers.containsKey("long_time")) {
					this->long_time = timers["long_time"];
					result += "long_time, ";
				}
			}

		}

		int len = result.length();
		if (len > 2) {
			result = "These params were updated: " + result.substring(0, len-2);
		}
		else
			result = "Nothing updated";

		return result;
	}


	void updateNsave(JsonObject& root) {
		this->update(root);
		this->save();
	}
	 */

	bool check() {
		//TODO: need to be coded check for mandatory fields
		return true;
	}

};

static ApplicationSettingsStorage AppSettings;

#endif /* INCLUDE_APPSETTINGS_H_ */
