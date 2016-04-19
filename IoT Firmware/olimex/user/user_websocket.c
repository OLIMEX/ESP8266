#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "ip_addr.h"
#include "espconn.h"
#include "mem.h"
#include "stdout.h"

#include "json/jsonparse.h"

#include "user_json.h"
#include "user_timer.h"
#include "user_misc.h"
#include "user_config.h"
#include "user_websocket.h"
#include "user_webserver.h"

LOCAL uint32 websocket_timeout_timer = 0;

named_connection_queue websockets = {
	.head = STAILQ_HEAD_INITIALIZER(websockets.head),
	.name = "WebSockets"
};

/******************************************************************************
 * FunctionName : websocket_opcode_str
 * Description  : convert opcode to human readable string
 * Parameters   : 
 * Returns      : string
*******************************************************************************/
LOCAL const char ICACHE_FLASH_ATTR *websocket_opcode_str(websocket_opcode opcode) {
	switch (opcode) {
		case WEBSOCKET_CONTINUE : return "CONTINUE";
		case WEBSOCKET_TEXT     : return "TEXT";
		case WEBSOCKET_BINARY   : return "BINARY";
		case WEBSOCKET_CLOSE    : return "CLOSE";
		case WEBSOCKET_PING     : return "PING";
		case WEBSOCKET_PONG     : return "PONG";
	}
	
	return "UNKNOWN";
}

/******************************************************************************
 * FunctionName : websocket_apply_mask
 * Description  : Apply mask to the message
 * Parameters   : 
*******************************************************************************/
LOCAL uint8 ICACHE_FLASH_ATTR websocket_apply_mask(websocket_header *header, uint8 *payload, uint16 payload_len, uint8 m) {
	if (header->masked && payload != NULL && payload_len > 0) {
		uint16 i;
		for (i=0; i < payload_len; i++) {
			payload[i] = payload[i] ^ header->mask[m];
			m = (m+1) % 4;
		}
	}
	
	return m;
}

/******************************************************************************
 * FunctionName : websocket_set_header
 * Description  : Converts websocket_header structure to its actual representation
 * Parameters   : 
*******************************************************************************/
LOCAL uint16 ICACHE_FLASH_ATTR websocket_set_header(websocket_header header, uint8 *frame) {
	uint16 i=0;
	
	frame[i++] = header.fin << 7  | header.opcode;
	
	if (header.payload_len < 126) {
		frame[i++] = header.masked << 7 | header.payload_len;
	} else {
		frame[i++] = header.masked << 7 | 126;
		frame[i++] = header.payload_len / 256;
		frame[i++] = header.payload_len % 256;
	}
	
	if (header.masked) {
		uint8 j;
		for (j=0; j<4; j++) {
			frame[i++] = header.mask[j];
		}
	}
	
	return i;
}

/******************************************************************************
 * FunctionName : websocket_get_header
 * Description  : Converts frame data to websocket_header structure
 * Parameters   : 
*******************************************************************************/
LOCAL uint16 ICACHE_FLASH_ATTR websocket_get_header(websocket_header *header, uint8 *frame) {
	os_memset(header, 0, sizeof(websocket_header));
	
	uint16 i=0;
	uint8 j;
	
	header->fin = frame[i] >> 7;
	header->opcode = frame[i++] & 0x0F;
	
	header->masked = frame[i] >> 7;
	header->payload_len = frame[i++] & 0x7F;
	
	if (header->payload_len > 125) {
		uint8 s = (header->payload_len == 126) ? 2 : 8;
		header->payload_len = 0;
		for (j=0; j<s; j++) {
			header->payload_len = header->payload_len * 256 + frame[i++];
		}
	}
	
	if (header->masked) {
		for (j=0; j<4; j++) {
			header->mask[j] = frame[i++];
		}
	}
	
#if WEBSOCKET_DEBUG
#if WEBSOCKET_VERBOSE_OUTPUT
	debug("WebSocket: HEADER:\n");
	for (j=0; j<i; j++) {
		debug("%02X ", frame[j]);
	}
	debug("\n\n");
#endif
#endif
	
	return i;
}

LOCAL void ICACHE_FLASH_ATTR data_send(struct espconn *pConnection, uint8 *data, uint16 data_len) {
#if SSL_ENABLE
	if (pConnection->proto.tcp->local_port == WEBSERVER_SSL_PORT) {
		espconn_secure_send(pConnection, data, data_len);
	} else {
		espconn_send(pConnection, data, data_len);
	}
#else
	espconn_send(pConnection, data, data_len);
#endif
}

/******************************************************************************
 * FunctionName : websocket_send
 * Description  : Send Message 
 * Parameters   : 
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR websocket_send(connections_queue *request, struct espconn *pConnection, websocket_opcode opcode, uint8 *data, uint16 data_len) {
	websocket_header header;
	websocket_extra *extra = request->extra;
	
	header.fin = 1;
	header.opcode = opcode;
	header.masked = (extra->type == WEBSOCKET_CLIENT);
	if (header.masked) {
		uint8 i;
		for (i=0; i<4; i++) {
			header.mask[i] = rand() % 256;
		}
	}
	header.payload_len = data_len;
	
	uint8 *frame = (uint8 *)os_zalloc(4 + (header.masked ? 4 : 0) + data_len);
	uint16 header_len = websocket_set_header(header, frame);
	
	if (data != NULL && data_len > 0) {
		os_memcpy(frame + header_len, data, data_len);
		websocket_apply_mask(&header, frame + header_len, data_len, 0);
	}
	
	uint16 frame_len = header_len + data_len;
	
	if (extra->sending) {
		webserver_queue_message(request, frame, frame_len);
		return;
	}
	
	extra->sending = true;
	
	data_send(pConnection, frame, frame_len);
	os_free(frame);
}

void ICACHE_FLASH_ATTR websocket_sent(void *arg) {
	struct espconn *pConnection = arg;
	connections_queue *request;
	websocket_extra *extra;
	
#if WEBSOCKET_DEBUG
	debug("WebSocket: Sent\n");
#endif
	STAILQ_FOREACH(request, &(websockets.head), entries) {
		extra = request->extra;
		if (webserver_connection_match(pConnection, &(request->footprint))) {
			messages_queue *message;
			message = webserver_fetch_message(request);
			if (message) {
				data_send(pConnection, message->data, message->data_len);
				os_free(message->data);
				os_free(message);
				return;
			} else {
				extra->sending = false;
			}
		}
	}
}

/******************************************************************************
 * FunctionName : websocket_ping
 * Description  : Send WebSocket ping frame
 * Parameters   : 
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR websocket_ping(connections_queue *request, struct espconn *pConnection, char *msg) {
	websocket_extra *extra = request->extra;
	if (extra->state != WEBSOCKET_OPEN) {
		return;
	}
	
#if WEBSOCKET_DEBUG
	debug(
		"WebSocket: Send PING [%s] [%d.%d.%d.%d:%d]\n",
		msg,
		IP2STR(pConnection->proto.tcp->remote_ip),
		pConnection->proto.tcp->remote_port
	);
#endif
	extra->timeout = true;
	websocket_send(request, pConnection, WEBSOCKET_PING, msg, msg ? os_strlen(msg) : 0);
}

/******************************************************************************
 * FunctionName : websocket_pong
 * Description  : Send WebSocket pong frame
 * Parameters   : 
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR websocket_pong(connections_queue *request, struct espconn *pConnection, char *msg) {
	websocket_extra *extra = request->extra;
	if (extra->state != WEBSOCKET_OPEN) {
		return;
	}
	
#if WEBSOCKET_DEBUG
	debug(
		"WebSocket: Send PONG [%s] [%d.%d.%d.%d:%d]\n",
		msg,
		IP2STR(pConnection->proto.tcp->remote_ip),
		pConnection->proto.tcp->remote_port
	);
#endif
	websocket_send(request, pConnection, WEBSOCKET_PONG, msg, msg ? os_strlen(msg) : 0);
}

/******************************************************************************
 * FunctionName : websocket_close
 * Description  : Send WebSocket pong frame
 * Parameters   : 
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR websocket_close(connections_queue *request, struct espconn *pConnection, uint16 status, const char *msg) {
	websocket_extra *extra = request->extra;
	switch (extra->state) {
		case WEBSOCKET_OPEN:
#if WEBSOCKET_DEBUG
			debug("WebSocket: State CLOSING\n");
#endif
			extra->state = WEBSOCKET_CLOSING;
			webserver_free_message_queue(request);
		break;
		
		case WEBSOCKET_CLOSING:
#if WEBSOCKET_DEBUG
			debug("WebSocket: State CLOSED\n");
#endif
			extra->state = WEBSOCKET_CLOSED;
			setTimeout(
#if SSL_ENABLE	
				pConnection->proto.tcp->local_port == WEBSERVER_SSL_PORT ?
					(os_timer_func_t *)espconn_secure_disconnect
					:
#endif
					(os_timer_func_t *)espconn_disconnect
				, 
				pConnection,
				100
			);
			return;
		break;
		
		case WEBSOCKET_CLOSED:
			return;
		break;
	}
	
#if WEBSOCKET_DEBUG
	debug(
		"WebSocket: Send CLOSE [%d] [%s] [%d.%d.%d.%d:%d]\n",
		status,
		msg,
		IP2STR(pConnection->proto.tcp->remote_ip),
		pConnection->proto.tcp->remote_port
	);
#endif
	
	uint16 len = 2 + (msg ? os_strlen(msg) : 0);
	uint8 data[len];
	
	data[0] = status / 256;
	data[1] = status % 256;
	
	if (msg) {
		os_memcpy(data + 2, msg, len - 2);
	}
	
	websocket_send(request, pConnection, WEBSOCKET_CLOSE, data, len);
}

/******************************************************************************
 * FunctionName : websocket_authentication
 * Description  : Check for authentication message
 * Parameters   : 
*******************************************************************************/
LOCAL bool ICACHE_FLASH_ATTR websocket_authentication(char *msg) {
	struct jsonparse_state parser;
	int json_type;
	
	char user[USER_CONFIG_USER_SIZE] = "";
	char password[USER_CONFIG_USER_SIZE] = "";
	
	bool unknown_property = false;

	jsonparse_setup(&parser, msg, os_strlen(msg));
	while ((json_type = jsonparse_next(&parser)) != 0) {
		if (json_type == JSON_TYPE_PAIR_NAME) {
			if (jsonparse_strcmp_value(&parser, "User") == 0) {
				jsonparse_next(&parser);
				jsonparse_next(&parser);
				jsonparse_copy_value(&parser, user, USER_CONFIG_USER_SIZE);
			} else if (jsonparse_strcmp_value(&parser, "Password") == 0) {
				jsonparse_next(&parser);
				jsonparse_next(&parser);
				jsonparse_copy_value(&parser, password, USER_CONFIG_USER_SIZE);
			} else {
				unknown_property = true;
			}
		}
	}
	
	if (unknown_property) {
		return false;
	}
	
	return webserver_authentication(user, password);
}

/******************************************************************************
 * FunctionName : websocket_handle_message
 * Description  : Handle received WebSocket text message
 * Parameters   : 
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR websocket_handle_message(connections_queue *request, struct espconn *pConnection, char *msg, uint16 msg_len, uint32 content_len, uint16 chunkID, bool isLast) {
	websocket_extra *extra = request->extra;
	
	if (msg == NULL || msg_len == 0) {
		return;
	}
	
	if (!extra->authorized) {
		if (websocket_authentication(msg)) {
			char status[WEBSERVER_MAX_VALUE];
			extra->authorized = true;
			websocket_send_message(request->pURL, json_status(status, ESP8266, "Authorization success", NULL), pConnection);
		} else {
#if WEBSOCKET_DEBUG
			debug("WebSocket: Unauthorized Message [%s] [%s]\n", request->pURL, msg);
#endif
			websocket_close(request, pConnection, 1008, "Unauthorized");
		}
		return;
	}
	
#if WEBSOCKET_DEBUG
#if WEBSOCKET_VERBOSE_OUTPUT
	debug("WebSocket: Message [%s] [%d] [%s]\n", request->pURL, msg_len, msg);
#endif
#endif
	
	if (chunkID != 0) {
		http_chunk_callback webserver_chunk = webserver_chunk_get();
		if (webserver_chunk) {
			(*webserver_chunk)(pConnection, msg, msg_len, chunkID, isLast);
			return;
		}
		
		debug(
			"WebSocket: IGNORED chunked request [%d][%d] [%d.%d.%d.%d:%d]\n", 
			chunkID, 
			msg_len, 
			IP2STR(pConnection->proto.tcp->remote_ip), 
			pConnection->proto.tcp->local_port
		);
		return;
	}
	
	struct jsonparse_state parser;
	int json_type;
	
	char cURL[WEBSERVER_MAX_VALUE] = "";
	char cMethod[WEBSERVER_MAX_VALUE] = "";
	char cData[WEBSERVER_MAX_PACKET_LEN] = "";
	request_method method = UNKNOWN;

	jsonparse_setup(&parser, msg, os_strlen(msg));
	while ((json_type = jsonparse_next(&parser)) != 0) {
		if (json_type == JSON_TYPE_PAIR_NAME) {
			if (jsonparse_strcmp_value(&parser, "URL") == 0) {
				jsonparse_next(&parser);
				jsonparse_next(&parser);
				jsonparse_copy_value(&parser, cURL, WEBSERVER_MAX_VALUE);
			} else if (jsonparse_strcmp_value(&parser, "Method") == 0) {
				jsonparse_next(&parser);
				jsonparse_next(&parser);
				jsonparse_copy_value(&parser, cMethod, WEBSERVER_MAX_VALUE);
				if (os_strcmp(cMethod, "GET") == 0) {
					method = GET;
				} else if (os_strcmp(cMethod, "POST") == 0) {
					method = POST;
				}
			} else if (jsonparse_strcmp_value(&parser, "Data") == 0) {
				jsonparse_next(&parser);
				jsonparse_next(&parser);
				jsonparse_object_str(&parser, cData, WEBSERVER_MAX_PACKET_LEN);
			}
		}
	}
	
#if WEBSOCKET_DEBUG
#if WEBSOCKET_VERBOSE_OUTPUT
	debug("WebSocket: Message [%s] [%s] [%s]\n", cURL, cMethod, cData);
#else
	debug("WebSocket: Message [%s] [%s]\n", cURL, cMethod);
#endif
#endif
	
	char response[WEBSERVER_MAX_RESPONSE_LEN];
	os_memset(response, '\0', sizeof(response));
	
	http_handler_callback callback;
	callback = webserver_find_handler(cURL);
	
	if (callback == NULL) {
		char error[WEBSERVER_MAX_VALUE];
		user_websocket_event(cURL, json_error(error, ESP8266, "URL Not Found", NULL), pConnection);
		return;
	}
	
	content_len = content_len - os_strlen(msg) + os_strlen(cData);
	
	uint16 data_len = os_strlen(cData);
	uint16 extra_len = msg_len - os_strlen(msg);
	
	if (extra_len > 0) {
		os_memcpy(cData + data_len + 1, msg + os_strlen(msg) + 1, extra_len);
		data_len += extra_len;
	}
	
	(*callback)(pConnection, method, cURL, cData, data_len, content_len, response, sizeof(response));
	if (os_strlen(response) == 0) {
		return;
	}
	
	if (method == POST) {
		user_event_raise(cURL, response);
	} else {
		user_websocket_event(cURL, response, pConnection);
	}
}

/******************************************************************************
 * FunctionName : websocket_recv
 * Description  : Receive WebSocket frame
 * Parameters   : 
 * Returns      : boolean
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR websocket_recv(connections_queue *request, struct espconn *pConnection, char *frame, unsigned short frame_len) {
	LOCAL _uint64_ requestTotalLen = 0;
	LOCAL uint16 chunkID = 0;
	LOCAL connect_footprint footprint;
	LOCAL uint8 m = 0;
	
	LOCAL websocket_header header;
	websocket_extra *extra = request->extra;
	
recursion:
	extra->timeout = false;
	uint16 header_len;
	uint16 data_len;
	bool frame_split = false;
	
	if (frame == NULL || frame_len == 0) {
		websocket_close(request, pConnection, 1002 , "No data received");
		requestTotalLen = 0;
		return;
	}
	
	if (requestTotalLen == 0) {
		header_len = websocket_get_header(&header, frame);
		m = 0;
		
		requestTotalLen = header_len + header.payload_len;
		frame_split = (frame_len > requestTotalLen);
		
		data_len = frame_split ?
			header.payload_len
			:
			frame_len - header_len
		;
		
		if (header.opcode != WEBSOCKET_CONTINUE) {
			chunkID = 0;
			webserver_register_chunk_callback(NULL, NULL);
			connection_get_footprint(pConnection, &footprint);
		}
#if WEBSOCKET_DEBUG
#if WEBSOCKET_VERBOSE_OUTPUT
		debug("WebSocket: Request Total Len [%d]\n", requestTotalLen);
#endif
#endif
	} else {
		if (!webserver_connection_match(pConnection, &footprint)) {
#if WEBSOCKET_DEBUG
			debug("WebSocket: IGNORED new request during chunk data handle\n");
#endif
			return;
		}
		
		frame_split = (frame_len > requestTotalLen);
		
		chunkID++;
		header_len = 0;
		
		data_len = frame_split ?
			requestTotalLen
			:
			frame_len
		;
	}
	
	requestTotalLen = frame_split ?
		0
		:
		requestTotalLen - frame_len
	;
	
#if WEBSOCKET_DEBUG
#if WEBSOCKET_VERBOSE_OUTPUT
	debug("WebSocket: Chunk Len [%d]\n", frame_len);
	debug("WebSocket: Header Len [%d]\n", header_len);
	debug("WebSocket: Data Len [%d]\n", data_len);
	debug("WebSocket: Request Remaining [%d]\n", requestTotalLen);
#endif
#endif
	
	if (extra->state != WEBSOCKET_OPEN && header.opcode < 8) {
		debug("WebSocket: IGNORING %d bytes\n", frame_len);
		return;
	}
	
	if (header.masked && extra->type == WEBSOCKET_CLIENT) {
		websocket_close(request, pConnection, 1002 , "Masked frame received");
		return;
	}
	
	if (!header.masked && extra->type == WEBSOCKET_SERVER) {
		websocket_close(request, pConnection, 1002 , "Not masked frame received");
		return;
	}
	
	uint8 *data = NULL;
	
	if (data_len > 0) {
		uint16 i = 0;
		data = (uint8 *)os_zalloc(data_len + 1);
		os_memcpy(data, frame + header_len, data_len);
		
		m = websocket_apply_mask(&header, data, data_len, m);
	}
	
	if (header.opcode == WEBSOCKET_CLOSE) {
		uint16 status = 0;
		char *msg = NULL;
		
		if (data) {
			status = data[0] * 256 + data[1];
			if (data_len > 2) {
				msg = data + 2;
			}
		}
		
#if WEBSOCKET_DEBUG
		debug(
			"WebSocket: Received CLOSE [%d] [%s] [%d.%d.%d.%d:%d]\n",
			status,
			msg,
			IP2STR(pConnection->proto.tcp->remote_ip),
			pConnection->proto.tcp->remote_port
		);
#endif
		websocket_close(request, pConnection, status, msg);
		
		if (extra->state == WEBSOCKET_CLOSING) {
			websocket_close(request, pConnection, status, msg);
		}
		goto clear;
	}
	
#if WEBSOCKET_DEBUG
	uint8 debug_msg[WEBSERVER_MAX_NAME];
	os_sprintf(debug_msg, "%d bytes", data_len);
	
	debug(
		"WebSocket: Received %s [%s] [%d.%d.%d.%d:%d]\n", 
		websocket_opcode_str(header.opcode), 
		(header.opcode > 7 || WEBSOCKET_VERBOSE_OUTPUT) ?
			data
			:
			debug_msg
		,
		IP2STR(pConnection->proto.tcp->remote_ip),
		pConnection->proto.tcp->remote_port
	);
#endif
	
	if (header.opcode == WEBSOCKET_PING) {
		websocket_pong(request, pConnection, data);
		goto clear;
	}
	
	if (header.opcode == WEBSOCKET_TEXT || header.opcode == WEBSOCKET_BINARY || header.opcode == WEBSOCKET_CONTINUE) {
		websocket_handle_message(request, pConnection, data, data_len, header.payload_len, chunkID, header.fin && requestTotalLen == 0);
		goto clear;
	}
	
clear: 
	if (data) os_free(data);
	
	if (frame_split) {
#if WEBSOCKET_DEBUG
		debug("WebSocket: 2 frames - same packet\n");
#endif
		// Feed the watchdog to prevent reset
		system_soft_wdt_feed();
		
		// continue processing
		// websocket_recv(request, pConnection, frame + header_len + data_len, frame_len - header_len - data_len);
		frame = frame + header_len + data_len;
		frame_len = frame_len - header_len - data_len;
		goto recursion;
	}
}

/******************************************************************************
 * FunctionName : websocket_debug_message
 * Description  : send message to client
 * Parameters   : 
 * Returns      : boolean
*******************************************************************************/
void ICACHE_FLASH_ATTR websocket_debug_message(char *pData) {
	connections_queue *request;
	websocket_extra *extra;
	
	if (pData == NULL) {
		return;
	}
	
	STAILQ_FOREACH(request, &(websockets.head), entries) {
		extra = request->extra;
		if (extra->authorized && extra->state == WEBSOCKET_OPEN) {
			websocket_send(request, request->pConnection, WEBSOCKET_TEXT, pData, os_strlen(pData));
		}
	}
}

/******************************************************************************
 * FunctionName : websocket_send_message
 * Description  : send message to client
 * Parameters   : 
 * Returns      : boolean
*******************************************************************************/
void ICACHE_FLASH_ATTR websocket_send_message(char *pURL, char *pData, struct espconn *pConnection) {
	connections_queue *request;
	websocket_extra *extra;
	
	if (pData == NULL) {
		return;
	}
	
	STAILQ_FOREACH(request, &(websockets.head), entries) {
		extra = request->extra;
		if (
			extra->authorized && 
			extra->state == WEBSOCKET_OPEN &&
			(
				(pConnection == NULL && extra->type == WEBSOCKET_SERVER) || 
				webserver_connection_match(pConnection, &(request->footprint))
			) &&
			(pURL == NULL || os_strcmp(pURL, request->pURL) == 0)
		) {
#if WEBSOCKET_DEBUG
			char debug_msg[WEBSERVER_MAX_NAME];
			os_sprintf(debug_msg, "%d bytes", os_strlen(pData));
			
			debug(
				"WebSocket: Send %s [%s] [%d.%d.%d.%d:%d]\n", 
				websocket_opcode_str(WEBSOCKET_TEXT), 
				WEBSOCKET_VERBOSE_OUTPUT ? 
					pData 
					: 
					debug_msg
				,
				IP2STR(request->pConnection->proto.tcp->remote_ip),
				request->pConnection->proto.tcp->remote_port
			);
#endif
			websocket_send(request, request->pConnection, WEBSOCKET_TEXT, pData, os_strlen(pData));
		}
	}
}

/******************************************************************************
 * FunctionName : websocket_close_all
 * Description  : Close all active connections
 * Parameters   : 
 * Returns      : boolean
*******************************************************************************/
void ICACHE_FLASH_ATTR websocket_close_all(const char *reason, struct espconn *pConnection) {
	connections_queue *request;
	
	STAILQ_FOREACH(request, &(websockets.head), entries) {
		if (pConnection == NULL || !webserver_connection_match(pConnection, &(request->footprint))) {
			websocket_close(request, request->pConnection, 1001, reason);
		}
	}
}

/******************************************************************************
 * FunctionName : websocket_timeout
 * Description  : check if client respond to ping keep alive within timeout
 * Parameters   : 
 * Returns      : boolean
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR websocket_timeout(void *arg) {
	connections_queue *request;
	
	STAILQ_FOREACH(request, &(websockets.head), entries) {
		websocket_extra *extra = request->extra;
		if (extra->timeout) {
			websocket_close(request, request->pConnection, 1002, "Timeout");
		}
	}
}

/******************************************************************************
 * FunctionName : websocket_keep_alive
 * Description  : ping client to keep connection alive
 * Parameters   : 
 * Returns      : boolean
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR websocket_keep_alive(void *arg) {
	connections_queue *request;
	
	http_chunk_callback webserver_chunk = webserver_chunk_get();
	if (webserver_chunk) {
		clearTimeout(websocket_timeout_timer);
		return;
	}
	
	bool f = true;
	STAILQ_FOREACH(request, &(websockets.head), entries) {
#if WEBSOCKET_DEBUG	
		if (f) debug("\n");
#endif
		websocket_ping(request, request->pConnection, "Keep-Alive");
		f = false;
	}
#if WEBSOCKET_DEBUG	
	if (!f) debug("\n");
#endif
	
	websocket_timeout_timer = setTimeout(websocket_timeout, NULL, WEBSOCKET_TIMEOUT * 1000);
}

char ICACHE_FLASH_ATTR *websocket_accept(char *key) {
	uint8 digest[SHA1_SIZE];
	
	char *accept = os_strcat(
		os_strcat(
			(char *)os_zalloc(os_strlen(key) + 36 + 1), 
			key
		),
		"258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
	);
	
	char *b64Accept = (char *)os_zalloc(30);
	
	sha1(accept, os_strlen(accept), digest);
	base64_encode(digest, sizeof(digest), b64Accept, 30);
	
	os_free(accept);
	return b64Accept;
}

/******************************************************************************
 * FunctionName : websocket_server_upgrade
 * Description  : check for WebSocket upgrade handshake
 * Parameters   : 
 * Returns      : boolean
*******************************************************************************/
bool ICACHE_FLASH_ATTR websocket_server_upgrade(struct espconn *pConnection, char *pURL, char *pData) {
	char *hConnection = NULL;
	char *hUpgrade = NULL;
	char *hVersion = NULL;
	char *hKey = NULL;
	char *hAccept = NULL;
	
	bool result = false;
	
	hConnection = webserver_get_header(pData, "Connection");
	if (hConnection == NULL || !webserver_check_token(hConnection, "Upgrade")) {
		goto clean;
	}
	
	hUpgrade = webserver_get_header(pData, "Upgrade");
	if (hUpgrade == NULL || !webserver_check_token(hUpgrade, "WebSocket")) {
		goto clean;
	}
	
	hVersion = webserver_get_header(pData, "Sec-WebSocket-Version");
	if (hVersion == NULL || os_strcmp(hVersion, "13") != 0) {
		goto clean;
	}
	
	hKey = webserver_get_header(pData, "Sec-WebSocket-Key");
	if (hKey == NULL) {
		goto clean;
	}
	
	hAccept = websocket_accept(hKey);
	
	webserver_set_status(101);
	webserver_set_response_header("Connection", "Upgrade", true);
	webserver_set_response_header("Upgrade", "websocket", true);
	webserver_set_response_header("Sec-WebSocket-Accept", hAccept, true);
	webserver_send_response(pConnection, NULL);
	
	debug(
		"WebSocket: Accepted new connection [%s] [%d.%d.%d.%d:%d]\n",
		pURL,
		IP2STR(pConnection->proto.tcp->remote_ip),
		pConnection->proto.tcp->remote_port
	);

	bool ssl = pConnection->proto.tcp->local_port == WEBSERVER_SSL_PORT;
	uint32 keep_alive = ssl ? WEBSOCKET_KEEP_ALIVE * 4 : WEBSOCKET_KEEP_ALIVE;
	connections_queue *request = webserver_connection_store(&websockets, pConnection, pURL, keep_alive);
	request->extra = (websocket_extra *)os_zalloc(sizeof(websocket_extra));
	websocket_extra *extra = request->extra;
	extra->authorized = !user_config_authentication();
	extra->timeout = false;
	extra->sending = false;
	extra->type = WEBSOCKET_SERVER;
	extra->state = WEBSOCKET_OPEN;
	
	espconn_regist_sentcb(pConnection, websocket_sent);
	
	result = true;
	
	clean:
		if (hConnection) os_free(hConnection);
		if (hUpgrade) os_free(hUpgrade);
		if (hVersion) os_free(hVersion);
		if (hKey) os_free(hKey);
		if (hAccept) os_free(hAccept);
	return result;
}

/******************************************************************************
 * FunctionName : websocket_client_upgrade
 * Description  : check for WebSocket upgrade handshake
 * Parameters   : 
 * Returns      : boolean
*******************************************************************************/
bool ICACHE_FLASH_ATTR websocket_client_upgrade(struct espconn *pConnection, char *pURL, char *pData, char *request_headers) {
	char *hConnection = NULL;
	char *hUpgrade = NULL;
	char *hKey = NULL;
	char *hAccept = NULL;
	char *hAcceptCheck = NULL;
	
	bool result = false;
	
	hConnection = webserver_get_header(pData, "Connection");
	if (hConnection == NULL || !webserver_check_token(hConnection, "Upgrade")) {
		goto clean;
	}
	
	hUpgrade = webserver_get_header(pData, "Upgrade");
	if (hUpgrade == NULL || os_strcmp(hUpgrade, "websocket") != 0) {
		goto clean;
	}
	
	hKey = webserver_get_header(request_headers, "Sec-WebSocket-Key");
	if (hKey == NULL) {
		goto clean;
	}
	
	hAccept = webserver_get_header(pData, "Sec-WebSocket-Accept");
	if (hAccept == NULL) {
		goto clean;
	}
	
	hAcceptCheck = websocket_accept(hKey);
	if (os_strcmp(hAccept, hAcceptCheck) != 0) {
		goto clean;
	}
	
	debug(
		"WebSocket: Open new connection [%s] [%d.%d.%d.%d:%d]\n",
		pURL,
		IP2STR(pConnection->proto.tcp->remote_ip),
		pConnection->proto.tcp->remote_port
	);
	
	bool ssl = pConnection->proto.tcp->local_port == WEBSERVER_SSL_PORT;
	uint32 keep_alive = ssl ? WEBSOCKET_KEEP_ALIVE * 4 : WEBSOCKET_KEEP_ALIVE;
	connections_queue *request = webserver_connection_store(&websockets, pConnection, pURL, keep_alive);
	request->extra = (websocket_extra *)os_zalloc(sizeof(websocket_extra));
	websocket_extra *extra = request->extra;
	extra->authorized = true;
	extra->timeout = false;
	extra->sending = false;
	extra->type = WEBSOCKET_CLIENT;
	extra->state = WEBSOCKET_OPEN;
	
	espconn_regist_sentcb(pConnection, websocket_sent);
	
	result = true;
	
	clean:
		if (hConnection) os_free(hConnection);
		if (hUpgrade) os_free(hUpgrade);
		if (hKey) os_free(hKey);
		if (hAccept) os_free(hAccept);
		if (hAcceptCheck) os_free(hAcceptCheck);
	return result;
}

/******************************************************************************
 * FunctionName : is_websocket
 * Description  : check if connection is switched to WebSocket
 * Returns	    : boolean
*******************************************************************************/
bool ICACHE_FLASH_ATTR is_websocket(struct espconn *pConnection) {
	connections_queue *request;
	
	STAILQ_FOREACH(request, &(websockets.head), entries) {
		if (webserver_connection_match(pConnection, &(request->footprint))) {
			return true;
		}
	}
	
	return false;
}

/******************************************************************************
 * FunctionName : websocket_switched
 * Description  : handle data received if connection is switched to WebSocket
 * Returns	    : boolean
*******************************************************************************/
bool ICACHE_FLASH_ATTR websocket_switched(struct espconn *pConnection, char *pData, unsigned short length) {
	connections_queue *request;
	
	STAILQ_FOREACH(request, &(websockets.head), entries) {
		if (webserver_connection_match(pConnection, &(request->footprint))) {
			websocket_recv(request, pConnection, pData, length);
			return true;
		}
	}
	
	return false;
}

void ICACHE_FLASH_ATTR websocket_connection_clear(struct espconn *pConnection) {
	webserver_connection_clear(&websockets, pConnection);
}

/******************************************************************************
 * FunctionName : websocket_init
 * Description  : initialize as websocket keep-alive timer
 * Returns	    : none
*******************************************************************************/
void ICACHE_FLASH_ATTR websocket_init() {
	setInterval(websocket_keep_alive, NULL, (WEBSOCKET_KEEP_ALIVE - 1) * 1000);
}
