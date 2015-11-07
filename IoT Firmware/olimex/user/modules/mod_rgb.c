#include "user_config.h"
#if MOD_RGB_ENABLE & I2C_ENABLE

#include "ets_sys.h"
#include "osapi.h"
#include "mem.h"
#include "queue.h"

#include "mod_rgb.h"
#include "driver/i2c_master.h"
#include "driver/i2c_extention.h"

i2c_command rgb_commands[] = { 
	{ .code = 0x01, .description = "Enable PWM"      }, 
	{ .code = 0x02, .description = "Disable PWM"     }, 
	{ .code = 0x03,	.description = "Set RGB"         }, 
	{ .code = 0x20,	.description = "Get ID"          }, 
	{ .code = 0xF0, .description = "Change address", } 
};

LOCAL i2c_status ICACHE_FLASH_ATTR rgb_on(uint8 address) {
	i2c_master_start();

	/* Send address */
	i2c_master_writeByte(address << 1 | 0);
	if (i2c_master_getAck()) {
		i2c_master_stop();
		return I2C_ADDRESS_NACK;
	}

	/* Send command */
	i2c_master_writeByte(0x01);
	if (i2c_master_getAck()) {
		i2c_master_stop();
		return I2C_DATA_NACK;
	}

	i2c_master_stop();
	return I2C_OK;
}

i2c_config ICACHE_FLASH_ATTR *rgb_init(uint8 *address, i2c_status *status) {
	i2c_config *config = NULL;
	
	*status = I2C_OK;
	if (*address == 0) {
		*address = MOD_RGB_DEFAULT_ADDR;
	}
	
	config = i2c_find_config(*address, MOD_RGB_ID);
	
	/* Check if there is device at address */
	if (i2c_check_device(*address) != I2C_OK) {
		*status = I2C_DEVICE_NOT_FOUND;
		return config;
	}

	uint8 id;

	/* Check if device is MOD-RGB */
	if (i2c_read_id(*address, &id) != I2C_OK) {
		*status = I2C_COMMUNICATION_FAILED;
		return config;
	}
	
	if (id != MOD_RGB_ID) {
		*status = I2C_DEVICE_ID_DONT_MATCH;
		return config;
	}

	os_delay_us(1000);
	if (rgb_on(*address) != I2C_OK) {
		*status = I2C_COMMUNICATION_FAILED;
		return config;
	}
	
	if (config == NULL) {
		config = i2c_add_config(*address, MOD_RGB_ID, (void *)os_zalloc(sizeof(rgb_config_data)));
	}
	
	return config;
}

i2c_status ICACHE_FLASH_ATTR rgb_set(i2c_config *config) {
	rgb_config_data *config_data = (rgb_config_data *)config->data;
	i2c_master_start();

	/* Send address */
	i2c_master_writeByte(config->address << 1 | 0);
	if (i2c_master_getAck()) {
		i2c_master_stop();
		return I2C_ADDRESS_NACK;
	}

	/* Send command */
	i2c_master_writeByte(0x03);
	if (i2c_master_getAck())
		goto error;

	i2c_master_writeByte(config_data->red);
	if (i2c_master_getAck())
		goto error;

	i2c_master_writeByte(config_data->green);
	if (i2c_master_getAck())
		goto error;

	i2c_master_writeByte(config_data->blue);
	if (i2c_master_getAck())
		goto error;

	i2c_master_stop();
	return I2C_OK;

	error: i2c_master_stop();
	return I2C_DATA_NACK;
}
#endif
