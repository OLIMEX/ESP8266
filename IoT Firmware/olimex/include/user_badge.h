#ifndef __USER_BADGE_H__
	#define __USER_BADGE_H__
	
	#include "user_config.h"
	#if DEVICE == BADGE

		#define BADGE_URL                "/badge"
		#define BADGE_MAX_SPEED          100
		#define BADGE_MAX_TEXT           255
		#define BADGE_BATTERY_UPDATE   60000
		
		#define SPI_BADGE_DATA_WAIT       50
		
		// One module requires 8x8x1 bits = 8 bytes buffer
		#define BADGE_BUFF_SIZE            8
		
		typedef void (*badge_done_callback)();
		
		void spi_badge_init();
		
		void spi_badge_start();
		void spi_badge_write(uint8 data);
		void spi_badge_stop();

		void   badge_show();
		void   badge_cls();
		bool   badge_set(uint8 x, uint8 y, uint8 l);
		
		uint16 badge_size_x(char *c);
		bool   badge_print(uint8 x, uint8 y, char *c);
		bool   badge_scroll(char *c, uint8 speed, badge_done_callback done);
		void  _badge_scroll_();
		
		void badge_wifi_animation_start();
		void badge_wifi_animation_stop();
		
		void badge_server_animation_start();
		void badge_server_animation_stop();
		
		void badge_handler(
			struct espconn *pConnection, 
			request_method method, 
			char *url, 
			char *data, 
			uint16 data_len, 
			uint32 content_len, 
			char *response,
			uint16 response_len
		);
		
		void badge_init();
	#endif
#endif