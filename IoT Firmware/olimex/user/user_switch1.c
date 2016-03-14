#include "user_config.h"
#if DEVICE == SWITCH1

#include "ets_sys.h"
#include "osapi.h"
#include "stdout.h"
#include "gpio.h"

#include "json/jsonparse.h"

#include "user_json.h"
#include "user_webserver.h"
#include "user_switch1.h"
#include "user_devices.h"

#define SWITCH_COUNT          2
#define SWITCH_STATE_FILTER   5

LOCAL void switch1_toggle();

LOCAL switch1_config switch1_hardware[SWITCH_COUNT] = {
	{
		.type       = SWITCH1_RELAY,
		.gpio = {
			.gpio_id    = 5,
			.gpio_name  = PERIPHS_IO_MUX_GPIO5_U,
			.gpio_func  = FUNC_GPIO5
		},
		.handler    = NULL,
		.state      = 1,
		.state_buf  = 0,
		.timer      = 0
	}, 
	
	{
		.type       = SWITCH1_SWITCH,
		.gpio = {
			.gpio_id    = 14,
			.gpio_name  = PERIPHS_IO_MUX_MTMS_U,
			.gpio_func  = FUNC_GPIO14
		},
		.handler    = switch1_toggle,
		.state      = 0,
		.state_buf  = SWITCH_STATE_FILTER / 2,
		.timer      = 0
	}
};

LOCAL gpio_config switch1_reset_hardware = {
	.gpio_id    = 15,
	.gpio_name  = PERIPHS_IO_MUX_MTDO_U,
	.gpio_func  = FUNC_GPIO15
};

LOCAL void ICACHE_FLASH_ATTR user_switch1_state(char *response) {
	char data_str[WEBSERVER_MAX_VALUE];
	json_data(
		response, SWITCH1_STR, OK_STR,
		json_sprintf(
			data_str,
			"\"Relay\" : %d, "
			"\"Switch\" : %d ",
			switch1_hardware[0].state,
			switch1_hardware[1].state
		),
		NULL
	);
}

LOCAL void ICACHE_FLASH_ATTR user_switch1_event() {
	char response[WEBSERVER_MAX_VALUE];
	user_switch1_state(response);
	user_event_raise(SWITCH1_URL, response);

}

LOCAL void user_switch1_off(void *arg);

LOCAL void ICACHE_FLASH_ATTR user_switch1_set(uint8 i, int state) {
	if (i >= SWITCH_COUNT || switch1_hardware[i].type != SWITCH1_RELAY) {
		return;
	}
	
	if (switch1_hardware[i].timer != 0 || state == switch1_hardware[i].state) {
		return;
	}
	
	if (state < 0) {
		// On for (-state) milliseconds then Off
		switch1_hardware[i].timer = setTimeout(user_switch1_off, &switch1_hardware[i], -state);
		state = 1;
	} else if (state > 1) {
		// Toggle
		state = switch1_hardware[i].state == 0 ? 1 : 0;
	}
	
	GPIO_OUTPUT_SET(GPIO_ID_PIN(switch1_hardware[i].gpio.gpio_id), state);
	switch1_hardware[i].state = state;
}

LOCAL void user_switch1_off(void *arg) {
	switch1_config *config = arg;
	
	config->timer = 0;
	user_switch1_set(config->id, 0);
	
	user_switch1_event();
}
	
LOCAL void ICACHE_FLASH_ATTR switch1_toggle(void *arg) {
	LOCAL event_timer = 0;
	switch1_config *config = arg;
	
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
			user_switch1_set(config->id - 1, 2);
			
			clearTimeout(event_timer);
			event_timer = setTimeout(user_switch1_event, NULL, 50);
		}
	}
}

void ICACHE_FLASH_ATTR switch1_handler(
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
	int state = 0;
	
	if (method == POST && data != NULL && data_len != 0) {
		jsonparse_setup(&parser, data, data_len);
		
		while ((type = jsonparse_next(&parser)) != 0) {
			if (type == JSON_TYPE_PAIR_NAME) {
				if (jsonparse_strcmp_value(&parser, "Relay") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					state = jsonparse_get_value_as_sint(&parser);
					user_switch1_set(0, state);
				}
			}
		}
	}
	
	user_switch1_state(response);
}

void ICACHE_FLASH_ATTR switch1_init() {
#if SWITCH1_DEBUG
	debug("SWITCH1: Init\n");
#endif
	// MOD_EMTR enable
	PIN_FUNC_SELECT(switch1_reset_hardware.gpio_name, switch1_reset_hardware.gpio_func);
	GPIO_OUTPUT_SET(GPIO_ID_PIN(switch1_reset_hardware.gpio_id), 0);
	os_delay_us(10000);
	GPIO_OUTPUT_SET(GPIO_ID_PIN(switch1_reset_hardware.gpio_id), 1);
}

void ICACHE_FLASH_ATTR switch1_down() {
#if SWITCH1_DEBUG
	debug("SWITCH1: Down\n");
#endif
	// MOD_EMTR reset
	GPIO_OUTPUT_SET(GPIO_ID_PIN(switch1_reset_hardware.gpio_id), 0);
}

void ICACHE_FLASH_ATTR user_switch1_init() {
	uint8 i;
	
	for (i=0; i<SWITCH_COUNT; i++) {
		switch1_hardware[i].id = i;
		PIN_FUNC_SELECT(switch1_hardware[i].gpio.gpio_name, switch1_hardware[i].gpio.gpio_func);
		switch1_hardware[i].state = GPIO_INPUT_GET(GPIO_ID_PIN(switch1_hardware[i].gpio.gpio_id));
		
		if (switch1_hardware[i].type == SWITCH1_SWITCH) {
			gpio_output_set(0, 0, 0, GPIO_ID_PIN(switch1_hardware[i].gpio.gpio_id));
			setInterval(switch1_toggle, &switch1_hardware[i], 10);
		}
	}
	
	webserver_register_handler_callback(SWITCH1_URL, switch1_handler);
	device_register(NATIVE, 0, SWITCH1_URL, switch1_init, switch1_down);
}
#endif
