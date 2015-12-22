#ifndef __USER_DIMMER_H__
	#define __USER_DIMMER_H__
	
	#include "user_config.h"
	#if DEVICE == DIMMER

		#define DIMMER_URL                "/dimmer"
		#define DIMMER_URL_ANY            "/dimmer/*"
		
		#define DIMMER_REFRESH_DEFAULT    1000
		#define DIMMER_EACH_DEFAULT       10
		#define DIMMER_THRESHOLD_DEFAULT  5
		
		void dimmer_handler(
			struct espconn *pConnection, 
			request_method method, 
			char *url, 
			char *data, 
			uint16 data_len, 
			uint32 content_len, 
			char *response,
			uint16 response_len
		);
		
		void user_dimmer_init();
	#endif
#endif