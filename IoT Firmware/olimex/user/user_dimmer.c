#include "user_config.h"
#if DEVICE == DIMMER

#include "ets_sys.h"
#include "stdout.h"
#include "osapi.h"
#include "queue.h"

#include "json/jsonparse.h"

#include "user_json.h"
#include "user_misc.h"
#include "user_webserver.h"
#include "user_dimmer.h"
#include "user_devices.h"
#include "modules/dimmer.h"

char* const DIMMER_URLs[] = {
	DIMMER_URL, 
	DIMMER_URL_ANY
};

const uint8 DIMMER_URLs_COUNT = sizeof(DIMMER_URLs) / sizeof(char *);

LOCAL uint32 dimmer_refresh   = DIMMER_REFRESH_DEFAULT;
LOCAL uint8  dimmer_each      = DIMMER_EACH_DEFAULT;
LOCAL uint32 dimmer_threshold = DIMMER_THRESHOLD_DEFAULT;

LOCAL uint32 dimmer_current_timer = 0;

LOCAL void ICACHE_FLASH_ATTR dimmer_response(i2c_config *config, char *response, bool poll) {
	dimmer_config_data *config_data = (dimmer_config_data *)config->data;
	
	char address_str[MAX_I2C_ADDRESS];
	json_i2c_address(address_str, config->address);
	
	char poll_str[WEBSERVER_MAX_VALUE];
	if (poll) {
		json_poll_str(poll_str, dimmer_refresh / 1000, dimmer_each, dimmer_threshold);
	} else {
		poll_str[0] = '\0';
	}
		
	char data_str[WEBSERVER_MAX_VALUE];
	json_data(
		response, DIMMER_STR, OK_STR,
		json_sprintf(
			data_str,
			"\"Relay\" : %d, "
			"\"Brightness\" : %d,"
			"\"Current\" : %d"
			"%s",
			config_data->relay, 
			config_data->brightness,
			config_data->current,
			poll_str
		),
		address_str
	);
}


LOCAL void ICACHE_FLASH_ATTR dimmer_event(i2c_config *config) {
	LOCAL uint8 count = 0;
	i2c_status status;
	
	dimmer_config_data *config_data = (dimmer_config_data *)config->data;
	uint8  old_relay = config_data->relay;
	uint8  old_brightness = config_data->brightness;
	uint16 old_current = config_data->current;
	
	count++;
	uint8 addr = config->address;
	dimmer_init(&addr, &status);
	
	if (
		status == I2C_OK && 
		(
			config_data->relay != old_relay ||
			config_data->brightness != old_brightness ||
			abs(config_data->current - old_current) > dimmer_threshold || 
			(count > dimmer_each && config_data->current != old_current)
		)
	) {
		count = 0;
		
		char response[WEBSERVER_MAX_VALUE];
		dimmer_response(config, response, false);
		user_event_raise(DIMMER_URL, response);
	}
	
	if (count > dimmer_each) {
		count = 0;
	}
}

LOCAL void ICACHE_FLASH_ATTR dimmer_timer_init() {
	if (dimmer_current_timer != 0) {
		clearInterval(dimmer_current_timer);
	}
	
	if (dimmer_refresh == 0) {
		dimmer_current_timer = 0;
	} else {
		dimmer_current_timer = setInterval((os_timer_func_t *)dimmer_foreach, dimmer_event, dimmer_refresh);
	}
}

void ICACHE_FLASH_ATTR dimmer_handler(
	struct espconn *pConnection, 
	request_method method, 
	char *url, 
	char *data, 
	uint16 data_len, 
	uint32 content_len, 
	char *response,
	uint16 response_len
) {
	i2c_status status = I2C_OK;
	i2c_config *config = i2c_init_handler(DIMMER_STR, DIMMER_URL, dimmer_init, url, response);
	if (config == NULL) {
		return;
	}
	
	dimmer_config_data *config_data = (dimmer_config_data *)config->data;
	
	struct jsonparse_state parser;
	int type;

	if (method == POST && data != NULL && data_len != 0) {
		jsonparse_setup(&parser, data, data_len);

		while ((type = jsonparse_next(&parser)) != 0) {
			if (type == JSON_TYPE_PAIR_NAME) {
				if (jsonparse_strcmp_value(&parser, "Relay") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					config_data->relay = jsonparse_get_value_as_int(&parser);
				} else if (jsonparse_strcmp_value(&parser, "Brightness") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					config_data->brightness = jsonparse_get_value_as_int(&parser);
				} else if (jsonparse_strcmp_value(&parser, "Refresh") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					dimmer_refresh = jsonparse_get_value_as_int(&parser) * 1000;
				} else if (jsonparse_strcmp_value(&parser, "Each") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					dimmer_each = jsonparse_get_value_as_int(&parser);
				} else if (jsonparse_strcmp_value(&parser, "Threshold") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					dimmer_threshold = jsonparse_get_value_as_int(&parser);
				}
			}
		}
		
		dimmer_timer_init();
		status = dimmer_set(config);
	}
	
	char address_str[MAX_I2C_ADDRESS];
	json_i2c_address(address_str, config->address);
	
	if (status == I2C_OK) {
		dimmer_response(config, response, true);
	} else {
		json_error(response, DIMMER_STR, i2c_status_str(status), address_str);
	}
}

void ICACHE_FLASH_ATTR user_dimmer_init() {
	uint8 addr = 0;
	i2c_status status;
	#ifdef DIMMER_REV_A   // DIMMER - Revision A
	stdout_disable();
	#endif
	dimmer_init(&addr, &status);
	
	uint8 i;
	for (i=0; i<DIMMER_URLs_COUNT; i++) {
		webserver_register_handler_callback(DIMMER_URLs[i], dimmer_handler);
	}
	
	device_register(I2C, DIMMER_ID, DIMMER_URL, NULL, NULL);
	
	if (status == I2C_OK) {
		dimmer_timer_init();
	}
}

#endif
