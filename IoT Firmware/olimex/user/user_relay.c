#include "user_config.h"
#if RELAY_ENABLE

#include "ets_sys.h"
#include "osapi.h"
#include "stdout.h"
#include "gpio.h"

#include "json/jsonparse.h"

#include "user_json.h"
#include "user_webserver.h"
#include "user_relay.h"
#include "user_devices.h"

LOCAL uint8  relay_state = 0;
LOCAL uint32 relay_timer = 0;

LOCAL void ICACHE_FLASH_ATTR user_relay_state(char *response) {
	char data_str[WEBSERVER_MAX_VALUE];
	json_data(
		response, ESP8266, OK_STR,
		json_sprintf(
			data_str,
			"\"Relay\" : %d",
			relay_state
		),
		NULL
	);
}

LOCAL void ICACHE_FLASH_ATTR user_relay_set(uint8 state) {
	if (relay_timer == 0) {
		GPIO_OUTPUT_SET(GPIO_ID_PIN(5), state);
		relay_state = state;
	}
}

LOCAL void ICACHE_FLASH_ATTR user_relay_off() {
char response[WEBSERVER_MAX_VALUE];
	relay_timer = 0;
	user_relay_set(0);
	
	user_relay_state(response);
	user_event_raise(RELAY_URL, response);
}

LOCAL void ICACHE_FLASH_ATTR user_relay_on_off(uint16 delay) {
	if (relay_timer == 0) {
		user_relay_set(1);
		relay_timer = setTimeout(user_relay_off, NULL, delay);
	}
}

void ICACHE_FLASH_ATTR relay_handler(
	struct espconn *pConnection, 
	request_method method, 
	char *url, 
	char *data, 
	uint16 data_len, 
	uint32 content_len, 
	char *response,
	uint16 response_len
) {
	struct jsonparse_state parser;
	int type;
	uint16 state = relay_state;
	
	if (method == POST && data != NULL && data_len != 0) {
		jsonparse_setup(&parser, data, data_len);
		
		while ((type = jsonparse_next(&parser)) != 0) {
			if (type == JSON_TYPE_PAIR_NAME) {
				if (jsonparse_strcmp_value(&parser, "Relay") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					state = jsonparse_get_value_as_int(&parser);
				}
			}
		}
		
		if (state == 0) {
			user_relay_set(0);
		} else if (state == 1) {
			user_relay_set(1);
		} else {
			user_relay_on_off(state);
		}
	}
	
	user_relay_state(response);
}

void ICACHE_FLASH_ATTR user_relay_init() {
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5);
	webserver_register_handler_callback(RELAY_URL, relay_handler);
	device_register(NATIVE, 0, RELAY_URL, NULL, NULL);
}

#endif
