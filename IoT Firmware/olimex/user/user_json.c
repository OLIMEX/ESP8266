#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "stdarg.h"
#include "mem.h"

#include "user_json.h"

const char ESP8266[]           = "ESP8266";

const char MOD_IO2[]           = "MOD-IO2";
const char MOD_IRDA[]          = "MOD-IRDA";
const char MOD_LED8x8RGB[]     = "MOD-LED8x8RGB";
const char MOD_RFID[]          = "MOD-RFID";
const char MOD_RGB[]           = "MOD-RGB";
const char MOD_TC_MK2[]        = "MOD-TC-MK2";

const char OK_STR[]            = "OK";
const char REBOOTING[]         = "Rebooting";
const char CONNECTED[]         = "Connected";
const char RECONNECT[]         = "Reconnect station";
const char DONE[]              = "Done";
const char RESTORED_DEFAULTS[] = "Restored defaults";

const char EMPTY_DATA[]        = "\"Data\" : {}";

char ICACHE_FLASH_ATTR *json_sprintf(char *buffer, const char *fmt, ...) {
	va_list args;

	va_start(args, fmt);
	ets_vsprintf(buffer, fmt, args);
	va_end(args);
	
	return buffer;
}

char ICACHE_FLASH_ATTR *json_error(char *buffer, const char *device, const char *error, const char *extra) {
	os_sprintf(
		buffer, 
		"{\"Device\" : \"%s\", \"Error\" : \"%s\"%s%s}", 
		device, error,
		extra == NULL ? "" : ", ", 
		extra == NULL ? "" : extra
	);
	return buffer;
}

char ICACHE_FLASH_ATTR *json_status(char *buffer, const char *device, const char *status, const char *extra) {
	os_sprintf(
		buffer, 
		"{\"Device\" : \"%s\", \"Status\" : \"%s\"%s%s}", 
		device, status, 
		extra == NULL ? "" : ", ", 
		extra == NULL ? "" : extra
	);
	return buffer;
}

char ICACHE_FLASH_ATTR *json_data(char *buffer, const char *device, const char *status, const char *data, const char *extra) {
	uint32 extra_len = extra == NULL ? 0 : os_strlen(extra);
	char data_buff[os_strlen(data) + extra_len + 20];
	return json_status(
		buffer, device, status, 
		json_sprintf(
			data_buff, 
			"\"Data\" : {%s}%s%s", 
			data,
			extra_len == 0 ? "" : ", ",
			extra_len == 0 ? "" : extra
		)
	);
}

char ICACHE_FLASH_ATTR *json_i2c_address(char *address_str, uint8 address) {
	os_sprintf(address_str,	"\"I2C_Address\" : \"0x%02x\"", address);
	return address_str;
}

/******************************************************************************
 * FunctionName : json_poll_str
 * Description  : build poll json string
 * Parameters   : 
*******************************************************************************/
char ICACHE_FLASH_ATTR *json_poll_str(char *poll_str, uint32 refresh, uint8 each, uint32 threshold) {
	os_sprintf(
		poll_str, 
		", \"Poll\" : {\"Refresh\" : %d, \"Each\" : %d, \"Threshold\" : %d}",
		refresh,
		each,
		threshold
	);
	return poll_str;
}

/******************************************************************************
 * FunctionName : jsonparse_object_str
 * Description  : 
 * Parameters   : 
*******************************************************************************/
void ICACHE_FLASH_ATTR jsonparse_object_str(struct jsonparse_state *parser, char *dst, int dst_len) {
	int pos = parser->pos;
	int depth = parser->depth;
	
	*dst = '\0';
	
	int json_type = jsonparse_get_type(parser);
	
	while (json_type != 0 && json_type != '}' && parser->depth != depth-1) {
		json_type = jsonparse_next(parser);
	}
	
	if (json_type == '}') {
		int len = parser->pos - pos + 1;
		if (len < dst_len) {
			os_memcpy(dst, parser->json + pos - 1, len);
			dst[len] = '\0';
		}
	}
}
