#include "ets_sys.h"
#include "stdout.h"
#include "osapi.h"
#include "mem.h"
#include "queue.h"

#include "user_interface.h"
#include "user_i2c_scan.h"
#include "user_devices.h"

STAILQ_HEAD(device_descriptions, _device_description_) device_descriptions = STAILQ_HEAD_INITIALIZER(device_descriptions);

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

void ICACHE_FLASH_ATTR device_register(device_type type, uint8 id, char *url) {
	device_description *description = device_find(url);
	
	if (description != NULL) {
		debug("WARNING: Device [%s] already registered\n", url);
		return;
	}
	
	description = (device_description *)os_zalloc(sizeof(device_description));
	
	description->type = type;
	description->url  = url;
	description->id   = id;
	
	STAILQ_INSERT_TAIL(&device_descriptions, description, entries);
}

char ICACHE_FLASH_ATTR *devices_scan_result(i2c_devices_queue *i2c, char *devices) {
	device_description *description;
	
	devices[0] = '\0';
	STAILQ_FOREACH(description, &device_descriptions, entries) {
		char device[WEBSERVER_MAX_VALUE];
		char i2c_str[WEBSERVER_MAX_VALUE];
		
		i2c_str[0] = '\0';
		if (description->type == I2C) {
			i2c_device *d;
			char addresses[WEBSERVER_MAX_VALUE];
			addresses[0] = '\0';
			STAILQ_FOREACH(d, i2c, entries) {
				if (d->id == description->id) {
					char address[10];
					os_sprintf(address, "%s\"0x%02x\"", addresses[0] == '\0' ? "" : ", ", d->address);
					os_strcat(
						addresses, 
						address
					);
				}
			}
			
			os_sprintf(i2c_str, ", \"ID\" : \"0x%02x\", \"Addresses\" : [%s]", description->id, addresses);
		}
		
		os_sprintf(
			device, 
			"%s{"
			"\"Type\" : \"%s\", "
			"\"URL\" : \"%s\" "
			"%s}", 
			devices[0] == '\0' ? "" : ", ",
			device_type_str(description->type),
			description->url,
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
	i2c_scan_start(devices_i2c_done);
}