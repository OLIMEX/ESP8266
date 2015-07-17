#ifndef __USER_EMTR_H__
	#define __USER_EMTR_H__
	
	#include "user_webserver.h"
	
	#define EMTR_URL       "/mod-emtr"
	
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