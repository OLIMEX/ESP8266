#ifndef __USER_SWITCH1_H__
	#define __USER_SWITCH1_H__
	
	#include "user_config.h"
	#if DEVICE == SWITCH1

		#include "user_webserver.h"
		#include "user_devices.h"
		#include "gpio_config.h"
		
		#define SWITCH1_URL              "/switch1"
		#define SWITCH1_DEBUG            1
		
		typedef enum {
			SWITCH1_RELAY = 1,
			SWITCH1_SWITCH
		} switch1_type;
		
		typedef struct _switch1_config_ {
			uint8       id;
			uint8       type;
			
			gpio_config gpio;
			
			void_func   handler;
			uint8       state;
			uint8       state_buf;
			uint32      timer;
		} switch1_config;
		
		void switch1_handler(
			struct espconn *pConnection, 
			request_method method, 
			char *url, 
			char *data, 
			uint16 data_len, 
			uint32 content_len, 
			char *response,
			uint16 response_len
		);
		
		void switch1_init();
		void switch1_down();
		
		void user_switch1_init();
	#endif
#endif