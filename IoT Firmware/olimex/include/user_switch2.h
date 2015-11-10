#ifndef __USER_SWITCH2_H__
	#define __USER_SWITCH2_H__
	
	#include "user_config.h"
	#if DEVICE == SWITCH2

		#include "user_webserver.h"
		#include "user_devices.h"
		
		#define SWITCH2_URL "/switch2"
		
		typedef enum {
			SWITCH2_RELAY = 1,
			SWITCH2_SWITCH
		} switch2_type;
		
		typedef struct _switch2_config_ {
			uint8     id;
			uint8     type;
			uint8     gpio_id;
			uint32    gpio_name;
			uint8     gpio_func;
			void_func handler;
			uint8     state;
			uint8     state_buf;
		} switch2_config;
		
		void switch2_handler(
			struct espconn *pConnection, 
			request_method method, 
			char *url, 
			char *data, 
			uint16 data_len, 
			uint32 content_len, 
			char *response,
			uint16 response_len
		);
		
		void user_switch2_init();
	#endif
#endif