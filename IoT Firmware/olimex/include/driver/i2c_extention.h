#ifndef __I2C_EXTENTION_H__
	#define __I2C_EXTENTION_H__
	
	#define I2C_DEBUG       0
	
	typedef struct {
		uint8 code;
		char description[20];
	} i2c_command;

	typedef enum {
		I2C_OK = 0,
		I2C_ADDRESS_NACK,
		I2C_DATA_NACK,
		I2C_DEVICE_NOT_FOUND,
		I2C_DEVICE_ID_DONT_MATCH,
		I2C_COMMUNICATION_FAILED
	} i2c_status;
	
	typedef struct _i2c_config_ {
		uint8  address;
		uint8  id;
		
		void  *data;
		
		STAILQ_ENTRY(_i2c_config_) entries;
	} i2c_config;
	
	typedef void (*i2c_callback)(i2c_config *config);
	typedef i2c_config *(*i2c_init)(uint8 *address, i2c_status *status);
	
	const char *i2c_status_str(i2c_status status);
	uint8       i2c_addr_parse(char *cAddr, const char **error);
	i2c_status  i2c_check_device(uint8 address);
	i2c_status  i2c_read_id(uint8 address, uint8 *id);
	
	i2c_config *i2c_find_config(uint8 address, uint8 id);
	i2c_config *i2c_add_config(uint8 address, uint8 id, void *data);
	
	void        i2c_foreach(uint8 id, i2c_callback function);
	
	i2c_config *i2c_init_handler(const char *device, const char *device_url, i2c_init init, const char *url, char *response);
#endif