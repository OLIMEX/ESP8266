#ifndef __USER_PLUG_H__
	#define __USER_PLUG_H__
	
	#include "user_config.h"
	#if DEVICE == PLUG
		#define PLUG_DEBUG     1
	
		typedef enum {
			PLUG_LED1 = 0,
			PLUG_LED2,
			PLUG_LED_COUNT
		} plug_led_type; 
		
		void plug_led(plug_led_type led, uint8 state);
		
		void plug_wifi_blink_start();
		void plug_wifi_blink_stop();
		
		void plug_down();
		void plug_init();
		
		void user_plug_init();
	#endif
#endif