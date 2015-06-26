#include "ets_sys.h"
#include "stdout.h"
#include "osapi.h"
#include "mem.h"
#include "queue.h"

#include "json/jsonparse.h"

#include "user_json.h"
#include "user_misc.h"
#include "user_webserver.h"
#include "user_mod_led_8x8_rgb.h"
#include "user_devices.h"

#include "modules/mod_led_8x8_rgb.h"
#include "user_mod_led_8x8_rgb.h"

void ICACHE_FLASH_ATTR mod_led_8x8_rgb_init() {
	spi_led_8x8_rgb_init();
	led_8x8_rgb_set_dimensions(1, 1);
	
	webserver_register_handler_callback(MOD_LED_8x8_RGB_URL, mod_led_8x8_rgb_handler);
	device_register(SPI, 0, MOD_LED_8x8_RGB_URL);
}

void ICACHE_FLASH_ATTR mod_led_8x8_rgb_scroll_done() {
	char status[WEBSERVER_MAX_VALUE];
	user_event_raise(MOD_LED_8x8_RGB_URL, json_status(status, MOD_LED8x8RGB, DONE, NULL));
}

void ICACHE_FLASH_ATTR mod_led_8x8_rgb_handler(
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
	
	LOCAL uint8  cols = 1;
	LOCAL uint8  rows = 1;
	
	LOCAL uint8  speed = 85;
	
	LOCAL uint8  r = 1;
	LOCAL uint8  g = 1;
	LOCAL uint8  b = 1;
	LOCAL char  *text = NULL;
	
	if (text == NULL) {
		text = (char *)os_zalloc(MOD_LED_8x8_RGB_MAX_TEXT);
	}
	
	if (method == POST && data != NULL && data_len != 0) {
		jsonparse_setup(&parser, data, data_len);

		while ((type = jsonparse_next(&parser)) != 0) {
			if (type == JSON_TYPE_PAIR_NAME) {
				if (jsonparse_strcmp_value(&parser, "cols") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					cols = jsonparse_get_value_as_int(&parser);
				} else if (jsonparse_strcmp_value(&parser, "rows") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					rows = jsonparse_get_value_as_int(&parser);
				} else if (jsonparse_strcmp_value(&parser, "Speed") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					speed = jsonparse_get_value_as_int(&parser);
					if (speed > MOD_LED_8x8_RGB_MAX_SPEED) {
						speed = MOD_LED_8x8_RGB_MAX_SPEED;
					}
				} else if (jsonparse_strcmp_value(&parser, "R") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					r = jsonparse_get_value_as_int(&parser);
				} else if (jsonparse_strcmp_value(&parser, "G") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					g = jsonparse_get_value_as_int(&parser);
				} else if (jsonparse_strcmp_value(&parser, "B") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					b = jsonparse_get_value_as_int(&parser);
				} else if (jsonparse_strcmp_value(&parser, "Text") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					jsonparse_copy_value(&parser, text, MOD_LED_8x8_RGB_MAX_TEXT);
				}
			}
		}
		
		if (!led_8x8_rgb_set_dimensions(cols, rows)) {
			json_error(response, MOD_LED8x8RGB, "Dimensions can not be set", NULL);
			return;
		}
	
		if (!led_8x8_rgb_scroll(r, g, b, text, MOD_LED_8x8_RGB_MAX_SPEED - speed + 1, mod_led_8x8_rgb_scroll_done)) {
			json_error(response, MOD_LED8x8RGB, "Busy", NULL);
			return;
		}
	}
	
	char data_str[WEBSERVER_MAX_VALUE * 2];
	json_data(
		response, MOD_LED8x8RGB, OK_STR,
		json_sprintf(
			data_str,
			"\"cols\" : %d, "
			"\"rows\" : %d, "
			"\"Speed\" : %d, "
			"\"R\" : %d, "
			"\"G\" : %d, "
			"\"B\" : %d, "
			"\"Text\" : \"%s\"",
			led_8x8_rgb_get_cols(),
			led_8x8_rgb_get_rows(),
			speed,
			r,
			g,
			b,
			text
		),
		NULL
	);
}
