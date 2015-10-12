#ifndef __USER_WEBSERVER_H__
	#define __USER_WEBSERVER_H__

	#define WEBSERVER_PORT                80
	#define WEBSERVER_SSL_PORT           443
	
	#define WEBSERVER_LONG_POLL_TIME      30

	#define WEBSERVER_MAX_NAME            50
	#define WEBSERVER_MAX_VALUE          150
	#define WEBSERVER_MAX_RESPONSE_LEN  1024
	
	#define WEBSERVER_MAX_PACKET_LEN    4096
	
	#define WEBSERVER_DEBUG                0
	#define WEBSERVER_VERBOSE_OUTPUT       0

	#include "queue.h"
	#include "user_interface.h"
	#include "espconn.h"
	
	typedef enum request_method {
		GET = 0,
		POST,
		OPTIONS,
		UNKNOWN
	} request_method;

	typedef bool (*basic_authentication_callback)(char *user, char *passwd);
	
	typedef struct _http_header_ {
		char *name;
		char *value;
		STAILQ_ENTRY(_http_header_) entries;
	} http_header;

	typedef void (*http_handler_callback)(
		struct espconn *pConnection, 
		request_method method, 
		char *url, 
		char *data, 
		uint16 data_len, 
		uint32 content_len, 
		char *response,
		uint16 response_len
	);
	
	typedef void (*http_chunk_callback)(
		struct espconn *pConnection, 
		char *data,
		uint16 data_len,
		uint16 chunkID, 
		bool isLast
	);
	
	typedef struct _http_callback_ {
		char *url;
		http_handler_callback handler;
		STAILQ_ENTRY(_http_callback_) entries;
	} http_callback;
	
	typedef struct {
		enum espconn_type type;
		int remote_port;
		int local_port;
		uint8 local_ip[4];
		uint8 remote_ip[4];
	} connect_footprint;
	
	typedef struct _messages_queue_ {
		uint8  *data;
		uint16  data_len;
		STAILQ_ENTRY(_messages_queue_) entries;
	} messages_queue;
	typedef STAILQ_HEAD(messages_queue_head, _messages_queue_) messages_queue_head;
	
	typedef struct _connections_queue_ {
		char *pURL;
		struct espconn *pConnection;
		connect_footprint footprint;
		void *extra;
		messages_queue_head messages;
		STAILQ_ENTRY(_connections_queue_) entries;
	} connections_queue;
	
	typedef STAILQ_HEAD(connections_queue_head, _connections_queue_) connections_queue_head;
	
	typedef struct{
		connections_queue_head head;
		char name[20];
	} named_connection_queue;
	
	typedef struct _http_connection_queue_ {
		named_connection_queue *collection;
		STAILQ_ENTRY(_http_connection_queue_) entries;
	} http_connection_queue;
	
	void memory_info();
	void webserver_init();

	uint16 webserver_get_status();
	void webserver_set_status(uint16 status);

	void webserver_register_basic_authentication(basic_authentication_callback function);
	void webserver_register_chunk_callback(http_chunk_callback function, char *url);
	
	bool webserver_authentication(char *user, char *passwd);
	http_chunk_callback webserver_chunk_get();
	char *webserver_chunk_url();
	
	void webserver_register_handler_callback(char *url, http_handler_callback function);
	http_handler_callback webserver_find_handler(char *url);
	
	void webserver_set_response_header(char *name, char *value, bool overwrite);
	char *webserver_get_header(char *pData, char *header);
	bool webserver_check_token(char *data, char *token);
	
	void webserver_send_response(struct espconn *pConnection, char *pData);
	void connection_get_footprint(struct espconn *pConnection, connect_footprint *pFootprint);

	connections_queue *webserver_connection_store(named_connection_queue *collection, struct espconn *pConnection, char *pURL, uint32 timeout);
	void webserver_connection_clear(named_connection_queue *collection, struct espconn *pConnection);
	bool webserver_connection_match(struct espconn *pConnection, connect_footprint *pFootprint);
	connections_queue *webserver_connection_find(named_connection_queue *collection, struct espconn *pConnection);
	
	void webserver_queue_message(connections_queue *request, uint8 *data, uint16 data_len);
	messages_queue *webserver_fetch_message(connections_queue *request);
#endif
