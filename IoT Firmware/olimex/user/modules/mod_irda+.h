#ifndef __MOD_IRDA_H__
	#define __MOD_IRDA_H__
	
	#include "user_config.h"
	#if MOD_IRDA_ENABLE & I2C_ENABLE

		#define MOD_IRDA_DEBUG        0
		
		#define MOD_IRDA_ID           0x54
		#define MOD_IRDA_DEFAULT_ADDR 0x24

		#include "driver/i2c_extention.h"
		
		typedef struct _irda_config_data_ {
			uint8 mode;
			uint8 command;
			uint8 device;
		} irda_config_data;
		
		const char *irda_mode_str(uint8 mode);
		i2c_config *irda_init(uint8 *address, i2c_status *status);
		i2c_status  irda_set(i2c_config *config);
	#endif
#endif