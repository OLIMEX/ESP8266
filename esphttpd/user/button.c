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

//Cgi that turns the LED on or off according to the 'led' param in the POST data
int ICACHE_FLASH_ATTR cgiButton(HttpdConnData *connData) {
	int len;
	char buff[1024];
	
	if (connData->conn==NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

	httpdStartResponse(connData, 200);
	httpdHeader(connData, "Content-Type", "application/json");
	httpdEndHeaders(connData);

	/* This will return JSON:
	 * {"button":
	 * 	{"status":"on"}
	 * }
	 */
	len = os_sprintf(buff, "{\n \"button\": { \n\"status\":\"%s\"\n}\n}\n", (GPIO_INPUT_GET(0)) ? "off" : "on");
	espconn_sent(connData->conn, (uint8 *)buff, len);

	return HTTPD_CGI_DONE;
}
