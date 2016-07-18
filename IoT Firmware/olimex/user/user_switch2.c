#include "user_config.h"
#if DEVICE == SWITCH2

#include "ets_sys.h"
#include "osapi.h"
#include "stdout.h"
#include "gpio.h"

#include "json/jsonparse.h"

#include "user_json.h"
#include "user_webserver.h"
#include "user_switch2.h"
#include "user_devices.h"

#define SWITCH_COUNT          4
#define SWITCH_STATE_FILTER   5

LOCAL void switch2_toggle();

LOCAL switch2_config switch2_hardware[SWITCH_COUNT] = {
	{
		.type       = SWITCH2_RELAY,
		.gpio = {
			.gpio_id    = 5,
			.gpio_name  = PERIPHS_IO_MUX_GPIO5_U,
			.gpio_func  = FUNC_GPIO5
		},
		.handler    = NULL,
		.state      = 1,
		.state_buf  = 0,
		.timer      = 0,
		.preference = 0
	}, 
	
	{ 
		.type       = SWITCH2_RELAY,
		.gpio = {
			.gpio_id    = 4,
			.gpio_name  = PERIPHS_IO_MUX_GPIO4_U,
			.gpio_func  = FUNC_GPIO4
		},
		.handler    = NULL,
		.state      = 1,
		.state_buf  = 0,
		.timer      = 0,
		.preference = 0
	}, 
	
	{
		.type       = SWITCH2_SWITCH,
		.gpio = {
			.gpio_id    = 12,
			.gpio_name  = PERIPHS_IO_MUX_MTDI_U,
			.gpio_func  = FUNC_GPIO12
		},
		.handler    = switch2_toggle,
		.state      = 0,
		.state_buf  = SWITCH_STATE_FILTER / 2,
		.timer      = 0,
		.preference = 2
	}, 
	
	{
		.type       = SWITCH2_SWITCH,
		.gpio = {
			.gpio_id    = 14,
			.gpio_name  = PERIPHS_IO_MUX_MTMS_U,
			.gpio_func  = FUNC_GPIO14
		},
		.handler    = switch2_toggle,
		.state      = 0,
		.state_buf  = SWITCH_STATE_FILTER / 2,
		.timer      = 0,
		.preference = 2
	}
};

LOCAL gpio_config switch2_reset_hardware = {
	.gpio_id    = 15,
	.gpio_name  = PERIPHS_IO_MUX_MTDO_U,
	.gpio_func  = FUNC_GPIO15
};

LOCAL void ICACHE_FLASH_ATTR user_switch2_state(char *response) {
	char data_str[WEBSERVER_MAX_VALUE];
	json_data(
		response, SWITCH2_STR, OK_STR,
		json_sprintf(
			data_str,
			"\"Relay1\" : %d, "
			"\"Relay2\" : %d, "
			"\"Switch1\" : %d, "
			"\"Preference1\" : %d, "
			"\"Switch2\" : %d, "
			"\"Preference2\" : %d",
			switch2_hardware[0].state,
			switch2_hardware[1].state,
			switch2_hardware[2].state,
			switch2_hardware[2].preference,
			switch2_hardware[3].state,
			switch2_hardware[3].preference
		),
		NULL
	);
}

LOCAL void ICACHE_FLASH_ATTR user_switch2_event() {
	char response[WEBSERVER_MAX_VALUE];
	user_switch2_state(response);
	user_event_raise(SWITCH2_URL, response);
}

LOCAL void user_switch2_on(void *arg);
LOCAL void user_switch2_off(void *arg);

void ICACHE_FLASH_ATTR user_switch2_set(uint8 i, int state) {
	if (i >= SWITCH_COUNT || switch2_hardware[i].type != SWITCH2_RELAY) {
		return;
	}
	
	if (switch2_hardware[i].timer != 0 || state == switch2_hardware[i].state) {
		return;
	}
	
	uint8 store_state = state;
	if (state < 0) {
		// Off for (-state) milliseconds then On
		switch2_hardware[i].timer = setTimeout(user_switch2_on, &switch2_hardware[i], -state);
		state = 0;
		store_state = 1;
	} else if (state == 2) {
		// Toggle
		state = switch2_hardware[i].state == 0 ? 1 : 0;
		store_state = state;
	} else if (state > 2) {
		// On for (state) milliseconds then Off
		switch2_hardware[i].timer = setTimeout(user_switch2_off, &switch2_hardware[i], state);
		state = 1;
		store_state = 0;
	}
	
	GPIO_OUTPUT_SET(GPIO_ID_PIN(switch2_hardware[i].gpio.gpio_id), state);
	switch2_hardware[i].state = state;
	emtr_relay_set(i, store_state);
}

LOCAL void user_switch2_on(void *arg) {
	switch2_config *config = arg;
	
	config->timer = 0;
	user_switch2_set(config->id, 1);
	
	user_switch2_event();
}

LOCAL void user_switch2_off(void *arg) {
	switch2_config *config = arg;
	
	config->timer = 0;
	user_switch2_set(config->id, 0);
	
	user_switch2_event();
}

LOCAL void ICACHE_FLASH_ATTR switch2_toggle(void *arg) {
	LOCAL event_timer = 0;
	switch2_config *config = arg;
	
	bool event = false;
	if (1 == GPIO_INPUT_GET(GPIO_ID_PIN(config->gpio.gpio_id))) {
		if (config->state_buf < SWITCH_STATE_FILTER) {
			config->state_buf++;
			event = (config->state_buf == SWITCH_STATE_FILTER);
		}
	} else {
		if (config->state_buf > 0) {
			config->state_buf--;
			event = (config->state_buf == 0);
		}
	}
	
	if (event) {
		uint8 state = (config->state_buf == SWITCH_STATE_FILTER);
		if (config->state != state) {
			config->state = state;
			
			if (config->preference == 0) {
				return;
			}
			
			int relay_state = 0;
			if (config->preference < 0 || config->preference >= 2) {
				// Relay Off-On || On-Off || Toggle
				relay_state = config->preference;
			} else if (config->preference == 1) {
				// Relay Set
				relay_state = state;
			}
			
			user_switch2_set(config->id - 2, relay_state);
			
			clearTimeout(event_timer);
			event_timer = setTimeout(user_switch2_event, NULL, 50);
		}
	}
}

LOCAL bool ICACHE_FLASH_ATTR switch2_parse(char *data, uint16 data_len) {
	struct jsonparse_state parser;
	int type;
	
	int preference1 = switch2_hardware[2].preference;
	int preference2 = switch2_hardware[3].preference;
	
	jsonparse_setup(&parser, data, data_len);
	while ((type = jsonparse_next(&parser)) != 0) {
		if (type == JSON_TYPE_PAIR_NAME) {
			if (jsonparse_strcmp_value(&parser, "Relay1") == 0) {
				jsonparse_next(&parser);
				jsonparse_next(&parser);
				user_switch2_set(0, jsonparse_get_value_as_sint(&parser));
			} else if (jsonparse_strcmp_value(&parser, "Relay2") == 0) {
				jsonparse_next(&parser);
				jsonparse_next(&parser);
				user_switch2_set(1, jsonparse_get_value_as_sint(&parser));
			} else if (jsonparse_strcmp_value(&parser, "Preference1") == 0) {
				jsonparse_next(&parser);
				jsonparse_next(&parser);
				switch2_hardware[2].preference = jsonparse_get_value_as_sint(&parser);
			} else if (jsonparse_strcmp_value(&parser, "Preference2") == 0) {
				jsonparse_next(&parser);
				jsonparse_next(&parser);
				switch2_hardware[3].preference = jsonparse_get_value_as_sint(&parser);
			}
		}
	}
	
	return (
		preference1 != switch2_hardware[2].preference ||
		preference2 != switch2_hardware[3].preference
	);
}

LOCAL void ICACHE_FLASH_ATTR switch2_preferences_set() {
	char preferences[WEBSERVER_MAX_VALUE];
	os_sprintf(
		preferences, 
		"{"
			"\"Preference1\": %d,"
			"\"Preference2\": %d"
		"}",
		switch2_hardware[2].preference,
		switch2_hardware[3].preference
	);
	preferences_set(SWITCH2_STR, preferences);
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
	if (method == POST && data != NULL && data_len != 0) {
		if (switch2_parse(data, data_len)) {
			switch2_preferences_set();
		}
	}
	
	user_switch2_state(response);
}

void ICACHE_FLASH_ATTR switch2_init() {
#if SWITCH2_DEBUG
	debug("SWITCH2: Init\n");
#endif
	// MOD_EMTR enable
	PIN_FUNC_SELECT(switch2_reset_hardware.gpio_name, switch2_reset_hardware.gpio_func);
	GPIO_OUTPUT_SET(GPIO_ID_PIN(switch2_reset_hardware.gpio_id), 0);
	os_delay_us(10000);
	GPIO_OUTPUT_SET(GPIO_ID_PIN(switch2_reset_hardware.gpio_id), 1);
}

void ICACHE_FLASH_ATTR switch2_down() {
#if SWITCH2_DEBUG
	debug("SWITCH2: Down\n");
#endif
	// MOD_EMTR reset
	GPIO_OUTPUT_SET(GPIO_ID_PIN(switch2_reset_hardware.gpio_id), 0);
}

void ICACHE_FLASH_ATTR user_switch2_init() {
	uint8 i;
	
	for (i=0; i<SWITCH_COUNT; i++) {
		switch2_hardware[i].id = i;
		PIN_FUNC_SELECT(switch2_hardware[i].gpio.gpio_name, switch2_hardware[i].gpio.gpio_func);
		switch2_hardware[i].state = GPIO_INPUT_GET(GPIO_ID_PIN(switch2_hardware[i].gpio.gpio_id));
		
		if (switch2_hardware[i].type == SWITCH2_SWITCH) {
			gpio_output_set(0, 0, 0, GPIO_ID_PIN(switch2_hardware[i].gpio.gpio_id));
			setInterval(switch2_toggle, &switch2_hardware[i], 10);
		}
	}
	
	preferences_get(SWITCH2_STR, switch2_parse);
	
	webserver_register_handler_callback(SWITCH2_URL, switch2_handler);
	device_register(NATIVE, 0, SWITCH2_STR, SWITCH2_URL, switch2_init, switch2_down);
}
#endif
