#include "user_config.h"
#if DEVICE == DIMMER

#include "ets_sys.h"
#include "osapi.h"
#include "mem.h"
#include "queue.h"
#include "stdout.h"

#include "dimmer.h"
#include "driver/i2c_master.h"
#include "driver/i2c_extention.h"

void ICACHE_FLASH_ATTR dimmer_foreach(i2c_callback function) {
	i2c_foreach(DIMMER_ID, function);
}

LOCAL i2c_status ICACHE_FLASH_ATTR dimmer_set_byte(uint8 address, uint8 command, uint8 *data) {
	i2c_master_start();

	/* Send address */
	i2c_master_writeByte(address << 1 | 0);
	if (i2c_master_getAck()) {
		i2c_master_stop();
		return I2C_ADDRESS_NACK;
	}

	/* Send command */
	i2c_master_writeByte(command);
	if (i2c_master_getAck()) {
		goto error;
	}
	
	if (data != NULL) {
		/* Send data */
		i2c_master_writeByte(*data);
		if (i2c_master_getAck()) {
			goto error;
		}
	}
	
	i2c_master_stop();
	return I2C_OK;
	
	error: i2c_master_stop();
	return I2C_DATA_NACK;
}

LOCAL i2c_status ICACHE_FLASH_ATTR dimmer_get_byte(uint8 address, uint8 command, uint8 *data) {
	i2c_master_start();

	/* Send address */
	i2c_master_writeByte(address << 1 | 0);
	if (i2c_master_getAck()) {
		i2c_master_stop();
		return I2C_ADDRESS_NACK;
	}

	/* Send command */
	i2c_master_writeByte(command);
	if (i2c_master_getAck()) {
		goto error;
	}
	
	i2c_master_stop();
	i2c_master_start();

	/* Send address */
	i2c_master_writeByte(address << 1 | 1);
	if (i2c_master_getAck()) {
		i2c_master_stop();
		return I2C_ADDRESS_NACK;
	}
	
	/* Read data */
	*data = i2c_master_readByte();
	i2c_master_send_nack();
	
	i2c_master_stop();
	return I2C_OK;
	
	error: i2c_master_stop();
	return I2C_DATA_NACK;
}

LOCAL i2c_status ICACHE_FLASH_ATTR dimmer_get_word(uint8 address, uint8 command, uint16 *data) {
	uint8 d[2];
	
	i2c_master_start();

	/* Send address */
	i2c_master_writeByte(address << 1 | 0);
	if (i2c_master_getAck()) {
		i2c_master_stop();
		return I2C_ADDRESS_NACK;
	}

	/* Send command */
	i2c_master_writeByte(command);
	if (i2c_master_getAck()) {
		goto error;
	}
	
	i2c_master_stop();
	i2c_master_start();

	/* Send address */
	i2c_master_writeByte(address << 1 | 1);
	if (i2c_master_getAck()) {
		i2c_master_stop();
		return I2C_ADDRESS_NACK;
	}
	
	/* Read data */
	d[0] = i2c_master_readByte();
	i2c_master_send_ack();
	d[1] = i2c_master_readByte();
	i2c_master_send_nack();
	i2c_master_stop();
	
	*data = d[0] + 256 * d[1];
	return I2C_OK;
	
	error: i2c_master_stop();
	return I2C_DATA_NACK;
}

i2c_config ICACHE_FLASH_ATTR *dimmer_init(uint8 *address, i2c_status *status) {
	i2c_config *config = NULL;
	
	*status = I2C_OK;
	if (*address == 0) {
		*address = DIMMER_DEFAULT_ADDR;
	}
	
	config = i2c_find_config(*address, DIMMER_ID);
	
	i2c_master_stop();
	
	// Check if there is device at address 
	if (i2c_check_device(*address) != I2C_OK) {
		*status = I2C_DEVICE_NOT_FOUND;
		return config;
	}
	
	uint8 id;
	// Check if device is DIMMER
	if (i2c_read_id(*address, &id) != I2C_OK) {
		*status = I2C_COMMUNICATION_FAILED;
		return config;
	}
	
	if (id != DIMMER_ID) {
		#if DIMMER_DEBUG
		debug("DIMMER: ID do not match [0x%02X] \n", id);
		#endif
		*status = I2C_DEVICE_ID_DONT_MATCH;
		return config;
	}

	if (config == NULL) {
		config = i2c_add_config(*address, DIMMER_ID, (void *)os_zalloc(sizeof(dimmer_config_data)));
	}
	
	uint8 data;
	uint16 current;
	dimmer_config_data *config_data = (dimmer_config_data *)config->data;
	
	// get relay
	*status = dimmer_get_byte(*address, 0x43, &data);
	if (*status != I2C_OK) {
		return config;
	}
	config_data->relay = data;
	
	// get brightness
	*status = dimmer_get_byte(*address, 0x11, &data);
	if (*status != I2C_OK) {
		return config;
	}
	config_data->brightness = data;
	
	// get current
	*status = dimmer_get_word(*address, 0x10, &current);
	if (*status != I2C_OK) {
		return config;
	}
	config_data->current = current;
	
	return config;
}

i2c_status ICACHE_FLASH_ATTR dimmer_set(i2c_config *config) {
	i2c_status status;
	dimmer_config_data *config_data = (dimmer_config_data *)config->data;
	
	// Dimmer set relay state
	status = dimmer_set_byte(
		config->address, 
		config_data->relay == 0 ?
			0x42
			:
			0x41
		, 
		NULL
	);
	if (status != I2C_OK) {
		return status;
	}
	
	// Dimmer set brightness
	status = dimmer_set_byte(
		config->address, 
		config_data->relay == 0 ?
			0x51           // Set brightness
			:
			0x50           // Slow to brightness
		, 
		&(config_data->brightness)
	);
	if (status != I2C_OK) {
		return status;
	}
	
	return I2C_OK;
}
#endif
