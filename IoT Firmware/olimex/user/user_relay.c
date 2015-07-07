#include "ets_sys.h"
#include "stdout.h"
#include "osapi.h"
#include "gpio.h"

#include "json/jsonparse.h"

#include "user_json.h"
#include "user_webserver.h"
#include "user_relay.h"
#include "user_devices.h"

LOCAL uint8 state = 0;

void ICACHE_FLASH_ATTR user_relay_init() {
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5);
	webserver_register_handler_callback(RELAY_URL, relay_handler);
	device_register(NATIVE, 0, RELAY_URL, NULL);
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
		
		GPIO_OUTPUT_SET(GPIO_ID_PIN(5), state);
	}
	
	char data_str[WEBSERVER_MAX_VALUE];
	json_data(
		response, ESP8266, OK_STR,
		json_sprintf(
			data_str,
			"\"Relay\" : %d",
			state
		),
		NULL
	);
}