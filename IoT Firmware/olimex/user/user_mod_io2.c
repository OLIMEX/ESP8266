#include "ets_sys.h"
#include "stdout.h"
#include "osapi.h"
#include "queue.h"

#include "json/jsonparse.h"

#include "user_json.h"
#include "user_misc.h"
#include "user_webserver.h"
#include "user_mod_io2.h"
#include "user_devices.h"
#include "modules/mod_io2.h"

char* const MOD_IO2_URLs[] = {
	MOD_IO2_URL, 
	MOD_IO2_URL_ANY
};

const uint8 MOD_IO2_URLs_COUNT = sizeof(MOD_IO2_URLs) / sizeof(char *);

void ICACHE_FLASH_ATTR mod_io2_init() {
	uint8 addr = 0;
	i2c_status status;
	io2_init(&addr, &status);
	
	uint8 i;
	for (i=0; i<MOD_IO2_URLs_COUNT; i++) {
		webserver_register_handler_callback(MOD_IO2_URLs[i], mod_io2_handler);
	}
	
	device_register(I2C, MOD_IO2_ID, MOD_IO2_URL);
}

void ICACHE_FLASH_ATTR mod_io2_handler(
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
	i2c_config *config = i2c_init_handler(MOD_IO2, MOD_IO2_URL, io2_init, url, response);
	if (config == NULL) {
		return;
	}
	
	io2_config_data *config_data = (io2_config_data *)config->data;
	
	struct jsonparse_state parser;
	int type;

	if (method == POST && data != NULL && data_len != 0) {
		jsonparse_setup(&parser, data, data_len);

		while ((type = jsonparse_next(&parser)) != 0) {
			if (type == JSON_TYPE_PAIR_NAME) {
				if (jsonparse_strcmp_value(&parser, "Relay1") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					config_data->relay1 = jsonparse_get_value_as_int(&parser);
				} else if (jsonparse_strcmp_value(&parser, "Relay2") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					config_data->relay2 = jsonparse_get_value_as_int(&parser);
				} else if (jsonparse_strcmp_value(&parser, "GPIO0") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					config_data->gpio0 = jsonparse_get_value_as_int(&parser);
				} else if (jsonparse_strcmp_value(&parser, "GPIO1") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					config_data->gpio1 = jsonparse_get_value_as_int(&parser);
				} else if (jsonparse_strcmp_value(&parser, "GPIO2") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					config_data->gpio2 = jsonparse_get_value_as_int(&parser);
				} else if (jsonparse_strcmp_value(&parser, "GPIO3") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					config_data->gpio3 = jsonparse_get_value_as_int(&parser);
				} else if (jsonparse_strcmp_value(&parser, "GPIO4") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					config_data->gpio4 = jsonparse_get_value_as_int(&parser);
				} else if (jsonparse_strcmp_value(&parser, "GPIO5") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					config_data->gpio5 = jsonparse_get_value_as_int(&parser);
				} else if (jsonparse_strcmp_value(&parser, "GPIO6") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					config_data->gpio6 = jsonparse_get_value_as_int(&parser);
				}
			}
		}
	}
	
	char address_str[MAX_I2C_ADDRESS];
	json_i2c_address(address_str, config->address);
	
	status = io2_set(config);
	if (status == I2C_OK) {
		char data_str[WEBSERVER_MAX_VALUE];
		json_data(
			response, MOD_IO2, OK_STR,
			json_sprintf(
				data_str,
				"\"Relay1\" : %d, "
				"\"Relay2\" : %d, "
				"\"GPIO0\" : %d, "
				"\"GPIO1\" : %d, "
				"\"GPIO2\" : %d, "
				"\"GPIO3\" : %d, "
				"\"GPIO4\" : %d, "
				"\"GPIO5\" : %d, "
				"\"GPIO6\" : %d",
				config_data->relay1, 
				config_data->relay2,
				config_data->gpio0,
				config_data->gpio1,
				config_data->gpio2,
				config_data->gpio3,
				config_data->gpio4,
				config_data->gpio5,
				config_data->gpio6
			),
			address_str
		);
	} else {
		json_error(response, MOD_IO2, i2c_status_str(status), address_str);
	}
}
