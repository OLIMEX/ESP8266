#ifndef __STD_OUT_H__
	#define __STD_OUT_H__
	
	#define debug os_printf

	#define WIFI_DEBUG_BUFFER_LEN  81
	
	void stdout_init();
	void stdout_disable();
	void stdout_debug_disable();
	
	void stdout_uart_debug();
	
	void stdout_wifi_debug();
#endif