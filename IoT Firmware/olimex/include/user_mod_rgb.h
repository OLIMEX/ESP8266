#ifndef __USER_MOD_RGB_H__
	#define __USER_MOD_RGB_H__
	
	#include "user_config.h"
	#if MOD_RGB_ENABLE & I2C_ENABLE

		#define MOD_RGB_URL      "/mod-rgb"
		#define MOD_RGB_URL_ANY  "/mod-rgb/*"
		
		void mod_rgb_init();
		
		void mod_rgb_handler(
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