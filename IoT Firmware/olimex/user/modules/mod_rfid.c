#include "ets_sys.h"
#include "osapi.h"
#include "mem.h"
#include "stdout.h"

#include "user_devices.h"

#include "driver/uart.h"
#include "mod_rfid.h"

LOCAL char             *rfid_buff      = NULL;
LOCAL uint8             rfid_buff_pos  = 0;
LOCAL rfid_module       rfid_mod       = RFID_NONE;
LOCAL rfid_tag_callback rfid_tag_read  = NULL;

const char ICACHE_FLASH_ATTR *rfid_mod_str() {
	switch (rfid_mod) {
		case MOD_RFID125:  return "MOD-RFID125-BOX";
		case MOD_RFID1356: return "MOD-RFID1356-BOX";
	}
	return "NONE";
}

void ICACHE_FLASH_ATTR rfid_set_tag_callback(rfid_tag_callback func) {
	rfid_tag_read = func;
}

rfid_module ICACHE_FLASH_ATTR rfid_get_mod() {
	return rfid_mod;
}

LOCAL void ICACHE_FLASH_ATTR rfid_line() {
	if (rfid_buff[0] == '>') {
		// echo ready
		rfid_buff_pos = 0;
	} else if (rfid_buff[0] == '-') {
		// tag read
#if RFID_DEBUG
		debug("RFID: %s\n", rfid_buff+1);
#endif
		if (rfid_tag_read != NULL) {
			(*rfid_tag_read)(rfid_buff+1);
		}
	} else if (rfid_buff[0] > '@') {
		if (os_strcmp(rfid_buff, "MOD-RFID125-BOX") == 0) {
			device_set_uart(UART_RFID);
			rfid_mod = MOD_RFID125;
		}
		if (os_strcmp(rfid_buff, "MOD-RFID1356-BOX") == 0) {
			device_set_uart(UART_RFID);
			rfid_mod = MOD_RFID1356;
		}
#if RFID_DEBUG
		debug("%s\n", rfid_buff);
#endif
	}
}

LOCAL void ICACHE_FLASH_ATTR rfid_char_in(char c) {
	if (c > 126) {
		// bad char - may be UART is not initialized
		return;
	}
	
	if (rfid_buff == NULL) {
		rfid_buff = (char *)os_zalloc(RFID_BUFFER_LEN);
	}
	
	if (c == '\r' || c == '\n') {
		c = '\0';
	}
	
	rfid_buff[rfid_buff_pos++] = c;
	
	if (c == '\0' || rfid_buff_pos == RFID_BUFFER_LEN-1) {
		rfid_buff_pos = 0;
		rfid_line();
	}
}

void ICACHE_FLASH_ATTR rfid_info() {
	rfid_buff_pos = 0;
	rfid_mod = RFID_NONE;
	uart_write_str("\r");
	uart_write_str("i\r");
}

LOCAL void ICACHE_FLASH_ATTR rfid_set_led(uint8 led) {
	char command[10];
	if (led < 2) {
		os_sprintf(command, "ml%d\r", led);
		uart_write_str(command);
#if RFID_DEBUG
		debug("RFID: %s\n", command);
#endif
	}
}

void ICACHE_FLASH_ATTR rfid_set(uint8 freq, uint8 led) {
	char command[10];
	
	if (freq < 10) {
		os_sprintf(command, "mc%d0\r", freq);
		uart_write_str(command);
#if RFID_DEBUG
		debug("RFID: %s\n", command);
#endif
	}
	
	setTimeout(rfid_set_led, led, 10);
}

void ICACHE_FLASH_ATTR rfid_check(rfid_module module) {
	if (rfid_mod != RFID_NONE) {
		debug("RFID: %s found\n", rfid_mod_str());
		return;
	}
	
	debug("\n\nRFID: No readers found\n\n");
}

void ICACHE_FLASH_ATTR rfid_init() {
	if (device_get_uart() != UART_NONE) {
		return;
	}
	
	rfid_module module = MOD_RFID125;
	
	stdout_disable();
	uart_char_in_set(rfid_char_in);
	
	if (module == MOD_RFID125 || module == RFID_ANY) {
		uart_init(BIT_RATE_9600, EIGHT_BITS, NONE_BITS, ONE_STOP_BIT);
	}
	
	if (module == MOD_RFID1356) {
		uart_init(BIT_RATE_57600, EIGHT_BITS, NONE_BITS, ONE_STOP_BIT);
	}
	
	setTimeout(rfid_info, NULL, 50);
	setTimeout(rfid_check, module, 400);
}
