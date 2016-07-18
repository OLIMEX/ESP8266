#include "user_config.h"
#if MOD_LED_8x8_RGB_ENABLE

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

LOCAL uint8  mod_led_8x8_cols = 1;
LOCAL uint8  mod_led_8x8_rows = 1;

LOCAL uint8  mod_led_8x8_speed = 85;

LOCAL uint8  mod_led_8x8_r = 1;
LOCAL uint8  mod_led_8x8_g = 1;
LOCAL uint8  mod_led_8x8_b = 1;
LOCAL char  *mod_led_8x8_text = NULL;

void ICACHE_FLASH_ATTR mod_led_8x8_rgb_scroll_done() {
	char status[WEBSERVER_MAX_VALUE];
	user_event_raise(MOD_LED_8x8_RGB_URL, json_status(status, MOD_LED8x8RGB, DONE, NULL));
}

LOCAL bool ICACHE_FLASH_ATTR mod_led_8x8_rgb_parse(char *data, uint16 data_len) {
	struct jsonparse_state parser;
	int type;
	
	uint8  cols  = mod_led_8x8_cols;
	uint8  rows  = mod_led_8x8_rows;
	uint8  speed = mod_led_8x8_speed;
	
	if (mod_led_8x8_text == NULL) {
		mod_led_8x8_text = (char *)os_zalloc(MOD_LED_8x8_RGB_MAX_TEXT);
	}
	
	jsonparse_setup(&parser, data, data_len);
	while ((type = jsonparse_next(&parser)) != 0) {
		if (type == JSON_TYPE_PAIR_NAME) {
			if (jsonparse_strcmp_value(&parser, "cols") == 0) {
				jsonparse_next(&parser);
				jsonparse_next(&parser);
				mod_led_8x8_cols = jsonparse_get_value_as_int(&parser);
			} else if (jsonparse_strcmp_value(&parser, "rows") == 0) {
				jsonparse_next(&parser);
				jsonparse_next(&parser);
				mod_led_8x8_rows = jsonparse_get_value_as_int(&parser);
			} else if (jsonparse_strcmp_value(&parser, "Speed") == 0) {
				jsonparse_next(&parser);
				jsonparse_next(&parser);
				mod_led_8x8_speed = jsonparse_get_value_as_int(&parser);
				if (mod_led_8x8_speed > MOD_LED_8x8_RGB_MAX_SPEED) {
					mod_led_8x8_speed = MOD_LED_8x8_RGB_MAX_SPEED;
				}
			} else if (jsonparse_strcmp_value(&parser, "R") == 0) {
				jsonparse_next(&parser);
				jsonparse_next(&parser);
				mod_led_8x8_r = jsonparse_get_value_as_int(&parser);
			} else if (jsonparse_strcmp_value(&parser, "G") == 0) {
				jsonparse_next(&parser);
				jsonparse_next(&parser);
				mod_led_8x8_g = jsonparse_get_value_as_int(&parser);
			} else if (jsonparse_strcmp_value(&parser, "B") == 0) {
				jsonparse_next(&parser);
				jsonparse_next(&parser);
				mod_led_8x8_b = jsonparse_get_value_as_int(&parser);
			} else if (jsonparse_strcmp_value(&parser, "Text") == 0) {
				jsonparse_next(&parser);
				jsonparse_next(&parser);
				jsonparse_copy_value(&parser, mod_led_8x8_text, MOD_LED_8x8_RGB_MAX_TEXT);
			}
		}
	}
	
	return (
		cols  != mod_led_8x8_cols ||
		rows  != mod_led_8x8_rows ||
		speed != mod_led_8x8_speed
	);
}

LOCAL void ICACHE_FLASH_ATTR mod_led_8x8_rgb_preferences_set() {
	char preferences[WEBSERVER_MAX_VALUE];
	os_sprintf(
		preferences, 
		"{"
			"\"cols\": %d, "
			"\"rows\": %d, "
			"\"Speed\": %d"
		"}",
		mod_led_8x8_cols,
		mod_led_8x8_rows,
		mod_led_8x8_speed
	);
	preferences_set(MOD_LED8x8RGB, preferences);
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
	
	
	if (mod_led_8x8_text == NULL) {
		mod_led_8x8_text = (char *)os_zalloc(MOD_LED_8x8_RGB_MAX_TEXT);
	}
	
	if (method == POST && data != NULL && data_len != 0) {
		if (led_8x8_rgb_busy()) {
			json_error(response, MOD_LED8x8RGB, BUSY_STR, NULL);
			return;
		}
		
		if (mod_led_8x8_rgb_parse(data, data_len)) {
			if (!led_8x8_rgb_set_dimensions(mod_led_8x8_cols, mod_led_8x8_rows)) {
				json_error(response, MOD_LED8x8RGB, "Dimensions can not be set", NULL);
				return;
			}
			
			mod_led_8x8_rgb_preferences_set();
		}
		
		if (
			!led_8x8_rgb_scroll(
				mod_led_8x8_r, 
				mod_led_8x8_g, 
				mod_led_8x8_b, 
				mod_led_8x8_text, 
				MOD_LED_8x8_RGB_MAX_SPEED - mod_led_8x8_speed + 1, 
				mod_led_8x8_rgb_scroll_done
			)
		) {
			json_error(response, MOD_LED8x8RGB, BUSY_STR, NULL);
			return;
		}
	}
	
	char data_str[WEBSERVER_MAX_VALUE * 2];
	char *escaped = json_escape_str(mod_led_8x8_text, MOD_LED_8x8_RGB_MAX_TEXT);
	
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
			mod_led_8x8_speed,
			mod_led_8x8_r,
			mod_led_8x8_g,
			mod_led_8x8_b,
			escaped
		),
		NULL
	);
	
	os_free(escaped);
}

void ICACHE_FLASH_ATTR mod_led_8x8_rgb_init() {
	spi_led_8x8_rgb_init();
	
	preferences_get(MOD_LED8x8RGB, mod_led_8x8_rgb_parse);
	led_8x8_rgb_set_dimensions(mod_led_8x8_cols, mod_led_8x8_rows);
	
	webserver_register_handler_callback(MOD_LED_8x8_RGB_URL, mod_led_8x8_rgb_handler);
	device_register(SPI, 0, MOD_LED8x8RGB, MOD_LED_8x8_RGB_URL, NULL, NULL);
}

#endif
