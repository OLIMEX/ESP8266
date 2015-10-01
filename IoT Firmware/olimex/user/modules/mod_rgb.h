#ifndef __MOD_RGB_MOD_RGB_H__
	#define __MOD_RGB_MOD_RGB_H__

	#include "user_config.h"
	#if MOD_RGB_ENABLE

		#define MOD_RGB_ID           0x64
		#define MOD_RGB_DEFAULT_ADDR 0x20

		#include "driver/i2c_extention.h"

		typedef struct _rgb_config_data_ {
			uint8 red;
			uint8 green;
			uint8 blue;
		} rgb_config_data;

		i2c_config *rgb_init(uint8 *address, i2c_status *status);
		i2c_status  rgb_set(i2c_config *config);

	#endif
#endif
