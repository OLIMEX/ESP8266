/*
Some random cgi routines.
*/

/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */


#include <string.h>
#include <osapi.h>
#include "user_interface.h"
#include "mem.h"
#include "httpd.h"
#include "io.h"
#include "gpio.h"
#include "espmissingincludes.h"

void ICACHE_FLASH_ATTR ioRelay(int value) {
	if(value) {
		gpio_output_set((1<<5), 0, (1<<5), 0);
	} else {
		gpio_output_set(0, (1<<5), (1<<5), 0);
	}
}

int ICACHE_FLASH_ATTR cgiRelay(HttpdConnData *connData) {
	int len;
	char buff[1024];
	
	if (connData->conn==NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

	os_printf("POST_DATA: %s\n", connData->postBuff);

	len=httpdFindArg(connData->postBuff, "relay", buff, sizeof(buff));
	if (len != 0) {
		ioRelay(atoi(buff));
	}

	httpdStartResponse(connData, 200);
	httpdHeader(connData, "Content-Type", "application/json");
	httpdEndHeaders(connData);

	/* This will return JSON:
	 * {"relay":
	 * 	{"status":"ok"}
	 * }
	 */
	len = os_sprintf(buff, "{\n \"relay\": { \n\"status\":\"ok\"\n}\n}\n");
	espconn_sent(connData->conn, (uint8 *)buff, len);

	return HTTPD_CGI_DONE;
}
