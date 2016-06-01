#include "user_config.h"
#if MOD_TC_MK2_ENABLE & I2C_ENABLE

#include "ets_sys.h"
#include "stdout.h"
#include "osapi.h"
#include "queue.h"

#include "json/jsonparse.h"

#include "user_json.h"
#include "user_webserver.h"
#include "user_mod_tc_mk2.h"
#include "user_devices.h"
#include "user_timer.h"

#include "modules/mod_tc_mk2.h"

LOCAL uint32 tc_refresh   = MOD_TC_MK2_REFRESH_DEFAULT;
LOCAL uint8  tc_each      = MOD_TC_MK2_EACH_DEFAULT;
LOCAL uint32 tc_threshold = MOD_TC_MK2_THRESHOLD_DEFAULT;

LOCAL uint32 tc_refresh_timer = 0;

char* const MOD_TC_MK2_URLs[] = {
	MOD_TC_MK2_URL, 
	MOD_TC_MK2_URL_ANY
};

const uint8 MOD_TC_MK2_URLs_COUNT = sizeof(MOD_TC_MK2_URLs) / sizeof(char *);

LOCAL void ICACHE_FLASH_ATTR mod_tc_mk2_read(i2c_config *config, char *response, bool poll) {
	poll = true;
	
	tc_config_data *config_data = (tc_config_data *)config->data;
	
	char address_str[MAX_I2C_ADDRESS];
	json_i2c_address(address_str, config->address);
	
	i2c_status status = tc_read(config);
	if (status == I2C_OK) {
		char poll_str[WEBSERVER_MAX_VALUE];
		if (poll) {
			json_poll_str(poll_str, tc_refresh / 1000, tc_each, tc_threshold);
		} else {
			poll_str[0] = '\0';
		}
		
		char data_str[WEBSERVER_MAX_VALUE];
		json_data(
			response, MOD_TC_MK2, OK_STR,
			json_sprintf(
				data_str,
				"\"Temperature\" : %s %s",
				config_data->temperature_str,
				poll_str
			),
			address_str
		);
	} else {
		json_error(response, MOD_TC_MK2, i2c_status_str(status), address_str);
	}
}

LOCAL void ICACHE_FLASH_ATTR mod_tc_mk2_event(i2c_config *config) {
	char response[WEBSERVER_MAX_RESPONSE_LEN];
	LOCAL sint16 old_temperature = 0;
	LOCAL int count = 0;

	i2c_status status;
	
	tc_config_data *config_data = (tc_config_data *)config->data;
	uint8 addr = config->address;
	tc_init(&addr, &status);
	if (status != I2C_OK) {
		return;
	}
	
	if (config_data->temperature > old_temperature) {
		count++;
	} else if (config_data->temperature < old_temperature) {
		count--;
	} else {
		count = 0;
	}
	
	mod_tc_mk2_read(config, response, false);
	if (
		abs(config_data->temperature - old_temperature) > tc_threshold 
		|| 
		(abs(count) > tc_each && config_data->temperature != old_temperature)
	) {
#if MOD_TC_MO2_DEBUG
		debug("MOD-TC-MK2: Temperature change [%d] -> [%d]\n", old_temperature, config_data->temperature);
#endif		
		count = 0;
		old_temperature = config_data->temperature;
		
		user_event_raise(MOD_TC_MK2_URL, response);
	}
}

LOCAL void ICACHE_FLASH_ATTR mod_tc_mk2_timer_init() {
	if (tc_refresh_timer != 0) {
		clearInterval(tc_refresh_timer);
	}
	
	if (tc_refresh == 0) {
		tc_refresh_timer = 0;
	} else {
		tc_refresh_timer = setInterval((os_timer_func_t *)tc_foreach, mod_tc_mk2_event, tc_refresh);
	}
}

void ICACHE_FLASH_ATTR mod_tc_mk2_handler(
	struct espconn *pConnection, 
	request_method method, 
	char *url, 
	char *data, 
	uint16 data_len, 
	uint32 content_len, 
	char *response,
	uint16 response_len
) {
	i2c_status status;
	i2c_config *config = i2c_init_handler("MOD-TC-MK2", MOD_TC_MK2_URL, tc_init, url, response);
	if (config == NULL) {
		return;
	}
	
	tc_config_data *config_data = (tc_config_data *)config->data;
	
	struct jsonparse_state parser;
	int type;

	if (method == POST && data != NULL && data_len != 0) {
		jsonparse_setup(&parser, data, data_len);

		while ((type = jsonparse_next(&parser)) != 0) {
			if (type == JSON_TYPE_PAIR_NAME) {
				if (jsonparse_strcmp_value(&parser, "Refresh") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					tc_refresh = jsonparse_get_value_as_int(&parser) * 1000;
				} else if (jsonparse_strcmp_value(&parser, "Each") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					tc_each = jsonparse_get_value_as_int(&parser);
				} else if (jsonparse_strcmp_value(&parser, "Threshold") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					tc_threshold = jsonparse_get_value_as_int(&parser);
				}
			}
		}
		
		mod_tc_mk2_timer_init();
	}
	
	mod_tc_mk2_read(config, response, true);
}

void ICACHE_FLASH_ATTR mod_tc_mk2_init() {
	uint8 addr = 0;
	i2c_status status;
	tc_init(&addr, &status);
	
	uint8 i;
	for (i=0; i<MOD_TC_MK2_URLs_COUNT; i++) {
		webserver_register_handler_callback(MOD_TC_MK2_URLs[i], mod_tc_mk2_handler);
	}
	
	device_register(I2C, MOD_TC_MK2_ID, MOD_TC_MK2, MOD_TC_MK2_URL, NULL, NULL);
	
	if (status == I2C_OK) {
		mod_tc_mk2_timer_init();
	}
}
#endif
