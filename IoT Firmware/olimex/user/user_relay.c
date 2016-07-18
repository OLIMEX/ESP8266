#include "user_config.h"
#if RELAY_ENABLE

#include "ets_sys.h"
#include "osapi.h"
#include "stdout.h"
#include "gpio.h"
#include "gpio_config.h"

#include "json/jsonparse.h"

#include "user_json.h"
#include "user_webserver.h"
#include "user_events.h"
#include "user_relay.h"
#include "user_devices.h"
#include "user_plug.h"

LOCAL uint8  relay_state = 0;
LOCAL uint32 relay_timer = 0;

LOCAL gpio_config relay_hardware = {
	.gpio_id    = 5,
	.gpio_name  = PERIPHS_IO_MUX_GPIO5_U,
	.gpio_func  = FUNC_GPIO5
};

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

uint8 ICACHE_FLASH_ATTR user_relay_get() {
	return relay_state;
}

LOCAL void user_relay_on();
LOCAL void user_relay_off();

void ICACHE_FLASH_ATTR user_relay_set(int state) {
	if (relay_timer != 0 || state == relay_state) {
		return;
	}
	
	uint8 store_state = state;
	if (state < 0) {
		// Off for (-state) milliseconds then On
		relay_timer = setTimeout(user_relay_on, NULL, -state);
		state = 0;
		store_state = 1;
	} else if (state == 2) {
		// Toggle
		state = relay_state ? 0 : 1;
		store_state = state;
	} else if (state > 2) {
		// On for (state) milliseconds then Off
		relay_timer = setTimeout(user_relay_off, NULL, state);
		state = 1;
		store_state = 0;
	}
	
	GPIO_OUTPUT_SET(GPIO_ID_PIN(relay_hardware.gpio_id), state);
	relay_state = state;
	
#if DEVICE == PLUG
	plug_led(PLUG_LED2, relay_state);
	emtr_relay_set(0, store_state);
#endif
}

uint8 ICACHE_FLASH_ATTR user_relay_toggle() {
	user_relay_set(2);
	return relay_state;
}

LOCAL void ICACHE_FLASH_ATTR user_relay_on() {
char response[WEBSERVER_MAX_VALUE];
	relay_timer = 0;
	user_relay_set(1);
	
	user_relay_state(response);
	user_event_raise(RELAY_URL, response);
}

LOCAL void ICACHE_FLASH_ATTR user_relay_off() {
char response[WEBSERVER_MAX_VALUE];
	relay_timer = 0;
	user_relay_set(0);
	
	user_relay_state(response);
	user_event_raise(RELAY_URL, response);
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
	int state = relay_state;
	
	if (method == POST && data != NULL && data_len != 0) {
		jsonparse_setup(&parser, data, data_len);
		
		while ((type = jsonparse_next(&parser)) != 0) {
			if (type == JSON_TYPE_PAIR_NAME) {
				if (jsonparse_strcmp_value(&parser, "Relay") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					state = jsonparse_get_value_as_sint(&parser);
				}
			}
		}
		
		user_relay_set(state);
	}
	
	user_relay_state(response);
}

void ICACHE_FLASH_ATTR user_relay_init() {
	PIN_FUNC_SELECT(relay_hardware.gpio_name, relay_hardware.gpio_func);
	relay_state = GPIO_INPUT_GET(GPIO_ID_PIN(relay_hardware.gpio_id));
	webserver_register_handler_callback(RELAY_URL, relay_handler);
	device_register(NATIVE, 0, ESP8266, RELAY_URL, NULL, NULL);
}

#endif
