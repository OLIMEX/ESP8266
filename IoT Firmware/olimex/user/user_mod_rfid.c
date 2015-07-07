#include "ets_sys.h"
#include "stdout.h"
#include "osapi.h"
#include "mem.h"

#include "json/jsonparse.h"

#include "user_json.h"
#include "user_events.h"
#include "user_webserver.h"
#include "user_config.h"
#include "user_devices.h"

#include "user_mod_rfid.h"
#include "modules/mod_rfid.h"

LOCAL uint8  rfid_scan_freq = 0;
LOCAL uint8  rfid_led_state = 1;

const char RFID_FORMAT[] = "\"Module\" : \"%s\", \"Frequency\" : %d, \"Led\" : %d, \"Tag\" : \"%s\"";

void ICACHE_FLASH_ATTR mod_rfid_tag(char *tag) {
	char response[WEBSERVER_MAX_VALUE];
	char data_str[WEBSERVER_MAX_VALUE];
	json_data(
		response, MOD_RFID, OK_STR,
		json_sprintf(
			data_str,
			RFID_FORMAT,
			rfid_mod_str(),
			rfid_scan_freq,
			rfid_led_state,
			tag
		),
		NULL
	);
	
	user_event_raise(RFID_URL, response);
}

void ICACHE_FLASH_ATTR mod_rfid_init() {
	rfid_set_tag_callback(mod_rfid_tag);
	webserver_register_handler_callback(RFID_URL, rfid_handler);
	device_register(UART, 0, RFID_URL, rfid_init);
}

void ICACHE_FLASH_ATTR rfid_handler(
	struct espconn *pConnection, 
	request_method method, 
	char *url, 
	char *data, 
	uint16 data_len, 
	uint32 content_len, 
	char *response,
	uint16 response_len
) {
	if (rfid_get_mod() == RFID_NONE) {
		json_error(response, MOD_RFID, DEVICE_NOT_FOUND, NULL);
		return;
	}
	
	struct jsonparse_state parser;
	int type;
	
	uint8 frequency = rfid_scan_freq;
	uint8 led = rfid_led_state;
	
	if (method == POST && data != NULL && data_len != 0) {
		jsonparse_setup(&parser, data, data_len);
		
		while ((type = jsonparse_next(&parser)) != 0) {
			if (type == JSON_TYPE_PAIR_NAME) {
				if (jsonparse_strcmp_value(&parser, "Frequency") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					frequency = jsonparse_get_value_as_int(&parser);
					if (frequency < 10) {
						rfid_scan_freq = frequency;
					}
				} else if (jsonparse_strcmp_value(&parser, "Led") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					led = jsonparse_get_value_as_int(&parser);
					if (led < 2) {
						rfid_led_state = led;
					}
				}
			}
		}
		
		rfid_set(rfid_scan_freq, rfid_led_state);
	}
	
	char data_str[WEBSERVER_MAX_VALUE];
	json_data(
		response, MOD_RFID, OK_STR,
		json_sprintf(
			data_str,
			RFID_FORMAT,
			rfid_mod_str(),
			rfid_scan_freq,
			rfid_led_state,
			""
		),
		NULL
	);
}