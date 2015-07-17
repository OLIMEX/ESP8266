#include "ets_sys.h"
#include "osapi.h"
#include "mem.h"
#include "stdout.h"

#include "user_devices.h"
#include "user_json.h"

#include "driver/uart.h"
#include "mod_emtr.h"

LOCAL emtr_packet *emtr_receive_packet = NULL;
LOCAL emtr_packet *emtr_send_packet    = NULL;

LOCAL emtr_callback emtr_command_done    = NULL;
LOCAL emtr_callback emtr_command_timeout = NULL;
LOCAL emtr_callback emtr_user_callback   = NULL;

LOCAL uint32 emtr_timeout = 0;

LOCAL emtr_sys_params emtr_params = {
	.address = 0
};

LOCAL void ICACHE_FLASH_ATTR emtr_default_timeout(emtr_packet *packet) {
	debug("EMTR: %s\n", TIMEOUT);
}

void ICACHE_FLASH_ATTR emtr_set_timeout_callback(emtr_callback command_timeout) {
	emtr_command_timeout = command_timeout;
}

LOCAL uint8 ICACHE_FLASH_ATTR emtr_check_sum(emtr_packet *packet) {
	uint8 check_sum = packet->header + packet->len;
	uint8 i;
	for (i=0; i<packet->len-3; i++) {
		check_sum += packet->data[i];
	}
	return check_sum;
}

LOCAL void ICACHE_FLASH_ATTR emtr_packet_init(emtr_packet **packet, uint8 header, uint8 len) {
	if (*packet == NULL) {
		*packet = (emtr_packet *)os_zalloc(sizeof(emtr_packet));
	}
	
	(*packet)->header = header;
	(*packet)->len = len;
	
	if ((*packet)->data != NULL) {
		os_free((*packet)->data);
	}
	
	(*packet)->data = (len == 0 ?
		NULL
		:
		(uint8 *)os_zalloc(len-2)
	);
}

LOCAL void ICACHE_FLASH_ATTR emtr_receive(emtr_packet *packet) {
	if (packet == NULL) {
		debug("EMTR: Not initialized\n");
		return;
	}
	
	if (packet->check_sum != emtr_check_sum(packet)) {
		debug("EMTR: Checksum not match\n");
		return;
	}
	
	device_set_uart(UART_EMTR);
	
#if EMTR_DEBUG
#if EMTR_VERBOSE_OUTPUT
	debug("\nEMTR: Received packet\n");
	debug("Header: 0x%02x\n", packet->header);
	debug("Len: 0x%04x\n", packet->len);
	debug("Data: ");
	uint16 j;
	for (j=0; j<packet->len-2; j++) {
		debug("0x%02x ", packet->data[j]);
	}
	debug("\n\n");
#endif
#endif
	
	if (emtr_command_done != NULL) {
		(*emtr_command_done)(packet);
	}
	
	clearTimeout(emtr_timeout);
	emtr_timeout = 0;
}

LOCAL void ICACHE_FLASH_ATTR emtr_char_in(char c) {
LOCAL emtr_state state = EMTR_STATE_IDLE;
LOCAL uint8 count = 0;

	if (state == EMTR_STATE_IDLE || state == EMTR_STATE_ERROR) {
		emtr_packet_init(&emtr_receive_packet, EMTR_ACK_FRAME, 0);
		if (c == EMTR_ACK_FRAME) {
			count = 1;
			state = EMTR_STATE_LEN;
			return;
		}
		
		if (c == EMTR_ERROR_CRC) {
			debug("EMTR: Wrong checksum sent\n");
		}
		
		count = 0;
		state = EMTR_STATE_ERROR;
		return;
	}
	
	if (state == EMTR_STATE_LEN) {
		count++;
		emtr_receive_packet->len = (uint8)c;
		state = EMTR_STATE_DATA;
		return;
	}
	
	if (state == EMTR_STATE_DATA) {
		count++;
		
		if (count == 3) {
			if (emtr_receive_packet->data != NULL) {
				os_free(emtr_receive_packet->data);
			}
			emtr_receive_packet->data = (uint8 *)os_zalloc(emtr_receive_packet->len-2);
		}
		
		emtr_receive_packet->data[count-3] = (uint8)c;
		
		if (count == emtr_receive_packet->len) {
			emtr_receive_packet->check_sum = (uint8)c;
			state = EMTR_STATE_IDLE;
			count = 0;
			emtr_receive(emtr_receive_packet);
		}
		return;
	}
}

LOCAL void ICACHE_FLASH_ATTR emtr_send(emtr_packet *packet) {
	if (emtr_timeout != 0) {
		setTimeout((os_timer_func_t *)emtr_send, packet, 100);
		return;
	}
	
	emtr_command_done = packet->callback;
	packet->check_sum = emtr_check_sum(packet);
	packet->data[packet->len-3] = packet->check_sum;
	
#if EMTR_DEBUG
#if EMTR_VERBOSE_OUTPUT
	debug("\nEMTR: Sending packet...\n");
	debug("Header: 0x%02x\n", packet->header);
	debug("Len: 0x%04x\n", packet->len);
	debug("Data: ");
	uint16 j;
	for (j=0; j<packet->len-2; j++) {
		debug("0x%02x ", packet->data[j]);
	}
	debug("\n\n");
#endif
#endif
	
	// Start
	uart_write_byte(packet->header);
	
	// Len
	uart_write_byte(packet->len);
	
	// Data
	uart_write_buff(packet->data, packet->len-2);
	
	if (emtr_command_timeout == NULL) {
		emtr_set_timeout_callback(emtr_default_timeout);
	}
	emtr_timeout = setTimeout((os_timer_func_t *)emtr_command_timeout, packet, EMTR_TIMEOUT);
}

LOCAL uint32 emtr_extract_int(uint8 *data, uint8 offset, uint8 size) {
	uint8 i;
	uint32 pow = 1;
	uint32 result = 0;
	for (i=0; i<size; i++) {
		result = result + data[offset+i]*pow;
		pow = pow * 256;
	}
	return result;
}

void ICACHE_FLASH_ATTR emtr_all_parse(emtr_packet *packet, emtr_registers *registers) {
	if (packet->len != 31) {
		os_memset(registers, 0, sizeof(emtr_registers));
		return;
	}
	registers->current_rms         = emtr_extract_int(packet->data,  0, 4);
	registers->voltage_rms         = emtr_extract_int(packet->data,  4, 2);
	registers->active_power        = emtr_extract_int(packet->data,  6, 4);
	registers->reactive_power      = emtr_extract_int(packet->data, 10, 4);
	registers->apparent_power      = emtr_extract_int(packet->data, 14, 4);
	registers->power_factor        = emtr_extract_int(packet->data, 18, 2);
	registers->line_frequency      = emtr_extract_int(packet->data, 20, 2);
	registers->thermistor_voltage  = emtr_extract_int(packet->data, 22, 2);
	registers->event_flag          = emtr_extract_int(packet->data, 24, 2);
	registers->system_status       = emtr_extract_int(packet->data, 26, 2);
}

void ICACHE_FLASH_ATTR emtr_get_all(emtr_callback command_done) {
	emtr_packet_init(&emtr_send_packet, EMTR_START_FRAME, 8);
	
	emtr_send_packet->data[0] = 0x41; // Set adress pointer
	emtr_send_packet->data[1] = 0x00; // address
	emtr_send_packet->data[2] = 0x04; // address
	emtr_send_packet->data[3] = 0x4E; // Read N bytes
	emtr_send_packet->data[4] =   28; // N = 28 bytes
	
	emtr_send_packet->callback = command_done;
	
	emtr_send(emtr_send_packet);
}

uint16 ICACHE_FLASH_ATTR emtr_address() {
	return emtr_params.address;
}

LOCAL void ICACHE_FLASH_ATTR emtr_address_receive(emtr_packet *packet) {
	emtr_params.address = packet->data[0] * 256 + packet->data[1];
}

void ICACHE_FLASH_ATTR emtr_get_address(emtr_callback command_done) {
	emtr_packet_init(&emtr_send_packet, EMTR_START_FRAME, 7);
	
	emtr_send_packet->data[0] = 0x41; // Set adress pointer
	emtr_send_packet->data[1] = 0x00; // address
	emtr_send_packet->data[2] = 0x26; // address
	emtr_send_packet->data[3] = 0x52; // Read 2 bytes
	
	emtr_send_packet->callback = command_done;
	
	emtr_send(emtr_send_packet);
}

void ICACHE_FLASH_ATTR emtr_init() {
	if (device_get_uart() != UART_NONE) {
		return;
	}
	
	stdout_disable();
	uart_char_in_set(emtr_char_in);
	
	uart_init(BIT_RATE_4800, EIGHT_BITS, NONE_BITS, ONE_STOP_BIT);
	
	// Get address
	setTimeout((os_timer_func_t *)emtr_get_address, emtr_address_receive, 100);
}
