#include "ets_sys.h"
#include "osapi.h"
#include "mem.h"
#include "stdout.h"

#include "user_timer.h"
#include "user_devices.h"
#include "user_json.h"

#include "driver/uart.h"
#include "mod_finger.h"

LOCAL finger_packet *finger_receive_packet = NULL;
LOCAL finger_packet *finger_send_packet    = NULL;

LOCAL finger_sys_params finger_params = {
	.status         = 0,
	.id             = 0,
	.db_size        = 0,
	.security_level = 0,
	.address        = 0xFFFFFFFF,
	.packet_size    = 0,
	.baud_rate      = 0,
	
	.password       = 0x00000000,
	.db_stored      = 0,
	.first_free_id  = -1
};

LOCAL bool finger_present = false;
LOCAL finger_callback finger_command_done    = NULL;
LOCAL finger_callback finger_command_timeout = NULL;
LOCAL finger_callback finger_user_callback   = NULL;

LOCAL timer *finger_timeout = NULL;

const char ICACHE_FLASH_ATTR *finger_error_str(uint8 status) {
	switch (status) {
		case FINGER_OK                  : return "OK";
		case FINGER_PACKET_ERROR        : return "Invalid packet";
		case FINGER_NO_FINGER_ON_SENSOR : return "No finger on sensor";
		case FINGER_INPUT_IMAGE_FAIL    : return "Input image failure";
		case FINGER_IMAGE_MESSY         : return "Image is messy";
		case FINGER_IMAGE_TOO_SMALL     : return "Image is too small";
		case FINGER_MISMATCH            : return "Mismatch";
		case FINGER_NOT_FOUND           : return "Not found";
		case FINGER_MERGE_FAILED        : return "Merge failed";
		case FINGER_OUT_OF_RANGE        : return "Out of range";
		case FINGER_READ_ERROR          : return "Read error";
		case FINGER_UPLOAD_ERROR        : return "Upload error";
		case FINGER_DATA_FAIL           : return "Data failure";
		case FINGER_UPLOAD_IMAGE_FAIL   : return "Upload image failure";
		case FINGER_DELETE_FAIL         : return "Delete failure";
		case FINGER_DATABASE_FAIL       : return "Database failure";
		case FINGER_WRONG_PASSWORD      : return "Wrong password";
		case FINGER_BUFFER_FAIL         : return "Buffer failure";
		case FINGER_FLASH_IO_ERROR      : return "Flash IO error";
		case FINGER_INVALID_REGISTER    : return "Invalid register";
		case FINGER_WRONG_ADDRESS       : return "Wrong address";
		case FINGER_CHECK_PASSWORD      : return "Password MUST be verified";
	}
}

LOCAL void ICACHE_FLASH_ATTR finger_default_timeout(finger_packet *packet) {
	debug("FINGER: %s\n", TIMEOUT);
}

void ICACHE_FLASH_ATTR finger_set_timeout_callback(finger_callback command_timeout) {
	finger_command_timeout = command_timeout;
}

LOCAL void ICACHE_FLASH_ATTR finger_packet_init(finger_packet **packet, uint32 address, finger_pid pid, uint16 len) {
	if (*packet == NULL) {
		*packet = (finger_packet *)os_zalloc(sizeof(finger_packet));
	}
	
	(*packet)->address = address;
	(*packet)->pid = pid;
	(*packet)->len = len;
	
	if ((*packet)->data != NULL) {
		os_free((*packet)->data);
	}
	
	(*packet)->data = (len == 0 ?
		NULL
		:
		(uint8 *)os_zalloc(len)
	);
	
	(*packet)->callback = NULL;
}

LOCAL uint16 ICACHE_FLASH_ATTR finger_check_sum(finger_packet *packet) {
	uint16 check_sum = packet->pid + packet->len;
	uint16 i;
	for (i=0; i<packet->len-2; i++) {
		check_sum += packet->data[i];
	}
	return check_sum;
}

LOCAL void ICACHE_FLASH_ATTR finger_send(finger_packet *packet) {
	if (finger_timeout != NULL) {
		setTimeout((os_timer_func_t *)finger_send, packet, 100);
		return;
	}
	
#if FINGER_DEBUG
#if FINGER_VERBOSE_OUTPUT
	debug("\nFINGER: Sending packet...\n");
	debug("Address: 0x%08x\n", packet->address);
	debug("PID: 0x%02x\n", packet->pid);
	debug("Len: 0x%04x\n", packet->len);
	debug("Data: ");
	uint16 j;
	for (j=0; j<packet->len; j++) {
		debug("0x%02x ", packet->data[j]);
	}
	debug("\n\n");
#endif
#endif
	
	finger_command_done = packet->callback;
	
	// Start
	uart_write_byte(0xEF);
	uart_write_byte(0x01);
	
	// Address
	uint8 i;
	for (i=0; i<4; i++) {
		uart_write_byte(packet->address >> (8 * (3-i)) & 0xFF);
	}
	
	// PID
	uart_write_byte(packet->pid);
	
	// Len
	uart_write_byte((packet->len) >> 8);
	uart_write_byte((packet->len) & 0xFF);

	// Data
	uart_write_buff(packet->data, packet->len);
	
	if (finger_command_timeout == NULL) {
		finger_set_timeout_callback(finger_default_timeout);
	}
	finger_timeout = setTimeout((os_timer_func_t *)finger_command_timeout, packet, FINGER_TIMEOUT);
}

LOCAL void ICACHE_FLASH_ATTR finger_receive(finger_packet *packet) {
	if (packet == NULL) {
		debug("FINGER: Not initialized\n");
		return;
	}
	
	if (packet->check_sum != finger_check_sum(packet)) {
		debug("FINGER: Checksum not match\n");
		return;
	}
	
	finger_present = true;
	device_set_uart(UART_FINGER);
	
#if FINGER_DEBUG
#if FINGER_VERBOSE_OUTPUT
	debug("\nFINGER: Received packet\n");
	debug("Address: 0x%08x\n", packet->address);
	debug("PID: 0x%02x\n", packet->pid);
	debug("Len: 0x%04x\n", packet->len);
	debug("Data: ");
	uint16 i;
	for (i=0; i<packet->len; i++) {
		debug("0x%02x ", packet->data[i]);
	}
	debug("\n\n");
#endif
#endif
	
	if (packet->data[0] == FINGER_WRONG_ADDRESS) {
		debug("FINGER: Address not match. Resend packet\n");
		finger_params.address = packet->address;
		finger_send_packet->address = packet->address;
		setTimeout((os_timer_func_t *)finger_send, finger_send_packet, 100);
		return;
	}
	
	if (finger_command_done != NULL) {
		(*finger_command_done)(packet);
	}
	
	clearTimeout(finger_timeout);
	finger_timeout = NULL;
}

LOCAL void ICACHE_FLASH_ATTR finger_char_in(char c) {
LOCAL finger_state state = FINGER_STATE_IDLE;
LOCAL uint16 count = 0;
	
	if (state == FINGER_STATE_IDLE || state == FINGER_STATE_ERROR) {
		finger_packet_init(&finger_receive_packet, 0, 0, 0);
		if (c == 0xEF) {
			state = FINGER_STATE_START;
		} else {
			state = FINGER_STATE_ERROR;
		}
		return;
	}
	
	if (state == FINGER_STATE_START) {
		if (c == 0x01) {
			state = FINGER_STATE_ADDRESS;
			count = 0;
			finger_receive_packet->address = 0;
		} else {
			state = FINGER_STATE_ERROR;
		}
		return;
	}
	
	if (state == FINGER_STATE_ADDRESS) {
		finger_receive_packet->address = finger_receive_packet->address*256 + (uint8)c;
		count++;
		if (count == 4) {
			state = FINGER_STATE_PID;
			finger_receive_packet->pid = 0;
		}
		return;
	}
	
	if (state == FINGER_STATE_PID) {
		finger_receive_packet->pid = c;
		
		state = FINGER_STATE_LEN;
		finger_receive_packet->len = 0;
		count = 0;
		return;
	}
	
	if (state == FINGER_STATE_LEN) {
		finger_receive_packet->len = finger_receive_packet->len*256 + (uint8)c;
		count++;
		if (count == 2) {
			state = FINGER_STATE_DATA;
			count = 0;
		}
		return;
	}
	
	if (state == FINGER_STATE_DATA) {
		if (count == 0) {
			if (finger_receive_packet->data != NULL) {
				os_free(finger_receive_packet->data);
			}
			finger_receive_packet->data = (uint8 *)os_zalloc(finger_receive_packet->len);
		}
		finger_receive_packet->data[count] = (uint8)c;
		count++;
		if (count == finger_receive_packet->len) {
			finger_receive_packet->check_sum = finger_receive_packet->data[count-1] + 256*finger_receive_packet->data[count-2];
			state = FINGER_STATE_IDLE;
			count = 0;
			finger_receive(finger_receive_packet);
		}
		return;
	}
}

LOCAL void ICACHE_FLASH_ATTR finger_execute(uint32 address, uint8 command, uint8 *data, uint16 len, finger_callback command_done) {
	finger_packet_init(&finger_send_packet, address, FINGER_COMMAND, len+3);
	
	finger_send_packet->data[0] = command;
	if (data != NULL && len != 0) {
		os_memcpy(finger_send_packet->data + 1, data, len);
	}
	
	finger_send_packet->check_sum = finger_check_sum(finger_send_packet);
	finger_send_packet->data[len + 1] = finger_send_packet->check_sum >> 8;
	finger_send_packet->data[len + 2] = finger_send_packet->check_sum & 0xFF;
	
	finger_send_packet->callback = command_done;
	
	finger_send(finger_send_packet);
}

bool ICACHE_FLASH_ATTR finger_found() {
	return finger_present;
}

uint32 ICACHE_FLASH_ATTR finger_address() {
	return finger_params.address;
}

uint16 ICACHE_FLASH_ATTR finger_db_size() {
	return finger_params.db_size;
}

uint16 ICACHE_FLASH_ATTR finger_db_stored() {
	return finger_params.db_stored;
}

uint16 ICACHE_FLASH_ATTR finger_first_free_id() {
	return finger_params.first_free_id;
}

uint16 ICACHE_FLASH_ATTR finger_security_level() {
	return finger_params.security_level;
}

uint16 ICACHE_FLASH_ATTR finger_packet_size() {
	return finger_params.packet_size;
}

uint16 ICACHE_FLASH_ATTR finger_baud_rate() {
	return finger_params.baud_rate;
}

LOCAL uint32 finger_extract_int(uint8 *data, uint16 offset, uint8 size) {
	uint16 i;
	uint32 result = 0;
	for (i=0; i<size; i++) {
		result = result*256 + data[offset+i];
	}
	return result;
}

LOCAL void finger_system_params_response(finger_packet *packet) {
	if (packet->data[0] != FINGER_OK) {
		goto done;
	}
	
	finger_params.status         = finger_extract_int(packet->data,  1, 2);
	finger_params.id             = finger_extract_int(packet->data,  3, 2);
	finger_params.db_size        = finger_extract_int(packet->data,  5, 2);
	finger_params.security_level = finger_extract_int(packet->data,  7, 2);
	finger_params.address        = finger_extract_int(packet->data,  9, 4);
	finger_params.packet_size    = 32 << finger_extract_int(packet->data, 13, 2);
	finger_params.baud_rate      = finger_extract_int(packet->data, 15, 2) * 9600;
	
#if FINGER_DEBUG
#if FINGER_VERBOSE_OUTPUT
	debug("\nFINGER: System Parameters\n");
	debug("Status:      %d\n",     finger_params.status);
	debug("ID:          %d\n",     finger_params.id);
	debug("DB Size:     %d\n",     finger_params.db_size);
	debug("Security:    %d\n",     finger_params.security_level);
	debug("Address:     0x%08x\n", finger_params.address);
	debug("Packet size: %d\n",     finger_params.packet_size);
	debug("Baud rate:   %d\n\n",   finger_params.baud_rate);
#endif
#endif

done:
	if (finger_user_callback != NULL) {
		(*finger_user_callback)(packet);
	}
}

LOCAL void finger_stored_count_response(finger_packet *packet) {
	if (packet->data[0] != FINGER_OK) {
		goto done;
	}
	
	finger_params.db_stored = finger_extract_int(packet->data,  1, 2);
	
done:
	if (finger_user_callback != NULL) {
		(*finger_user_callback)(packet);
	}
}

LOCAL void finger_password_response(finger_packet *packet) {
	if (packet->data[0] != FINGER_OK) {
		return;
	}
	setTimeout((os_timer_func_t *)finger_read_sys_params, NULL, 100);
}

LOCAL void finger_find_first_free();

LOCAL void ICACHE_FLASH_ATTR finger_read_index_response(finger_packet *packet) {
	if (packet->data[0] != FINGER_OK) {
		finger_params.first_free_id = 0;
		goto done;
	}
	
	uint8 i, j;
	for (i=0; i<32; i++) {
		if (packet->data[1+i] != 0xFF) {
			for (j=0; j<7; j++) {
				if (((packet->data[1+i] >> j) & 0x01) == 0) {
					finger_params.first_free_id += (i*8 + j);
					if (finger_params.first_free_id > 0) {
						goto done;
					}
				}
			}
		}
	}
	
	if ((finger_params.first_free_id >> 8) == 3) {
		finger_params.first_free_id = -1;
		goto done;
	}
	
	finger_params.first_free_id += 0x10;
	setTimeout(finger_find_first_free, NULL, 100);
	return;
	
done:
	if (finger_user_callback != NULL) {
		(*finger_user_callback)(packet);
	}
}

LOCAL void ICACHE_FLASH_ATTR finger_find_first_free() {
	finger_read_index(finger_params.first_free_id >> 8, finger_read_index_response);
}

void ICACHE_FLASH_ATTR finger_verify_password(finger_callback command_done) {
	uint8 i;
	uint8 password[4];
	
	for (i=0; i<4; i++) {
		password[i] = finger_params.password >> (8 * (3-i)) & 0xFF;
	}
	
	finger_execute(finger_params.address, 0x13, password, 4, command_done);
}

void ICACHE_FLASH_ATTR finger_read_sys_params(finger_callback command_done) {
	finger_user_callback = command_done;
	finger_execute(finger_params.address, 0x0F, NULL, 0, finger_system_params_response);
}

void ICACHE_FLASH_ATTR finger_set_address(uint32 address, finger_callback command_done) {
	uint8 i;
	uint8 new_address[4];
	
	for (i=0; i<4; i++) {
		new_address[i] = address >> (8 * (3-i)) & 0xFF;
	}
	finger_execute(finger_params.address, 0x15, new_address, 4, command_done);
}

void ICACHE_FLASH_ATTR finger_set_security_lefel(uint8 security_level, finger_callback command_done) {
	uint8 buff[2];
	buff[0] = 5;
	buff[1] = security_level;
	finger_execute(finger_params.address, 0x0E, buff, 2, command_done);
}

void ICACHE_FLASH_ATTR finger_gen_img(finger_callback command_done) {
	finger_execute(finger_params.address, 0x01, NULL, 0, command_done);
}

void ICACHE_FLASH_ATTR finger_img_to_tz(uint8 buffID, finger_callback command_done) {
	finger_execute(finger_params.address, 0x02, &buffID, 1, command_done);
}

void ICACHE_FLASH_ATTR finger_reg_model(finger_callback command_done) {
	finger_execute(finger_params.address, 0x05, NULL, 0, command_done);
}

void ICACHE_FLASH_ATTR finger_store(uint8 buffID, uint16 fingerID, finger_callback command_done) {
	uint8 buff[3];
	buff[0] = buffID;
	buff[1] = (fingerID >> 8) & 0xFF;
	buff[2] = fingerID & 0xFF;
	finger_execute(finger_params.address, 0x06, buff, 3, command_done);
}

void ICACHE_FLASH_ATTR finger_stored_count(finger_callback command_done) {
	finger_user_callback = command_done;
	finger_execute(finger_params.address, 0x1D, NULL, 0, finger_stored_count_response);
}

void ICACHE_FLASH_ATTR finger_read_index(uint8 pageID, finger_callback command_done) {
	finger_execute(finger_params.address, 0x1F, &pageID, 1, command_done);
}

void ICACHE_FLASH_ATTR finger_get_first_free(finger_callback command_done) {
	finger_user_callback = command_done;
	finger_params.first_free_id = 0;
	finger_find_first_free();
}

void ICACHE_FLASH_ATTR finger_remove(uint16 fingerID, finger_callback command_done) {
	uint8 buff[4];
	buff[0] = fingerID >> 8 & 0xFF;
	buff[1] = fingerID & 0xFF;
	buff[2] = 0x00;
	buff[3] = 0x01;
	finger_execute(finger_params.address, 0x0C, buff, 4, command_done);
}

void ICACHE_FLASH_ATTR finger_empty_db(finger_callback command_done) {
	finger_execute(finger_params.address, 0x0D, NULL, 0, command_done);
}

void ICACHE_FLASH_ATTR finger_search(uint8 buffID, finger_callback command_done) {
	uint8 buff[5];
	buff[0] = buffID;
	buff[1] = 0;
	buff[2] = 0;
	buff[3] = finger_params.db_size >> 8 & 0xFF;
	buff[4] = finger_params.db_size & 0xFF;
	finger_execute(finger_params.address, 0x04, buff, 5, command_done);
}

void ICACHE_FLASH_ATTR finger_init() {
	if (device_get_uart() != UART_NONE) {
		return;
	}
	
	stdout_disable();
	
	uart_char_in_set(finger_char_in);
	uart_init(BIT_RATE_57600, EIGHT_BITS, NONE_BITS, ONE_STOP_BIT);
	
	finger_set_timeout_callback(finger_default_timeout);
	
	// Verify password
	setTimeout((os_timer_func_t *)finger_verify_password, finger_password_response, 100);
}
