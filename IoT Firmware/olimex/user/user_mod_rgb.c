#include "user_config.h"
#if MOD_RGB_ENABLE & I2C_ENABLE

#include "ets_sys.h"
#include "stdout.h"
#include "osapi.h"
#include "queue.h"

#include "json/jsonparse.h"

#include "user_json.h"
#include "user_misc.h"
#include "user_webserver.h"
#include "user_mod_rgb.h"
#include "user_devices.h"
#include "modules/mod_rgb.h"

char* const MOD_RGB_URLs[] = {
	MOD_RGB_URL, 
	MOD_RGB_URL_ANY
};

const uint8 MOD_RGB_URLs_COUNT = sizeof(MOD_RGB_URLs) / sizeof(char *);
	
void ICACHE_FLASH_ATTR mod_rgb_init() {
	uint8 i;
	for (i=0; i<MOD_RGB_URLs_COUNT; i++) {
		webserver_register_handler_callback(MOD_RGB_URLs[i], mod_rgb_handler);
	}
	device_register(I2C, MOD_RGB_ID, MOD_RGB, MOD_RGB_URL, NULL, NULL);
}

void ICACHE_FLASH_ATTR mod_rgb_handler(
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
	i2c_config *config = i2c_init_handler(MOD_RGB, MOD_RGB_URL, rgb_init, url, response);
	if (config == NULL) {
		return;
	}
	
	rgb_config_data *config_data = (rgb_config_data *)config->data;
	
	struct jsonparse_state parser;
	int type;
	
	if (method == POST && data != NULL && data_len != 0) {
		jsonparse_setup(&parser, data, data_len);

		while ((type = jsonparse_next(&parser)) != 0) {
			if (type == JSON_TYPE_PAIR_NAME) {
				if (jsonparse_strcmp_value(&parser, "R") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					config_data->red = jsonparse_get_value_as_int(&parser);
				} else if (jsonparse_strcmp_value(&parser, "G") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					config_data->green = jsonparse_get_value_as_int(&parser);
				} else if (jsonparse_strcmp_value(&parser, "B") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					config_data->blue = jsonparse_get_value_as_int(&parser);
				}
			}
		}
	}
	
	char address_str[MAX_I2C_ADDRESS];
	json_i2c_address(address_str, config->address);
	
	status = rgb_set(config);
	if (status == I2C_OK) {
		char data_str[WEBSERVER_MAX_VALUE];
		json_data(
			response, MOD_RGB, OK_STR,
			json_sprintf(
				data_str,
				"\"R\" : %d, \"G\" : %d, \"B\" : %d",
				config_data->red, 
				config_data->green, 
				config_data->blue
			),
			address_str
		);
	} else {
		json_error(response, MOD_RGB, i2c_status_str(status), address_str);
	}
}
#endif
