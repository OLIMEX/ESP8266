#ifndef __USER_BUTTON_H__
	#define __USER_BUTTON_H__
	
	#include "user_webserver.h"
	
	#define BUTTON_URL "/button"
	
	void button_handler(
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