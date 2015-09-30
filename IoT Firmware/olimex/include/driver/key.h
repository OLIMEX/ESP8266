#ifndef __KEY_H__
	#define __KEY_H__
	
	#include "gpio.h"
	
	#define KEYS_MAX_COUNT 2
	
	typedef void (* key_function)(void);
	
	struct single_key_param {
		uint8 key_level;
		uint8 is_long;
		
		uint8  gpio_id;
		uint32 gpio_name;
		uint8  gpio_func;
		
		os_timer_t key_press;
		os_timer_t key_5s;
		os_timer_t key_50ms;
		
		key_function press;
		key_function short_release;
		key_function long_press;
		key_function long_release;
	};
	
	struct keys_param {
		uint8  key_num;
		struct single_key_param **single_key;
	};

	struct single_key_param *key_init_single(
		uint8  gpio_id, 
		uint32 gpio_name, 
		uint8  gpio_func, 
		key_function press, 
		key_function short_release, 
		key_function long_press, 
		key_function long_release
	);
	
	void key_init();

#endif
