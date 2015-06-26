#ifndef __USER_LONG_POLL_H__
	#define __USER_LONG_POLL_H__
	
	#include "os_type.h"
	#include "espconn.h"
	
	#define LONG_POLL_DEBUG     0
	
	void long_poll_register(struct espconn *pConnection, char *pURL);
	void long_poll_response(char *pURL, char *pData);
	void long_poll_close_all();
#endif
