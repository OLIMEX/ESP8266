#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "mem.h"
#include "queue.h"
#include "user_interface.h"

#include "stdout.h"
#include "user_long_poll.h"

named_connection_queue http_long_polls = {
	.head = STAILQ_HEAD_INITIALIZER(http_long_polls.head),
	.name = "LongPolls"
};

/******************************************************************************
 * FunctionName : long_poll_register
 * Description  : register long poll request
 * Parameters   : pConnection
 *              : pURL
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR long_poll_register(struct espconn *pConnection, char *pURL) {
	bool ssl = pConnection->proto.tcp->local_port == WEBSERVER_SSL_PORT;
	uint32 timeout = ssl ? 4 * WEBSERVER_LONG_POLL_TIME : WEBSERVER_LONG_POLL_TIME;
	
	webserver_connection_store(&http_long_polls, pConnection, pURL, timeout);
	
#if LONG_POLL_DEBUG
	connections_queue *request;
	STAILQ_FOREACH(request, &(http_long_polls.head), entries) {
		debug(
			"LongPoll: dump [%s] [%d.%d.%d.%d:%d]\n", 
			request->pURL,
			IP2STR(request->footprint.remote_ip),
			request->footprint.remote_port
		);
	}
#endif
}

/******************************************************************************
 * FunctionName : long_poll_close_all
 * Description  : close all registered long poll request
 * Parameters   : pConnection
 *              : pURL
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR long_poll_close_all() {
	connections_queue *request;
	STAILQ_FOREACH(request, &(http_long_polls.head), entries) {
		if (request->pConnection->proto.tcp->local_port == WEBSERVER_SSL_PORT) {
			espconn_secure_disconnect(request->pConnection);
		} else {
			espconn_disconnect(request->pConnection);
		}
	}
}

/******************************************************************************
 * FunctionName : long_poll_response
 * Description  : send response to long poll requests
 * Parameters   : pURL
 *              : pData
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR long_poll_response(char *pURL, char *pData) {
	connections_queue *request;
	
	if (pData == NULL) {
		return;
	}
	
	webserver_set_status(200);
	
	STAILQ_FOREACH(request, &(http_long_polls.head), entries) {
		if (pURL == NULL || os_strcmp(pURL, request->pURL) == 0) {
#if LONG_POLL_DEBUG
			debug(
				"LongPoll: response [%s] [%d.%d.%d.%d:%d]\n", 
				request->pURL,
				IP2STR(request->pConnection->proto.tcp->remote_ip),
				request->pConnection->proto.tcp->remote_port
			);
#endif
			webserver_send_response(request->pConnection, pData);
			
			STAILQ_REMOVE(&(http_long_polls.head), request, _connections_queue_, entries);
			os_free(request->pURL);
			os_free(request);
#if LONG_POLL_DEBUG
			debug("\n");
#endif
		}
	}
}

