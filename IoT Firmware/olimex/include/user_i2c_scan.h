#ifndef __USER_I2C_SCAN_H__
	#define __USER_I2C_SCAN_H__
	
	typedef struct _i2c_device_ {
		uint8 address;
		uint8 id;
		
		STAILQ_ENTRY(_i2c_device_) entries;
	} i2c_device;
	
	typedef STAILQ_HEAD(i2c_devices_queue, _i2c_device_) i2c_devices_queue;
	
	typedef void (*i2c_scan_done_callback)(i2c_devices_queue *queue);
	
	bool i2c_scan_start(i2c_scan_done_callback done_callback);
#endif
