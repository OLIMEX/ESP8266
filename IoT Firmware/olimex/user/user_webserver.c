#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "mem.h"
#include "queue.h"
#include "user_interface.h"

#include "espconn.h"
#include "stdout.h"

#include "user_misc.h"
#include "user_json.h"
#include "user_config.h"
#include "user_events.h"
#include "user_i2c_scan.h"
#include "user_devices.h"
#include "user_webserver.h"

#include "user_long_poll.h"
#include "user_websocket.h"

LOCAL basic_authentication_callback basic_authentication = NULL;
LOCAL http_chunk_callback webserver_chunk = NULL;
LOCAL char *chunk_url = NULL;

LOCAL uint16 http_status = 200;

STAILQ_HEAD(http_headers, _http_header_) http_headers = STAILQ_HEAD_INITIALIZER(http_headers);
STAILQ_HEAD(http_handler_callbacks, _http_callback_) http_handler_callbacks = STAILQ_HEAD_INITIALIZER(http_handler_callbacks);
STAILQ_HEAD(http_queues, _http_connection_queue_) http_queues = STAILQ_HEAD_INITIALIZER(http_queues);

/******************************************************************************
 * FunctionName : webserver_find_handler
 * Description  : search for registered HTTP request handler
 * Parameters   : url
 * Returns      : pointer to the registered callback
*******************************************************************************/
http_handler_callback ICACHE_FLASH_ATTR webserver_find_handler(char *url) {
	http_callback *callback;
	STAILQ_FOREACH(callback, &http_handler_callbacks, entries) {
		if (str_match(callback->url, url)) {
			return callback->handler;
		}
	}
	
	return NULL;
}

/******************************************************************************
 * FunctionName : webserver_register_handler_callback
 * Description  : register HTTP request handler
 * Parameters   : url 
 *              : callback function
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR webserver_register_handler_callback(char *url, http_handler_callback function) {
	if (webserver_find_handler(url)) {
		debug("WARNING: Handler callback [%s] already registered\n", url);
		return;
	}
	
	http_callback *callback = (http_callback *)os_zalloc(sizeof(http_callback));
	
	callback->url     = url;
	callback->handler = function;
	
	STAILQ_INSERT_TAIL(&http_handler_callbacks, callback, entries);
}

void ICACHE_FLASH_ATTR default_handler_done(i2c_devices_queue *i2c) {
	char response[WEBSERVER_MAX_RESPONSE_LEN * 2];
	char data[WEBSERVER_MAX_RESPONSE_LEN * 2];
	
	char urls[WEBSERVER_MAX_RESPONSE_LEN] = "";
	char devices[WEBSERVER_MAX_RESPONSE_LEN] = "";
	
	char current[WEBSERVER_MAX_VALUE] = "";
	
	http_callback *callback;
	STAILQ_FOREACH(callback, &http_handler_callbacks, entries) {
		os_sprintf(current, "%s\"%s\"", urls[0] == '\0' ? "" : ", ", callback->url);
		os_strcat(urls, current);
	}
	
	json_data(
		response, ESP8266, OK_STR, 
		json_sprintf(
			data,
			"\"URLs\" : [%s], "
			"\"Devices\" : [%s]",
			urls,
			devices_scan_result(i2c, devices)
		),
		NULL
	);
	
	user_event_raise("/", response);
}

void ICACHE_FLASH_ATTR default_handler(
	struct espconn *pConnection, 
	request_method method, 
	char *url, 
	char *data, 
	uint16 data_len, 
	uint32 content_len, 
	char *response,
	uint16 response_len
) {
	webserver_set_status(0);
	i2c_scan_start(default_handler_done);
}

uint16 ICACHE_FLASH_ATTR webserver_get_status() {
	return http_status;
}

void ICACHE_FLASH_ATTR webserver_set_status(uint16 status) {
	http_status = status;
}

/******************************************************************************
 * FunctionName : webserver_find_header
 * Description  : search for set HTTP request header
 * Parameters   : name
 * Returns      : pointer to the set header
*******************************************************************************/
LOCAL http_header ICACHE_FLASH_ATTR *webserver_find_header(char *name) {
	http_header *header;
	
	STAILQ_FOREACH(header, &http_headers, entries) {
		if (os_strcmp(header->name, name) == 0) {
			return header;
		}
	}
	
	return NULL;
}

/******************************************************************************
 * FunctionName : webserver_set_response_header
 * Description  : set HTTP header
 * Parameters   : name 
 *              : value
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR webserver_set_response_header(char *name, char *value, bool overwrite) {
	http_header *header;
	bool is_new = false;
	
	if (header = webserver_find_header(name)) {
		if (!overwrite) {
#if WEBSERVER_DEBUG
			debug("WARNING: Header [%s] already set\n", name);
#endif			
			return;
		}
	} else {
		header = (http_header *)os_zalloc(sizeof(http_header));
		is_new = true;
	}
	
	header->name  = name;
	header->value = value;
	
	if (is_new) {
		STAILQ_INSERT_TAIL(&http_headers, header, entries);
	}
}

/******************************************************************************
 * FunctionName : webserver_fetch_response_headers
 * Description  : fetches HTTP headers set by webserver_set_response_header
 * Parameters   : buffer to fetch data
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR webserver_fetch_response_headers(char *httphead) {
	http_header *header;
	
	STAILQ_FOREACH(header, &http_headers, entries) {
		os_sprintf(
			httphead + os_strlen(httphead),
			"%s: %s\r\n",
			header->name,
			header->value
		);
	}
	
	os_sprintf(
		httphead + os_strlen(httphead),
		"\r\n"
	);
	
	STAILQ_FOREACH(header, &http_headers, entries) {
		STAILQ_REMOVE(&http_headers, header, _http_header_, entries);
		os_free(header);
	}
	STAILQ_INIT(&http_headers);	
}

/******************************************************************************
 * FunctionName : webserver_get_header
 * Description  : get value of HTTP header - client MUST free memory allocated
 * Parameters   : 
 * Returns      : pointer to string
*******************************************************************************/
char ICACHE_FLASH_ATTR *webserver_get_header(char *pData, char *header) {
	char *pStart, *pEnd;
	char *headerSearch;
	
	headerSearch = os_strcat(
		os_strcat(
			os_strcat(
				(char *)os_zalloc(os_strlen(header)+3), 
				"\n"
			),
			header
		), 
		":"
	);
	
	pStart = (char *)strstr_end(pData, headerSearch);
	os_free(headerSearch);
	if (pStart == NULL) {
		return false;
	}
	
	while (*pStart == ' ') {
		pStart++;
	}
	
	pEnd = (char *)os_strstr(pStart, "\r\n");
	if (pEnd == NULL) {
		pEnd = pStart + os_strlen(pStart);
	}
	
	char *value;
	
	value = (char *)os_zalloc(pEnd - pStart + 1);
	os_memcpy(value, pStart, pEnd - pStart);
	
	return value;
}

/******************************************************************************
 * FunctionName : webserver_check_token
 * Description  : check if data contains token
 * Parameters   : 
 * Returns      : boolean
*******************************************************************************/
bool ICACHE_FLASH_ATTR webserver_check_token(char *data, char *token) {
	const char *delimiters = " ,:";
	char *token_start = token;
	do {
		while (*data != '\0' && os_strchr(delimiters, *data)) {
			data++;
		}
		
		while (*data != '\0' && *token != '\0' && tolower(*data) == tolower(*token)) {
			data++;
			token++;
		}
		
		if (
			tolower(*data) == tolower(*token) || 
			(*token == '\0' && os_strchr(delimiters, *data))
		) {
			return true;
		}
		token = token_start;
		
		while (*data != '\0' && !os_strchr(delimiters, *data)) {
			data++;
		}
		
	} while (*data != '\0');
	
	return false;
}

/******************************************************************************
 * FunctionName : connection_err_str
 * Description  : return error code as string
 * Parameters   : err
 * Returns      : 
*******************************************************************************/
const char ICACHE_FLASH_ATTR *connection_err_str(int err) {
	switch (err) {
		case ESPCONN_MEM        : return "Out of memory";
		case ESPCONN_TIMEOUT    : return "Timeout";
		case ESPCONN_RTE        : return "Routing problem";
		case ESPCONN_INPROGRESS : return "Operation in progress";
		case ESPCONN_ABRT       : return "Connection aborted";
		case ESPCONN_RST        : return "Connection reset";
		case ESPCONN_CLSD       : return "Connection closed";
		case ESPCONN_CONN       : return "Not connected";
		case ESPCONN_ARG        : return "Illegal argument";
		case ESPCONN_ISCONN     : return "Already connected";
	}
	
	return "Unknown";
}

/******************************************************************************
 * FunctionName : connection_state_str
 * Description  : return error code as string
 * Parameters   : err
 * Returns      : 
*******************************************************************************/
const char ICACHE_FLASH_ATTR *connection_state_str(enum espconn_state state) {
	switch (state) {
		case ESPCONN_NONE    : return "None";
		case ESPCONN_WAIT    : return "Wait";
		case ESPCONN_LISTEN  : return "Listen";
		case ESPCONN_CONNECT : return "Connect";
		case ESPCONN_WRITE   : return "Write";
		case ESPCONN_READ    : return "Read";
		case ESPCONN_CLOSE   : return "Close";
	}
	
	return "Unknown";
}

/******************************************************************************
 * FunctionName : connection_get_footprint
 * Description  : get connection footprint for later matching
 * Parameters   : pConnection
 *              : pFootprint
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR connection_get_footprint(struct espconn *pConnection, connect_footprint *pFootprint) {
	pFootprint->type = pConnection->type;
	pFootprint->remote_port = pConnection->proto.tcp->remote_port;
	pFootprint->local_port  = pConnection->proto.tcp->local_port;
	os_memcpy(pFootprint->local_ip, pConnection->proto.tcp->local_ip, sizeof(pConnection->proto.tcp->local_ip));
	os_memcpy(pFootprint->remote_ip, pConnection->proto.tcp->remote_ip, sizeof(pConnection->proto.tcp->remote_ip));
}

/******************************************************************************
 * FunctionName : webserver_connection_match
 * Description  : check if connection matches footprint
 * Parameters   : pConnection
 *              : pFootprint
 * Returns      : boolean
*******************************************************************************/
bool ICACHE_FLASH_ATTR webserver_connection_match(struct espconn *pConnection, connect_footprint *pFootprint) {
	uint8 i;
	
	if (pConnection == NULL) {
		return false;
	}
	
	if (pConnection->type != pFootprint->type) {
		return false;
	}
	
	if (pConnection->proto.tcp->local_port != pFootprint->local_port) {
		return false;
	}
	
	if (pConnection->proto.tcp->remote_port != pFootprint->remote_port) {
		return false;
	}
	
	for (i=0; i<4; i++) {
		if (pConnection->proto.tcp->local_ip[i] != pFootprint->local_ip[i]) {
			return false;
		}
	}
	
	for (i=0; i<4; i++) {
		if (pConnection->proto.tcp->remote_ip[i] != pFootprint->remote_ip[i]) {
			return false;
		}
	}
	
	return true;
}

/******************************************************************************
 * FunctionName : webserver_connection_find
 * Description  : 
 * Parameters   : collection
                  pConnection
 * Returns      : none
*******************************************************************************/
connections_queue ICACHE_FLASH_ATTR *webserver_connection_find(named_connection_queue *collection, struct espconn *pConnection) {
	connections_queue *request;
	
	STAILQ_FOREACH(request, &(collection->head), entries) {
		if (webserver_connection_match(pConnection, &(request->footprint))) {
			return request;
		}
	}
	
	return NULL;
}

/******************************************************************************
 * FunctionName : webserver_connection_clear
 * Description  : remove request from connection queue (closed connection)
 * Parameters   : collection
                  pConnection
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR webserver_connection_clear(named_connection_queue *collection, struct espconn *pConnection) {
	connections_queue *request;
	
	while (request = webserver_connection_find(collection, pConnection)) {
#if WEBSERVER_DEBUG
		debug(
			"WEBSERVER: %s remove [%s] [%d.%d.%d.%d:%d]\n", 
			collection->name,
			request->pURL,
			IP2STR(pConnection->proto.tcp->remote_ip),
			pConnection->proto.tcp->remote_port
		);
#endif
			
		STAILQ_REMOVE(&(collection->head), request, _connections_queue_, entries);
		if (request->extra) {
			os_free(request->extra);
		}
		os_free(request->pURL);
		os_free(request);
	}
}

/******************************************************************************
 * FunctionName : webserver_connection_reconnect
 * Description  : 
 * Parameters   : collection
                  pConnection
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR webserver_connection_reconnect(named_connection_queue *collection, struct espconn *pConnection, sint8 err) {
	connections_queue *request;
	
	while (request = webserver_connection_find(collection, pConnection)) {
#if WEBSERVER_DEBUG
		debug(
			"WEBSERVER: %s reconnect [%s] [%d.%d.%d.%d:%d] %d [%s] [%s]\n", 
			collection->name,
			request->pURL,
			IP2STR(pConnection->proto.tcp->remote_ip),
			pConnection->proto.tcp->remote_port,
			err,
			connection_err_str(err),
			connection_state_str(pConnection->state)
		);
#endif
		if (pConnection->state != ESPCONN_CLOSE)  {
			request->pConnection = pConnection;
		} else {
			webserver_connection_clear(collection, pConnection);
		}
	}
}

/******************************************************************************
 * FunctionName : webserver_connection_store
 * Description  : store long poll request
 * Parameters   : pConnection
 *              : pURL
 * Returns      : none
*******************************************************************************/
connections_queue ICACHE_FLASH_ATTR *webserver_connection_store(named_connection_queue *collection, struct espconn *pConnection, char *pURL, uint32 timeout) {
	connections_queue *request;
	
	sint8 error = espconn_regist_time(pConnection, timeout, 1);
#if WEBSERVER_DEBUG
	if (error) {
		debug("WEBSERVER: Set timeout failed [%d]\n", error);
	}
#endif
	
	request = (connections_queue *)os_zalloc(sizeof(connections_queue));
	request->pURL = (char *)os_zalloc(os_strlen(pURL)+1);
	
	os_strcpy(request->pURL, pURL);
	request->pConnection = pConnection;
	request->extra = NULL;
	connection_get_footprint(pConnection, &(request->footprint));
	
	STAILQ_INSERT_TAIL(&(collection->head), request, entries);
	
	// register collection into http_queues
	http_connection_queue *queue;
	STAILQ_FOREACH(queue, &http_queues, entries) {
		if (queue->collection == collection) {
			return request;
		}
	}
	
	queue = (http_connection_queue *)os_malloc(sizeof(http_connection_queue));
	queue->collection = collection;
	
	STAILQ_INSERT_TAIL(&http_queues, queue, entries);
	return request;
}

/******************************************************************************
 * FunctionName : webserver_connections_info
 * Description  : 
 * Parameters   : 
 * Returns      : boolean
*******************************************************************************/
void ICACHE_FLASH_ATTR webserver_connections_info() {
	http_connection_queue *queue;
	
	uint8 total = 0;
	bool  no_connections = true;
	
	debug("WebServer Connections Info\n", total);
	STAILQ_FOREACH(queue, &http_queues, entries) {
		total = 0;
		connections_queue *request;
		STAILQ_FOREACH(request, &(queue->collection->head), entries) {
			total++;
		}
		debug("   %s [%d]\n", queue->collection->name, total);
		no_connections = false;
	}
	
	if (no_connections) {
		debug("   No connections found\n");
	}
}

/******************************************************************************
 * FunctionName : webserver_status_str
 * Description  : convert status code to proper http header string
 * Parameters   : status -- HTTP Status code
 *              : pStatus -- status string
 * Returns      : status string
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR webserver_status_str(uint16 status, char *pStatus) {
	switch (status) {
		case 101 :
			os_sprintf(pStatus, "%d Switching Protocols", status);
		break;
		
		case 200 :
			os_sprintf(pStatus, "%d OK", status);
		break;
		
		case 301 :
			os_sprintf(pStatus, "%d Moved Permanently", status);
		break;
		
		case 302 :
			os_sprintf(pStatus, "%d Found", status);
		break;
		
		case 307 :
			os_sprintf(pStatus, "%d Temporary Redirect", status);
		break;
		
		case 401 :
			os_sprintf(pStatus, "%d Authorization Required", status);
		break;
		
		case 404 :
			os_sprintf(pStatus, "%d Not Found", status);
		break;
		
		case 503 :
			os_sprintf(pStatus, "%d Service Unavailable", status);
		break;
		
		default:
			os_sprintf(pStatus, "%d Not implemented", 501);
	}
}

/******************************************************************************
 * FunctionName : webserver_send_response
 * Description  : processing the data as http format and send to the client or server
 * Parameters   : pConnection
 *                pData -- The data
 * Returns	  :
*******************************************************************************/
void ICACHE_FLASH_ATTR webserver_send_response(struct espconn *pConnection, char *pData) {
	uint16 length = 0;
	char *pBuf = NULL;
	
	char httphead[256];
	os_memset(httphead, '\0', sizeof(httphead));
	
	char cStatus[WEBSERVER_MAX_NAME];
	os_memset(cStatus, '\0', sizeof(cStatus));
	
	uint16 contentLength = pData ? os_strlen(pData) : 0;
	char cLength[WEBSERVER_MAX_NAME];
	char cRealm[WEBSERVER_MAX_VALUE];
	
	if (pConnection->state == ESPCONN_CLOSE) {
#if WEBSERVER_DEBUG
		debug("WEBSERVER: discard response - client closed connection\n");
#endif
		return;
	}
	
#if WEBSERVER_DEBUG
	debug("WEBSERVER: Status response: %d %s\n", http_status, http_status == 200 ? "" : pData);
#endif
	
	webserver_status_str(http_status, cStatus);
	os_sprintf(httphead, "HTTP/1.1 %s\r\n", cStatus);
	
	webserver_set_response_header("Access-Control-Allow-Origin", "*", true);
	webserver_set_response_header("Access-Control-Allow-Headers", "Authorization, Content-Type", true);
	
	os_sprintf(cLength, "%d", contentLength);
	webserver_set_response_header("Content-Length", cLength, true);
	
	webserver_set_response_header("Server", "Olimex ESP/1.0.0", true);
	
	if (http_status == 401) {
		os_sprintf(cRealm, "Basic realm=\"%s\"", SERVER_BA_REALM);
		webserver_set_response_header("WWW-Authenticate", cRealm, true);
	}
	
	if (pData) {
		webserver_set_response_header("Content-Type", "application/json", true);
		webserver_set_response_header("Expires", "Fri, 10 Apr 2008 14:00:00 GMT", true);
		webserver_set_response_header("Pragma", "no-cache", true);
	}
	
	webserver_set_response_header("Connection", "close", false);
	
	webserver_fetch_response_headers(httphead);
	length = os_strlen(httphead) + contentLength;
	pBuf = (char *)os_zalloc(length + 1);
	
	os_memcpy(pBuf, httphead, os_strlen(httphead));
	if (pData) {
		os_memcpy(pBuf + os_strlen(httphead), pData, os_strlen(pData));
	}
	
#if WEBSERVER_DEBUG
#if	WEBSERVER_VERBOSE_OUTPUT
	debug("WEBSERVER RESPONSE: \nLength: %d\n%s\n", length, pBuf);
#endif
#endif

#if SSL_ENABLE	
	if (pConnection->proto.tcp->local_port == WEBSERVER_SSL_PORT) {
		espconn_secure_sent(pConnection, pBuf, length);
	} else {
		espconn_sent(pConnection, pBuf, length);
	}
#else
	espconn_sent(pConnection, pBuf, length);
#endif
	
	os_free(pBuf);
	pBuf = NULL;
	
#if WEBSERVER_DEBUG
	debug("WEBSERVER: response sent %d.%d.%d.%d:%d\n",
		IP2STR(pConnection->proto.tcp->remote_ip),
		pConnection->proto.tcp->remote_port
	);
#endif
}

/******************************************************************************
 * FunctionName : webserver_register_chunk_callback
 * Description  : Register HTTP chunk processing callback function
 * Parameters   : function
 * Returns      : boolean
*******************************************************************************/
void ICACHE_FLASH_ATTR webserver_register_chunk_callback(http_chunk_callback function, char *url) {
	if (chunk_url != NULL) {
		os_free(chunk_url);
	}
	
	chunk_url = NULL;
	
	if (url != NULL) {
		chunk_url = (char *)os_zalloc(os_strlen(url)+1);
		os_strcpy(chunk_url, url);
	}
	
	webserver_chunk = function;
}

/******************************************************************************
 * FunctionName : webserver_chunk_get
 * Description  : Get registered HTTP chunk processing callback function
 * Parameters   : function
 * Returns      : boolean
*******************************************************************************/
http_chunk_callback ICACHE_FLASH_ATTR webserver_chunk_get() {
	return webserver_chunk;
}

/******************************************************************************
 * FunctionName : webserver_chunk_url
 * Description  : Get registered chunk URL
 * Parameters   : function
 * Returns      : boolean
*******************************************************************************/
char ICACHE_FLASH_ATTR *webserver_chunk_url() {
	return chunk_url;
}

/******************************************************************************
 * FunctionName : webserver_register_basic_authentication
 * Description  : Register HTTP basic authentication callback function
 * Parameters   : function
 * Returns      : boolean
*******************************************************************************/
void ICACHE_FLASH_ATTR webserver_register_basic_authentication(basic_authentication_callback function) {
	basic_authentication = function;
}

/******************************************************************************
 * FunctionName : default_basic_authentication_callback
 * Description  : check for HTTP basic authorization credentials
 * Parameters   : user
 *                passwd
 * Returns      : boolean
*******************************************************************************/
LOCAL bool ICACHE_FLASH_ATTR default_basic_authentication_callback(char *user, char *passwd) {
	if (os_strcmp(user, user_config_user()) != 0) {
		return false;
	}
	
	if (os_strcmp(passwd, user_config_password()) != 0) {
		return false;
	}
	
	return true;
}

/******************************************************************************
 * FunctionName : webserver_authentication
 * Description  : check for HTTP authorization credentials
 * Parameters   : user
 *                passwd
 * Returns      : boolean
*******************************************************************************/
bool ICACHE_FLASH_ATTR webserver_authentication(char *user, char *passwd) {
	return (basic_authentication == NULL) ?
		default_basic_authentication_callback(user, passwd)
		:
		(*basic_authentication)(user, passwd)
	;
}

/******************************************************************************
 * FunctionName : webserver_basic_authentication
 * Description  : check for HTTP basic authorization
 * Parameters   : pData -- HTTP request
 * Returns      : boolean
*******************************************************************************/
LOCAL bool ICACHE_FLASH_ATTR webserver_basic_authentication(char *pData) {
	char *pStart = NULL;
	char *pEnd = NULL;
	
	int credentialsLen = 0;
	
	pStart = (char *)strstr_end(pData, "Authorization: Basic ");
	if (pStart == NULL) {
#if WEBSERVER_DEBUG
		debug("Authorization header not found!\n");
#endif
		return false;
	}
	
	pEnd = (char *)os_strstr(pStart, "\r\n");
	if (pEnd == NULL) {
#if WEBSERVER_DEBUG
		debug("Authorization header END not found!\n");
#endif
		return false;
	}
	
	credentialsLen = pEnd-pStart+1;
	
	char b64Credentials[credentialsLen];
	
	char credentials[credentialsLen];
	os_memset(credentials, '\0', sizeof(credentials));
	
	char user[credentialsLen];
	char passwd[credentialsLen];
	
	strncpy_null(b64Credentials, pStart, pEnd-pStart);
#if WEBSERVER_DEBUG
#if	WEBSERVER_VERBOSE_OUTPUT
	debug("Base64 encoded credentials [%s]\n", b64Credentials);
#endif
#endif
	
	int b64Res = base64_decode(b64Credentials, os_strlen(b64Credentials), credentials, &credentialsLen);
	
	pStart = (char *)os_strstr(credentials, ":");
	if (pStart == NULL) {
#if WEBSERVER_DEBUG
		debug("Invalid credentials! [%s] [%s] [%d] [%d]\n", b64Credentials, credentials, credentialsLen, b64Res);
#endif
		return false;
	}
	
	strncpy_null(user, credentials, pStart-credentials);
	strncpy_null(passwd, pStart+1, os_strlen(credentials) - (pStart-credentials));
	
#if WEBSERVER_DEBUG
#if	WEBSERVER_VERBOSE_OUTPUT
	debug("User: [%s]\n", user);
	debug("Passwd: [%s]\n", passwd);
#endif
#endif
	
	return webserver_authentication(user, passwd);
}

/******************************************************************************
 * FunctionName : webserver_method
 * Description  : Extract HTTP method from request data
 * Parameters   : pData -- The received data (or NULL when the connection has been closed!)
 * Returns      : request_method
*******************************************************************************/
LOCAL request_method ICACHE_FLASH_ATTR webserver_method(char *pData, unsigned short length) {
	LOCAL char first = ' ';
	
	if (length == 1) {
		first = *pData;
		return UNKNOWN;
	}
	
	if (
		(first == ' ' && os_strstr(pData, "GET ") != 0) ||
		(first == 'G' && os_strstr(pData,  "ET ") != 0)
	) {
#if WEBSERVER_DEBUG
		debug("WEBSERVER: Method GET\n");
#endif
		first = ' ';
		return GET;
	}
	
	if (
		(first == ' ' && os_strstr(pData, "POST ") != 0) ||
		(first == 'P' && os_strstr(pData,  "OST ") != 0)
	) {
#if WEBSERVER_DEBUG
		debug("WEBSERVER: Method POST\n");
#endif
		first = ' ';
		return POST;
	}
	
	if (
		(first == ' ' && os_strstr(pData, "OPTIONS ") != 0) ||
		(first == 'O' && os_strstr(pData,  "PTIONS ") != 0)
	) {
#if WEBSERVER_DEBUG
		debug("WEBSERVER: Method OPTIONS\n");
#endif
		first = ' ';
		return OPTIONS;
	}
	
	first = ' ';
	return UNKNOWN;
}

/******************************************************************************
 * FunctionName : webserver_url
 * Description  : Extract URL from request data
 * Parameters   : pData -- The received data (or NULL when the connection has been closed!)
 *                pURL -- requested URL
 * Returns      : boolean
*******************************************************************************/
LOCAL bool ICACHE_FLASH_ATTR webserver_url(char *pData, char *pURL, uint16 limit) {
	char *pStart = NULL;
	char *pEnd = NULL;

	// find first space after request method
	pStart = (char *)strstr_end(pData, " ");
	
	if (pStart == NULL) {
		return false;
	}
	
	pEnd = (char *)os_strstr(pStart, " HTTP/");
	if (pEnd == NULL) {
		return false;
	}
	
	os_memcpy(pURL, pStart, limit < pEnd-pStart ? limit : pEnd-pStart);
	return true;
}

LOCAL bool ICACHE_FLASH_ATTR webserver_abs_url(char *pData, char *pURL, uint16 limit) {
	char *pStart = NULL;
	char *pEnd = NULL;
	uint16 len = 0;
	
	pStart = (char *)strstr_end(pData, "Host: ");
	
	if (pStart == NULL) {
		return false;
	}
	
	pEnd = (char *)os_strstr(pStart, "\r\n");
	if (pEnd == NULL) {
		return false;
	}
	
	len = limit < pEnd-pStart ? limit : pEnd-pStart;
	os_memcpy(pURL, pStart, len);
	
	return webserver_url(pData, (char *)(pURL+len), limit-len);
}

/******************************************************************************
 * FunctionName : webserver_content
 * Description  : Extract HTTP content from request data
 * Parameters   : pData -- The received data
 *                pContent -- pointer to content
 *                length - content length
 * Returns      : boolean
*******************************************************************************/
LOCAL bool ICACHE_FLASH_ATTR webserver_content(char *pData, char **pContent, uint32 *length) {
	*pContent = NULL;
	*length = 0;
	
	
	char *cLength = webserver_get_header(pData, "Content-Length");
	if (cLength == NULL) {
		return false;
	}
	
	*length = strtoul(cLength, NULL, 10);
	os_free(cLength);
	
	*pContent = (char *)strstr_end(pData, "\r\n\r\n");
	if (*pContent == NULL) {
		*length = 0;
		return false;
	}
	
	return true;
}

LOCAL uint32 ICACHE_FLASH_ATTR webserver_request_total_len(char *pData, uint32 length) {
	char *pContent;
	uint32 contentLength;
	uint32 requestTotalLen = length;
	if (webserver_content(pData, &pContent, &contentLength)) {
		requestTotalLen = pContent - pData + contentLength;
	}
	return requestTotalLen;
}

/******************************************************************************
 * FunctionName : webserver_request
 * Description  : Processing the received data from the server
 * Parameters   : arg -- Additional argument to pass to the callback function
 *                pData -- The received data (or NULL when the connection has been closed!)
 *                length -- The length of received data
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR webserver_request(void *arg, char *pData, unsigned short length, uint16 chunkID, bool isLast) {
	struct espconn *pConnection = arg;
	request_method method = UNKNOWN;
	http_handler_callback callback = NULL;
	
	char cURL[WEBSERVER_MAX_VALUE];
	os_memset(cURL, '\0', sizeof(cURL));
	
#if WEBSERVER_DEBUG
#if	WEBSERVER_VERBOSE_OUTPUT
	debug("WEBSERVER REQUEST:\nLength: %d [%d]\n%s\n", length, os_strlen(pData), pData);
#endif
#endif
	
	if (chunkID != 0) {
		if (webserver_chunk) {
			(*webserver_chunk)(pConnection, pData, length, chunkID, isLast);
			return;
		}
		
		debug("WEBSERVER: IGNORED chunked request [%d][%d]\n", chunkID, length);
		return;
	}
	
	webserver_chunk = NULL;
	webserver_set_status(200);
	
	STAILQ_INIT(&http_headers);
	
#if SSL_ENABLE
	if (user_config_ssl() && pConnection->proto.tcp->local_port != WEBSERVER_SSL_PORT) {
		LOCAL char redirect[WEBSERVER_MAX_VALUE];
		os_memset(redirect, '\0', sizeof(redirect));
		
		os_sprintf(redirect, "https://");
		uint16 len = os_strlen(redirect);
		webserver_abs_url(pData, redirect + len, sizeof(redirect)-(len+1));
		
		webserver_set_status(302);
		webserver_set_response_header("Location", redirect, true);
		webserver_send_response(pConnection, NULL);
		return;
	}
#endif
	
	method = webserver_method(pData, length);
	switch (method) {
		case OPTIONS :
			webserver_set_status(200);
			webserver_set_response_header("Connection", "Keep-Alive", true);
			webserver_send_response(pConnection, NULL);
			return;
		break;
		
		case UNKNOWN :
			if (length == 1) {
#if WEBSERVER_DEBUG
				debug("WEBSERVER: HTTPS Request split [%s]\n", pData);
#endif
				return;
			}
			
			webserver_set_status(501);
			webserver_send_response(pConnection, "Request method not supported");
		return;
	}
	
	if (!webserver_url(pData, cURL, sizeof(cURL)-1)) {
		webserver_set_status(501);
		webserver_send_response(pConnection, "Malformed HTTP header. URL can not be found");
		return;
	}
#if WEBSERVER_DEBUG
	debug("WEBSERVER: URL %s\n", cURL);
#endif
	
	if (websocket_server_upgrade(pConnection, cURL, pData)) {
		return;
	}
	
	if (user_config_authentication() && !webserver_basic_authentication(pData)) {
		webserver_set_status(401);
		webserver_send_response(pConnection, "Authorization Required");
		return;
	}
	
	char response[WEBSERVER_MAX_RESPONSE_LEN];
	os_memset(response, '\0', sizeof(response));
	
	char *pContent = NULL;
	uint32 contentLength = 0;
	
	if (method == POST && !webserver_content(pData, &pContent, &contentLength)) {
		webserver_set_status(501);
		webserver_send_response(pConnection, "Malformed HTTP POST data");
		return;
	}
	
	callback = webserver_find_handler(cURL);
	
	if (callback == NULL) {
		webserver_set_status(404);
		webserver_send_response(pConnection, "Not Found");
		return;
	}
	
	(*callback)(pConnection, method, cURL, pContent, length - (pContent - pData), contentLength, response, sizeof(response));
	if (http_status == 0) {
#if WEBSERVER_DEBUG
		debug("WEBSERVER: Long poll...\n");
#endif
		long_poll_register(pConnection, cURL);
		return;
	}
	
	if (http_status == 200 && webserver_find_header("Location")) {
		webserver_set_status(302);
	}
	
	if (http_status == 200 && webserver_find_header("Retry-After")) {
		webserver_set_status(503);
	}
	
	webserver_send_response(pConnection, response);
	if (method == POST) {
		user_event_raise(cURL, response);
	}
}

/******************************************************************************
 * FunctionName : webserver_recv
 * Description  : Processing the received data from the server
 * Parameters   : arg -- Additional argument to pass to the callback function
 *                pData -- The received data (or NULL when the connection has been closed!)
 *                length -- The length of received data
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR webserver_recv(void *arg, char *pData, unsigned short length) {
	struct espconn *pConnection = arg;
	
	LOCAL _uint64_ requestTotalLen = 0;
	LOCAL uint16 chunkID = 0;
	LOCAL char *request = NULL;
	LOCAL uint16 requestLen = 0;
	LOCAL connect_footprint footprint;
	
	if (websocket_switched(pConnection, pData, length)) {
		return;
	}
	
#if WEBSERVER_DEBUG
	debug(
		"WEBSERVER: %d.%d.%d.%d:%d%sdata [%d]\n", 
		IP2STR(pConnection->proto.tcp->remote_ip),
		pConnection->proto.tcp->remote_port,
		pConnection->proto.tcp->local_port == WEBSERVER_SSL_PORT ? " SSL " : " ",
		length
	);
#endif
	
	if (pData == NULL || length == 0) {
		webserver_set_status(501);
		webserver_send_response(pConnection, "Connection closed");
		
		if (request != NULL) {
#if WEBSERVER_DEBUG
			debug("WEBSERVER: free memory for closed connection\n");
#endif
			os_free(request);
		}
		request = NULL;
		
		requestTotalLen = 0;
		return;
	}
	
	if (requestTotalLen == 0) {
		chunkID = 0;
		webserver_register_chunk_callback(NULL, NULL);
		connection_get_footprint(pConnection, &footprint);
		requestTotalLen = webserver_request_total_len(pData, length);
		
#if WEBSERVER_DEBUG
		debug("WEBSERVER: request total length [%d]\n", requestTotalLen);
#endif
		if (requestTotalLen != length && requestTotalLen < WEBSERVER_MAX_PACKET_LEN) {
#if WEBSERVER_DEBUG
			debug("WEBSERVER: request allocate memory...\n");
#endif
			request = (char *)os_zalloc(requestTotalLen+1);
			requestLen = 0;
			
#if WEBSERVER_DEBUG
			if (request == NULL) {
				debug("WEBSERVER: not enough memory\n");
			}
#endif
		} else {
			request = NULL;
		}
	} else {
		if (!webserver_connection_match(pConnection, &footprint)) {
#if WEBSERVER_DEBUG
			debug("WEBSERVER: IGNORED new request during chunk data handle\n");
#endif
			return;
		}
	}
	
	if (length > requestTotalLen) {
		debug("WEBSERVER: request total length overflow [%d] [%d]\n", requestTotalLen, length);
	}

	if (request != NULL) {
#if WEBSERVER_DEBUG
		debug("WEBSERVER: request concatenate...\n");
#endif
		os_memcpy(request + requestLen, pData, length);
		requestLen += length;
	} else {
#if WEBSERVER_DEBUG
		debug("WEBSERVER: request - handle without concatenate...\n");
#endif
		webserver_request(arg, pData, length, chunkID, length == requestTotalLen);
	}
	requestTotalLen -= length;
	chunkID++;
	
	if (requestTotalLen == 0) {
		if (request != NULL) {
#if WEBSERVER_DEBUG
			debug("WEBSERVER: request handle...\n");
#endif
			webserver_request(arg, request, requestLen, 0, true);
			
#if WEBSERVER_DEBUG
			debug("WEBSERVER: free memory\n");
#endif
			os_free(request);
			request = NULL;
		}
#if WEBSERVER_DEBUG
		debug("WEBSERVER: request done\n\n");
#endif
	}
}

/******************************************************************************
 * FunctionName : webserver_recon
 * Description  : the connection has been err, reconnection
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns	  : none
*******************************************************************************/
LOCAL ICACHE_FLASH_ATTR void webserver_recon(void *arg, sint8 err) { 
	struct espconn *pConnection = arg;

#if WEBSERVER_DEBUG
	debug(
		"WEBSERVER: Reconnect [%d.%d.%d.%d:%d] %d [%s] [%s]\n", 
		IP2STR(pConnection->proto.tcp->remote_ip),
		pConnection->proto.tcp->remote_port, 
		err,
		connection_err_str(err),
		connection_state_str(pConnection->state)
	);
#endif
	
	if (err != 0) {
		webserver_recv(pConnection, NULL, 0);
	}
	
	http_connection_queue *queue;
	STAILQ_FOREACH(queue, &http_queues, entries) {
		webserver_connection_reconnect(queue->collection, pConnection, err);
	}
}

/******************************************************************************
 * FunctionName : webserver_discon
 * Description  : Disconnect
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns	  : none
*******************************************************************************/
void ICACHE_FLASH_ATTR webserver_discon(void *arg) {
	struct espconn *pConnection = arg;
	
#if CONNECTIONS_DEBUG || WEBSERVER_DEBUG
	debug(
		"WEBSERVER: Disconnected [%d.%d.%d.%d:%d] [%s]\n",
		IP2STR(pConnection->proto.tcp->remote_ip),
		pConnection->proto.tcp->remote_port,
		connection_state_str(pConnection->state)
	);
#endif
	
	http_connection_queue *queue;
	STAILQ_FOREACH(queue, &http_queues, entries) {
		webserver_connection_clear(queue->collection, pConnection);
	}
}

/******************************************************************************
 * FunctionName : webserver_connect
 * Description  : server listened a connection successfully
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns	    : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR webserver_connect(void *arg) {
	struct espconn *pConnection = arg;
	
	espconn_regist_recvcb(pConnection, webserver_recv);
	espconn_regist_reconcb(pConnection, webserver_recon);
	espconn_regist_disconcb(pConnection, webserver_discon);
	
#if CONNECTIONS_DEBUG || WEBSERVER_DEBUG
	debug(
		"WEBSERVER: Connected [%d.%d.%d.%d:%d]\n", 
		IP2STR(pConnection->proto.tcp->remote_ip),
		pConnection->proto.tcp->remote_port
	);
#endif
	espconn_set_opt(pConnection, ESPCONN_REUSEADDR | ESPCONN_NODELAY);
	espconn_clear_opt(pConnection, ESPCONN_COPY);
}

void ICACHE_FLASH_ATTR webserver_init_http() {
	LOCAL struct espconn connection;
	LOCAL esp_tcp tcp;
	
	connection.type = ESPCONN_TCP;
	connection.state = ESPCONN_NONE;
	connection.proto.tcp = &tcp;
	connection.proto.tcp->local_port = WEBSERVER_PORT;
	espconn_regist_connectcb(&connection, webserver_connect);
	
	espconn_accept(&connection);
	espconn_tcp_set_max_con_allow(&connection, 3);
	debug("WEBSERVER: Maximum allowed HTTP connections [%d]\n", espconn_tcp_get_max_con_allow(&connection));
}

#if SSL_ENABLE
void ICACHE_FLASH_ATTR webserver_init_https() {
	LOCAL struct espconn connection;
	LOCAL esp_tcp tcp;
	
	connection.type = ESPCONN_TCP;
	connection.state = ESPCONN_NONE;
	connection.proto.tcp = &tcp;
	connection.proto.tcp->local_port = WEBSERVER_SSL_PORT;
	espconn_regist_connectcb(&connection, webserver_connect);
	
	espconn_secure_set_size(ESPCONN_SERVER, WEBSERVER_MAX_PACKET_LEN);
	
	espconn_secure_accept(&connection);
	debug("WEBSERVER: Maximum allowed HTTPS connections [%d]\n", espconn_tcp_get_max_con_allow(&connection));
	debug("WEBSERVER: Client SSL buffer size [%d]\n", espconn_secure_get_size(ESPCONN_CLIENT));
	debug("WEBSERVER: Server SSL buffer size [%d]\n", espconn_secure_get_size(ESPCONN_SERVER));
}
#endif

/******************************************************************************
 * FunctionName : webserver_init
 * Description  : initialize as a server
 * Returns	    : none
*******************************************************************************/
void ICACHE_FLASH_ATTR webserver_init() {
	webserver_register_handler_callback("/", default_handler);
	
	webserver_init_http();
#if SSL_ENABLE
	webserver_init_https();
#endif
	
	websocket_init();
	
	debug("\nWEBSERVER: initialized\n\n");
}
