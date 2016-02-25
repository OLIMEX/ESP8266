#ifndef __USER_CONFIG_H__
	#define __USER_CONFIG_H__
	
	#define USE_OPTIMIZE_PRINTF
	
	#define CONNECTIONS_DEBUG                  1
	#define WIFI_DEBUG_ENABLE                  0
	
	#define SSL_ENABLE                       SSL
	#if SSL_ENABLE
		#define SSL_KEY_SIZE                1024
	#endif
	
/***********************************************
 * Devices
 ***********************************************/
	#define EVB_ONLY                           1
	#define PLUG                               2
	#define SWITCH1                            3
	#define SWITCH2                            4
	#define BADGE                              5
	#define DIMMER                             6
	
/***********************************************
 * Modules conditional compiling
 ***********************************************/
	#if DEVICE == PLUG
		#define UART0_SWAP                     0
		#define UART1_ENABLE                   1
		
		// ESP8266-EVB native
		#define BUTTON_ENABLE                  1
		#define RELAY_ENABLE                   1
		#define ADC_ENABLE                     0
		// ESP8266-EVB-BAT native
		#define BATTERY_ENABLE                 0
		
		// UEXT modules
		// UART
		#define MOD_RFID_ENABLE                0
		#define MOD_FINGER_ENABLE              0
		#define MOD_EMTR_ENABLE                1
		// I2C
		#define MOD_RGB_ENABLE                 0
		#define MOD_TC_MK2_ENABLE              0
		#define MOD_IO2_ENABLE                 0
		#define MOD_IRDA_ENABLE                0
		// SPI
		#define MOD_LED_8x8_RGB_ENABLE         0
	#elif DEVICE == SWITCH1
		#define UART0_SWAP                     0
		#define UART1_ENABLE                   1
		
		// ESP8266-EVB native
		#define BUTTON_ENABLE                  0
		#define RELAY_ENABLE                   0
		#define ADC_ENABLE                     0
		// ESP8266-EVB-BAT native
		#define BATTERY_ENABLE                 0
		
		// UEXT modules
		// UART
		#define MOD_RFID_ENABLE                0
		#define MOD_FINGER_ENABLE              0
		#define MOD_EMTR_ENABLE                1
		// I2C
		#define MOD_RGB_ENABLE                 0
		#define MOD_TC_MK2_ENABLE              0
		#define MOD_IO2_ENABLE                 0
		#define MOD_IRDA_ENABLE                0
		// SPI
		#define MOD_LED_8x8_RGB_ENABLE         0
	#elif DEVICE == SWITCH2
		#define UART0_SWAP                     0
		#define UART1_ENABLE                   1
		
		// ESP8266-EVB native
		#define BUTTON_ENABLE                  1
		#define RELAY_ENABLE                   0
		#define ADC_ENABLE                     0
		// ESP8266-EVB-BAT native
		#define BATTERY_ENABLE                 0
		
		// UEXT modules
		// UART
		#define MOD_RFID_ENABLE                0
		#define MOD_FINGER_ENABLE              0
		#define MOD_EMTR_ENABLE                1
		// I2C
		#define MOD_RGB_ENABLE                 0
		#define MOD_TC_MK2_ENABLE              0
		#define MOD_IO2_ENABLE                 0
		#define MOD_IRDA_ENABLE                0
		// SPI
		#define MOD_LED_8x8_RGB_ENABLE         0
	#elif DEVICE == BADGE
		#define UART0_SWAP                     0
		#define UART1_ENABLE                   0
		
		// ESP8266-EVB native
		#define BUTTON_ENABLE                  1
		#define RELAY_ENABLE                   0
		#define ADC_ENABLE                     0
		// ESP8266-EVB-BAT native
		#define BATTERY_ENABLE                 1
		
		// UEXT modules
		// UART
		#define MOD_RFID_ENABLE                0
		#define MOD_FINGER_ENABLE              0
		#define MOD_EMTR_ENABLE                0
		// I2C
		#define MOD_RGB_ENABLE                 0
		#define MOD_TC_MK2_ENABLE              0
		#define MOD_IO2_ENABLE                 0
		#define MOD_IRDA_ENABLE                0
		// SPI
		#define MOD_LED_8x8_RGB_ENABLE         0
	#elif DEVICE == DIMMER
		#define DIMMER_REV_A                   1
		
		#define UART0_SWAP                     0
		#define UART1_ENABLE                   0
		
		// ESP8266-EVB native
		#define BUTTON_ENABLE                  0
		#define RELAY_ENABLE                   0
		#define ADC_ENABLE                     0
		// ESP8266-EVB-BAT native
		#define BATTERY_ENABLE                 0
		
		// UEXT modules
		// UART
		#define MOD_RFID_ENABLE                0
		#define MOD_FINGER_ENABLE              0
		#define MOD_EMTR_ENABLE                0
		// I2C
		#define MOD_RGB_ENABLE                 0
		#define MOD_TC_MK2_ENABLE              0
		#define MOD_IO2_ENABLE                 0
		#define MOD_IRDA_ENABLE                0
		// SPI
		#define MOD_LED_8x8_RGB_ENABLE         0
	#elif DEVICE == EVB_ONLY
		#define UART0_SWAP                     0
		#define UART1_ENABLE                   0
		
		// ESP8266-EVB native
		#define BUTTON_ENABLE                  1
		#define RELAY_ENABLE                   1
		#define ADC_ENABLE                     0
		// ESP8266-EVB-BAT native
		#define BATTERY_ENABLE                 1
		
		// UEXT modules
		// UART
		#define MOD_RFID_ENABLE                0
		#define MOD_FINGER_ENABLE              0
		#define MOD_EMTR_ENABLE                0
		// I2C
		#define MOD_RGB_ENABLE                 0
		#define MOD_TC_MK2_ENABLE              0
		#define MOD_IO2_ENABLE                 0
		#define MOD_IRDA_ENABLE                0
		// SPI
		#define MOD_LED_8x8_RGB_ENABLE         0
	#else 
		#define UART0_SWAP                     0
		#define UART1_ENABLE                   0
		
		// ESP8266-EVB native
		#define BUTTON_ENABLE                  1
		#define RELAY_ENABLE                   1
		#define ADC_ENABLE                     1
		// ESP8266-EVB-BAT native
		#define BATTERY_ENABLE                 0
		
		// UEXT modules
		// UART
		#define MOD_RFID_ENABLE                1
		#define MOD_FINGER_ENABLE              1
		#define MOD_EMTR_ENABLE                1
		// I2C
		#define MOD_RGB_ENABLE                 1
		#define MOD_TC_MK2_ENABLE              1
		#define MOD_IO2_ENABLE                 1
		#define MOD_IRDA_ENABLE                1
		// SPI
		#define MOD_LED_8x8_RGB_ENABLE         1
	#endif
/***********************************************/

	#if UART1_ENABLE || (DEVICE == PLUG) || (DEVICE == SWITCH2)
		#define I2C_ENABLE                     0
	#else
		#define I2C_ENABLE                     1
	#endif
	
	#if MOD_LED_8x8_RGB_ENABLE || (DEVICE == BADGE)
		#define FONT_UTF8_ENABLE               1
	#else
		#define FONT_UTF8_ENABLE               0
	#endif
	
	#define USER_CONFIG_RECONNECT_AFTER     3000
	#define USER_CONFIG_REBOOT_AFTER        5000
	
	#define USER_CONFIG_START_SECTOR       0x100
	#define USER_CONFIG_USER_SIZE             64
	#define USER_CONFIG_EVENTS_SIZE          128
	#define USER_CONFIG_PATH_SIZE            256
	#define USER_CONFIG_TOKEN_SIZE            64
	#define USER_CONFIG_HOSTNAME_SIZE         32
	                                             
	#define MAX_WIFI_STORED_AP_NUMBER          5
	#define WIFI_STORED_AP_NUMBER              1
	
	#define SERVER_BA_REALM                    "ESP8266 Autentication"
	
	// URLs
	#define USER_CONFIG_URL                    "/config"
	#define USER_CONFIG__URL                   "/config/"
	#define USER_CONFIG_IOT_URL                "/config/iot"
	#define USER_CONFIG_AP_URL                 "/config/access-point"
	#define USER_CONFIG_STATION_URL            "/config/station"
	#define USER_CONFIG_FIRMWARE_URL           "/config/firmware"
	#define USER_CONFIG_SSL_URL                "/config/ssl"
	
	// Defaults
	#define USER_CONFIG_DEFAULT_USER           "olimex"
	#define USER_CONFIG_DEFAULT_PASSWD         "olimex"
	
	#if DEVICE == BADGE
		#define USER_CONFIG_DEFAULT_AP_SSID    "ESP_BADGE"
	#elif  DEVICE == PLUG
		#define USER_CONFIG_DEFAULT_AP_SSID    "ESP_PLUG"
	#elif  DEVICE == SWITCH1
		#define USER_CONFIG_DEFAULT_AP_SSID    "ESP_SWITCH1"
	#elif  DEVICE == SWITCH2
		#define USER_CONFIG_DEFAULT_AP_SSID    "ESP_SWITCH2"
	#elif  DEVICE == DIMMER
		#define USER_CONFIG_DEFAULT_AP_SSID    "ESP_DIMMER"
	#else
		#define USER_CONFIG_DEFAULT_AP_SSID    "ESP_OLIMEX"
	#endif
	
	#define USER_CONFIG_DEFAULT_AP_PASSWD      "olimex-ap"
	
	#define USER_CONFIG_DEFAULT_EVENT_SERVER   "iot.olimex.com"
	#if DEVICE == BADGE
		#define USER_CONFIG_DEFAULT_EVENT_PATH "/events"
	#else
		#define USER_CONFIG_DEFAULT_EVENT_PATH "/"
	#endif
	
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
		uint8     station_auto_connect;
		
		char      events_server[USER_CONFIG_EVENTS_SIZE];
		char      events_user[USER_CONFIG_USER_SIZE];
		char      events_password[USER_CONFIG_USER_SIZE];
		
		uint8     events_ssl;
        uint8     events_websocket;
		char      events_path[USER_CONFIG_PATH_SIZE];
		char      events_name[USER_CONFIG_USER_SIZE];
		char      events_token[USER_CONFIG_TOKEN_SIZE];
		char      station_hostname[USER_CONFIG_HOSTNAME_SIZE];
		
		char      check[7];
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
	
	bool  user_config_ssl();
	bool  user_config_authentication();
	char *user_config_user();
	char *user_config_password();
	
	bool  user_config_station_auto_connect();
	
	char *user_config_events_server();
	char *user_config_events_user();
	char *user_config_events_password();
	bool  user_config_events_ssl();
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