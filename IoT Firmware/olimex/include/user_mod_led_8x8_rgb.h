#ifndef __USER_MOD_LED_8x8_RGB_H__
	#define __USER_MOD_LED_8x8_RGB_H__
	
	#include "user_config.h"
	#if MOD_LED_8x8_RGB_ENABLE

		#define MOD_LED_8x8_RGB_URL         "/mod-led-8x8-rgb"
		#define MOD_LED_8x8_RGB_MAX_SPEED   100
		#define MOD_LED_8x8_RGB_MAX_TEXT    255
		
		void mod_led_8x8_rgb_init();
		
		void mod_led_8x8_rgb_handler(
			struct espconn *pConnection, 
			request_method method, 
			char *url, 
			char *data, 
			uint16 data_len, 
			uint32 content_len, 
			char *response,
			uint16 response_len
		);
	#endif
#endif