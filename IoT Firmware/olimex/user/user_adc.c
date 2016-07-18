#include "user_config.h"
#if ADC_ENABLE

#include "ets_sys.h"
#include "stdout.h"
#include "osapi.h"
#include "gpio.h"

#include "json/jsonparse.h"

#include "user_json.h"
#include "user_webserver.h"
#include "user_devices.h"
#include "user_timer.h"
#include "user_adc.h"

LOCAL uint16 state = 0;

LOCAL uint32 adc_refresh   = ADC_REFRESH_DEFAULT;
LOCAL uint8  adc_each      = ADC_EACH_DEFAULT;
LOCAL uint32 adc_threshold = ADC_THRESHOLD_DEFAULT;

LOCAL uint32 adc_refresh_timer = 0;

LOCAL void ICACHE_FLASH_ATTR adc_read(char *response, bool poll) {
	char data[WEBSERVER_MAX_VALUE];
	char poll_str[WEBSERVER_MAX_VALUE];
	
	state = system_adc_read();
	
	if (poll) {
		json_poll_str(poll_str, adc_refresh / 1000, adc_each, adc_threshold);
	} else {
		poll_str[0] = '\0';
	}
	
	json_data(
		response, ESP8266, OK_STR, 
		json_sprintf(
			data,
			"\"ADC\" : {\"Value\" : %d %s}",
			state,
			poll_str
		),
		NULL
	);
}

void ICACHE_FLASH_ATTR adc_update() {
	LOCAL uint16 old_state = 0;
	LOCAL uint8 count = 0;
	char response[WEBSERVER_MAX_VALUE];
	
	adc_read(response, false);
	count++;
	
	if (abs(state - old_state) > adc_threshold || (count >= adc_each && state != old_state)) {
#if ADC_DEBUG
		debug("ADC: Change [%d] -> [%d]\n", old_state, state);
#endif
		old_state = state;
		count = 0;
		
		user_event_raise(ADC_URL, response);
	}
}

void ICACHE_FLASH_ATTR user_adc_timer_init() {
	if (adc_refresh_timer != 0) {
		clearInterval(adc_refresh_timer);
	}
	
	if (adc_refresh == 0) {
		adc_refresh_timer = 0;
	} else {
		adc_refresh_timer = setInterval(adc_update, NULL, adc_refresh);
	}
}

LOCAL bool ICACHE_FLASH_ATTR adc_parse(char *data, uint16 data_len) {
	uint32 refresh   = adc_refresh;
	uint8  each      = adc_each;
	uint32 threshold = adc_threshold;
	
	struct jsonparse_state parser;
	int type;

	jsonparse_setup(&parser, data, data_len);

	while ((type = jsonparse_next(&parser)) != 0) {
		if (type == JSON_TYPE_PAIR_NAME) {
			if (jsonparse_strcmp_value(&parser, "Refresh") == 0) {
				jsonparse_next(&parser);
				jsonparse_next(&parser);
				adc_refresh = jsonparse_get_value_as_int(&parser) * 1000;
			} else if (jsonparse_strcmp_value(&parser, "Each") == 0) {
				jsonparse_next(&parser);
				jsonparse_next(&parser);
				adc_each = jsonparse_get_value_as_int(&parser);
			} else if (jsonparse_strcmp_value(&parser, "Threshold") == 0) {
				jsonparse_next(&parser);
				jsonparse_next(&parser);
				adc_threshold = jsonparse_get_value_as_int(&parser);
			}
		}
	}
	
	return (
		refresh   != adc_refresh ||
		each      != adc_each ||
		threshold != adc_threshold
	);
}

LOCAL void ICACHE_FLASH_ATTR adc_preferences_set() {
	char preferences[WEBSERVER_MAX_VALUE];
	os_sprintf(
		preferences, 
		"{"
			"\"Refresh\" : %d, "
			"\"Each\" : %d, "
			"\"Threshold\" : %d"
		"}",
		adc_refresh / 1000,
		adc_each,
		adc_threshold
	);
	preferences_set(ADC_URL, preferences);
}

void ICACHE_FLASH_ATTR adc_handler(
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
		if (adc_parse(data, data_len)) {
			adc_preferences_set();
		}
		
		user_adc_timer_init();
	}

	adc_read(response, true);
}

void ICACHE_FLASH_ATTR user_adc_init() {
	preferences_get(ADC_URL, adc_parse);
	webserver_register_handler_callback(ADC_URL, adc_handler);
	device_register(NATIVE, 0, ESP8266, ADC_URL, NULL, NULL);
	user_adc_timer_init();
}
#endif
