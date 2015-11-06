#ifndef __USER_EVENTS_H__
	#define __USER_EVENTS_H__
	
	#define EVENTS_DEBUG                1
	#define EVENTS_VERBOSE_OUTPUT       0
	
	#define EVENTS_URL      "/events"
	#define EVENTS_TIMEOUT   5000
	
	#include "queue.h"
	
	typedef struct _event_data_ {
		int port;
		char *url;
		char *data;
		STAILQ_ENTRY(_event_data_) entries;
	} event_data;
	
	void user_events_init();
	void user_event_raise(char *url, char *data);
	void user_event_progress(uint8 progress);
	void user_websocket_event(char *url, char *data, struct espconn *pConnection);
	
	void user_event_server_error();
	void user_event_server_ok();
	
	void user_event_reboot();
	void user_event_connect();
	void user_event_reconnect();
#endif