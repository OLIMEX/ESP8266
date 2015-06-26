#include "ets_sys.h"
#include "stdout.h"
#include "osapi.h"
#include "mem.h"
#include "queue.h"

#include "driver/i2c_master.h"
#include "driver/i2c_extention.h"

#include "user_interface.h"
#include "user_webserver.h"
#include "user_i2c_scan.h"

LOCAL i2c_scan_done_callback i2c_scan_done = NULL;

LOCAL uint8 i2c_current = 0x01;

i2c_devices_queue i2c_devices = STAILQ_HEAD_INITIALIZER(i2c_devices);

LOCAL void ICACHE_FLASH_ATTR i2c_scan() {
	i2c_device *device;
	uint8 id;
	
	if (i2c_read_id(i2c_current, &id) == I2C_OK) {
		device = (i2c_device *)os_zalloc(sizeof(i2c_device));
		
		device->address = i2c_current;
		device->id = id;
		
		STAILQ_INSERT_TAIL(&i2c_devices, device, entries);
	}
	
	if (i2c_current++ == 0x80) {
		(*i2c_scan_done)(&i2c_devices);
		i2c_scan_done = NULL;
		
		STAILQ_FOREACH(device, &i2c_devices, entries) {
			STAILQ_REMOVE(&i2c_devices, device, _i2c_device_, entries);
			os_free(device);
		}
		return;
	}
	
	setTimeout(i2c_scan, NULL, 1);
}

bool ICACHE_FLASH_ATTR i2c_scan_start(i2c_scan_done_callback done_callback) {
	if (i2c_scan_done != NULL) {
		return false;
	}
	
	i2c_scan_done = done_callback;
	i2c_current = 0x01;
	
	setTimeout(i2c_scan, NULL, 1);
	return true;
}
