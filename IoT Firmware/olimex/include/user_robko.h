#ifndef __USER_ROBKO_H__
	#define __USER_ROBKO_H__
	
	#include "user_config.h"
	#if DEVICE == ROBKO

		#define ROBKO_URL                "/robko"
		#define ROBKO_URL_ANY            "/robko/*"
		
		#define ROBKO_READ_INTERVAL      500
		
		void robko_handler(
			struct espconn *pConnection, 
			request_method method, 
			char *url, 
			char *data, 
			uint16 data_len, 
			uint32 content_len, 
			char *response,
			uint16 response_len
		);
		
		void user_robko_init();
	#endif
#endif