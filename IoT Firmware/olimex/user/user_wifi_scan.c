#include "ets_sys.h"
#include "stdout.h"
#include "osapi.h"
#include "mem.h"
#include "queue.h"

#include "user_json.h"
#include "user_interface.h"
#include "user_webserver.h"
#include "user_long_poll.h"
#include "user_misc.h"

#include "user_wifi_scan.h"

LOCAL bool   wifi_scan_in_progress = false;
LOCAL bool   wifi_auto_connect     = false;
LOCAL sint8  wifi_auto_rssi        = SINT8_MIN;

LOCAL uint32 wifi_clean_timeout    = 0;
LOCAL uint32 wifi_detect_timeout   = 0;

LOCAL ap_info **wifi_scan_result = NULL;
LOCAL uint8 wifi_scan_ap_count = 0;

LOCAL struct station_config wifi_auto_config;

LOCAL void wifi_scan_done(void *arg, STATUS status);

LOCAL void ICACHE_FLASH_ATTR wifi_scan_clean()  {
	uint8 i=0;

	// clear stored AP data
	if (wifi_scan_ap_count != 0 && wifi_scan_result != NULL) {
		for (i=0; i<wifi_scan_ap_count; i++) {
			os_free(wifi_scan_result[i]);
		}
		os_free(wifi_scan_result);
	}
	wifi_scan_result = NULL;
	wifi_scan_ap_count = 0;
#if WIFI_SCAN_DEBUG	
	debug("WiFi Scan: Cache Cleaned\n");
#endif
}

LOCAL bool ICACHE_FLASH_ATTR wifi_scan_start() {
	if (wifi_scan_in_progress) {
		return true;
	}
	
#if WIFI_SCAN_DEBUG	
	debug("WiFi Scan: Start...\n");
	debug("\tMode: %s\n", wifi_op_mode_str(wifi_get_opmode()));
	debug("\tPHY Mode: %d\n", wifi_get_phy_mode());
	debug("\tSleep Type: %d\n", wifi_get_sleep_type());
#endif
	clearTimeout(wifi_clean_timeout);
	wifi_scan_clean();
	
	wifi_scan_in_progress = wifi_station_scan(NULL, wifi_scan_done);
	return wifi_scan_in_progress;
}

void ICACHE_FLASH_ATTR wifi_auto_detect() {
	if (user_config_station_auto_connect() == 0) {
		return;
	}
	
#if WIFI_SCAN_DEBUG	
	debug("WiFi Scan: Auto Detect...\n");
#endif
	clearTimeout(wifi_detect_timeout);
	wifi_station_get_config_default(&wifi_auto_config);
	if (wifi_auto_config.ssid[0] == '\0') {
		wifi_auto_connect   = true;
		wifi_auto_rssi      = SINT8_MIN;
		
		wifi_auto_config.ssid[0]     = '\0';
		wifi_auto_config.password[0] = '\0';
		wifi_auto_config.bssid[0]    = '\0';
		wifi_auto_config.bssid_set   = 0;
		
		wifi_station_set_reconnect_policy(0);
		wifi_scan_start();
	}
}

LOCAL void ICACHE_FLASH_ATTR wifi_scan_get_result(char *response) {
	if (wifi_scan_in_progress) {
		webserver_set_status(0);
		return;
	}
	
	if (wifi_scan_ap_count == 0 || wifi_scan_result == NULL) {
		if (wifi_scan_start()) {
			webserver_set_status(0);
			return;
		}
		
		webserver_set_status(200);
		json_error(response, ESP8266, "Can not start scan", NULL);
		return;
	}
	
	webserver_set_status(200);
	
	char result[WEBSERVER_MAX_VALUE*wifi_scan_ap_count];
	os_memset(result, '\0', sizeof(result));
	
	uint8 i=0;
	for (i=0; i<wifi_scan_ap_count; i++) {
		os_sprintf(
			result + os_strlen(result), 
			"%s{\"SSID\" : \"%s\", \"Channel\" : %d, \"Strength\" : %d, \"Mode\" : \"%s\"}", 
			i > 0 ? ", " : "",
			wifi_scan_result[i]->ssid, 
			wifi_scan_result[i]->channel, 
			wifi_scan_result[i]->rssi, 
			wifi_auth_mode_str(wifi_scan_result[i]->authmode)
		);
	}
	
	char data_str[WEBSERVER_MAX_VALUE*wifi_scan_ap_count];
	json_data(
		response, ESP8266, OK_STR,
		json_sprintf(
			data_str,
			"\"WiFi\" : [%s]",
			result
		),
		NULL
	);
}

LOCAL void ICACHE_FLASH_ATTR wifi_scan_done(void *arg, STATUS status) {
	wifi_scan_in_progress = false;
	
	if (status != OK || arg == NULL) {
#if WIFI_SCAN_DEBUG	
		debug("WiFi Scan: Failed [%d].\n", status);
#endif
		if (wifi_auto_connect) {
			setTimeout(wifi_auto_detect, NULL, WIFI_SCAN_RESULT_CACHE / 2);
		}
		return;
	}
	
#if WIFI_SCAN_DEBUG	
	debug("WiFi Scan: Done...\n");
#endif
	if (wifi_scan_ap_count != 0 || wifi_scan_result != NULL) {
		debug("WiFi Scan: Cache Not Clean.\n");
		return;
	}
	
	// count AP data
	struct bss_info *ap = (struct bss_info *)arg;
	while (ap != NULL && wifi_scan_ap_count < WIFI_SCAN_LIMIT) {
		wifi_scan_ap_count++;
		if (ap->ssid_len != 0 && ap->ssid_len < WIFI_SSID_LEN) {
			ap->ssid[ap->ssid_len] = '\0';
		}
#if WIFI_SCAN_DEBUG	
		debug("\tSSID: [%s] Channel: [%d] Strength: [%d] Mode: [%d]\n", ap->ssid, ap->channel, ap->rssi, ap->authmode);
#endif
		ap = ap->next.stqe_next;
	}
#if WIFI_SCAN_DEBUG	
	debug("WiFi Scan: %d APs found.\n", wifi_scan_ap_count);
#endif
	
	char response[WEBSERVER_MAX_RESPONSE_LEN] = "";
	
	if (wifi_scan_ap_count == 0) {
		json_data(
			response, ESP8266, OK_STR,
			"\"WiFi\" : []",
			NULL
		);
	} else {
		// Store AP data
		wifi_scan_result = (ap_info **)os_malloc(sizeof(ap_info *) * wifi_scan_ap_count);
		
		uint8 i=0;
		uint8 j=0;
		sint8 min = 0;
		ap = (struct bss_info *)arg;
		while (ap != NULL) {
			if (i < wifi_scan_ap_count) {
				wifi_scan_result[i] = (ap_info *)os_malloc(sizeof(ap_info));
				
				// Force to absolute minimum
				min = SINT8_MIN;
				j = i;
			} else {
				// Find minimal rssi to be replaced
				uint8 k;
				min = SINT8_MAX;
				for (k = 0; k < wifi_scan_ap_count; k++) {
					if (min > wifi_scan_result[k]->rssi) {
						min = wifi_scan_result[k]->rssi;
						j = k;
					}
				}
			}
			
			if (min < ap->rssi) {
				os_memcpy(wifi_scan_result[j]->ssid, ap->ssid, WIFI_SSID_LEN);
				
				wifi_scan_result[j]->rssi = ap->rssi;
				wifi_scan_result[j]->channel = ap->channel;
				wifi_scan_result[j]->authmode = ap->authmode;
				
				if (
					wifi_auto_connect && 
					ap->authmode == AUTH_OPEN &&
					ap->rssi > wifi_auto_rssi
				) {
					wifi_auto_rssi = ap->rssi;
					os_memcpy(&(wifi_auto_config.ssid), ap->ssid, WIFI_SSID_LEN);
				}
			}
			
			i++;
			ap = ap->next.stqe_next;
		}
		
		wifi_clean_timeout = setTimeout(wifi_scan_clean, NULL, WIFI_SCAN_RESULT_CACHE);
		wifi_scan_get_result(response);
	}
	
	if (wifi_auto_connect) {
		if (wifi_auto_config.ssid[0] != '\0')  {
			wifi_auto_connect = false;
			wifi_station_set_config_current(&wifi_auto_config);
			wifi_station_connect();
		} else {
			wifi_detect_timeout = setTimeout(wifi_auto_detect, NULL, WIFI_SCAN_RESULT_CACHE / 2);
		}
	}
	
#if WIFI_SCAN_DEBUG	
	debug("WiFi Scan: Event Raise.\n");
#endif
	user_event_raise(WIFI_SCAN_URL, response);
}

void ICACHE_FLASH_ATTR wifi_scan_handler(
	struct espconn *pConnection, 
	request_method method, 
	char *url, 
	char *data, 
	uint16 data_len, 
	uint32 content_len, 
	char *response,
	uint16 response_len
) {
	wifi_scan_get_result(response);
}
