#ifndef __USER_CONFIG_H__
	#define __USER_CONFIG_H__

	#define USE_OPTIMIZE_PRINTF
	
	#define SSL_ENABLE                        0
	#if SSL_ENABLE
		#define SSL_KEY_SIZE               1024
	#endif
	
	#define USER_CONFIG_RECONNECT_AFTER    3000
	#define USER_CONFIG_REBOOT_AFTER       5000
	
	#define USER_CONFIG_START_SECTOR       0x100
	#define USER_CONFIG_USER_SIZE          64
	#define USER_CONFIG_EVENTS_SIZE        128
	#define USER_CONFIG_PATH_SIZE          256
	#define USER_CONFIG_TOKEN_SIZE         64
	
	#define MAX_WIFI_STORED_AP_NUMBER      5
	#define WIFI_STORED_AP_NUMBER          1
	
	#define SERVER_BA_REALM                "ESP8266 Autentication"
	
	// Defaults
	#define USER_CONFIG_DEFAULT_USER       "olimex"
	#define USER_CONFIG_DEFAULT_PASSWD     "olimex"
	
	#define USER_CONFIG_DEFAULT_AP_SSID    "ESP_OLIMEX"
	#define USER_CONFIG_DEFAULT_AP_PASSWD  "olimex-ap"
	
	#include "ip_addr.h"
	#include "user_webserver.h"
	
	typedef struct {
		uint8          dhcp;
		struct ip_info ip;
	} ip_config;
	
	typedef struct {
		uint8     ssl;
		
		uint8     authentication;
		char      user[USER_CONFIG_USER_SIZE];
		char      password[USER_CONFIG_USER_SIZE];
		
		ip_config access_point;
		ip_config station;
		
		char      events_server[USER_CONFIG_EVENTS_SIZE];
		char      events_user[USER_CONFIG_USER_SIZE];
		char      events_password[USER_CONFIG_USER_SIZE];
		
		uint8     events_ssl;
        uint8     events_websocket;
		char      events_path[USER_CONFIG_PATH_SIZE];
		char      events_name[USER_CONFIG_USER_SIZE];
		char      events_token[USER_CONFIG_TOKEN_SIZE];
		
		char      check[4];
	} user_config;
	
	typedef void (*wifi_station_connected_callback)(void);
	
	void user_config_init();
	void user_config_load();
	bool user_config_store();

#if SSL_ENABLE	
	void user_config_load_private_key();
	void user_config_load_certificate();
#endif
	
	char *config_firmware_bin();
	
	bool user_config_ssl();
	bool user_config_authentication();
	char *user_config_user();
	char *user_config_password();
	
	char *user_config_events_server();
	char *user_config_events_user();
	char *user_config_events_password();
	bool user_config_events_ssl();
	char *user_config_events_path();
	char *user_config_events_name();
	char *user_config_events_token();
	
	void config_handler(
		struct espconn *pConnection, 
		request_method method, 
		char *url, 
		char *data, 
		uint16 data_len, 
		uint32 content_len, 
		char *response,
		uint16 response_len
	);
	
	void config_iot_handler(
		struct espconn *pConnection, 
		request_method method, 
		char *url, 
		char *data, 
		uint16 data_len, 
		uint32 content_len, 
		char *response,
		uint16 response_len
	);
	
	void config_ap_handler(
		struct espconn *pConnection, 
		request_method method, 
		char *url, 
		char *data, 
		uint16 data_len, 
		uint32 content_len, 
		char *response,
		uint16 response_len
	);
	
	void config_station_handler(
		struct espconn *pConnection, 
		request_method method, 
		char *url, 
		char *data, 
		uint16 data_len, 
		uint32 content_len, 
		char *response,
		uint16 response_len
	);

	void config_ssl_handler(
		struct espconn *pConnection, 
		request_method method, 
		char *url, 
		char *data, 
		uint16 data_len, 
		uint32 content_len, 
		char *response,
		uint16 response_len
	);
	
	void config_firmware_handler(
		struct espconn *pConnection, 
		request_method method, 
		char *url, 
		char *data, 
		uint16 data_len, 
		uint32 content_len, 
		char *response,
		uint16 response_len
	);
	
#endif