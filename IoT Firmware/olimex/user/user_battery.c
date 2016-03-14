#include "user_config.h"
#if BATTERY_ENABLE

#include "ets_sys.h"
#include "stdout.h"
#include "osapi.h"
#include "mem.h"

#include "driver/gpio16.h"

#include "user_json.h"
#include "user_timer.h"
#include "user_events.h"
#include "user_webserver.h"
#include "user_websocket.h"
#include "user_config.h"
#include "user_battery.h"
#include "user_devices.h"

LOCAL uint8 battery_state = 0;
LOCAL uint8 battery_percent = 0;

LOCAL uint16 ICACHE_FLASH_ATTR battery_adc_filter() {
	LOCAL uint32 adc = 0;
	uint8 i;
	for (i=0; i<BATTERY_FILTER_COUNT; i++) {
		adc = adc - (adc >> BATTERY_FILTER_SHIFT) + system_adc_read();
	}
	return adc >> BATTERY_FILTER_SHIFT;
}

uint8 ICACHE_FLASH_ATTR battery_percent_get() {
	uint16 adc = battery_adc_filter();
	uint8  percent;
	
	if (adc <= BATTERY_MIN_ADC) {
		percent = 0;
	} else if (adc >= BATTERY_MAX_ADC) {
		percent = 100;
	} else {
		percent = 100 * (adc - BATTERY_MIN_ADC) / (BATTERY_MAX_ADC - BATTERY_MIN_ADC);
	}
	
	return percent;
}

LOCAL void ICACHE_FLASH_ATTR battery_set_response(char *response) {
	char data[WEBSERVER_MAX_VALUE];
#if BATTERY_DEBUG
	debug(
		"BATTERY:\n"
		"    State %d\n"
		"    Percent %d\n", 
		battery_state, 
		battery_percent
	);
#endif	
	json_data(
		response, ESP8266, OK_STR,
		json_sprintf(
			data,
			"\"Battery\": {"
				"\"State\": \"%s\","
				"\"Percent\": %d"
			"}",
			battery_state ?
				"Charging"
				:
				"On Battery"
			,
			battery_percent
		),
		NULL
	);
}

LOCAL void ICACHE_FLASH_ATTR battery_state_get() {
	LOCAL uint8 count = 0;
	
	char response[WEBSERVER_MAX_VALUE];
	uint8 state = gpio16_input_get();
	uint8 percent = battery_percent_get();
	
	if (percent != battery_percent) {
		count++;
	} else {
		count = 0;
	}
	
	if (
		state != battery_state || 
		(percent != battery_percent && count > BATTERY_FILTER_COUNT)
	) {
		count = 0;
		battery_state = state;
		battery_percent = percent;
		
		battery_set_response(response);
		user_event_raise(BATTERY_URL, response);
	}
}

void ICACHE_FLASH_ATTR battery_handler(
	struct espconn *pConnection, 
	request_method method, 
	char *url, 
	char *data, 
	uint16 data_len, 
	uint32 content_len, 
	char *response,
	uint16 response_len
) {
	battery_state = gpio16_input_get();
	battery_percent = battery_percent_get();
	
	battery_set_response(response);
}

void ICACHE_FLASH_ATTR user_battery_init() {
	gpio16_input_conf();
	
	webserver_register_handler_callback(BATTERY_URL, battery_handler);
	device_register(NATIVE, 0, ESP8266, BATTERY_URL, NULL, NULL);
	
	setInterval(battery_state_get, NULL, BATTERY_STATE_REFRESH);
}
#endif