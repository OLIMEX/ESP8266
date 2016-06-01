#ifndef __USER_BATTERY_H__
	#define __USER_BATTERY_H__
	
	#include "user_config.h"
	#if BATTERY_ENABLE
		
		#include "user_webserver.h"
		
		#define BATTERY_DEBUG                  0
		
		#define BATTERY_URL           "/battery"
		#define BATTERY_STATE_REFRESH       1000
		
		#if DEVICE == BADGE
			#define BATTERY_MIN_ADC          500
			#define BATTERY_MAX_ADC          750
		#else 
			#define BATTERY_MIN_ADC          685
			#define BATTERY_MAX_ADC          810
		#endif
		
		#define BATTERY_FILTER_COUNT          10
		#define BATTERY_FILTER_FACTOR         16
		
		void  user_battery_init();
		uint8 battery_percent_get();
		
		void battery_handler(
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
#endif