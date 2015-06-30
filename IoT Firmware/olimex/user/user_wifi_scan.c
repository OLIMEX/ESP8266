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

bool wifi_scan_in_progress = false;

typedef struct {
	char ssid[32];
    sint8 rssi;
	AUTH_MODE authmode;
} ap_info;

ap_info **wifi_scan_result = NULL;
uint8 wifi_scan_ap_count = 0;

void wifi_scan_done(void *arg, STATUS status);

bool ICACHE_FLASH_ATTR wifi_scan_start() {
	if (!wifi_scan_in_progress) {
		wifi_scan_in_progress = wifi_station_scan(NULL, wifi_scan_done);
		return wifi_scan_in_progress;
	}
	return true;
}

void ICACHE_FLASH_ATTR wifi_scan_get_result(char *response) {
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
			"%s{\"SSID\" : \"%s\", \"Strength\" : %d, \"Mode\" : \"%s\"}", 
			i > 0 ? ", " : "",
			wifi_scan_result[i]->ssid, 
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

void ICACHE_FLASH_ATTR wifi_scan_clean(void)  {
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
}

void ICACHE_FLASH_ATTR wifi_scan_done(void *arg, STATUS status) {
	wifi_scan_in_progress = false;
	wifi_scan_clean();
	
	if (status != OK) {
		return;
	}
	
	// count AP data
	struct bss_info *ap = (struct bss_info *)arg;
	while (ap = ap->next.stqe_next) {
		wifi_scan_ap_count++;
		debug("SSID: [%s] Strength [%d] Mode: [%d]\n", ap->ssid, ap->rssi, ap->authmode);
	}
	
	// Store AP data
	wifi_scan_result = (ap_info **)os_malloc(sizeof(ap_info *) * wifi_scan_ap_count);
	
	uint8 i=0;
	ap = (struct bss_info *)arg;
	while (ap = ap->next.stqe_next) {
		wifi_scan_result[i] = (ap_info *)os_malloc(sizeof(ap_info));
		os_memcpy(wifi_scan_result[i]->ssid, ap->ssid, 32);
		wifi_scan_result[i]->rssi = ap->rssi;
		wifi_scan_result[i]->authmode = ap->authmode;
		i++;
	}
	
	setTimeout(wifi_scan_clean, NULL, 30000);
	
	char response[WEBSERVER_MAX_RESPONSE_LEN];
	
	wifi_scan_get_result(response);
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
