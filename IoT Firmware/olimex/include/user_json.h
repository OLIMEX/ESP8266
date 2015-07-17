#ifndef __USER_JSON_H__
	#define __USER_JSON_H__
	
	#include "json/jsonparse.h"
	
	#define MAX_I2C_ADDRESS 30
	
	char *json_sprintf(char *buffer, const char *fmt, ...);
	char *json_error (char *buffer, const char *device, const char *error,  const char *extra);
	char *json_status(char *buffer, const char *device, const char *status, const char *extra);
	char *json_data  (char *buffer, const char *device, const char *status, const char *data, const char *extra);
	
	char *json_i2c_address(char *address_str, uint8 address);
	char *json_poll_str(char *poll_str, uint32 refresh, uint8 each, uint32 threshold);
	
	void  jsonparse_object_str(struct jsonparse_state *parser, char *dst, int dst_len);
	
	extern const char ESP8266[];
	extern const char MOD_IO2[];
	extern const char MOD_IRDA[];
	extern const char MOD_LED8x8RGB[];
	extern const char MOD_RFID[];
	extern const char MOD_RGB[];
	extern const char MOD_TC_MK2[];
	extern const char MOD_FINGER[];
	extern const char MOD_EMTR[];
	
	extern const char DEVICE_NOT_FOUND[];
	extern const char TIMEOUT[];
	
	extern const char OK_STR[];
	extern const char REBOOTING[];
	extern const char CONNECTED[];
	extern const char RECONNECT[];
	extern const char DONE[];
	extern const char RESTORED_DEFAULTS[];
	
	extern const char EMPTY_DATA[];
#endif
