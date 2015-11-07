#include "user_config.h"
#if MOD_IRDA_ENABLE & I2C_ENABLE

#include "ets_sys.h"
#include "osapi.h"
#include "mem.h"
#include "queue.h"

#include "mod_irda+.h"
#include "driver/i2c_master.h"
#include "driver/i2c_extention.h"

i2c_command irda_commands[] = { 
	{ .code = 0x01, .description = "Set mode"  },  // 0x00 - RC5, 0x01 - SIRC
	{ .code = 0x02, .description = "Send data" },
	{ .code = 0x03, .description = "Read data" }
};

const char ICACHE_FLASH_ATTR *irda_mode_str(uint8 mode) {
	switch (mode) {
		case 0 : return "RC5";
		case 1 : return "SIRC";
	}
	return "UNKNOWN";
}

LOCAL i2c_status ICACHE_FLASH_ATTR irda_set_mode(uint8 address, uint8 mode) {
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
		goto error;
	}
	
	/* Send mode */
	i2c_master_writeByte(mode);
	if (i2c_master_getAck()) {
		goto error;
	}
	
	i2c_master_stop();
	
	return I2C_OK;
	
error: 
	i2c_master_stop();
	return I2C_DATA_NACK;
}

LOCAL i2c_status ICACHE_FLASH_ATTR irda_send_command(uint8 address, uint8 command, uint8 device) {
	i2c_master_start();

	/* Send address */
	i2c_master_writeByte(address << 1 | 0);
	if (i2c_master_getAck()) {
		i2c_master_stop();
		return I2C_ADDRESS_NACK;
	}

	/* Send command */
	i2c_master_writeByte(0x02);
	if (i2c_master_getAck()) {
		goto error;
	}
	
	/* Send device */
	i2c_master_writeByte(device);
	if (i2c_master_getAck()) {
		goto error;
	}
	
	/* Send command */
	i2c_master_writeByte(command);
	if (i2c_master_getAck()) {
		goto error;
	}
	
	i2c_master_stop();
	
	return I2C_OK;
	
error: 
	i2c_master_stop();
	return I2C_DATA_NACK;
}

i2c_config ICACHE_FLASH_ATTR *irda_init(uint8 *address, i2c_status *status) {
	i2c_config *config = NULL;
	
	*status = I2C_OK;
	if (*address == 0) {
		*address = MOD_IRDA_DEFAULT_ADDR;
	}
	
	config = i2c_find_config(*address, MOD_IRDA_ID);
	/*
	// Check if there is device at address 
	if (i2c_check_device(*address) != I2C_OK) {
		*status = I2C_DEVICE_NOT_FOUND;
		return config;
	}
	*/
	
	uint8 id;
	// Check if device is MOD-IRDA
	if (i2c_read_id(*address, &id) != I2C_OK) {
		// *status = I2C_COMMUNICATION_FAILED;
		*status = I2C_DEVICE_NOT_FOUND;
		return config;
	}
	
	if (id != MOD_IRDA_ID) {
#if MOD_IRDA_DEBUG
		debug("MOD_IRDA: ID do not match [0x%02x] != [0x%02x]\n", id, MOD_IRDA_ID);
#endif
		*status = I2C_DEVICE_ID_DONT_MATCH;
		return config;
	}
	
	if (config == NULL) {
		config = i2c_add_config(*address, MOD_IRDA_ID, (void *)os_zalloc(sizeof(irda_config_data)));
	}
	
	return config;
}

i2c_status ICACHE_FLASH_ATTR irda_set(i2c_config *config) {
	i2c_status status;
	irda_config_data *config_data = (irda_config_data *)config->data;
	
	// Set mode
#if MOD_IRDA_DEBUG
	debug("MOD_IRDA: SET Mode [%d]...\n", config_data->mode);
#endif
	status = irda_set_mode(config->address, config_data->mode);
	if (status != I2C_OK) {
		return status;
	}
#if MOD_IRDA_DEBUG
	debug("MOD_IRDA: OK\n");
#endif
	
	// Send command
#if MOD_IRDA_DEBUG
	debug("MOD_IRDA: SEND Command [%d][%d]...\n", config_data->command, config_data->device);
#endif
	status = irda_send_command(config->address, config_data->command, config_data->device);
	if (status != I2C_OK) {
		return status;
	}
#if MOD_IRDA_DEBUG
	debug("MOD_IRDA: OK\n");
#endif
	return I2C_OK;
}
#endif
