#include "ets_sys.h"
#include "stdout.h"
#include "osapi.h"
#include "mem.h"

#include "json/jsonparse.h"

#include "user_json.h"
#include "user_events.h"
#include "user_webserver.h"
#include "user_config.h"
#include "user_devices.h"

#include "user_mod_finger.h"
#include "modules/mod_finger.h"

LOCAL finger_mode finger_current_mode = FINGER_READ;
LOCAL uint8  finger_current_buff = 0;

LOCAL const char ICACHE_FLASH_ATTR *finger_mode_str(uint8 mode) {
	switch (mode) {
		case FINGER_READ    : return "Read";
		case FINGER_NEW      : return "New";
		case FINGER_DELETE   : return "Delete";
		case FINGER_EMPTY_DB : return "Empty DB";
	}
}

LOCAL void ICACHE_FLASH_ATTR finger_params_ready(finger_packet *packet) {
	char response[WEBSERVER_MAX_RESPONSE_LEN];
	char data_str[WEBSERVER_MAX_VALUE];
	
	json_data(
		response, MOD_FINGER, OK_STR,
		json_sprintf(
			data_str,
			"\"Address\" : \"0x%08X\", "
			"\"DBSize\" : %d, "
			"\"DBStored\" : %d, "
			"\"FirstFree\" : %d, "
			"\"SecurityLevel\" : %d, "
			"\"Mode\" : \"%s\"",
			finger_address(), 
			finger_db_size(),
			finger_db_stored(),
			finger_first_free_id(),
			finger_security_level(),
			finger_mode_str(finger_current_mode)
		),
		NULL
	);
	
	user_event_raise(FINGER_URL, response);
}

LOCAL void ICACHE_FLASH_ATTR finger_free_id(finger_packet *packet) {
	setTimeout(finger_get_first_free, finger_params_ready, 100);
}

LOCAL void ICACHE_FLASH_ATTR finger_db_stats(finger_packet *packet) {
	setTimeout(finger_stored_count, finger_free_id, 100);
}

LOCAL void ICACHE_FLASH_ATTR finger_frech_params() {
	setTimeout(finger_read_sys_params, finger_db_stats, 100);
}

LOCAL void ICACHE_FLASH_ATTR finger_read(finger_packet *packet);

LOCAL void ICACHE_FLASH_ATTR finger_start_read() {
LOCAL uint32 finger_read_timer = 0;
	if (device_get_uart() != UART_FINGER) {
#if FINGER_DEBUG
		debug("FINGER: %s\n", DEVICE_NOT_FOUND);
#endif
		return;
	}

	finger_current_buff = 0;
	clearTimeout(finger_read_timer);
	finger_read_timer = setTimeout(finger_gen_img, finger_read, FINGER_TIMEOUT);
}

LOCAL void ICACHE_FLASH_ATTR finger_timeout(finger_packet *packet) {
	char response[WEBSERVER_MAX_VALUE];
	json_error(response, MOD_FINGER, TIMEOUT, NULL);
	user_event_raise(FINGER_URL, response);
	finger_start_read();
}

LOCAL void ICACHE_FLASH_ATTR finger_default(finger_packet *packet) {
	if (packet->pid == FINGER_ACK && packet->data[0] != FINGER_OK) {
		char response[WEBSERVER_MAX_VALUE];
		json_error(response, MOD_FINGER, finger_error_str(packet->data[0]), NULL);
		user_event_raise(FINGER_URL, response);
	}
}

LOCAL void ICACHE_FLASH_ATTR finger_find_done(finger_packet *packet) {
	if (packet->data[0] != FINGER_OK) {
		finger_default(packet);
		finger_start_read();
		return;
	}
	
	uint16 id = packet->data[1]*256 + packet->data[2];
	uint16 score = packet->data[3]*256 + packet->data[4];
	
	char response[WEBSERVER_MAX_VALUE];
	char data[WEBSERVER_MAX_VALUE];
	json_status(
		response, MOD_FINGER, "Match found",
		json_sprintf(
			data,
			"\"Match\" : {\"ID\" : %d, \"Score\" : %d}",
			id,
			score
		)
	);
	
#if FINGER_DEBUG
	debug("FINGER: Match: ID = %d, Score = %d\n", id, score);
#endif
	
	user_event_raise(FINGER_URL, response);
	finger_start_read();
}

LOCAL void ICACHE_FLASH_ATTR finger_find(finger_packet *packet) {
	if (packet->data[0] != FINGER_OK) {
		finger_default(packet);
		finger_start_read();
		return;
	}
	
	finger_search(finger_current_buff, finger_find_done);
}

LOCAL void ICACHE_FLASH_ATTR finger_save_done(finger_packet *packet) {
	finger_current_mode = FINGER_READ;
	
	if (packet->data[0] != FINGER_OK) {
		finger_default(packet);
		finger_frech_params();
		finger_start_read();
		return;
	}
	
	char response[WEBSERVER_MAX_VALUE];
	char data[WEBSERVER_MAX_VALUE];
	json_status(
		response, MOD_FINGER, 
		"New fingerprint stored",
		json_sprintf(data, "\"ID\" : %d", finger_first_free_id())
	);
	
#if FINGER_DEBUG
	debug("FINGER: Stored: ID = %d\n", finger_first_free_id());
#endif
	
	user_event_raise(FINGER_URL, response);
	finger_frech_params();
	finger_start_read();
}

LOCAL void ICACHE_FLASH_ATTR finger_save(finger_packet *packet) {
	if (packet->data[0] != FINGER_OK) {
		finger_default(packet);
		finger_start_read();
		return;
	}
	
	finger_store(finger_current_buff, finger_first_free_id(), finger_save_done);
}

LOCAL void ICACHE_FLASH_ATTR finger_register(finger_packet *packet) {
	if (packet->data[0] != FINGER_OK) {
		finger_start_read();
		return;
	}
	
	if (finger_current_buff == 1) {
		finger_gen_img(finger_read);
		return;
	}
	
	finger_reg_model(finger_save);
}

LOCAL void ICACHE_FLASH_ATTR finger_read(finger_packet *packet) {
LOCAL bool finger_search_enable = true;
	
	if (packet->data[0] != FINGER_OK) {
		finger_search_enable = true;
		finger_start_read();
		return;
	}
	
	finger_current_buff = (finger_current_buff == 1 ? 2 : 1);
	
	if (finger_current_mode == FINGER_READ) {
		if (finger_search_enable) {
			finger_search_enable = false;
			finger_img_to_tz(finger_current_buff, finger_find);
		} else {
			finger_start_read();
		}
		return;
	}
	
	if (finger_current_mode == FINGER_NEW) {
		finger_img_to_tz(finger_current_buff, finger_register);
	}
}

void ICACHE_FLASH_ATTR finger_handler(
	struct espconn *pConnection, 
	request_method method, 
	char *url, 
	char *data, 
	uint16 data_len, 
	uint32 content_len, 
	char *response,
	uint16 response_len
) {
	if (device_get_uart() != UART_FINGER) {
		json_error(response, MOD_FINGER, DEVICE_NOT_FOUND, NULL);
		return;
	}
	
	struct jsonparse_state parser;
	int type, delete_len;
	uint16 delete_id;
	
	if (method == POST && data != NULL && data_len != 0) {
		jsonparse_setup(&parser, data, data_len);
		
		while ((type = jsonparse_next(&parser)) != 0) {
			if (type == JSON_TYPE_PAIR_NAME) {
				if (jsonparse_strcmp_value(&parser, "Address") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					
					char *convert_err = NULL;
					char address_str[20];
					jsonparse_copy_value(&parser, address_str, 20);
					uint32 address = strtoul(address_str, &convert_err, 16);
					
					if (*convert_err == '\0' && address != finger_address()) {
						finger_set_address(address, finger_default);
					}
				} else if (jsonparse_strcmp_value(&parser, "SecurityLevel") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					uint8 security_level = jsonparse_get_value_as_int(&parser);
					
					if (security_level != finger_security_level()) {
						finger_set_security_lefel(security_level, finger_default);
					}
				} else if (jsonparse_strcmp_value(&parser, "Mode") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					if (jsonparse_strcmp_value(&parser, "Read") == 0) {
						finger_current_mode = FINGER_READ;
					} else if (jsonparse_strcmp_value(&parser, "New") == 0) {
						finger_current_mode = FINGER_NEW;
					} else if (jsonparse_strcmp_value(&parser, "Delete") == 0) {
						finger_current_mode = FINGER_DELETE;
					} else if (jsonparse_strcmp_value(&parser, "Empty DB") == 0) {
						finger_current_mode = FINGER_EMPTY_DB;
					}
				} else if (jsonparse_strcmp_value(&parser, "DeleteID") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					delete_id = jsonparse_get_value_as_int(&parser);
					delete_len = jsonparse_get_len(&parser);
				}
			}
		}
		
		if (finger_current_mode == FINGER_DELETE && delete_len > 0) {
			finger_current_mode = FINGER_READ;
			finger_remove(delete_id, NULL);
#if FINGER_DEBUG
			debug("FINGER: Delete ID: %d\n", delete_id);
#endif
		}
		
		if (finger_current_mode == FINGER_EMPTY_DB) {
			finger_current_mode = FINGER_READ;
			finger_empty_db(NULL);
#if FINGER_DEBUG
			debug("FINGER: Empty DB\n");
#endif
		}
	}
	
	webserver_set_status(0);
	finger_frech_params();
	finger_start_read();
}

void ICACHE_FLASH_ATTR mod_finger_init() {
	finger_set_timeout_callback(finger_timeout);
	webserver_register_handler_callback(FINGER_URL, finger_handler);
	device_register(UART, 0, FINGER_URL, finger_init, NULL);
	
	setTimeout(finger_start_read, NULL, 2000);
}
