#include "user_config.h"
#if BUTTON_ENABLE

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

LOCAL void ICACHE_FLASH_ATTR button_press() {
	button_set_response("Press");
}

LOCAL void ICACHE_FLASH_ATTR button_short_release() {
	button_set_response("Short Release");
	setTimeout(memory_info,  NULL, 100);
}

LOCAL void ICACHE_FLASH_ATTR button_close_connections() {
	debug("BUTTON: Close connections\n");
	
	websocket_close_all(RECONNECT, NULL);
	long_poll_close_all();
}

LOCAL void ICACHE_FLASH_ATTR button_restore_defaults() {
	debug("BUTTON: Restore defaults\n");
	
	user_config_restore_defaults();
	user_config_load();
}

LOCAL void ICACHE_FLASH_ATTR button_long_press() {
	button_set_response("Long Press");
	setTimeout(button_close_connections, NULL, 500);
	setTimeout(button_restore_defaults,  NULL, 1000);
}

LOCAL void ICACHE_FLASH_ATTR button_long_release() {
	button_set_response("Long Release");
}

void ICACHE_FLASH_ATTR user_button_init() {
	struct single_key_param *key = NULL;
	key = key_init_single(
		GPIO_ID_PIN(0), 
		PERIPHS_IO_MUX_GPIO0_U,
		FUNC_GPIO0,
		button_press,
		button_short_release,
		button_long_press,
		button_long_release
	);
	
	if (key == NULL) {
		debug("BUTTON: Initialization Fail\n");
		return;
	}
	
	webserver_register_handler_callback(BUTTON_URL, button_handler);
	device_register(NATIVE, 0, ESP8266, BUTTON_URL, NULL, NULL);
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
#endif