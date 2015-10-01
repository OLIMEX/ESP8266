#ifndef __USER_FINGER_H__
	#define __USER_FINGER_H__
	
	#include "user_config.h"
	#if MOD_FINGER_ENABLE

		#include "user_webserver.h"
		
		#define FINGER_URL       "/mod-finger"
		
		typedef enum {
			FINGER_READ,
			FINGER_NEW,
			FINGER_DELETE,
			FINGER_EMPTY_DB
		} finger_mode;
		
		void finger_handler(
			struct espconn *pConnection, 
			request_method method, 
			char *url, 
			char *data, 
			uint16 data_len, 
			uint32 content_len, 
			char *response,
			uint16 response_len
		);
		
		void mod_finger_init();
	#endif
#endif