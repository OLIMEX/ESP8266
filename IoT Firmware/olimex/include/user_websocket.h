#ifndef __USER_WEBSOCKET_H__
	#define __USER_WEBSOCKET_H__
	
	#include "c_types.h"
	#include "os_type.h"
	#include "espconn.h"
	#include "user_misc.h"
	
	#define WEBSOCKET_DEBUG            0
	#define WEBSOCKET_VERBOSE_OUTPUT   0
	
	#define WEBSOCKET_KEEP_ALIVE       20
	#define WEBSOCKET_TIMEOUT          5
	
	typedef struct _websocket_header_ {
		bool     fin;
		uint8    opcode;
		bool     masked;
		_uint64_ payload_len;
		uint8    mask[4];
	} websocket_header;
	
	typedef enum _websocket_opcode_ {
		WEBSOCKET_CONTINUE = 0x0,
		WEBSOCKET_TEXT     = 0x1,
		WEBSOCKET_BINARY   = 0x2,
		WEBSOCKET_CLOSE    = 0x8,
		WEBSOCKET_PING     = 0x9,
		WEBSOCKET_PONG     = 0xA
	} websocket_opcode;
	
	typedef enum _websocket_type_ {
		WEBSOCKET_CLIENT = 0,
		WEBSOCKET_SERVER
	} websocket_type;
	
	typedef enum _websocket_state_ {
		WEBSOCKET_OPEN = 0,
		WEBSOCKET_CLOSING,
		WEBSOCKET_CLOSED
	} websocket_state;
	
	typedef struct {
		bool authorized;
		bool timeout;
		bool sending;
		websocket_type type;
		websocket_state state;
	} websocket_extra;
	
	bool websocket_server_upgrade(struct espconn *pConnection, char *pURL, char *pData);
	bool websocket_client_upgrade(struct espconn *pConnection, char *pURL, char *pData, char *request_headers);
	
	bool is_websocket(struct espconn *pConnection);
	bool websocket_switched(struct espconn *pConnection, char *pData, unsigned short length);
	void websocket_sent(void *arg);
	
	void websocket_send_message(char *pURL, char *pData, struct espconn *pConnection);
	void websocket_debug_message(char *pData);
	void websocket_close_all(const char *reason, struct espconn *pConnection);
#endif
