#ifndef __USER_JSON_H__
	#define __USER_JSON_H__
	
	#include "json/jsonparse.h"
	
	#define MAX_I2C_ADDRESS 30
	
	char *json_sprintf(char *buffer, const char *fmt, ...);
	char *json_error (char *buffer, const char *device, const char *error,  const char *extra);
	char *json_status(char *buffer, const char *device, const char *status, const char *extra);
	char *json_data  (char *buffer, const char *device, const char *status, const char *data, const char *extra);
	
	char *json_i2c_address(char *address_str, uint8 address);
	char *json_escape_str(char *str, uint16 max_len);
	char *json_poll_str(char *poll_str, uint32 refresh, uint8 each, uint32 threshold);
	
	void  jsonparse_object_str(struct jsonparse_state *parser, char *dst, int dst_len);
	char *jsonparse_alloc_object_str(struct jsonparse_state *parser);
	
	extern const char ESP8266[];
	#if DEVICE == SWITCH1
	extern const char SWITCH1_STR[];
	#endif
	#if DEVICE == SWITCH2
	extern const char SWITCH2_STR[];
	#endif
	#if DEVICE == BADGE
	extern const char BADGE_STR[];
	#endif
	#if DEVICE == DIMMER
	extern const char DIMMER_STR[];
	#endif
	#if DEVICE == ROBKO
	extern const char ROBKO_STR[];
	#endif
	
	#if MOD_IO2_ENABLE
	extern const char MOD_IO2[];
	#endif
	#if MOD_IRDA_ENABLE
	extern const char MOD_IRDA[];
	#endif
	#if MOD_LED_8x8_RGB_ENABLE
	extern const char MOD_LED8x8RGB[];
	#endif
	#if MOD_RFID_ENABLE
	extern const char MOD_RFID[];
	#endif
	#if MOD_RGB_ENABLE
	extern const char MOD_RGB[];
	#endif
	#if MOD_TC_MK2_ENABLE
	extern const char MOD_TC_MK2[];
	#endif
	#if MOD_FINGER_ENABLE
	extern const char MOD_FINGER[];
	#endif
	#if MOD_EMTR_ENABLE
	extern const char MOD_EMTR[];
	#endif
	
	extern const char DEVICE_NOT_FOUND[];
	extern const char TIMEOUT[];
	
	extern const char OK_STR[];
	extern const char REBOOTING[];
	extern const char CONNECTED[];
	extern const char DISCONNECTED[];
	extern const char RECONNECT[];
	extern const char BUSY_STR[];
	extern const char DONE[];
	extern const char RESTORED_DEFAULTS[];
	
	extern const char EMPTY_DATA[];
#endif
