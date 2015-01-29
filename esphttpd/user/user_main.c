

/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */


#include "espmissingincludes.h"
#include "ets_sys.h"
#include "i2c/i2c.h"
#include "osapi.h"
#include "httpd.h"
#include "io.h"
#include "httpdespfs.h"
#include "button.h"
#include "relay.h"
#include "stdout.h"
//#include "cgii2c.h"
//
//#include "i2c/i2c.h"
//#include "spi/spi.h"

HttpdBuiltInUrl builtInUrls[]={
	{"/", cgiRedirect, "/index.html"},
	{"/button.cgi", cgiButton, NULL},
	{"/relay.cgi", cgiRelay, NULL},
	{"*", cgiEspFsHook, NULL}, //Catch-all cgi function for the filesystem
	{NULL, NULL, NULL}
};



void user_init(void) {
	stdoutInit();
	ioInit();
	httpdInit(builtInUrls, 80);
	os_printf("Ready!\n");

}
