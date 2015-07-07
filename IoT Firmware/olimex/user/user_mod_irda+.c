#include "ets_sys.h"
#include "stdout.h"
#include "osapi.h"
#include "queue.h"

#include "json/jsonparse.h"

#include "user_json.h"
#include "user_misc.h"
#include "user_webserver.h"
#include "user_mod_irda+.h"
#include "user_devices.h"
#include "modules/mod_irda+.h"

char* const MOD_IRDA_URLs[] = {
	MOD_IRDA_URL, 
	MOD_IRDA_URL_ANY
};

const uint8 MOD_IRDA_URLs_COUNT = sizeof(MOD_IRDA_URLs) / sizeof(char *);

void ICACHE_FLASH_ATTR mod_irda_init() {
	uint8 addr = 0;
	i2c_status status;
	irda_init(&addr, &status);
	
	uint8 i;
	for (i=0; i<MOD_IRDA_URLs_COUNT; i++) {
		webserver_register_handler_callback(MOD_IRDA_URLs[i], mod_irda_handler);
	}
	
	device_register(I2C, MOD_IRDA_ID, MOD_IRDA_URL, NULL);
}

void ICACHE_FLASH_ATTR mod_irda_handler(
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
	i2c_config *config = i2c_init_handler(MOD_IRDA, MOD_IRDA_URL, irda_init, url, response);
	if (config == NULL) {
		return;
	}
	
	irda_config_data *config_data = (irda_config_data *)config->data;
	
	struct jsonparse_state parser;
	int type;
	
	if (method == POST && data != NULL && data_len != 0) {
		jsonparse_setup(&parser, data, data_len);

		while ((type = jsonparse_next(&parser)) != 0) {
			if (type == JSON_TYPE_PAIR_NAME) {
				if (jsonparse_strcmp_value(&parser, "Mode") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					if (jsonparse_strcmp_value(&parser, "SIRC") == 0) {
						config_data->mode = 1;
					} else {
						config_data->mode = 0;
					}
				} else if (jsonparse_strcmp_value(&parser, "Device") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					config_data->device = jsonparse_get_value_as_int(&parser);
				} else if (jsonparse_strcmp_value(&parser, "Command") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					config_data->command = jsonparse_get_value_as_int(&parser);
				}
			}
		}
		
		status = irda_set(config);
	}
	
	char address_str[MAX_I2C_ADDRESS];
	json_i2c_address(address_str, config->address);
	
	if (status == I2C_OK) {
		char data_str[WEBSERVER_MAX_VALUE];
		json_data(
			response, MOD_IRDA, OK_STR,
			json_sprintf(
				data_str,
				"\"Mode\" : \"%s\", "
				"\"Device\" : %d, "
				"\"Command\" : %d ",
				irda_mode_str(config_data->mode), 
				config_data->device,
				config_data->command
			),
			address_str
		);
	} else {
		json_error(response, MOD_IRDA, i2c_status_str(status), address_str);
	}
}
