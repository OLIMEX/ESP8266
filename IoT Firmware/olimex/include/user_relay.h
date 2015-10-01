#ifndef __USER_RELAY_H__
	#define __USER_RELAY_H__
	
	#include "user_config.h"
	#if RELAY_ENABLE

		#include "user_webserver.h"
		
		#define RELAY_URL "/relay"
		
		void relay_handler(
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