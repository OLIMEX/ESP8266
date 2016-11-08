#ifndef __USER_WIFI_SCAN_H__
	#define __USER_WIFI_SCAN_H__
	
	#define WIFI_SCAN_DEBUG          1
	
	#define WIFI_SCAN_URL            "/wifi-scan"
	#define WIFI_SCAN_RESULT_CACHE   30000
	#define WIFI_SCAN_LIMIT          5
	
	#define WIFI_SSID_LEN            32
	
	typedef struct {
		char ssid[32];
		sint8 rssi;
		uint8 channel;
		AUTH_MODE authmode;
	} ap_info;

	void wifi_auto_detect();
	
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