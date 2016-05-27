#ifndef __MOD_TC_MK2_H__
	#define __MOD_TC_MK2_H__

	#include "user_config.h"
	#if MOD_TC_MK2_ENABLE & I2C_ENABLE

		#define MOD_TC_MK2_ID             0x27
		#define MOD_TC_MK2_DEFAULT_ADDR   0x23
		
		#define MOD_TC_MK2_FILTER_FACTOR     4

		#include "driver/i2c_extention.h"

		typedef struct _tc_config_data_ {
			bool   first;
			sint16 raw;                  // raw data read
			sint16 temperature;          // integer representation 1 = 0.25 degrees
			char   temperature_str[10];  // actual temperature string representation
		} tc_config_data;
		
		i2c_config  *tc_init(uint8 *address, i2c_status *status);
		i2c_status   tc_read(i2c_config *config);
		void         tc_foreach(i2c_callback function);
	#endif
#endif
