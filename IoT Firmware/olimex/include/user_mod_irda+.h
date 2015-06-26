#ifndef __USER_MOD_IRDA_H__
	#define __USER_MOD_IRDA_H__
	
	#define MOD_IRDA_URL      "/mod-irda"
	#define MOD_IRDA_URL_ANY  "/mod-irda/*"
	
	void mod_irda_handler(
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