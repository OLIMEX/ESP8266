#ifndef __USER_RFID_H__
	#define __USER_RFID_H__
	
	#include "user_config.h"
	#if MOD_RFID_ENABLE

		#include "user_webserver.h"
		
		#define RFID_URL "/mod-rfid"
		
		void rfid_handler(
			struct espconn *pConnection, 
			request_method method, 
			char *url, 
			char *data, 
			uint16 data_len, 
			uint32 content_len, 
			char *response,
			uint16 response_len
		);
		
		void mod_rfid_init();
		
	#endif
#endif