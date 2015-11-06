#include "ets_sys.h"
#include "stdout.h"
#include "osapi.h"
#include "mem.h"
#include "queue.h"

#include "driver/uart.h"

#include "user_interface.h"
#include "user_config.h"
#include "user_i2c_scan.h"
#include "user_devices.h"

#include "user_mod_rfid.h"
#include "user_mod_finger.h"
#include "user_mod_emtr.h"

STAILQ_HEAD(device_descriptions, _device_description_) device_descriptions = STAILQ_HEAD_INITIALIZER(device_descriptions);

LOCAL device_uart uart_device = UART_NONE;

device_uart ICACHE_FLASH_ATTR device_get_uart() {
	return uart_device;
}

void ICACHE_FLASH_ATTR device_set_uart(device_uart device) {
	uart_device = device;
}

LOCAL const char ICACHE_FLASH_ATTR *device_type_str(device_type type) {
	switch (type) {
		case NATIVE: return "NATIVE";
		case UART:   return "UART";
		case I2C:    return "I2C";
		case SPI:    return "SPI";
	}
	
	return "UNKNOWN";
}

LOCAL device_description ICACHE_FLASH_ATTR *device_find(char *url) {
	device_description *description;
	STAILQ_FOREACH(description, &device_descriptions, entries) {
		if (str_match(description->url, url)) {
			return description;
		}
	}
	
	return NULL;
}

char ICACHE_FLASH_ATTR *device_find_url(device_type type, uint8 id) {
	device_description *description;
	STAILQ_FOREACH(description, &device_descriptions, entries) {
		if (
			description->type == type && 
			(id == 0 || description->id == id)
		) {
			return description->url;
		}
	}
	
	return NULL;
}

LOCAL void ICACHE_FLASH_ATTR devices_init_done() {
#if UART1_ENABLE
	stdout_init(UART1);
#else
	if (device_get_uart() == UART_NONE) {
		stdout_init(UART0);
	} else {
		stdout_wifi_debug();
	}
#endif
}

void ICACHE_FLASH_ATTR devices_init() {
	device_description *description;
	uint16 timeout = 10;
	
	STAILQ_FOREACH(description, &device_descriptions, entries) {
		if (description->init != NULL) {
			setTimeout(description->init, NULL, timeout);
			timeout += DEVICES_TIMEOUT;
		}
	}
	setTimeout(devices_init_done, NULL, timeout);
}

void ICACHE_FLASH_ATTR devices_down() {
	device_description *description;
	uint16 timeout = 10;
	
	STAILQ_FOREACH(description, &device_descriptions, entries) {
		if (description->down != NULL) {
			setTimeout(description->down, NULL, timeout);
			timeout += DEVICES_TIMEOUT;
		}
	}
}

void ICACHE_FLASH_ATTR device_register(device_type type, uint8 id, char *url, void_func init, void_func down) {
	device_description *description = device_find(url);
	
	if (description != NULL) {
		debug("WARNING: Device [%s] already registered\n", url);
		return;
	}
	
	description = (device_description *)os_zalloc(sizeof(device_description));
	
	description->type = type;
	description->url  = url;
	description->id   = id;
	description->init = init;
	description->down = down;
	
	STAILQ_INSERT_TAIL(&device_descriptions, description, entries);
}

char ICACHE_FLASH_ATTR *devices_scan_result(i2c_devices_queue *i2c, char *devices) {
	device_description *description;
	
	devices[0] = '\0';
	STAILQ_FOREACH(description, &device_descriptions, entries) {
		char device[WEBSERVER_MAX_VALUE];
		char i2c_str[WEBSERVER_MAX_VALUE];
		
		bool found = false;
		i2c_str[0] = '\0';
#if I2C_ENABLE
		if (description->type == I2C) {
			i2c_device *d;
			char addresses[WEBSERVER_MAX_VALUE];
			addresses[0] = '\0';
			STAILQ_FOREACH(d, i2c, entries) {
				if (d->id == description->id) {
					found = true;
					char address[10];
					os_sprintf(address, "%s\"0x%02x\"", addresses[0] == '\0' ? "" : ", ", d->address);
					os_strcat(
						addresses, 
						address
					);
				}
			}
			
			os_sprintf(i2c_str, ", \"ID\" : \"0x%02x\", \"Addresses\" : [%s]", description->id, addresses);
		} else 
#endif
		if (description->type == UART) {
			found = 
				false
#if MOD_RFID_ENABLE
				||
				(description->url == RFID_URL   && uart_device == UART_RFID) 
#endif
#if MOD_FINGER_ENABLE
				||
				(description->url == FINGER_URL && uart_device == UART_FINGER) 
#endif
#if MOD_EMTR_ENABLE	
				||
				(description->url == EMTR_URL   && uart_device == UART_EMTR)
#endif
			;
		} else {
			found = true;
		}
		
		os_sprintf(
			device, 
			"%s{"
			"\"Type\" : \"%s\", "
			"\"URL\" : \"%s\", "
			"\"Found\" : %d"
			"%s}", 
			devices[0] == '\0' ? "" : ", ",
			device_type_str(description->type),
			description->url,
			found,
			i2c_str
		);
		os_strcat(devices, device);
	}
	
	return devices;
}

void ICACHE_FLASH_ATTR devices_i2c_done(i2c_devices_queue *i2c) {
	char response[WEBSERVER_MAX_RESPONSE_LEN];
	char devices[WEBSERVER_MAX_RESPONSE_LEN];
	
	os_sprintf(
		response,
		"{\"Device\" : \"ESP8266\","
		" \"Status\" : \"OK\","
		" \"Data\" : {"
			"\"Devices\" : [%s]"
		"}}", 
		devices_scan_result(i2c, devices)
	);
	
	user_event_raise(DEVICES_URL, response);
}

void ICACHE_FLASH_ATTR devices_handler(
	struct espconn *pConnection, 
	request_method method, 
	char *url, 
	char *data, 
	uint16 data_len, 
	uint32 content_len, 
	char *response,
	uint16 response_len
) {
	webserver_set_status(0);
#if I2C_ENABLE	
	i2c_scan_start(devices_i2c_done);
#else
	setTimeout(devices_i2c_done, NULL, 10);
#endif
}