#include "ets_sys.h"
#include "stdout.h"
#include "osapi.h"
#include "mem.h"

#include "driver/key.h"

#include "user_json.h"
#include "user_timer.h"
#include "user_events.h"
#include "user_webserver.h"
#include "user_websocket.h"
#include "user_config.h"
#include "user_button.h"
#include "user_devices.h"

void ICACHE_FLASH_ATTR memory_info() {
	system_print_meminfo();
	debug("Free heap: %d\n", system_get_free_heap_size());
	timers_info();
	webserver_connections_info();
}

LOCAL void ICACHE_FLASH_ATTR button_set_response(char *state) {
	char response[WEBSERVER_MAX_VALUE];
	debug("BUTTON: %s\n", state);
	
	char data[WEBSERVER_MAX_VALUE];
	json_data(
		response, ESP8266, OK_STR,
		json_sprintf(
			data,
			"\"Button\" : \"%s\"",
			state
		),
		NULL
	);
	
	user_event_raise(BUTTON_URL, response);
}

void ICACHE_FLASH_ATTR button_short_press() {
	button_set_response("Short Press");
	setTimeout(memory_info,  NULL, 100);
}

void ICACHE_FLASH_ATTR button_long_press() {
	button_set_response("Long Press");
	
	debug("CONFIG: Restore defaults\n");
	user_config_restore_defaults();
	user_config_load();
}

void ICACHE_FLASH_ATTR button_long_release() {
	button_set_response("Long Release");
}

void ICACHE_FLASH_ATTR user_button_init() {
	static struct single_key_param *keys[1] = {NULL};
	
	keys[0] = key_init_single(
		GPIO_ID_PIN(0), 
		PERIPHS_IO_MUX_GPIO0_U,
		FUNC_GPIO0,
		button_short_press,
		button_long_press,
		button_long_release
	);
	
	static struct keys_param param = {
		.key_num = 1,
		.single_key = keys
	};
	
	key_init(&param);
	
	webserver_register_handler_callback(BUTTON_URL, button_handler);
	device_register(NATIVE, 0, BUTTON_URL, NULL);
}

void ICACHE_FLASH_ATTR button_handler(
	struct espconn *pConnection, 
	request_method method, 
	char *url, 
	char *data, 
	uint16 data_len, 
	uint32 content_len, 
	char *response,
	uint16 response_len
) {
	webserver_set_status(0);
}