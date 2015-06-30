#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "mem.h"

#include "driver/i2c_master.h"
#include "driver/i2c_extention.h"

#include "user_json.h"

STAILQ_HEAD(i2c_configs, _i2c_config_) i2c_configs = STAILQ_HEAD_INITIALIZER(i2c_configs);

/******************************************************************************
 * FunctionName : i2c_status_str
 * Description  : convert i2c_status to human readable string
 * Parameters   : status
 * Returns      : 
*******************************************************************************/
const char ICACHE_FLASH_ATTR *i2c_status_str(i2c_status status) {
	switch (status) {
		case I2C_OK                    : return "i2c Device OK";
		case I2C_ADDRESS_NACK          : return "Address not Acknowledged";
		case I2C_DATA_NACK             : return "Data not Acknowledged";
		case I2C_DEVICE_NOT_FOUND      : return "Device not found";
		case I2C_DEVICE_ID_DONT_MATCH  : return "Device ID don't match";
		case I2C_COMMUNICATION_FAILED  : return "Communication failed";
		
		default: return "UNKNOWN";
	}
}

/******************************************************************************
 * FunctionName : i2c_addr_parse
 * Description  : parse string I2C address in HEX representation to uint8
 * Parameters   : pAddress
 * Returns      : integer
*******************************************************************************/
uint8 ICACHE_FLASH_ATTR i2c_addr_parse(char *addr, const char **error) {
	uint32 address = 0;
	char *convert_err = NULL;
	char *end = NULL;
	
	if (*addr == '/') {
		addr++;
	}
	
	char cAddr[os_strlen(addr)+1];
	
	if (*addr != '\0') {
		end = os_strchr(addr, '/');
		if (end == NULL) {
			strncpy_null(cAddr, addr, os_strlen(addr));
		} else {
			strncpy_null(cAddr, addr, end - addr);
		}
		
		address = strtoul(cAddr, &convert_err, 16);
		if (*convert_err != '\0') {
			*error = "Invalid I2C address";	
			return 0;
		}
		
		if (address < 1 || address > 127) {
			*error = "I2C address not in range";	
			return 0;
		}
	}
	
	return address;
}

/******************************************************************************
 * FunctionName : i2c_check_device
 * Description  : Check if there is device at specified address.
 * Parameters   : address
 * Returns      : i2c_status
*******************************************************************************/
i2c_status ICACHE_FLASH_ATTR i2c_check_device(uint8 address) {
	bool result;
	
	/* Send address byte and check ACK */
	i2c_master_start();
	i2c_master_writeByte(address << 1 | 1);
	result = (bool)i2c_master_getAck();
	if (result) {
		i2c_master_stop();
#if I2C_DEBUG
		debug("i2c_check_device: Address ACK failed [0x%02x]\n", address);
#endif
		return I2C_ADDRESS_NACK;
	}
	
	i2c_master_readByte();
	i2c_master_send_nack();
	
	i2c_master_stop();
	
	return I2C_OK;
}

/******************************************************************************
 * FunctionName : i2c_read_id
 * Description  : Read i2c ID for device at specified address.
 * Parameters   : address
 * Returns      : i2c_status
*******************************************************************************/
i2c_status ICACHE_FLASH_ATTR i2c_read_id(uint8 address, uint8 *id) {

	*id = 0;

	i2c_master_start();

	/* Send address */
	i2c_master_writeByte(address << 1 | 0);
	if (i2c_master_getAck()) {
		i2c_master_stop();
#if I2C_DEBUG
		debug("i2c_read_id: Address ACK failed [0x%02x] [set command]\n", address);
#endif
		return I2C_ADDRESS_NACK;
	}
	
	/* Send command */
	i2c_master_writeByte(0x20);
	if (i2c_master_getAck()) {
		i2c_master_stop();
#if I2C_DEBUG
		debug("i2c_read_id: Command ACK failed [0x%02x]\n", address);
#endif
		return I2C_DATA_NACK;
	}

	/* Send restart */
	i2c_master_stop();
	i2c_master_start();

	/* Send address */
	i2c_master_writeByte(address << 1 | 1);
	if (i2c_master_getAck()) {
		i2c_master_stop();
#if I2C_DEBUG
		debug("i2c_read_id: Address ACK failed [0x%02x] [execute command]\n", address);
#endif
		return I2C_ADDRESS_NACK;
	}

	/* Read one byte of data */
	*id = i2c_master_readByte();
	i2c_master_send_nack();
	i2c_master_stop();
	
	return I2C_OK;
}

/******************************************************************************
 * FunctionName : i2c_find_config
 * Description  : Find i2c config
 * Parameters   : address
                  id
 * Returns      : i2c_config
*******************************************************************************/
i2c_config ICACHE_FLASH_ATTR *i2c_find_config(uint8 address, uint8 id) {
	i2c_config *config;
	STAILQ_FOREACH(config, &i2c_configs, entries) {
		if (config->address == address && config->id == id) {
			return config;
		}
	}
	
	return NULL;
}

/******************************************************************************
 * FunctionName : i2c_add_config
 * Description  : Add i2c config
 * Parameters   : address
                  id
				  data
 * Returns      : i2c_config
*******************************************************************************/
i2c_config ICACHE_FLASH_ATTR *i2c_add_config(uint8 address, uint8 id, void *data) {
	i2c_config *config;
	
	config = (i2c_config *)os_zalloc(sizeof(i2c_config));
	config->address = address;
	config->id = id;
	config->data = data;
	STAILQ_INSERT_TAIL(&i2c_configs, config, entries);
	
	return config;
}

/******************************************************************************
 * FunctionName : i2c_foreach
 * Description  : Add i2c config
 * Parameters   : id
 *				  function
*******************************************************************************/
void ICACHE_FLASH_ATTR i2c_foreach(uint8 id, i2c_callback function) {
	i2c_config *config;
	STAILQ_FOREACH(config, &i2c_configs, entries) {
		if (config->id == id) {
			(*function)(config);
		}
	}
}

i2c_config ICACHE_FLASH_ATTR *i2c_init_handler(
	const char *device,
	const char *device_url,
	i2c_init init,
	const char *url,
	char *response
) {
	char *cAddr = NULL;
	uint8 addr = 0;
	const char *error = NULL;
	
	if ((cAddr = (char *)strstr_end(url, device_url)) == NULL) {
		json_error(response, device, "Invalid URL", NULL);
		return NULL;
	}
	
	addr = i2c_addr_parse(cAddr, &error);
	if (error != NULL) {
		json_error(response, device, error, NULL);
		return NULL;
	}
	
	i2c_status status;
	i2c_config *config = (*init)(&addr, &status);
	if (status != I2C_OK) {
		char address_str[MAX_I2C_ADDRESS];
		json_error(response, device, i2c_status_str(status), json_i2c_address(address_str, addr));
		return NULL;
	}
	
	return config;
}