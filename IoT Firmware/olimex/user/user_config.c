#include "ets_sys.h"
#include "stdout.h"
#include "osapi.h"
#include "mem.h"
#include "ip_addr.h"
#include "spi_flash.h"
#include "upgrade.h"

#include "json/jsonparse.h"

#include "user_json.h"
#include "user_interface.h"
#include "user_webserver.h"
#include "user_misc.h"

#include "user_flash.h"
#include "user_config.h"
#include "user_events.h"
#include "user_devices.h"
#include "user_wifi_scan.h"

LOCAL user_config user_configuration;

LOCAL wifi_station_connected_callback station_connected = NULL;

void ICACHE_FLASH_ATTR user_config_init() {
	flash_region_register("boot.bin",     0x000, 0x001);
	flash_region_register("user1.bin",    0x001, 0x07B);
	flash_region_register("user2.bin",    0x081, 0x07B);
	flash_region_register("user-config",  0x100, 0x001);
	flash_region_register("PrivateKey",   0x101, 0x001);
	flash_region_register("Certificate",  0x102, 0x001);
	
	webserver_register_handler_callback(USER_CONFIG_URL,           config_handler);
	webserver_register_handler_callback(USER_CONFIG__URL,          config_handler);
	webserver_register_handler_callback(USER_CONFIG_IOT_URL,       config_iot_handler);
	webserver_register_handler_callback(USER_CONFIG_AP_URL,        config_ap_handler);
	webserver_register_handler_callback(USER_CONFIG_STATION_URL,   config_station_handler);
	webserver_register_handler_callback(USER_CONFIG_FIRMWARE_URL,  config_firmware_handler);
	webserver_register_handler_callback(USER_CONFIG_SSL_URL,       config_ssl_handler);
}

LOCAL void ICACHE_FLASH_ATTR user_config_station_disconnect() {
	setTimeout((os_timer_func_t *)wifi_station_disconnect, NULL, 1000);
}

LOCAL void ICACHE_FLASH_ATTR user_config_station_connect() {
	uint8 status = wifi_station_get_connect_status();
	
	if (status == STATION_GOT_IP) {
		user_event_reconnect();
		user_config_station_disconnect();
	}
	
	if (status != STATION_CONNECTING) {
		setTimeout((os_timer_func_t *)wifi_station_connect, NULL, 2000);
		setTimeout((os_timer_func_t *)wifi_auto_detect,  NULL, 3000);
	}
}

LOCAL void ICACHE_FLASH_ATTR config_default_ssid(char *ssid) {
	uint8 mac[6];
	wifi_get_macaddr(SOFTAP_IF, mac);
	os_sprintf(
		ssid, 
		USER_CONFIG_DEFAULT_AP_SSID
		"_%02X%02X%02X", 
		mac[3], mac[4], mac[5]
	);
}

LOCAL void ICACHE_FLASH_ATTR config_default_token(char *token) {
	uint8 mac[6];
	wifi_get_macaddr(SOFTAP_IF, mac);
	os_sprintf(
		token, 
		"%02X%02X%02X%02X%02X%02X", 
		mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]
	);
}

void ICACHE_FLASH_ATTR user_config_restore_defaults() {
	user_configuration.ssl = false;
	
	user_configuration.authentication = true;
	os_sprintf(user_configuration.user,     USER_CONFIG_DEFAULT_USER);
	os_sprintf(user_configuration.password, USER_CONFIG_DEFAULT_PASSWD);
	
	os_memset(user_configuration.events_server,   '\0', sizeof(user_configuration.events_server));
	os_memset(user_configuration.events_user,     '\0', sizeof(user_configuration.events_user));
	os_memset(user_configuration.events_password, '\0', sizeof(user_configuration.events_password));
	os_memset(user_configuration.events_path,     '\0', sizeof(user_configuration.events_path));
	os_memset(user_configuration.events_name,     '\0', sizeof(user_configuration.events_name));
	os_memset(user_configuration.events_token,    '\0', sizeof(user_configuration.events_token));
	
	user_configuration.events_websocket = true;
	user_configuration.events_ssl = false;
	
#if DEVICE == BADGE	
	os_sprintf(user_configuration.events_server,   USER_CONFIG_DEFAULT_EVENT_SERVER);
#endif
	os_sprintf(user_configuration.events_path,     USER_CONFIG_DEFAULT_EVENT_PATH);
	os_sprintf(user_configuration.events_name,     USER_CONFIG_DEFAULT_AP_SSID);
	config_default_token(user_configuration.events_token);
	
	user_configuration.access_point.dhcp = 1;
	IP4_ADDR(&user_configuration.access_point.ip.ip,      192,168,  4,1);
	IP4_ADDR(&user_configuration.access_point.ip.netmask, 255,255,255,0);
	IP4_ADDR(&user_configuration.access_point.ip.gw,      192,168,  4,1);

#if DEVICE == BADGE	
	user_configuration.station_auto_connect = 1;
#else
	user_configuration.station_auto_connect = 0;
#endif
	user_configuration.station.dhcp = 1;
	IP4_ADDR(&user_configuration.station.ip.ip,           192,168, 10,2);
	IP4_ADDR(&user_configuration.station.ip.netmask,      255,255,255,0);
	IP4_ADDR(&user_configuration.station.ip.gw,           192,168, 10,1);
	
	os_sprintf(user_configuration.station_hostname,     USER_CONFIG_DEFAULT_AP_SSID);
	
	os_sprintf(user_configuration.check, "ESP");
	user_config_store(NULL);
	
	wifi_set_opmode(STATIONAP_MODE);
	
	struct softap_config ap_config = {
		.ssid = USER_CONFIG_DEFAULT_AP_SSID,
		.password = USER_CONFIG_DEFAULT_AP_PASSWD,
		.ssid_len = 0,
		.channel = 0,
		.authmode = AUTH_WPA2_PSK,
		.ssid_hidden = 0,
		.max_connection = 2,
		.beacon_interval = 100
	};
	config_default_ssid(ap_config.ssid);
	
	wifi_softap_set_config(&ap_config);

	struct station_config s_config = {
		.ssid = "",
		.password = "",
		.bssid_set = 0,
		.bssid = ""
	};
	wifi_station_set_config(&s_config);
	wifi_station_set_hostname(user_configuration.station_hostname);
	
	wifi_station_set_auto_connect(user_configuration.station_auto_connect);
	wifi_station_set_reconnect_policy(user_configuration.station_auto_connect);
	if (wifi_station_get_connect_status() != STATION_IDLE) {
		user_config_station_disconnect();
	}
	
	debug("CONFIG: %s settings\n", RESTORED_DEFAULTS);
	
	char status[WEBSERVER_MAX_VALUE];
	user_event_raise(NULL, json_status(status, ESP8266, RESTORED_DEFAULTS, NULL));
}

void ICACHE_FLASH_ATTR user_config_load() {
	wifi_station_ap_number_set(WIFI_STORED_AP_NUMBER);
	
	debug("CONFIG: Loading...\n");
	debug("CONFIG: Size of user_config %d\n", sizeof(user_config));
	SpiFlashOpResult result;
	
	result = spi_flash_read(
		USER_CONFIG_START_SECTOR * SPI_FLASH_SEC_SIZE,
		(uint32 *)&user_configuration, 
		sizeof(user_config)
	);
	
	if (result == SPI_FLASH_RESULT_OK && os_strcmp(user_configuration.check, "ESP") == 0) {
		debug("CONFIG: Load success\n\n");
	} else {
		debug("CONFIG: Can not load user_config. Restoring defaults\n");
		user_config_restore_defaults();
	}
	
	wifi_set_ip_info(SOFTAP_IF, &user_configuration.access_point.ip);
	if (user_configuration.access_point.dhcp == 0) {
		wifi_softap_dhcps_stop();
	} else {
		if (wifi_softap_dhcps_status() != DHCP_STARTED) {
			wifi_softap_dhcps_start();
		}
	}
	
	wifi_station_set_hostname(user_configuration.station_hostname);
	wifi_station_set_auto_connect(user_configuration.station_auto_connect);
	wifi_station_set_reconnect_policy(user_configuration.station_auto_connect);
	
	if (user_configuration.station.dhcp == 0) {
		wifi_station_dhcpc_stop();
		wifi_set_ip_info(STATION_IF, &user_configuration.station.ip);
	} else {
		if (wifi_station_dhcpc_status() != DHCP_STARTED) {
			wifi_station_dhcpc_start();
		}
	}
	
	if (user_configuration.station_auto_connect) {
		uint8 status = wifi_station_get_connect_status();
		uint16 reconnect_after = 300;
		
		if (status == STATION_GOT_IP) {
			reconnect_after = USER_CONFIG_RECONNECT_AFTER;
		}
		
		setTimeout(user_config_station_connect, NULL, reconnect_after);
	} else {
		if (wifi_station_get_connect_status() != STATION_IDLE) {
			user_config_station_disconnect();
		}
	}
}

bool ICACHE_FLASH_ATTR user_config_store(char *error) {
LOCAL const char NOT_ALIGNED[] = "Size of user_config must be aligned to 4 bytes";
LOCAL const char ERASE_ERR[]   = "Can not erase user_config sector";
LOCAL const char WRITE_ERR[]   = "Can not write user_config sector";

	debug("CONFIG: Storing...\n");
	debug("CONFIG: Size of user_config %d\n", sizeof(user_config));
	if (sizeof(user_config) % 4 != 0) {
		debug("CONFIG: %s\n", NOT_ALIGNED);
		if (error != NULL) {
			os_sprintf(error, NOT_ALIGNED);
		}
		return false;
	}
	
	if (spi_flash_erase_sector(USER_CONFIG_START_SECTOR) != SPI_FLASH_RESULT_OK) {
		debug("CONFIG: %s\n", ERASE_ERR);
		if (error != NULL) {
			os_sprintf(error, ERASE_ERR);
		}
		return false;
	}
	
	SpiFlashOpResult result;
	result = spi_flash_write(
		USER_CONFIG_START_SECTOR * SPI_FLASH_SEC_SIZE,
		(uint32 *)&user_configuration, 
		sizeof(user_config)
	);
	
	if (result != SPI_FLASH_RESULT_OK) {
		debug("CONFIG: %s\n", WRITE_ERR);
		if (error != NULL) {
			os_sprintf(error, WRITE_ERR);
		}
	} else {
		debug("CONFIG: Stored\n\n");
	}
	
	return (result == SPI_FLASH_RESULT_OK);
}

bool ICACHE_FLASH_ATTR user_config_ssl() {
	return (bool)user_configuration.ssl;
}

bool ICACHE_FLASH_ATTR user_config_authentication() {
	return (bool)user_configuration.authentication;
}

char ICACHE_FLASH_ATTR *user_config_user() {
	return (char *)&user_configuration.user;
}

char ICACHE_FLASH_ATTR *user_config_password() {
	return (char *)&user_configuration.password;
}

bool ICACHE_FLASH_ATTR user_config_station_auto_connect() {
	return (bool)user_configuration.station_auto_connect;
}


char ICACHE_FLASH_ATTR *user_config_events_server() {
	return (char *)&user_configuration.events_server;
}

char ICACHE_FLASH_ATTR *user_config_events_user() {
	return (char *)&user_configuration.events_user;
}

char ICACHE_FLASH_ATTR *user_config_events_password() {
	return (char *)&user_configuration.events_password;
}

bool ICACHE_FLASH_ATTR user_config_events_ssl() {
	return (bool)user_configuration.events_ssl;
}

bool ICACHE_FLASH_ATTR user_config_events_websocket() {
	return (bool)user_configuration.events_websocket;
}

char ICACHE_FLASH_ATTR *user_config_events_path() {
	return (char *)&user_configuration.events_path;
}

char ICACHE_FLASH_ATTR *user_config_events_name() {
	return (char *)&user_configuration.events_name;
}

char ICACHE_FLASH_ATTR *user_config_events_token() {
	return (char *)&user_configuration.events_token;
}

#if SSL_ENABLE
LOCAL int ICACHE_FLASH_ATTR user_config_load_ssl(char *region_name, char *match, uint8 *decoded, int decoded_len) {
	const char *delimiters = "\r\n";
	char *data = NULL;
	char *end = NULL;
	
	char buff[SPI_FLASH_SEC_SIZE];
	os_memset(buff, '\0', SPI_FLASH_SEC_SIZE);
	os_memset(decoded, 0, decoded_len);
	
	flash_region_open(region_name);
	data = (char *)flash_region_read();
	flash_region_close();
	
	if (data == NULL) {
		debug("Read failure\n");
		return 0;
	}
	
	data = (char *)strstr_end(data, "-----BEGIN");
	if (data == NULL) {
		debug("No BEGIN signature\n");
		return 0;
	}
	
	data = (char *)strstr_end(data, match);
	if (data == NULL) {
		debug("No MATCH signature\n");
		return 0;
	}
	
	end = (char *)os_strstr(data, "-----END ");
	if (end == NULL) {
		debug("No END signature\n");
		return 0;
	}
	
	uint16 len = 0;
	while (*data != '\0' && data < end) {
		if (!os_strchr(delimiters, *data)) {
			buff[len] = *data;
			len++;
		}
		data++;
	}
	
	if (decoded_len < (len * 3 / 4)) {
		debug("Output buffer size too small\n");
		return 0;
	}
	
	base64_decode(buff, len, decoded, &decoded_len);
	return decoded_len;
}

void ICACHE_FLASH_ATTR user_config_load_private_key() {
	extern unsigned char default_private_key[SSL_KEY_SIZE];
	extern unsigned int default_private_key_len;
	
	default_private_key_len = user_config_load_ssl("PrivateKey", " PRIVATE KEY-----", default_private_key, SSL_KEY_SIZE);
	espconn_secure_set_default_private_key(default_private_key, default_private_key_len);
	debug("PRIVATE KEY: Load %s [%d]\n\n", default_private_key_len > 0 ? "success" : "failure", default_private_key_len);
}

void ICACHE_FLASH_ATTR user_config_load_certificate() {
	extern unsigned char default_certificate[SSL_KEY_SIZE];
	extern unsigned int default_certificate_len;
	
	default_certificate_len = user_config_load_ssl("Certificate", " CERTIFICATE-----", default_certificate, SSL_KEY_SIZE);
	espconn_secure_set_default_certificate(default_certificate, default_certificate_len);
	debug("CERTIFICATE: Load %s [%d]\n\n", default_certificate_len > 0 ? "success" : "failure", default_certificate_len);
}
#endif

/* ============================================================================================= */

LOCAL char ICACHE_FLASH_ATTR *ip_str(ip_addr_t *ip, char *ip_str) {
	os_sprintf(ip_str, IPSTR, IP2STR(ip));
	return ip_str;
}

LOCAL char ICACHE_FLASH_ATTR *config_ip_info(uint8 interface) {
	struct ip_info ip;
	static char ip_info_str[WEBSERVER_MAX_VALUE];
	char addr[16];
	char mask[16];
	char gw[16];
	
	if (!wifi_get_ip_info(interface, &ip)) {
		ip.ip.addr      = IPADDR_ANY;
		ip.netmask.addr = IPADDR_ANY;
		ip.gw.addr      = IPADDR_ANY;
	};
	
	os_sprintf(
		ip_info_str,
		"{"
			"\"Address\" : \"%s\", "
			"\"NetMask\" : \"%s\", "
			"\"Gateway\" : \"%s\""
		"}",
		ip_str(&ip.ip, addr),
		ip_str(&ip.netmask, mask),
		ip_str(&ip.gw, gw)
	);
	
	return ip_info_str;
}

char ICACHE_FLASH_ATTR *config_wifi_ap() {
	struct softap_config config;
	static char ap_str[WEBSERVER_MAX_VALUE*2];
	os_sprintf(ap_str, "{}");
	
	if (!wifi_softap_get_config(&config)) {
		return ap_str;
	}
	
	if (config.ssid_len != 0) {
		config.ssid[config.ssid_len] = '\0';
	}
	
	os_sprintf(
		ap_str,
		"\"AccessPoint\" : {"
			"\"SSID\" : \"%s\", "
			"\"Password\" : \"%s\", "
			"\"Mode\" : \"%s\", "
			"\"Hidden\" : %d, "
			"\"MaxConnections\" : %d, "
			"\"BeaconInterval\" : %d, "
			"\"DHCP\" : %d, "
			"\"IP\" : %s"
		"}",
		config.ssid,
		config.password,
		wifi_auth_mode_str(config.authmode),
		config.ssid_hidden,
		config.max_connection,
		config.beacon_interval,
		wifi_softap_dhcps_status(),
		config_ip_info(SOFTAP_IF)
	);
	
	return ap_str;
}

LOCAL char ICACHE_FLASH_ATTR *config_wifi_stored_ap() {
	struct station_config config[MAX_WIFI_STORED_AP_NUMBER];
	uint8 count = wifi_station_get_ap_info(config);
	
	char ap[WEBSERVER_MAX_VALUE];
	static char stored_ap_str[WEBSERVER_MAX_VALUE*3];
	stored_ap_str[0] = '\0';
	
	uint8 i;
	for (i=0; i<count; i++) {
		os_sprintf(
			ap,
			"%s{\"SSID\" : \"%s\", \"Password\" : \"%s\"}",
			i == 0 ? "" : ", ",
			config[i].ssid,
			config[i].password
		);
		
		os_strcat(stored_ap_str, ap);
	}
	
	return stored_ap_str;
}

char ICACHE_FLASH_ATTR *config_wifi_station() {
	static char client_str[WEBSERVER_MAX_VALUE*3] = "";
	struct station_config config;
	
	wifi_station_get_config_default(&config);
	
	os_sprintf(
		client_str,
		"\"Station\" : {"
			"\"StoredAccessPoints\" : [%s], "
			"\"CurrentAP\" : %d, "
			"\"SSID\" : \"%s\", "
			"\"Password\" : \"%s\", "
			"\"AutoConnect\" : %d, "
			"\"DHCP\" : %d, "
			"\"IP\" : %s, "
			"\"Hostname\" : \"%s\""
		"}",
		config_wifi_stored_ap(),
		wifi_station_get_current_ap_id(),
		config.ssid,
		config.password,
		wifi_station_get_auto_connect(),
		wifi_station_dhcpc_status(),
		config_ip_info(STATION_IF),
		wifi_station_get_hostname()
	);
	
	return client_str;
}

LOCAL char ICACHE_FLASH_ATTR *config_mac(uint8 interface) {
	static char mac_str[2][20];
	uint8 mac[6];
	
	wifi_get_macaddr(interface, mac);
	os_sprintf(
		mac_str[interface], 
		"%02X:%02X:%02X:%02X:%02X:%02X", 
		mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]
	);
	
	return mac_str[interface];
}

LOCAL void config_response(char *response, char *error, char *data) {
	if (error[0] != '\0') {
		char data_buff[os_strlen(data) + 20];
		json_error(
			response, ESP8266, error,
			json_sprintf(
				data_buff,
				"\"Data\" : {%s}",
				data
			)
		);
	} else {
		json_data(response, ESP8266, OK_STR, data, NULL);
	}
}

void ICACHE_FLASH_ATTR config_handler(
	struct espconn *pConnection, 
	request_method method, 
	char *url, 
	char *data, 
	uint16 data_len, 
	uint32 content_len, 
	char *response,
	uint16 response_len
) {
	char error_str[WEBSERVER_MAX_VALUE] = "";
	
	if (method == POST && data != NULL && data_len != 0) {
		struct jsonparse_state parser;
		int json_type;
		
		jsonparse_setup(&parser, data, data_len);
		while ((json_type = jsonparse_next(&parser)) != 0) {
			if (json_type == JSON_TYPE_PAIR_NAME) {
				if (jsonparse_strcmp_value(&parser, "User") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					jsonparse_copy_value(&parser, user_configuration.user, USER_CONFIG_USER_SIZE);
				} else if (jsonparse_strcmp_value(&parser, "Password") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					jsonparse_copy_value(&parser, user_configuration.password, USER_CONFIG_USER_SIZE);
				} else if (jsonparse_strcmp_value(&parser, "Authentication") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					user_configuration.authentication = jsonparse_get_value_as_int(&parser);
				} else if (jsonparse_strcmp_value(&parser, "SSL") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					user_configuration.ssl = jsonparse_get_value_as_int(&parser);
				} else if (jsonparse_strcmp_value(&parser, "Mode") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					if (jsonparse_strcmp_value(&parser, "Access Point") == 0) {
						wifi_set_opmode(SOFTAP_MODE);
						debug("WIFI: Set SOFTAP_MODE\n");
					} else if (jsonparse_strcmp_value(&parser, "Station") == 0) {
						wifi_set_opmode(STATION_MODE);
						debug("WIFI: Set STATION_MODE\n");
					} else if (jsonparse_strcmp_value(&parser, "Access Point and Station") == 0) {
						wifi_set_opmode(STATIONAP_MODE);
						debug("WIFI: Set STATIONAP_MODE\n");
					}
				}
			}
		}
		user_config_store(error_str);
		user_config_load();
	}
	
	struct rst_info* rst;
	rst = system_get_rst_info();
	
	char data_str[WEBSERVER_MAX_RESPONSE_LEN];
	config_response(
		response,
		error_str,
		json_sprintf(
			data_str,
			"\"Config\" : {"
				"\"SDKVersion\" : \"%s\", "
				"\"ResetInfo\" : \"%d:%d:%08x\", "
				"\"PHYMode\" : \"%s Channel %d\", "
				"\"AccessPointMAC\" : \"%s\", "
				"\"StationMAC\" : \"%s\", "
				"\"User\" : \"%s\", "
				"\"Password\" : \"%s\", "
				"\"Mode\" : \"%s\", "
				"\"Authentication\" : %d, "
				"\"SSL\" : %d"
			"}",
			system_get_sdk_version(),
			rst->reason, rst->exccause, rst->epc1,
			wifi_phy_mode_str(wifi_get_phy_mode()), wifi_get_channel(),
			config_mac(SOFTAP_IF),
			config_mac(STATION_IF),
			user_config_user(),
			user_config_password(),
			wifi_op_mode_str(wifi_get_opmode()),
			user_config_authentication(),
			user_config_ssl()
		)
	);
}

void ICACHE_FLASH_ATTR config_iot_handler(
	struct espconn *pConnection, 
	request_method method, 
	char *url, 
	char *data, 
	uint16 data_len, 
	uint32 content_len, 
	char *response,
	uint16 response_len
) {
	char error_str[WEBSERVER_MAX_VALUE] = "";
	
	if (method == POST && data != NULL && data_len != 0) {
		struct jsonparse_state parser;
		int json_type;
		
		jsonparse_setup(&parser, data, data_len);
		while ((json_type = jsonparse_next(&parser)) != 0) {
			if (json_type == JSON_TYPE_PAIR_NAME) {
				if (jsonparse_strcmp_value(&parser, "WebSocket") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					user_configuration.events_websocket = jsonparse_get_value_as_int(&parser);
				} else if (jsonparse_strcmp_value(&parser, "SSL") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					user_configuration.events_ssl = jsonparse_get_value_as_int(&parser);
				} else if (jsonparse_strcmp_value(&parser, "Server") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					jsonparse_copy_value(&parser, user_configuration.events_server, USER_CONFIG_EVENTS_SIZE);
				} else if (jsonparse_strcmp_value(&parser, "User") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					jsonparse_copy_value(&parser, user_configuration.events_user, USER_CONFIG_USER_SIZE);
				} else if (jsonparse_strcmp_value(&parser, "Password") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					jsonparse_copy_value(&parser, user_configuration.events_password, USER_CONFIG_USER_SIZE);
				} else if (jsonparse_strcmp_value(&parser, "Path") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					jsonparse_copy_value(&parser, user_configuration.events_path, USER_CONFIG_PATH_SIZE);
					if (os_strlen(user_configuration.events_path) == 0) {
						os_sprintf(user_configuration.events_path, "/");
					}
				} else if (jsonparse_strcmp_value(&parser, "Name") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					jsonparse_copy_value(&parser, user_configuration.events_name, USER_CONFIG_USER_SIZE);
				} else if (jsonparse_strcmp_value(&parser, "Token") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					jsonparse_copy_value(&parser, user_configuration.events_token, USER_CONFIG_TOKEN_SIZE);
				}
			}
		}
		user_config_store(error_str);
		user_config_load();
	}
	
	char data_str[WEBSERVER_MAX_RESPONSE_LEN];
	config_response(
		response,
		error_str,
		json_sprintf(
			data_str,
			"\"IoT\" : {"
				"\"WebSocket\" : %d, "
				"\"SSL\" : %d, "
				"\"Server\" : \"%s\", "
				"\"User\" : \"%s\", "
				"\"Password\" : \"%s\", "
				"\"Path\" : \"%s\", "
				"\"Name\" : \"%s\", "
				"\"Token\" : \"%s\""
			"}",
			user_config_events_websocket(),
			user_config_events_ssl(),
			user_config_events_server(),
			user_config_events_user(),
			user_config_events_password(),
			user_config_events_path(),
			user_config_events_name(),
			user_config_events_token()
		)
	);
}

void ICACHE_FLASH_ATTR config_ap_handler(
	struct espconn *pConnection, 
	request_method method, 
	char *url, 
	char *data, 
	uint16 data_len, 
	uint32 content_len, 
	char *response,
	uint16 response_len
) {
	char error_str[WEBSERVER_MAX_VALUE] = "";
	
	if (method == POST && data != NULL && data_len != 0) {
		struct jsonparse_state parser;
		int json_type;
		
		struct softap_config config;
		wifi_softap_get_config(&config);
		
		jsonparse_setup(&parser, data, data_len);
		while ((json_type = jsonparse_next(&parser)) != 0) {
			if (json_type == JSON_TYPE_PAIR_NAME) {
				if (jsonparse_strcmp_value(&parser, "SSID") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					jsonparse_copy_value(&parser, config.ssid, 32);
					config.ssid_len = os_strlen(config.ssid);
				} else if (jsonparse_strcmp_value(&parser, "Password") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					jsonparse_copy_value(&parser, config.password, 64);
				} else if (jsonparse_strcmp_value(&parser, "Mode") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					if (jsonparse_strcmp_value(&parser, "Open") == 0) {
						config.authmode = AUTH_OPEN;
					} else if (jsonparse_strcmp_value(&parser, "WEP") == 0) {
						config.authmode = AUTH_WEP;
					} else if (jsonparse_strcmp_value(&parser, "WPA PSK") == 0) {
						config.authmode = AUTH_WPA_PSK;
					} else if (jsonparse_strcmp_value(&parser, "WPA2 PSK") == 0) {
						config.authmode = AUTH_WPA2_PSK;
					} else if (jsonparse_strcmp_value(&parser, "WPA WPA2 PSK") == 0) {
						config.authmode = AUTH_WPA_WPA2_PSK;
					}
				} else if (jsonparse_strcmp_value(&parser, "Hidden") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					config.ssid_hidden = jsonparse_get_value_as_int(&parser);
				} else if (jsonparse_strcmp_value(&parser, "BeaconInterval") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					config.beacon_interval = jsonparse_get_value_as_int(&parser);
				} else if (jsonparse_strcmp_value(&parser, "MaxConnections") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					config.max_connection = jsonparse_get_value_as_int(&parser);
				} else if (jsonparse_strcmp_value(&parser, "DHCP") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					user_configuration.access_point.dhcp = jsonparse_get_value_as_int(&parser);
				} else if (jsonparse_strcmp_value(&parser, "Address") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					char address[20];
					jsonparse_copy_value(&parser, address, 20);
					user_configuration.access_point.ip.ip.addr = ip4_addr_parse(address);
				} else if (jsonparse_strcmp_value(&parser, "NetMask") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					char mask[20];
					jsonparse_copy_value(&parser, mask, 20);
					user_configuration.access_point.ip.netmask.addr = ip4_addr_parse(mask);
				} else if (jsonparse_strcmp_value(&parser, "Gateway") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					char gw[20];
					jsonparse_copy_value(&parser, gw, 20);
					user_configuration.access_point.ip.gw.addr = ip4_addr_parse(gw);
				}
			}
		}
		
		if (wifi_softap_set_config(&config)) {
			debug("CONFIG: AP config set\n");
		} else {
			LOCAL const char AP_ERR[] = "Failed to set AP config";
			debug("CONFIG: %s\n", AP_ERR);
			os_sprintf(error_str, AP_ERR);
		}
		user_config_store(error_str);
		user_config_load();
	}
	
	config_response(
		response, 
		error_str,
		config_wifi_ap()
	);
}

void ICACHE_FLASH_ATTR config_station_handler(
	struct espconn *pConnection, 
	request_method method, 
	char *url, 
	char *data, 
	uint16 data_len, 
	uint32 content_len, 
	char *response,
	uint16 response_len
) {
	char error_str[WEBSERVER_MAX_VALUE] = "";
	
	if (method == POST && data != NULL && data_len != 0) {
		struct jsonparse_state parser;
		int json_type;
		
		struct station_config config;
		wifi_station_get_config_default(&config);
		
		jsonparse_setup(&parser, data, data_len);
		while ((json_type = jsonparse_next(&parser)) != 0) {
			if (json_type == JSON_TYPE_PAIR_NAME) {
				if (jsonparse_strcmp_value(&parser, "SSID") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					jsonparse_copy_value(&parser, config.ssid, 32);
				} else if (jsonparse_strcmp_value(&parser, "Password") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					jsonparse_copy_value(&parser, config.password, 64);
				} else if (jsonparse_strcmp_value(&parser, "AutoConnect") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					user_configuration.station_auto_connect = jsonparse_get_value_as_int(&parser);
					wifi_station_set_auto_connect(user_configuration.station_auto_connect);
				} else if (jsonparse_strcmp_value(&parser, "DHCP") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					user_configuration.station.dhcp = jsonparse_get_value_as_int(&parser);
				} else if (jsonparse_strcmp_value(&parser, "Address") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					char address[20];
					jsonparse_copy_value(&parser, address, 20);
					user_configuration.station.ip.ip.addr = ip4_addr_parse(address);
				} else if (jsonparse_strcmp_value(&parser, "NetMask") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					char mask[20];
					jsonparse_copy_value(&parser, mask, 20);
					user_configuration.station.ip.netmask.addr = ip4_addr_parse(mask);
				} else if (jsonparse_strcmp_value(&parser, "Gateway") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					char gw[20];
					jsonparse_copy_value(&parser, gw, 20);
					user_configuration.station.ip.gw.addr = ip4_addr_parse(gw);
				} else if (jsonparse_strcmp_value(&parser, "Hostname") == 0) {
					jsonparse_next(&parser);jsonparse_next(&parser);
					jsonparse_copy_value(&parser, user_configuration.station_hostname, USER_CONFIG_USER_SIZE);
				}

			}
		}
		
		if (wifi_station_set_config(&config)) {
			debug("CONFIG: Station config set\n");
		} else {
			LOCAL const char STATION_ERR[] = "Failed to set Station config";
			debug("CONFIG: %s\n", STATION_ERR);
			os_sprintf(error_str, STATION_ERR);
		}
		user_config_store(error_str);
		user_config_load();
	}
	
	config_response(
		response, 
		error_str,
		config_wifi_station()
	);
}

typedef void (*config_stream_end_callback)(bool success);
config_stream_end_callback config_stream_end = NULL;

void ICACHE_FLASH_ATTR config_stream_chunk(
	struct espconn *pConnection, 
	char *data,
	uint16 data_len,
	uint16 chunkID, 
	bool isLast
) {
	LOCAL bool first = true;
	LOCAL bool success = true;
	
	if (first || chunkID == 0) {
		first = false;
		// skip JSON and file meta data
		uint16 len = os_strlen(data) + 1;
		data += len;
		data_len -= len;
		
		success = flash_stream_init();
	}
	
	if (success) {
		clearAllTimers();
		success = flash_stream_data(data, data_len, true);
	}
	
	if (isLast) {
		if (config_stream_end) {
			setTimeout(config_stream_end, success, 100);
		}
		first = true;
	}
}

#if SSL_ENABLE
void ICACHE_FLASH_ATTR config_stream_end_ssl(bool success) {
	char status[WEBSERVER_MAX_VALUE];
	
	if (success) {
		json_data(status, ESP8266, OK_STR, "", NULL);
	} else {
		json_error(status, ESP8266, flash_error(), EMPTY_DATA);
	}
	user_event_raise(webserver_chunk_url(), status);
}
#endif

void ICACHE_FLASH_ATTR config_ssl_handler(
	struct espconn *pConnection, 
	request_method method, 
	char *url, 
	char *data, 
	uint16 data_len, 
	uint32 content_len, 
	char *response,
	uint16 response_len
) {
#if SSL_ENABLE
	if (method == POST && content_len != 0) {
		webserver_set_status(0);
		webserver_register_chunk_callback(config_stream_chunk, url);
		
		config_stream_end = config_stream_end_ssl;
		if (data_len != 0) {
			config_stream_chunk(pConnection, data, data_len, 0, data_len == content_len);
		}
	} else {
		json_data(response, ESP8266, OK_STR, "", NULL);
	}
#else
	json_error(response, ESP8266, "SSL is not enabled", NULL);
#endif
}

char ICACHE_FLASH_ATTR *config_boot_details() {
LOCAL char boot_details[WEBSERVER_MAX_VALUE];
	os_sprintf(
		boot_details, 
		"v%d %s 0x%08X", 
		system_get_boot_version(),
		system_get_boot_mode() == SYS_BOOT_ENHANCE_MODE ? "enhanced" : "normal",
		system_get_userbin_addr()
	);
	return boot_details;
}

char ICACHE_FLASH_ATTR *config_firmware_bin() {
LOCAL char firmware_bin[10];
	os_sprintf(firmware_bin, "user%d.bin", system_upgrade_userbin_check()+1);
	return firmware_bin;
}

char ICACHE_FLASH_ATTR *config_firmware_upgrade_bin() {
LOCAL char firmware_bin[10];
	os_sprintf(firmware_bin, "user%d.bin", (system_upgrade_userbin_check()+1) ^ 3);
	return firmware_bin;
}

LOCAL void ICACHE_FLASH_ATTR config_firmware_json(char *json, const char *status, const char *bin) {
	char data_str[WEBSERVER_MAX_VALUE];
	json_data(
		json, ESP8266, status,
		json_sprintf(
			data_str,
			"\"Firmware\" : {"
				"\"Current\" : \"%s\", "
				"\"Boot\" : \"%s\""
			"}",
			bin,
			config_boot_details()
		),
		NULL
	);
}

LOCAL void ICACHE_FLASH_ATTR config_upgrade_reboot() {
	system_upgrade_flag_set(UPGRADE_FLAG_FINISH);
	system_upgrade_reboot();
}

void ICACHE_FLASH_ATTR config_stream_end_firmware(bool success) {
	char status[WEBSERVER_MAX_VALUE];
	
	if (success) {
		devices_down();
		setTimeout(config_upgrade_reboot, NULL, USER_CONFIG_REBOOT_AFTER);
		
		config_firmware_json(status, DONE, config_firmware_upgrade_bin());
	} else {
		system_upgrade_flag_set(UPGRADE_FLAG_IDLE);
		json_error(status, ESP8266, flash_error(), EMPTY_DATA);
	}
	
	user_event_raise(webserver_chunk_url(), status);
	
	if (success) {
		setTimeout(user_event_reboot, NULL, USER_CONFIG_REBOOT_AFTER / 2);
	}
}

void ICACHE_FLASH_ATTR config_firmware_handler(
	struct espconn *pConnection, 
	request_method method, 
	char *url, 
	char *data, 
	uint16 data_len, 
	uint32 content_len, 
	char *response,
	uint16 response_len
) {
	if (method == POST && content_len != 0) {
		clearAllTimers();
		
		flash_region_protect(config_firmware_bin(), true);
		flash_region_protect(config_firmware_upgrade_bin(), false);
		system_upgrade_flag_set(UPGRADE_FLAG_START);
		
		webserver_set_status(0);
		webserver_register_chunk_callback(config_stream_chunk, url);
		
		config_stream_end = config_stream_end_firmware;
		if (data_len != 0) {
			config_stream_chunk(pConnection, data, data_len, 0, data_len == content_len);
		}
	} else {
		config_firmware_json(response, OK_STR, config_firmware_bin());
	}
}