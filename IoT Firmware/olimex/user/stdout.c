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
LOCAL uint32 t = 0;
LOCAL uint8  i = 0;
	
	clearTimeout(t);
	
	if (buff == NULL) {
		buff = (char *)os_zalloc(WIFI_DEBUG_BUFFER_LEN);
	}
	
	buff[i++] = c;
	
	if (c == '\0') {
		i = 0;
		websocket_debug_message(buff);
	} else if (i == WIFI_DEBUG_BUFFER_LEN-1) {
		uint8 j = i-1;
		while (j > 0 && buff[j] != '\n') {
			j--;
		}
		
		i = 0;
		if (j == 0) {
			websocket_debug_message(buff);
		} else {
			buff[j++] = '\0';
			websocket_debug_message(buff);
			while (j < WIFI_DEBUG_BUFFER_LEN-1) {
				buff[i++] = buff[j++];
			}
		}
	} else {
		t = setTimeout(wifi_debug_flush, NULL, 200);
	}
}

LOCAL void ICACHE_FLASH_ATTR wifi_debug_flush() {
	wifi_write_char('\0');
}

void ICACHE_FLASH_ATTR stdout_init(uint8 uart) {
	uart_init(uart, BIT_RATE_115200, EIGHT_BITS, NONE_BITS, ONE_STOP_BIT);
	if (uart == UART0) {
		uart_char_in_set(uart_write_char);
	} else {
		uart_char_in_set(NULL);
	}
	stdout_uart_debug(uart);
}

void ICACHE_FLASH_ATTR stdout_disable() {
	uart_char_in_set(NULL);
	stdout_debug_disable();
}

void ICACHE_FLASH_ATTR stdout_debug_disable() {
	os_install_putc1(NULL);
}

void ICACHE_FLASH_ATTR stdout_uart_debug(uint8 uart) {
	if (uart == UART0) {
		os_install_putc1((void *)uart_write_char);
	} else {
		os_install_putc1((void *)uart1_write_char);
	}
}

void ICACHE_FLASH_ATTR stdout_wifi_debug() {
	os_install_putc1((void *)wifi_write_char);
}
