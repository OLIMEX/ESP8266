#ifndef __MOD_IO2_H__
	#define __MOD_IO2_H__
	
	#include "user_config.h"
	#if MOD_IO2_ENABLE
		#define MOD_IO2_ID           0x23
		#define MOD_IO2_DEFAULT_ADDR 0x21

		#include "driver/i2c_extention.h"
		
		typedef struct _io2_config_data_ {
			uint8  relay1;
			uint8  relay2;
			uint16 gpio0;
			uint16 gpio1;
			uint8  gpio2;
			uint8  gpio3;
			uint8  gpio4;
			uint8  gpio5;
			uint8  gpio6;
		} io2_config_data;
		
		i2c_config *io2_init(uint8 *address, i2c_status *status);
		i2c_status  io2_set(i2c_config *config);
	#endif
#endif