#ifndef __USER_ADC_H__
	#define __USER_ADC_H__
	
	#include "user_webserver.h"
	
	#define ADC_URL                       "/adc"
	#define ADC_REFRESH_DEFAULT           10000
	#define ADC_EACH_DEFAULT              3
	#define ADC_THRESHOLD_DEFAULT         5
	
	#define ADC_DEBUG                     0
	
	void user_adc_init();
	
	void adc_handler(
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