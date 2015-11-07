#include "user_config.h"
#if DEVICE == SWITCH2

#include "ets_sys.h"
#include "osapi.h"
#include "stdout.h"
#include "gpio.h"

#include "driver/key.h"
#include "json/jsonparse.h"

#include "user_json.h"
#include "user_webserver.h"
#include "user_switch2.h"
#include "user_devices.h"

#define SWITCH_COUNT  4

LOCAL void switch1_toggle();
LOCAL void switch2_toggle();

LOCAL switch2_config switch2_def[SWITCH_COUNT] = {
	{ 
		.type       = SWITCH2_RELAY,
		.gpio_id    = 4,
		.gpio_name  = PERIPHS_IO_MUX_GPIO4_U,
		.gpio_func  = FUNC_GPIO4,
		.handler    = NULL
	}, 
	
	{
		.type       = SWITCH2_RELAY,
		.gpio_id    = 5,
		.gpio_name  = PERIPHS_IO_MUX_GPIO5_U,
		.gpio_func  = FUNC_GPIO5,
		.handler    = NULL
	}, 
	
	{
		.type       = SWITCH2_SWITCH,
		.gpio_id    = 12,
		.gpio_name  = PERIPHS_IO_MUX_MTDI_U,
		.gpio_func  = FUNC_GPIO12,
		.handler    = switch1_toggle
	}, 
	
	{
		.type       = SWITCH2_SWITCH,
		.gpio_id    = 14,
		.gpio_name  = PERIPHS_IO_MUX_MTMS_U,
		.gpio_func  = FUNC_GPIO14,
		.handler    = switch2_toggle
	}
};

LOCAL uint8 switch2_state[SWITCH_COUNT] = {0, 0, 0, 0};

LOCAL void ICACHE_FLASH_ATTR user_switch2_state(char *response) {
	char data_str[WEBSERVER_MAX_VALUE];
	
	uint8 i;
	for (i=0; i<SWITCH_COUNT; i++) {
		if (switch2_def[i].type == SWITCH2_SWITCH) {
			switch2_state[i] = GPIO_INPUT_GET(GPIO_ID_PIN(switch2_def[i].gpio_id));
		}
	}
	
	json_data(
		response, SWITCH2_STR, OK_STR,
		json_sprintf(
			data_str,
			"\"Relay1\" : %d, "
			"\"Relay2\" : %d, "
			"\"Switch1\" : %d, "
			"\"Switch2\" : %d",
			switch2_state[0],
			switch2_state[1],
			switch2_state[2],
			switch2_state[3]
		),
		NULL
	);
}

LOCAL void ICACHE_FLASH_ATTR user_switch2_event() {
	char response[WEBSERVER_MAX_VALUE];
	user_switch2_state(response);
	user_event_raise(SWITCH2_URL, response);
}

LOCAL void ICACHE_FLASH_ATTR user_switch2_set(uint8 i, uint8 state) {
	if (i > SWITCH_COUNT || switch2_def[i].type != SWITCH2_RELAY) {
		return;
	}
	
	if (state > 1) {
		state = switch2_state[i] == 0 ? 1 : 0;
	}
	
	GPIO_OUTPUT_SET(GPIO_ID_PIN(switch2_def[i].gpio_id), state);
	switch2_state[i] = state;
}

	
LOCAL void ICACHE_FLASH_ATTR switch1_toggle() {
	user_switch2_set(0, 2);
	user_switch2_event();
}

LOCAL void ICACHE_FLASH_ATTR switch2_toggle() {
	user_switch2_set(1, 2);
	user_switch2_event();
}

void ICACHE_FLASH_ATTR switch2_handler(
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
	uint16 state = 0;
	
	if (method == POST && data != NULL && data_len != 0) {
		jsonparse_setup(&parser, data, data_len);
		
		while ((type = jsonparse_next(&parser)) != 0) {
			if (type == JSON_TYPE_PAIR_NAME) {
				if (jsonparse_strcmp_value(&parser, "Relay1") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					state = jsonparse_get_value_as_int(&parser);
					user_switch2_set(0, state);
				} else if (jsonparse_strcmp_value(&parser, "Relay2") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					state = jsonparse_get_value_as_int(&parser);
					user_switch2_set(1, state);
				}
			}
		}
	}
	
	user_switch2_state(response);
}

void ICACHE_FLASH_ATTR user_switch2_init() {
	uint8 i;
	
	for (i=0; i<SWITCH_COUNT; i++) {
		if (switch2_def[i].type == SWITCH2_RELAY) {
			PIN_FUNC_SELECT(switch2_def[i].gpio_name, switch2_def[i].gpio_func);
		} else {
			key_init_single(
				GPIO_ID_PIN(switch2_def[i].gpio_id), 
				switch2_def[i].gpio_name,
				switch2_def[i].gpio_func,
				switch2_def[i].handler,
				switch2_def[i].handler,
				NULL,
				switch2_def[i].handler
			);
		}
	}
	
	webserver_register_handler_callback(SWITCH2_URL, switch2_handler);
	device_register(NATIVE, 0, SWITCH2_URL, NULL, NULL);
}

#endif
