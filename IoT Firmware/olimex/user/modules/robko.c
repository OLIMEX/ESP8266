#include "user_config.h"
#if DEVICE == ROBKO

#include "ets_sys.h"
#include "osapi.h"
#include "mem.h"
#include "queue.h"
#include "stdout.h"

#include "robko.h"
#include "driver/i2c_master.h"
#include "driver/i2c_extention.h"

void ICACHE_FLASH_ATTR robko_foreach(i2c_callback function) {
	i2c_foreach(ROBKO_ID, function);
}

LOCAL i2c_status ICACHE_FLASH_ATTR robko_read(uint8 address, uint8 reg, uint8 *data, uint8 len) {
	uint8 retry = 0;
	i2c_status status;
	do {
#if ROBKO_DEBUG
#if ROBKO_VERBOSE_OUTPUT
		debug("ROBKO: 0x%02X 0x%02X reading %d bytes...\n", address, reg, len);
#endif
#endif
		status = I2C_OK;
		i2c_master_start();
		
		/* Send address for write */
		i2c_master_writeByte(address << 1 | 0);
		if (i2c_master_getAck()) {
#if ROBKO_DEBUG
#if ROBKO_VERBOSE_OUTPUT
			debug("ROBKO: Address ACK failed [0x%02X] [set READ command] \n", address);
#endif
#endif
			status = I2C_ADDRESS_NACK;
			goto done;
		}
		
		/* Send register */
		i2c_master_writeByte(reg);
		if (i2c_master_getAck()) {
#if ROBKO_DEBUG
#if ROBKO_VERBOSE_OUTPUT
			debug("ROBKO: Register ACK failed [0x%02X] [set READ command] \n", reg);
#endif
#endif
			status = I2C_DATA_NACK;
			goto done;
		}
		
		i2c_master_stop();
		i2c_master_start();

		/* Send address for read */
		i2c_master_writeByte(address << 1 | 1);
		if (i2c_master_getAck()) {
#if ROBKO_DEBUG
#if ROBKO_VERBOSE_OUTPUT
			debug("ROBKO: Address ACK failed [0x%02X] [execute READ command] \n", address);
#endif
#endif
			status = I2C_ADDRESS_NACK;
			goto done;
		}
		
#if ROBKO_DEBUG
#if ROBKO_VERBOSE_OUTPUT
			debug("READ: ");
#endif
#endif
		/* Read data */
		uint8 i = 0;
		while (true) {
			data[i] = i2c_master_readByte();
#if ROBKO_DEBUG
#if ROBKO_VERBOSE_OUTPUT
			debug("0x%02X ", data[i]);
#endif
#endif
			i++;
			if (i < len) {
				i2c_master_send_ack();
			} else {
				i2c_master_send_nack();
				break;
			}
		}
		
done: 
		i2c_master_stop();
		retry++;
	} while (
		retry < ROBKO_I2C_RETRY && 
		status != I2C_OK
	);
	
	
#if ROBKO_DEBUG
#if ROBKO_VERBOSE_OUTPUT
	debug("\nROBKO: Done.\n\n");
#endif
#endif
	return status;
}

LOCAL i2c_status ICACHE_FLASH_ATTR robko_set(uint8 address, uint8 reg, uint8 *data, uint8 len) {
	uint8 retry = 0;
	i2c_status status;
	do {
#if ROBKO_DEBUG
#if ROBKO_VERBOSE_OUTPUT
		debug("ROBKO: 0x%02X 0x%02X set %d bytes...\n", address, reg, len);
#endif
#endif
		
		status = I2C_OK;
		i2c_master_start();
		
		/* Send address for write */
		i2c_master_writeByte(address << 1 | 0);
		if (i2c_master_getAck()) {
#if ROBKO_DEBUG
#if ROBKO_VERBOSE_OUTPUT
			debug("ROBKO: Address ACK failed [0x%02X] [set SET command] \n", address);
#endif
#endif
			status = I2C_ADDRESS_NACK;
			goto done;
		}

		/* Send register */
		i2c_master_writeByte(reg);
		if (i2c_master_getAck()) {
#if ROBKO_DEBUG
#if ROBKO_VERBOSE_OUTPUT
			debug("ROBKO: Register ACK failed [0x%02X] [set SET command] \n", reg);
#endif
#endif
			status = I2C_DATA_NACK;
			goto done;
		}
		
		/* Write data */
		uint8 i = 0;
		while (i < len) {
			i2c_master_writeByte(data[i]);
			if (i2c_master_getAck()) {
#if ROBKO_DEBUG
#if ROBKO_VERBOSE_OUTPUT
				debug("ROBKO: Data ACK failed [execute SET command] \n");
#endif
#endif
				status = I2C_DATA_NACK;
				break;
			}
#if ROBKO_DEBUG
#if ROBKO_VERBOSE_OUTPUT
			debug("0x%02X ", data[i]);
#endif
#endif
			i++;
		}
		
done:
		i2c_master_stop();
		retry++;
	} while (
		retry < ROBKO_I2C_RETRY &&
		status != I2C_OK
	);
#if ROBKO_DEBUG
#if ROBKO_VERBOSE_OUTPUT
	debug("\nROBKO: Done.\n\n");
#endif
#endif
	return status;
}

i2c_config ICACHE_FLASH_ATTR *robko_init(uint8 *address, i2c_status *status) {
	i2c_config *config = NULL;
	
	*status = I2C_OK;
	if (*address == 0) {
		*address = ROBKO_DEFAULT_ADDR;
	}
	
	config = i2c_find_config(*address, ROBKO_ID);
	
	i2c_master_stop();
	
	// Check if there is device at address 
	if (i2c_check_device(*address) != I2C_OK) {
		*status = I2C_DEVICE_NOT_FOUND;
		return config;
	}
	
	uint8 id;
	// Check if device is ROBKO
	if (i2c_read_id(*address, &id) != I2C_OK) {
		*status = I2C_COMMUNICATION_FAILED;
		return config;
	}
	
	if (id != ROBKO_ID) {
#if ROBKO_DEBUG
		debug("ROBKO: ID do not match [0x%02X] \n", id);
#endif
		*status = I2C_DEVICE_ID_DONT_MATCH;
		return config;
	}
	
	if (config == NULL) {
		config = i2c_add_config(*address, ROBKO_ID, (void *)os_zalloc(sizeof(robko_config_data)));
	}
	
	return config;
}

i2c_status ICACHE_FLASH_ATTR robko_read_servo(i2c_config *config) {
	if (config == NULL) {
		return I2C_OK;
	}
	
	robko_config_data *data = config->data;
	i2c_status status;
	
	status = robko_read(config->address, ROBKO_SERVO, (uint8 *)data->servo, sizeof(data->servo));
	if (status != I2C_OK) {
		return status;
	}
	
	data->servo_ready = 0;
	uint8 i;
	for (i=0; i<6; i++) {
		data->servo_ready = data->servo_ready | ((data->servo[i] >> 15) << i);
		data->servo[i] = data->servo[i] & 0x7FFF;
	}
	
	return status;
}

i2c_status ICACHE_FLASH_ATTR robko_read_events(i2c_config *config, bool only_new) {
	if (config == NULL) {
		return I2C_OK;
	}
	
	robko_config_data *data = config->data;
	i2c_status status;
	
	status = robko_read(config->address, ROBKO_EVENTS, &(data->events), sizeof(data->events));
	if (status != I2C_OK) {
		return status;
	}
	
	if ((data->events & ROBKO_EVENTS_NEW_MASK) == 0) {
		data->events = 0;
	}
	
	uint8 servo_ready = data->servo_ready;
	status = robko_read_servo(config);
	if (status != I2C_OK) {
		return status;
	}
	
	if (servo_ready != data->servo_ready) {
		data->events = data->events | ROBKO_EVENTS_NEW_MASK | ROBKO_EVENTS_SERVO_MASK;
	}
	
	if (
		only_new && 
		(data->events & ROBKO_EVENTS_NEW_MASK) == 0
	) {
		return status;
	}
	
	if (
		!only_new ||
		(data->events & ROBKO_EVENTS_PRESS_MASK) != 0
	) {
#if ROBKO_DEBUG
		debug("ROBKO: Read PRESS event data\n");
#endif
		status = robko_read(config->address, ROBKO_EVENTS_PRESS, &(data->events_press), sizeof(data->events_press));
		if (status != I2C_OK) {
			return status;
		}
	} else {
		data->events_press = 0;
	}
	
	if (
		!only_new ||
		(data->events & ROBKO_EVENTS_RELEASE_MASK) != 0
	) {
#if ROBKO_DEBUG
		debug("ROBKO: Read RELEASE event data\n");
#endif
		status = robko_read(config->address, ROBKO_EVENTS_RELEASE, &(data->events_release), sizeof(data->events_release));
		if (status != I2C_OK) {
			return status;
		}
	} else {
		data->events_release = 0;
	}
		
	if (
		!only_new ||
		(data->events & ROBKO_EVENTS_DBLCLICK_MASK)
	) {
#if ROBKO_DEBUG
		debug("ROBKO: Read DOUBLE_CLICK event data\n");
#endif
		status = robko_read(config->address, ROBKO_EVENTS_DBLCLICK, &(data->events_dbl_click), sizeof(data->events_dbl_click));
		if (status != I2C_OK) {
			return status;
		}
	} else {
		data->events_dbl_click = 0;
	}
	
	if (
		!only_new ||
		(data->events & ROBKO_EVENTS_ANALOG_MASK) != 0
	) {
#if ROBKO_DEBUG
		debug("ROBKO: Read ANALOG event data\n");
#endif
		status = robko_read(config->address, ROBKO_POTENTIOMETER, (uint8 *)data->potentiometer, sizeof(data->potentiometer));
		if (status != I2C_OK) {
			return status;
		}
	}
	
	return status;
}

i2c_status ICACHE_FLASH_ATTR robko_read_all(i2c_config *config) {
	if (config == NULL) {
		return I2C_OK;
	}
	
	robko_config_data *data = config->data;
	i2c_status status;
	
	status = robko_read_events(config, false);
	if (status != I2C_OK) {
		return status;
	}
	
	status = robko_read(config->address, ROBKO_LIMIT_LOW, (uint8 *)data->limit_low, sizeof(data->limit_low));
	if (status != I2C_OK) {
		return status;
	}
	
	status = robko_read(config->address, ROBKO_LIMIT_HIGH, (uint8 *)data->limit_high, sizeof(data->limit_high));
	if (status != I2C_OK) {
		return status;
	}
	
	status = robko_read(config->address, ROBKO_LIMIT_SPEED, &(data->limit_speed), sizeof(data->limit_speed));
	if (status != I2C_OK) {
		return status;
	}
	
	return I2C_OK;
}

i2c_status ICACHE_FLASH_ATTR robko_set_servo(i2c_config *config, uint8 index, sint16 value) {
	if (config == NULL) {
		return;
	}
	
	robko_config_data *data = config->data;
	data->servo[index] = value;
	
	return robko_set(
		config->address, 
		ROBKO_SERVO + index * sizeof(uint16), 
		(uint8 *)&(data->servo[index]),
		sizeof(uint16)
	);
}

i2c_status ICACHE_FLASH_ATTR robko_set_limit_low(i2c_config *config, uint8 index, sint16 value) {
	if (config == NULL) {
		return;
	}
	
	robko_config_data *data = config->data;
	data->limit_low[index] = value;
	
	return robko_set(
		config->address, 
		ROBKO_LIMIT_LOW + index * sizeof(uint16), 
		(uint8 *)&(data->limit_low[index]),
		sizeof(uint16)
	);
}

i2c_status ICACHE_FLASH_ATTR robko_set_limit_high(i2c_config *config, uint8 index, sint16 value) {
	if (config == NULL) {
		return;
	}
	
	robko_config_data *data = config->data;
	data->limit_high[index] = value;
	
	return robko_set(
		config->address, 
		ROBKO_LIMIT_HIGH + index * sizeof(uint16), 
		(uint8 *)&(data->limit_high[index]),
		sizeof(uint16)
	);
}

i2c_status ICACHE_FLASH_ATTR robko_set_limit_speed(i2c_config *config, uint8 value) {
	if (config == NULL) {
		return;
	}
	
	robko_config_data *data = config->data;
	data->limit_speed = value;
	
	return robko_set(
		config->address, 
		ROBKO_LIMIT_SPEED, 
		&(data->limit_speed),
		sizeof(data->limit_speed)
	);
}

#endif