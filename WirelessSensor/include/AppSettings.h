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

class AppSettings {

private:

	/* Singleton part
	// Конструкторы и оператор присваивания недоступны клиентам
	AppSettings();
	~AppSettings() {}
	AppSettings(AppSettings const&) = delete;
	AppSettings& operator= (AppSettings const&) = delete;
	*/

public:

	/* Singleton part
	static AppSettings& getInstance() {
		static AppSettings singleton;
		return singleton;
	}
	 */
	AppSettings();
	~AppSettings() {}

	uint32_t serial_speed = 115200;
	String version = "Unknown";
	byte init_cnt = 0;

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

	String topMHZ;

	String topSW;
	String topIN = "in";
	String topIN_L = "in_l";

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
	LED* led;

	byte* msw;
	byte msw_cnt=0;
	byte* min;
	byte min_cnt;
	byte m_int;

	// HTTP
	HttpClient httpClient;
	String urlFW[3] = {"http://10.0.1.22:8088/OTA/chldr/settings.conf", "http://nlpi.azurewebsites.net/OTA/chldr/settings.bin", "http://10.4.1.59:8080/chldr/settings.conf"};
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
	bool is_mhz = false;

	// TIMERS
	unsigned long shift_mqtt = 10000;
	unsigned long shift_dht = 2000;
	unsigned long shift_bmp = 0;
	unsigned long shift_ds = 6000;
	unsigned long shift_wifi = 25000;
	unsigned long shift_collector = 3000;
	unsigned long shift_receiver = 16000;
	unsigned long shift_mcp = 12000;
	unsigned long shift_mhz = 8000;
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
	unsigned long interval_mhz = 30000;

	unsigned long debounce_time = 20;
	unsigned long long_time = 500;

	byte getMInNumByPin(byte pin);
	//byte getMSwNumByPin(byte pin);
	byte getMSwPinByNum(byte num);
	byte getMInPinByNum(byte num);

	void loadWifiList();
	int8_t loadNetwork(String ssid = "");
	void load(char* jsonString);
	void load();

	void deleteConf();

	uint8_t urlIndexChange();
	void onComplete(HttpClient& client, bool successful);

	void downloadSettings();
	void loadHttp();

	void saveLastWifi();
	void save();
	//void save_new();
	bool exist();
	void rBootInit();
	//String update(JsonObject& root)
	bool check();

};


#endif /* INCLUDE_APPSETTINGS_H_ */
