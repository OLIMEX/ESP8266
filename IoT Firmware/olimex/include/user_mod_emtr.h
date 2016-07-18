#ifndef __USER_EMTR_H__
	#define __USER_EMTR_H__
	
	#include "user_config.h"
	#if MOD_EMTR_ENABLE

		#include "user_webserver.h"
		
		#define EMTR_URL                    "/mod-emtr"
		
		#define EMTR_DEFAULT_READ_INTERVAL  1000
		#define EMTR_MIN_READ_INTERVAL      500
		#define EMTR_MAX_READ_INTERVAL      5000
		
		#define EMTR_RESET_TIMEOUT          500
		
		typedef enum {
			EMTR_LOG = 1,
			EMTR_CONFIGURE,
			EMTR_CALIBRATION
		} emtr_mode;
		
		void emtr_handler(
			struct espconn *pConnection, 
			request_method method, 
			char *url, 
			char *data, 
			uint16 data_len, 
			uint32 content_len, 
			char *response,
			uint16 response_len
		);
		
		void mod_emtr_init();
	#endif
#endif