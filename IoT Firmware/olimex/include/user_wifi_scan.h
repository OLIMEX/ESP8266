#ifndef __USER_WIFI_SCAN_H__
	#define __USER_WIFI_SCAN_H__
	
	#define WIFI_SCAN_URL "/wifi-scan"
	
	void wifi_scan_handler(
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