#ifndef __USER_MOD_IO2_H__
	#define __USER_MOD_IO2_H__
	
	#include "user_config.h"
	#if MOD_IO2_ENABLE

		#define MOD_IO2_URL      "/mod-io2"
		#define MOD_IO2_URL_ANY  "/mod-io2/*"
		
		void mod_io2_handler(
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