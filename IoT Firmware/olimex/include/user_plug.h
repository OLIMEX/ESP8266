#ifndef __USER_PLUG_H__
	#define __USER_PLUG_H__
	
	#include "user_config.h"
	#if DEVICE == PLUG
	
		typedef enum {
			PLUG_LED1 = 0,
			PLUG_LED2,
			PLUG_LED_COUNT
		} plug_led_type; 
		
		typedef struct _plug_led_config_ {
			uint8     gpio_id;
			uint32    gpio_name;
			uint8     gpio_func;
		} plug_led_config;
		
		void plug_led(plug_led_type led, uint8 state);
		
		void plug_wifi_blink_start();
		void plug_wifi_blink_stop();
		
		void user_plug_init();
	#endif
#endif