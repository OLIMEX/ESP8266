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

LOCAL bool emtr_ask_only = false;

LOCAL emtr_callback emtr_command_done    = NULL;
LOCAL emtr_callback emtr_command_timeout = NULL;
LOCAL emtr_callback emtr_user_callback   = NULL;

LOCAL uint32 emtr_timeout = 0;

LOCAL emtr_sys_params emtr_params = {
	.address = 0
};

void ICACHE_FLASH_ATTR emtr_clear_timeout(emtr_packet *packet) {
	debug("EMTR: %s\n", TIMEOUT);
	emtr_timeout = 0;
}

void ICACHE_FLASH_ATTR emtr_set_timeout_callback(emtr_callback command_timeout) {
	emtr_command_timeout = command_timeout;
}

LOCAL uint8 ICACHE_FLASH_ATTR emtr_check_sum(emtr_packet *packet) {
	if (packet->len == 0 || (packet->ask_only && packet->header != EMTR_START_FRAME)) {
		return 0;
	}
	
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
	(*packet)->ask_only = false;
	(*packet)->check_sum = 0;
	
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
		goto clear;
	}
	
	if (packet->header == EMTR_ERROR_NAK) {
		debug("EMTR: Not acknowledged\n");
		goto clear;
	}
	
	if (packet->header == EMTR_ERROR_CRC) {
		debug("EMTR: Wrong checksum sent\n");
		goto clear;
	}
	
	if (packet->check_sum != emtr_check_sum(packet)) {
		debug("EMTR: Checksum not match [0x%02X] [0x%02X]\n", packet->check_sum, emtr_check_sum(packet));
#if EMTR_DEBUG
		debug("Header: 0x%02X\n", packet->header);
		debug("Len: 0x%02X\n", packet->len);
		debug("ASK only: 0x%02X\n", packet->ask_only);
#endif
		goto clear;
	}
	
	device_set_uart(UART_EMTR);
	
#if EMTR_DEBUG
	debug("\nEMTR: Received packet\n");
	debug("Header: 0x%02X\n", packet->header);
	debug("Len: 0x%02X\n", packet->len);
	debug("ASK only: 0x%02X\n", packet->ask_only);
#if EMTR_VERBOSE_OUTPUT
	if (packet->len > 0) {
		debug("Data: ");
		uint16 j;
		for (j=0; j<packet->len-2; j++) {
			debug("0x%02X ", packet->data[j]);
		}
	}
	debug("\n\n");
#endif
#endif
	
	if (emtr_command_done != NULL) {
		(*emtr_command_done)(packet);
	}
	
clear:	
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
			if (emtr_ask_only) {
				state = EMTR_STATE_IDLE;
				emtr_receive_packet->ask_only = true;
				emtr_receive(emtr_receive_packet);
			} else {
				state = EMTR_STATE_LEN;
			}
			return;
		}
		
		if (c == EMTR_ERROR_CRC || c == EMTR_ERROR_NAK) {
			emtr_receive_packet->header = c;
			emtr_receive_packet->ask_only = true;
			emtr_receive(emtr_receive_packet);
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
	
	emtr_ask_only = packet->ask_only;
	emtr_command_done = packet->callback;
	packet->check_sum = emtr_check_sum(packet);
	packet->data[packet->len-3] = packet->check_sum;
	
#if EMTR_DEBUG
	debug("\nEMTR: Sending packet%s...\n", packet->ask_only ? " [ASK-ONLY]" : "");
	debug("Header: 0x%02X\n", packet->header);
	debug("Len: 0x%02X\n", packet->len);
	debug("Checksum: 0x%02X\n", packet->check_sum);
#if EMTR_VERBOSE_OUTPUT
	debug("Data: ");
	uint16 j;
	for (j=0; j<packet->len-2; j++) {
		debug("0x%02X ", packet->data[j]);
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
		emtr_set_timeout_callback(emtr_clear_timeout);
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

LOCAL emtr_set_int(uint32 value, uint8 *data, uint8 offset, uint8 size) {
	uint8 i;
	for (i=0; i<size; i++) {
		data[offset+i] = ((value >> (i * 8)) & 0xFF);
	}
}

void ICACHE_FLASH_ATTR emtr_parse_output(emtr_packet *packet, emtr_output_registers *registers) {
	if (packet->len != EMTR_OUT_LEN+3) {
		os_memset(registers, 0, sizeof(emtr_output_registers));
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

void ICACHE_FLASH_ATTR emtr_get_output(emtr_callback command_done) {
#if EMTR_DEBUG
	debug("emtr_get_output()\n");
#endif
	emtr_packet_init(&emtr_send_packet, EMTR_START_FRAME, 8);
	
	emtr_send_packet->data[0] = 0x41; // Set adress pointer
	emtr_send_packet->data[1] = 0x00; // address
	emtr_send_packet->data[2] = 0x04; // address
	emtr_send_packet->data[3] = 0x4E; // Read N bytes
	emtr_send_packet->data[4] = EMTR_OUT_LEN;
	
	emtr_send_packet->ask_only = false;
	emtr_send_packet->callback = command_done;
	
	emtr_send(emtr_send_packet);
}

void ICACHE_FLASH_ATTR emtr_parse_event(emtr_packet *packet, emtr_event_registers *registers) {
#if EMTR_DEBUG
	debug("emtr_parse_event()\n");
#endif
	
	if (packet->len != EMTR_EVENTS_LEN+3) {
		os_memset(registers, 0, sizeof(emtr_event_registers));
		return;
	}
	
	registers->calibration_current        = emtr_extract_int(packet->data,  0, 4);
	registers->calibration_voltage        = emtr_extract_int(packet->data,  4, 2);
	registers->calibration_active_power   = emtr_extract_int(packet->data,  6, 4);
	registers->calibration_reactive_power = emtr_extract_int(packet->data, 10, 4);
	registers->accumulation_interval      = emtr_extract_int(packet->data, 14, 2);
	registers->reserved_00                = emtr_extract_int(packet->data, 16, 2);
	
	registers->over_current_limit         = emtr_extract_int(packet->data, 18, 4);
	registers->reserved_01                = emtr_extract_int(packet->data, 22, 2);
	registers->over_power_limit           = emtr_extract_int(packet->data, 24, 4);
	registers->reserved_02                = emtr_extract_int(packet->data, 28, 2);
	registers->over_frequency_limit       = emtr_extract_int(packet->data, 30, 2);
	registers->under_frequency_limit      = emtr_extract_int(packet->data, 32, 2);
	registers->over_temperature_limit     = emtr_extract_int(packet->data, 34, 2);
	registers->under_temperature_limit    = emtr_extract_int(packet->data, 36, 2);
	registers->voltage_sag_limit          = emtr_extract_int(packet->data, 38, 2);
	registers->voltage_surge_limit        = emtr_extract_int(packet->data, 40, 2);
	registers->over_current_hold          = emtr_extract_int(packet->data, 42, 2);
	registers->reserved_03                = emtr_extract_int(packet->data, 44, 2);
	registers->over_power_hold            = emtr_extract_int(packet->data, 46, 2);
	registers->reserved_04                = emtr_extract_int(packet->data, 48, 2);
	registers->over_frequency_hold        = emtr_extract_int(packet->data, 50, 2);
	registers->under_frequency_hold       = emtr_extract_int(packet->data, 52, 2);
	registers->over_temperature_hold      = emtr_extract_int(packet->data, 54, 2);
	registers->under_temperature_hold     = emtr_extract_int(packet->data, 56, 2);
	registers->reserved_05                = emtr_extract_int(packet->data, 58, 2);
	registers->reserved_06                = emtr_extract_int(packet->data, 60, 2);
	registers->event_enable               = emtr_extract_int(packet->data, 62, 2);
	registers->event_mask_critical        = emtr_extract_int(packet->data, 64, 2);
	registers->event_mask_standard        = emtr_extract_int(packet->data, 66, 2);
	registers->event_test                 = emtr_extract_int(packet->data, 68, 2);
	registers->event_clear                = emtr_extract_int(packet->data, 70, 2);
}

void ICACHE_FLASH_ATTR emtr_get_event(emtr_callback command_done) {
#if EMTR_DEBUG
	debug("emtr_get_event()\n");
#endif
	emtr_packet_init(&emtr_send_packet, EMTR_START_FRAME, 8);
	
	emtr_send_packet->data[0] = 0x41; // Set adress pointer
	emtr_send_packet->data[1] = 0x00; // address
	emtr_send_packet->data[2] = 0x4C; // address
	emtr_send_packet->data[3] = 0x4E; // Read N bytes
	emtr_send_packet->data[4] = EMTR_EVENTS_LEN;
	
	emtr_send_packet->ask_only = false;
	emtr_send_packet->callback = command_done;
	
	emtr_send(emtr_send_packet);
}

void ICACHE_FLASH_ATTR emtr_set_event(emtr_event_registers *registers, emtr_callback command_done) {
#if EMTR_DEBUG
	debug("emtr_set_event()\n");
#endif
	emtr_packet_init(&emtr_send_packet, EMTR_START_FRAME, 2+5+EMTR_EVENTS_LEN+2+1);
	
	emtr_send_packet->data[0] = 0x41; // Set adress pointer
	emtr_send_packet->data[1] = 0x00; // address
	emtr_send_packet->data[2] = 0x4C; // address
	emtr_send_packet->data[3] = 0x4D; // Write N bytes
	emtr_send_packet->data[4] = EMTR_EVENTS_LEN;
	
	emtr_set_int(registers->calibration_current,        emtr_send_packet->data,  0+5, 4);
	emtr_set_int(registers->calibration_voltage,        emtr_send_packet->data,  4+5, 2);
	emtr_set_int(registers->calibration_active_power,   emtr_send_packet->data,  6+5, 4);
	emtr_set_int(registers->calibration_reactive_power, emtr_send_packet->data, 10+5, 4);
	emtr_set_int(registers->accumulation_interval,      emtr_send_packet->data, 14+5, 2);
	emtr_set_int(registers->reserved_00,                emtr_send_packet->data, 16+5, 2);
	
	emtr_set_int(registers->over_current_limit,         emtr_send_packet->data, 18+5, 4);
	emtr_set_int(registers->reserved_01,                emtr_send_packet->data, 22+5, 2);
	emtr_set_int(registers->over_power_limit,           emtr_send_packet->data, 24+5, 4);
	emtr_set_int(registers->reserved_02,                emtr_send_packet->data, 28+5, 2);
	emtr_set_int(registers->over_frequency_limit,       emtr_send_packet->data, 30+5, 2);
	emtr_set_int(registers->under_frequency_limit,      emtr_send_packet->data, 32+5, 2);
	emtr_set_int(registers->over_temperature_limit,     emtr_send_packet->data, 34+5, 2);
	emtr_set_int(registers->under_temperature_limit,    emtr_send_packet->data, 36+5, 2);
	emtr_set_int(registers->voltage_sag_limit,          emtr_send_packet->data, 38+5, 2);
	emtr_set_int(registers->voltage_surge_limit,        emtr_send_packet->data, 40+5, 2);
	emtr_set_int(registers->over_current_hold,          emtr_send_packet->data, 42+5, 2);
	emtr_set_int(registers->reserved_03,                emtr_send_packet->data, 44+5, 2);
	emtr_set_int(registers->over_power_hold,            emtr_send_packet->data, 46+5, 2);
	emtr_set_int(registers->reserved_04,                emtr_send_packet->data, 48+5, 2);
	emtr_set_int(registers->over_frequency_hold,        emtr_send_packet->data, 50+5, 2);
	emtr_set_int(registers->under_frequency_hold,       emtr_send_packet->data, 52+5, 2);
	emtr_set_int(registers->over_temperature_hold,      emtr_send_packet->data, 54+5, 2);
	emtr_set_int(registers->under_temperature_hold,     emtr_send_packet->data, 56+5, 2);
	emtr_set_int(registers->reserved_05,                emtr_send_packet->data, 58+5, 2);
	emtr_set_int(registers->reserved_06,                emtr_send_packet->data, 60+5, 2);
	emtr_set_int(registers->event_enable,               emtr_send_packet->data, 62+5, 2);
	emtr_set_int(registers->event_mask_critical,        emtr_send_packet->data, 64+5, 2);
	emtr_set_int(registers->event_mask_standard,        emtr_send_packet->data, 66+5, 2);
	emtr_set_int(registers->event_test,                 emtr_send_packet->data, 68+5, 2);
	emtr_set_int(registers->event_clear,                emtr_send_packet->data, 70+5, 2);
	
	emtr_send_packet->data[EMTR_EVENTS_LEN+5] = 0x53;           // Save registers to flash
	emtr_send_packet->data[EMTR_EVENTS_LEN+6] = emtr_address(); // Device address
	
	emtr_send_packet->ask_only = true;
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
#if EMTR_DEBUG
	debug("emtr_get_address()\n");
#endif
	emtr_packet_init(&emtr_send_packet, EMTR_START_FRAME, 7);
	
	emtr_send_packet->data[0] = 0x41; // Set adress pointer
	emtr_send_packet->data[1] = 0x00; // address
	emtr_send_packet->data[2] = 0x26; // address
	emtr_send_packet->data[3] = 0x52; // Read 2 bytes
	
	emtr_send_packet->ask_only = false;
	emtr_send_packet->callback = command_done;
	
	emtr_send(emtr_send_packet);
}

void ICACHE_FLASH_ATTR emtr_clear_event(uint16 event, emtr_callback command_done) {
#if EMTR_DEBUG
	debug("emtr_clear_event()\n");
#endif
	emtr_packet_init(&emtr_send_packet, EMTR_START_FRAME, 9);
	
	emtr_send_packet->data[0] = 0x41; // Set adress pointer
	emtr_send_packet->data[1] = 0x00; // address
	emtr_send_packet->data[2] = 0x92; // address
	emtr_send_packet->data[3] = 0x57; // Write 2 bytes
	emtr_send_packet->data[4] = (event >> 8) & 0xFF; // event high
	emtr_send_packet->data[5] = event & 0xFF;        // event low
	
	emtr_send_packet->ask_only = true;
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
