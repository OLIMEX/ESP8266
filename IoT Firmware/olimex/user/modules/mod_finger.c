#include "ets_sys.h"
#include "osapi.h"
#include "mem.h"
#include "stdout.h"

#include "driver/uart.h"
#include "mod_finger.h"

LOCAL finger_packet *finger_buff = NULL;

LOCAL uint32 finger_address  = 0xFFFFFFFF;
LOCAL uint32 finger_password = 0x00000000;
LOCAL bool   finger_present  = false;

LOCAL void ICACHE_FLASH_ATTR finger_buff_init(uint32 address, uint8 pid, uint16 len) {
	if (finger_buff == NULL) {
		finger_buff = (finger_packet *)os_zalloc(sizeof(finger_packet));
	}
	
	finger_buff->address = address;
	finger_buff->pid = pid;
	finger_buff->len = len;
	
	if (finger_buff->data != NULL) {
		os_free(finger_buff->data);
	}
	
	finger_buff->data = (len == 0 ?
		NULL
		:
		(uint8 *)os_zalloc(len)
	);
}

LOCAL uint16 ICACHE_FLASH_ATTR finger_check_sum() {
	uint16 check_sum = finger_buff->pid + finger_buff->len;
	uint16 i;
	for (i=0; i<finger_buff->len-2; i++) {
		check_sum += finger_buff->data[i];
	}
	return check_sum;
}

LOCAL void ICACHE_FLASH_ATTR finger_send_buff() {
	uint8 i;
	
#if FINGER_DEBUG
	debug("\nFINGER: Sending packet...\n");
	debug("Address: 0x%08x\n", finger_buff->address);
	debug("PID: 0x%02x\n", finger_buff->pid);
	debug("Len: 0x%04x\n", finger_buff->len);
	debug("Data: ");
	uint16 j;
	for (j=0; j<finger_buff->len; j++) {
		debug("0x%02x ", finger_buff->data[j]);
	}
	debug("\n\n");
#endif
	
	// Start
	uart_write_byte(0xEF);
	uart_write_byte(0x01);
	
	// Address
	for (i=0; i<4; i++) {
		uart_write_byte(finger_buff->address >> (8 * (3-i)) & 0xFF);
	}
	
	// PID
	uart_write_byte(finger_buff->pid);
	
	// Len
	uart_write_byte((finger_buff->len) >> 8);
	uart_write_byte((finger_buff->len) & 0xFF);

	// Data
	uart_write_buff(finger_buff->data, finger_buff->len);
}

LOCAL void ICACHE_FLASH_ATTR finger_execute(uint32 address, uint8 command, uint8 *data, uint16 len) {
	finger_buff_init(address, 0x01, len+3);
	
	finger_buff->data[0] = command;
	if (data != NULL && len != 0) {
		os_memcpy(finger_buff->data + 1, data, len);
	}
	
	finger_buff->check_sum = finger_check_sum();
	finger_buff->data[len + 1] = finger_buff->check_sum >> 8;
	finger_buff->data[len + 2] = finger_buff->check_sum & 0xFF;
	
	finger_send_buff();
}

LOCAL void ICACHE_FLASH_ATTR finger_receive() {
	if (finger_buff == NULL) {
		debug("FINGER: Not initialized\n");
		return;
	}
	
	if (finger_buff->address != finger_address) {
		debug("FINGER: Address not match\n");
		// return;
	}
	
	if (finger_buff->check_sum != finger_check_sum()) {
		debug("FINGER: Checksum not match [0x%04x] [0x%04x]\n", finger_buff->check_sum, finger_check_sum());
		// return;
	}
	
	finger_present = true;

#if FINGER_DEBUG
	debug("\nFINGER: Received packet\n");
	debug("Address: 0x%08x\n", finger_buff->address);
	debug("PID: 0x%02x\n", finger_buff->pid);
	debug("Len: 0x%04x\n", finger_buff->len);
	debug("Data: ");
	uint16 i;
	for (i=0; i<finger_buff->len; i++) {
		debug("0x%02x ", finger_buff->data[i]);
	}
	debug("\n\n");
#endif
}

LOCAL void ICACHE_FLASH_ATTR finger_char_in(char c) {
LOCAL finger_state state = FINGER_IDLE;
LOCAL uint16 count = 0;
	
	if (state == FINGER_IDLE || state == FINGER_ERROR) {
		finger_buff_init(0, 0, 0);
		if (c == 0xEF) {
			state = FINGER_START;
		} else {
			state = FINGER_ERROR;
		}
		return;
	}
	
	if (state == FINGER_START) {
		if (c == 0x01) {
			state = FINGER_ADDRESS;
			count = 0;
			finger_buff->address = 0;
		} else {
			state = FINGER_ERROR;
		}
		return;
	}
	
	if (state == FINGER_ADDRESS) {
		finger_buff->address = finger_buff->address*256 + (uint8)c;
		count++;
		if (count == 4) {
			state = FINGER_PID;
			finger_buff->pid = 0;
		}
		return;
	}
	
	if (state == FINGER_PID) {
		finger_buff->pid = c;
		
		state = FINGER_LEN;
		finger_buff->len = 0;
		count = 0;
		return;
	}
	
	if (state == FINGER_LEN) {
		finger_buff->len = finger_buff->len*256 + (uint8)c;
		count++;
		if (count == 2) {
			state = FINGER_DATA;
			count = 0;
		}
		return;
	}
	
	if (state == FINGER_DATA) {
		if (count == 0) {
			if (finger_buff->data != NULL) {
				os_free(finger_buff->data);
			}
			finger_buff->data = (uint8 *)os_zalloc(finger_buff->len);
		}
		finger_buff->data[count] = (uint8)c;
		count++;
		if (count == finger_buff->len) {
			finger_buff->check_sum = finger_buff->data[count-1] + 256*finger_buff->data[count-2];
			state = FINGER_IDLE;
			count = 0;
			finger_receive();
		}
		return;
	}
}

LOCAL void ICACHE_FLASH_ATTR finger_verify_password() {
	finger_execute(finger_address, 0x13, (uint8 *)&finger_password, 4);
}

LOCAL void ICACHE_FLASH_ATTR finger_read_sys_params() {
	finger_execute(finger_address, 0x0F, NULL, 0);
}

LOCAL void ICACHE_FLASH_ATTR finger_check() {
	if (finger_present) {
		debug("FINGER: Device found\n");
	} else {
		debug("FINGER: Device not found\n");
	}
}

bool ICACHE_FLASH_ATTR finger_found() {
	return finger_present;
}

void ICACHE_FLASH_ATTR finger_init() {
	stdout_wifi_debug();
	
	uart_char_in_set(finger_char_in);
	uart_init(BIT_RATE_57600, EIGHT_BITS, NONE_BITS, ONE_STOP_BIT);
	
	// Verify password
	// finger_verify_password();
	
	// Read System Parameters
	finger_read_sys_params();
	
	setTimeout(finger_check, NULL, 500);
}

