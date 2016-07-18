#include "user_config.h"
#if DEVICE == ROBKO

#include "ets_sys.h"
#include "stdout.h"
#include "osapi.h"
#include "queue.h"

#include "json/jsonparse.h"

#include "user_json.h"
#include "user_misc.h"
#include "user_webserver.h"
#include "user_robko.h"
#include "user_devices.h"
#include "modules/robko.h"

char* const ROBKO_URLs[] = {
	ROBKO_URL, 
	ROBKO_URL_ANY
};

const uint8 ROBKO_URLs_COUNT = sizeof(ROBKO_URLs) / sizeof(char *);

LOCAL uint32 robko_timer = 0;

LOCAL void ICACHE_FLASH_ATTR robko_response(i2c_config *config, char *response, char *address_str, bool limits) {
	robko_config_data *config_data = (robko_config_data *)config->data;
	
	char data_str[WEBSERVER_MAX_RESPONSE_LEN / 2];
	char limit_str[WEBSERVER_MAX_RESPONSE_LEN / 2];
	
	json_data(
		response, ROBKO_STR, OK_STR,
		json_sprintf(
			data_str,
			"\"Events\": %d, "
			"\"EventsPress\": %d, "
			"\"EventsRelease\": %d, "
			"\"EventsDoubleClick\": %d, "
			
			"\"Potentiometer\": [%d, %d, %d, %d, %d, %d], "
			
			"\"ServoReady\": %d, "
			"\"Servo\": [%d, %d, %d, %d, %d, %d]"
			"%s",
			
			config_data->events,
			config_data->events_press,
			config_data->events_release,
			config_data->events_dbl_click,
			
			config_data->potentiometer[0], 
			config_data->potentiometer[1], 
			config_data->potentiometer[2], 
			config_data->potentiometer[3], 
			config_data->potentiometer[4], 
			config_data->potentiometer[5], 
			
			config_data->servo_ready,
			
			config_data->servo[0], 
			config_data->servo[1], 
			config_data->servo[2], 
			config_data->servo[3], 
			config_data->servo[4], 
			config_data->servo[5], 
			
			limits ? 
				json_sprintf(
					limit_str,
					", "
					"\"LimitLow\": [%d, %d, %d, %d, %d, %d], "
					"\"LimitHigh\": [%d, %d, %d, %d, %d, %d], "
					"\"LimitSpeed\": %d",
					
					config_data->limit_low[0], 
					config_data->limit_low[1], 
					config_data->limit_low[2], 
					config_data->limit_low[3], 
					config_data->limit_low[4], 
					config_data->limit_low[5], 
					
					config_data->limit_high[0], 
					config_data->limit_high[1], 
					config_data->limit_high[2], 
					config_data->limit_high[3], 
					config_data->limit_high[4], 
					config_data->limit_high[5], 
					
					config_data->limit_speed
				)
				:
				""
		),
		address_str
	);
}

LOCAL void ICACHE_FLASH_ATTR robko_event(i2c_config *config) {
	i2c_status status = robko_read_events(config, true);
	if (status != I2C_OK) {
		debug("ROBKO: Event read failed\n");
		return;
	}
	
	robko_config_data *data = config->data;
	if ((data->events & ROBKO_EVENTS_NEW_MASK) == 0) {
		return;
	}
	
	char response[WEBSERVER_MAX_RESPONSE_LEN];
	
	char address_str[MAX_I2C_ADDRESS];
	json_i2c_address(address_str, config->address);
	
	robko_response(config, response, address_str, false);
	user_event_raise(ROBKO_URL, response);
	
	// Clear new events bit
	data->events = data->events & 0x7F;
}

LOCAL void ICACHE_FLASH_ATTR robko_timer_init() {
	if (robko_timer != 0) {
		clearInterval(robko_timer);
	}
	
	robko_timer = setInterval((os_timer_func_t *)robko_foreach, robko_event, ROBKO_READ_INTERVAL);
}

void ICACHE_FLASH_ATTR robko_handler(
	struct espconn *pConnection, 
	request_method method, 
	char *url, 
	char *data, 
	uint16 data_len, 
	uint32 content_len, 
	char *response,
	uint16 response_len
) {
	LOCAL limits = true;
	
	i2c_status status = I2C_OK;
	i2c_config *config = i2c_init_handler(ROBKO_STR, ROBKO_URL, robko_init, url, response);
	if (config == NULL) {
		return;
	}
	
	struct jsonparse_state parser;
	int type;
	
	if (method == POST && data != NULL && data_len != 0) {
		jsonparse_setup(&parser, data, data_len);
		
		while ((type = jsonparse_next(&parser)) != 0) {
			if (type == JSON_TYPE_PAIR_NAME) {
				
				uint8 i;
				char name[20];
				
				for (i=0; i<6; i++) {
					os_sprintf(name, "LimitLow%d", i);
					if (jsonparse_strcmp_value(&parser, name) == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						status = robko_set_limit_low(config, i, jsonparse_get_value_as_int(&parser));
						limits = true;
						break;
					}
					
					os_sprintf(name, "LimitHigh%d", i);
					if (jsonparse_strcmp_value(&parser, name) == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						status = robko_set_limit_high(config, i, jsonparse_get_value_as_int(&parser));
						limits = true;
						break;
					}
					
					os_sprintf(name, "LimitSpeed", i);
					if (jsonparse_strcmp_value(&parser, name) == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						status = robko_set_limit_speed(config, jsonparse_get_value_as_int(&parser));
						limits = true;
						break;
					}
					
					os_sprintf(name, "Servo%d", i);
					if (jsonparse_strcmp_value(&parser, name) == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						status = robko_set_servo(config, i, jsonparse_get_value_as_int(&parser));
						break;
					}
				}
				
				if (status != I2C_OK) {
					break;
				}
			}
		}
	}
	
	char address_str[MAX_I2C_ADDRESS];
	json_i2c_address(address_str, config->address);
	
	if (status != I2C_OK) {
		json_error(response, ROBKO_STR, i2c_status_str(status), address_str);
	}
	
	robko_response(config, response, address_str, limits);
	limits = false;
}

void ICACHE_FLASH_ATTR user_robko_init() {
	uint8 addr = 0;
	i2c_status status;
	
	i2c_config *config = robko_init(&addr, &status);
	
	uint8 i;
	for (i=0; i<ROBKO_URLs_COUNT; i++) {
		webserver_register_handler_callback(ROBKO_URLs[i], robko_handler);
	}
	
	device_register(I2C, ROBKO_ID, ROBKO_STR, ROBKO_URL, NULL, NULL);
	
	if (status == I2C_OK) {
		robko_read_all(config);
		robko_timer_init();
	}
}

#endif
