#include "ets_sys.h"
#include "osapi.h"
#include "stdout.h"
#include "mem.h"

#include "driver/uart.h"

#include "user_timer.h"
#include "user_websocket.h"

LOCAL void ICACHE_FLASH_ATTR wifi_debug_flush();

LOCAL void ICACHE_FLASH_ATTR wifi_write_char(char c) {
LOCAL char  *buff = NULL;
LOCAL timer *t = NULL;
LOCAL uint8  i = 0;
	
	clearTimeout(t);
	
	if (buff == NULL) {
		buff = (char *)os_zalloc(WIFI_DEBUG_BUFFER_LEN);
	}
	
	buff[i++] = c;
	
	if (c == '\0' || i == WIFI_DEBUG_BUFFER_LEN-1) {
		i = 0;
		websocket_debug_message(buff);
	} else {
		t = setTimeout(wifi_debug_flush, NULL, 200);
	}
}

LOCAL void ICACHE_FLASH_ATTR wifi_debug_flush() {
	wifi_write_char('\0');
}

void ICACHE_FLASH_ATTR stdout_init() {
	uart_init(BIT_RATE_115200, EIGHT_BITS, NONE_BITS, ONE_STOP_BIT);
	uart_char_in_set(uart_write_char);
	stdout_uart_debug();
}

void ICACHE_FLASH_ATTR stdout_disable() {
	uart_char_in_set(NULL);
	stdout_debug_disable();
}

void ICACHE_FLASH_ATTR stdout_debug_disable() {
	os_install_putc1(NULL);
}

void ICACHE_FLASH_ATTR stdout_uart_debug() {
	os_install_putc1((void *)uart_write_char);
}

void ICACHE_FLASH_ATTR stdout_wifi_debug() {
	os_install_putc1((void *)wifi_write_char);
}
