#ifndef __STD_OUT_H__
	#define __STD_OUT_H__
	
	#define debug os_printf

	#define WIFI_DEBUG_BUFFER_LEN  81
	
	#include "user_config.h"
	#if WIFI_DEBUG_ENABLE
		void stdout_wifi_debug();
	#endif
	
	void stdout_init(uint8 uart);
	void stdout_disable();
	void stdout_debug_disable();
	
	void stdout_uart_debug(uint8 uart);
#endif