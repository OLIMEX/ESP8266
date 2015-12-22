#ifndef __DIMMER_H__
	#define __DIMMER_H__
	
	#include "user_config.h"
	#if DEVICE == DIMMER
		#define DIMMER_DEBUG        0
		
		#define DIMMER_ID           0x29
		#define DIMMER_DEFAULT_ADDR 0x29

		#include "driver/i2c_extention.h"
		
		typedef struct _dimmer_config_data_ {
			uint8   relay;
			uint8   brightness;
			uint16  current;
		} dimmer_config_data;
		
		void dimmer_foreach(i2c_callback function);
		
		i2c_config *dimmer_init(uint8 *address, i2c_status *status);
		i2c_status  dimmer_set(i2c_config *config);
	#endif
#endif