#ifndef __USER_MOD_TC_MK2_H__
	#define __USER_MOD_TC_MK2_H__
	
	#include "user_config.h"
	#if MOD_TC_MK2_ENABLE & I2C_ENABLE

		#define MOD_TC_MK2_URL      "/mod-tc-mk2"
		#define MOD_TC_MK2_URL_ANY  "/mod-tc-mk2/*"
		
		#define MOD_TC_MK2_REFRESH_DEFAULT      10000
		#define MOD_TC_MK2_EACH_DEFAULT         3
		#define MOD_TC_MK2_THRESHOLD_DEFAULT    2
		
		#define MOD_TC_MK2_DEBUG                0
		
		void mod_tc_mk2_handler(
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