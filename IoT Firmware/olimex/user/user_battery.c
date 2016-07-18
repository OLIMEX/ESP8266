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

LOCAL uint8  battery_state      = 0;
LOCAL sint32 battery_adc        = 0;
LOCAL sint32 battery_charge_fix = 0;
LOCAL uint8  battery_percent    = 0;

#if BATTERY_DEBUG
LOCAL void ICACHE_FLASH_ATTR battery_debug(uint8 percent) {
	float v = battery_adc;
	v = v / 1024;
	int    vi = v;
	uint16 vd = (v - vi) * 1000;
	
	float vb = v + v / 4.99 * 22;
	int    vbi = vb;
	uint16 vbd = (vb - vbi) * 1000;
	
	debug(
		"ADC=%d "
		"Vadc=%d.%03d "
		"Vbat=%d.%03d "
		"Percent=%d\n", 
		battery_adc, 
		vi, vd,
		vbi, vbd,
		percent
	);
}
#endif	

LOCAL sint32 ICACHE_FLASH_ATTR battery_adc_read() {
	uint8  state = gpio16_input_get();
	sint32 adc = system_adc_read();
	
	if (state != battery_state) {
		// Change of state - calculate new charge fix
		if (state == 0) {
			battery_charge_fix = (adc < battery_adc ? 
				battery_adc - adc
				:
				0
			);
		} else {
			battery_charge_fix = (adc > battery_adc ? 
				battery_adc - adc
				:
				0
			);
		}
	}
	
#if BATTERY_DEBUG
	debug(
		"ADC=%d "
		"fix=%d\n",
		adc, 
		battery_charge_fix
	);
#endif	
	
	battery_adc = adc + battery_charge_fix;
	return battery_adc;
}

uint8 ICACHE_FLASH_ATTR battery_percent_get() {
	LOCAL bool   first = true;
	LOCAL float  percent = 0;
	
	sint32 adc = battery_adc_read();
	
	float  p;
	if (adc <= BATTERY_MIN_ADC) {
		p = 0;
	} else if (adc >= BATTERY_MAX_ADC) {
		p = 100;
	} else {
		p = 100 * (adc - BATTERY_MIN_ADC) / (BATTERY_MAX_ADC - BATTERY_MIN_ADC);
	}
	
	if (first) {
		percent = p;
	} else {
		percent = percent + (p - percent) / BATTERY_FILTER_FACTOR;
	}
	
#if BATTERY_DEBUG
	battery_debug(percent + 0.5);
#endif
	
	first = false;
	return (percent + 0.5);
}

LOCAL void ICACHE_FLASH_ATTR battery_set_response(char *response) {
	char data[WEBSERVER_MAX_VALUE];
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
	LOCAL int count = 0;
	
	char response[WEBSERVER_MAX_VALUE];
	uint8 state = gpio16_input_get();
	uint8 percent = battery_percent_get();
	uint8 diff = 0;
	
	if (percent > battery_percent) {
		diff = percent - battery_percent;
		count += diff;
	} else if (percent < battery_percent) {
		diff = battery_percent - percent;
		count -= diff;
	} else {
		count = 0;
	}
	
#if BATTERY_DEBUG
	debug("State: %d Count: %d\n", state, count);
#endif

	if (
		state != battery_state || 
		(abs(count) > BATTERY_FILTER_COUNT && diff > BATTERY_FILTER_DIFF) || 
		(battery_state != 0 && percent > battery_percent &&  count > BATTERY_FILTER_COUNT) ||
		(battery_state == 0 && percent < battery_percent && -count > BATTERY_FILTER_COUNT)
	) {
		count = 0;
		battery_state = state;
		battery_percent = percent;
		
		battery_set_response(response);
		user_event_raise(BATTERY_URL, response);
	}
	
	if (abs(count) > BATTERY_FILTER_COUNT) {
		if (count > 0 && battery_charge_fix > 0) {
			battery_charge_fix--;
		} else if (count < 0 && battery_charge_fix < 0) {
			battery_charge_fix++;
		}
		count = 0;
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
	battery_set_response(response);
}

void ICACHE_FLASH_ATTR user_battery_init() {
	gpio16_input_conf();
	
	webserver_register_handler_callback(BATTERY_URL, battery_handler);
	device_register(NATIVE, 0, ESP8266, BATTERY_URL, NULL, NULL);
	
	uint8 i;
	battery_state = gpio16_input_get();
	for (i=0; i<BATTERY_FILTER_COUNT; i++) {
		battery_percent = battery_percent_get();
	}
	
	setInterval(battery_state_get, NULL, BATTERY_STATE_REFRESH);
}
#endif