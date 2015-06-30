#include "ets_sys.h"
#include "osapi.h"
#include "mem.h"
#include "queue.h"

#include "mod_io2.h"
#include "driver/i2c_master.h"
#include "driver/i2c_extention.h"

i2c_command io2_commands[] = { 
	{ .code = 0x01, .description = "Set GPIO direction" },
	{ .code = 0x02, .description = "Set GPIO level"     },
	{ .code = 0x03, .description = "Read GPIO level"    },
	{ .code = 0x10, .description = "Read GPIO0 ADC"     },
	{ .code = 0x11, .description = "Read GPIO1 ADC"     },
	{ .code = 0x04, .description = "Set GPIO pull up"   },
	{ .code = 0x40, .description = "Set Relays"         },
	{ .code = 0x51, .description = "Open and set PWM1"  },
	{ .code = 0x52, .description = "Open and set PWM2"  }
};

LOCAL i2c_status ICACHE_FLASH_ATTR io2_set_execute(uint8 address, uint8 command, uint8 data) {
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

	/* Send data */
	i2c_master_writeByte(data);
	if (i2c_master_getAck()) {
		goto error;
	}
	
	i2c_master_stop();
	return I2C_OK;
	
	error: i2c_master_stop();
	return I2C_DATA_NACK;
}

LOCAL i2c_status ICACHE_FLASH_ATTR io2_get_execute(uint8 address, uint8 command, uint8 *data) {
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

LOCAL i2c_status ICACHE_FLASH_ATTR io2_acp_execute(uint8 address, uint8 command, uint16 *data) {
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
	
	*data = 256 * d[1] + d[0];
	return I2C_OK;
	
	error: i2c_master_stop();
	return I2C_DATA_NACK;
}

i2c_config ICACHE_FLASH_ATTR *io2_init(uint8 *address, i2c_status *status) {
	i2c_config *config = NULL;
	
	*status = I2C_OK;
	if (*address == 0) {
		*address = MOD_IO2_DEFAULT_ADDR;
	}
	
	config = i2c_find_config(*address, MOD_IO2_ID);
	
	// Check if there is device at address 
	if (i2c_check_device(*address) != I2C_OK) {
		*status = I2C_DEVICE_NOT_FOUND;
		return config;
	}
	
	uint8 id;
	// Check if device is MOD-IO2
	if (i2c_read_id(*address, &id) != I2C_OK) {
		*status = I2C_COMMUNICATION_FAILED;
		return config;
	}
	
	if (id != MOD_IO2_ID) {
		*status = I2C_DEVICE_ID_DONT_MATCH;
		return config;
	}
	
	// Set GPIO direction
	// GPIO bit 7654 3210 
	//          0001 1011    [0 - out, 1 - in]
	*status = io2_set_execute(*address, 0x01, 0x1B);
	if (*status != I2C_OK) {
		return config;
	}
	// Set GPIO pull up
	*status = io2_set_execute(*address, 0x04, 0x18);
	if (*status != I2C_OK) {
		return config;
	}
	
	if (config == NULL) {
		config = i2c_add_config(*address, MOD_IO2_ID, (void *)os_zalloc(sizeof(io2_config_data)));
	}
	
	return config;
}

i2c_status ICACHE_FLASH_ATTR io2_set(i2c_config *config) {
	i2c_status status;
	uint8 data;
	uint16 acp;
	io2_config_data *config_data = (io2_config_data *)config->data;
	
	// Read ACP GPIO 0,1
	status = io2_acp_execute(config->address, 0x10, &acp);
	if (status != I2C_OK) {
		return status;
	}
	config_data->gpio0 = acp;
	
	status = io2_acp_execute(config->address, 0x11, &acp);
	if (status != I2C_OK) {
		return status;
	}
	config_data->gpio1 = acp;
	
	// Set Digital Outputs GPIO 2
	data = ((config_data->gpio2 != 0) << 2);
	status = io2_set_execute(config->address, 0x02, data);
	if (status != I2C_OK) {
		return status;
	}
	
	// Read Digital Inputs GPIO 3,4
	status = io2_get_execute(config->address, 0x03, &data);
	if (status != I2C_OK) {
		return status;
	}
	config_data->gpio3 = (data & 0x08) != 0;
	config_data->gpio4 = (data & 0x10) != 0;
	
	// Set Relays
	data = (config_data->relay2 != 0) << 1 | (config_data->relay1 != 0);
	status = io2_set_execute(config->address, 0x40, data);
	if (status != I2C_OK) {
		return status;
	}
	
	// Set PWM1 GPRIO6
	status = io2_set_execute(config->address, 0x51, config_data->gpio6);
	if (status != I2C_OK) {
		return status;
	}
	
	// Set PWM2 GPRIO5
	status = io2_set_execute(config->address, 0x52, config_data->gpio5);
	if (status != I2C_OK) {
		return status;
	}
	
	return I2C_OK;
}
