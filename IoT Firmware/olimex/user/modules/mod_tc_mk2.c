#include "user_config.h"
#if MOD_TC_MK2_ENABLE & I2C_ENABLE

#include "ets_sys.h"
#include "osapi.h"
#include "mem.h"
#include "queue.h"

#include "mod_tc_mk2.h"
#include "driver/i2c_master.h"
#include "driver/i2c_extention.h"

i2c_command tc_commands[] = { 
	{ .code = 0x20,	.description = "Get ID"          }, 
	{ .code = 0x21, .description = "Get Temperature" } 
};

void ICACHE_FLASH_ATTR tc_foreach(i2c_callback function) {
	i2c_foreach(MOD_TC_MK2_ID, function);
}

i2c_config ICACHE_FLASH_ATTR *tc_init(uint8 *address, i2c_status *status) {
	i2c_config *config = NULL;
	
	*status = I2C_OK;
	if (*address == 0) {
		*address = MOD_TC_MK2_DEFAULT_ADDR;
	}
	
	config = i2c_find_config(*address, MOD_TC_MK2_ID);
	
	// Check if there is device at address 
	if (i2c_check_device(*address) != I2C_OK) {
		*status = I2C_DEVICE_NOT_FOUND;
		return config;
	}
	
	uint8 id;
	// Check if device is MOD-TC-MK2
	if (i2c_read_id(*address, &id) != I2C_OK) {
		*status = I2C_COMMUNICATION_FAILED;
		return config;
	}
	
	if (id != MOD_TC_MK2_ID) {
		*status = I2C_DEVICE_ID_DONT_MATCH;
		return config;
	}
	
	if (config == NULL) {
		config = i2c_add_config(*address, MOD_TC_MK2_ID, (void *)os_zalloc(sizeof(tc_config_data)));
	}
	
	return config;
}

i2c_status ICACHE_FLASH_ATTR tc_read(i2c_config *config) {
	tc_config_data *config_data = (tc_config_data *)config->data;
	i2c_master_start();

	/* Send address */
	i2c_master_writeByte(config->address << 1 | 0);
	if (i2c_master_getAck()) {
		i2c_master_stop();
		return I2C_ADDRESS_NACK;
	}

	/* Send command */
	i2c_master_writeByte(0x21);
	if (i2c_master_getAck()) {
		i2c_master_stop();
		return I2C_DATA_NACK;	
	}

	i2c_master_stop();
	i2c_master_start();
	
	i2c_master_writeByte(config->address << 1 | 1);
	if (i2c_master_getAck()) {
		i2c_master_stop();
		return I2C_ADDRESS_NACK;
	}
	
	uint8 i;
	uint8 data[4];
	for (i=0; i < 4; i++) {
		data[i] = i2c_master_readByte();
		i2c_master_setAck(i == 3);
	}
	i2c_master_stop();
	
	if ((data[2] & 0x01) != 0) {
		return I2C_COMMUNICATION_FAILED;
	}
	
	sint16 d = data[3] * 256 + (data[2] & 0xFC);
	float tf = 0.0625 * d;
	int ti = tf;
	uint16 td = (tf - ti) * 100;
	
	config_data->temperature = d / 4;
	os_sprintf(config_data->temperature_str, "%d.%02d", ti, td);
	
	return I2C_OK;
}
#endif
