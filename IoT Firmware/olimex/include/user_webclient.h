#ifndef __USER_WEBCLIENT_H__
	#define __USER_WEBCLIENT_H__
	
	#define WEBCLIENT_DEBUG                0
	#define WEBCLIENT_VERBOSE_OUTPUT       0
	
	#define WEBCLIENT_TIMEOUT              5000
	#define WEBCLIENT_RETRY_AFTER          15000
	#define WEBCLIENT_RETRY_MAX            0
	
	#include "queue.h"
	#include "user_timer.h"
	#include "user_webserver.h"
	
	typedef enum _webclient_state_ {
		HTTP = 0,
		UPGRADE,
		WEBSOCKET
	} webclient_state;
	
	typedef struct _webclient_request_ {
		bool ssl;
		char *host;
		char *path;
		char *content;
		
		char *user;
		char *password;
		char *authorization;
		
		char *headers;
		
		request_method method;
		
		ip_addr_t *ip;
		struct espconn *connection;
		
		webclient_state state;
		
		uint8 retry;
		timer *retry_timer;
		
		STAILQ_ENTRY(_webclient_request_) entries;
	} webclient_request;
	
	void webclient_execute(webclient_request *request);
	void webclient_get(bool ssl, char *user, char *password, char *host, int port, char *path);
	void webclient_post(bool ssl, char *user, char *password, char *host, int port, char *path, char *content);
	void webclient_socket(bool ssl, char *user, char *password, char *host, int port, char *path, char *content);
#endif