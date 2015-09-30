#ifndef __USER_BATTERY_H__
	#define __USER_BATTERY_H__
	
	#include "user_webserver.h"
	
	#define BATTERY_URL             "/battery"
	#define BATTERY_STATE_REFRESH   1000
	
	#define BATTERY_MIN_ADC          685
	#define BATTERY_MAX_ADC          815
	
	#define BATTERY_FILTER_COUNT      10
	
	void user_battery_init();
	
	void button_handler(
		struct espconn *pConnection, 
		request_method method, 
		char *url, 
		char *data, 
		uint16 data_len, 
		uint32 content_len, 
		char *response,
		uint16 response_len
	);

#endif