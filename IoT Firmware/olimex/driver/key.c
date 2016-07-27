/******************************************************************************
 * Copyright 2013-2014 Espressif Systems (Wuxi)
 *
 * FileName: key.c
 *
 * Description: key driver, now can use different gpio and install different function
 *
 * Modification history:
 *	 2014/5/1, v1.0 create this file.
*******************************************************************************/
#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "mem.h"
#include "gpio.h"
#include "user_interface.h"

#include "driver/key.h"

LOCAL struct single_key_param *key_data[KEYS_MAX_COUNT];
LOCAL struct keys_param key_parameters = {
	.key_num = 0,
	.single_key = key_data
};

LOCAL void key_intr_handler(struct keys_param *keys);

/******************************************************************************
 * FunctionName : key_init_single
 * Description  : init single key's gpio and register function
 * Parameters   : uint8 gpio_id - which gpio to use
 *				uint32 gpio_name - gpio mux name
 *				uint32 gpio_func - gpio function
 *				key_function long_press - long press function, needed to install
 *				key_function short_press - short press function, needed to install
 * Returns	  : single_key_param - single key parameter, needed by key init
*******************************************************************************/
struct single_key_param ICACHE_FLASH_ATTR *key_init_single(
	uint8  gpio_id, 
	uint32 gpio_name, 
	uint8  gpio_func, 
	key_function press, 
	key_function short_release, 
	key_function long_press, 
	key_function long_release
) {
	if (key_parameters.key_num >= KEYS_MAX_COUNT) {
		return NULL;
	}
	
	struct single_key_param *single_key = (struct single_key_param *)os_zalloc(sizeof(struct single_key_param));
	
	single_key->gpio_id = gpio_id;
	single_key->gpio_name = gpio_name;
	single_key->gpio_func = gpio_func;
	single_key->press = press;
	single_key->short_release = short_release;
	single_key->long_press = long_press;
	single_key->long_release = long_release;
	
	key_parameters.single_key[key_parameters.key_num] = single_key;
	key_parameters.key_num++;
	
	return single_key;
}

/******************************************************************************
 * FunctionName : key_init
 * Description  : init keys
 * Parameters   : key_param *keys - keys parameter, which inited by key_init_single
 * Returns	  : none
*******************************************************************************/
void ICACHE_FLASH_ATTR key_init() {
	uint8 i;
	
	ETS_GPIO_INTR_ATTACH(key_intr_handler, &key_parameters);
	
	ETS_GPIO_INTR_DISABLE();
	
	for (i = 0; i < key_parameters.key_num; i++) {
		key_parameters.single_key[i]->key_level = 1;
		key_parameters.single_key[i]->is_long = 0;
		
		PIN_FUNC_SELECT(key_parameters.single_key[i]->gpio_name, key_parameters.single_key[i]->gpio_func);
		
		// Set GPIO as input
		gpio_output_set(0, 0, 0, GPIO_ID_PIN(key_parameters.single_key[i]->gpio_id));
		
		gpio_register_set(
			GPIO_PIN_ADDR(key_parameters.single_key[i]->gpio_id), 
			GPIO_PIN_INT_TYPE_SET(GPIO_PIN_INTR_DISABLE) | 
			GPIO_PIN_PAD_DRIVER_SET(GPIO_PAD_DRIVER_DISABLE) |
			GPIO_PIN_SOURCE_SET(GPIO_AS_PIN_SOURCE)
		);
		
		//clear gpio14 status
		GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, BIT(key_parameters.single_key[i]->gpio_id));
		
		//enable interrupt
		gpio_pin_intr_state_set(GPIO_ID_PIN(key_parameters.single_key[i]->gpio_id), GPIO_PIN_INTR_NEGEDGE);
	}
	
	ETS_GPIO_INTR_ENABLE();
}

/******************************************************************************
 * FunctionName : key_5s_cb
 * Description  : long press 5s timer callback
 * Parameters   : single_key_param *single_key - single key parameter
 * Returns	  : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR key_5s_cb(struct single_key_param *single_key) {
	os_timer_disarm(&single_key->key_5s);

	// low, then restart
	if (0 == GPIO_INPUT_GET(GPIO_ID_PIN(single_key->gpio_id))) {
		single_key->is_long = 1;
		if (single_key->long_press) {
			single_key->long_press();
		}
	}
}

/******************************************************************************
 * FunctionName : key_50ms_press
 * Description  : 50ms timer callback to check it's a real key push
 * Parameters   : single_key_param *single_key - single key parameter
 * Returns	  : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR key_50ms_press(struct single_key_param *single_key) {
	os_timer_disarm(&single_key->key_press);

	// low, then key is down
	if (0 == GPIO_INPUT_GET(GPIO_ID_PIN(single_key->gpio_id))) {
		// 5s long release timer
		os_timer_disarm(&single_key->key_5s);
		os_timer_setfn(&single_key->key_5s, (os_timer_func_t *)key_5s_cb, single_key);
		os_timer_arm(&single_key->key_5s, 5000, 0);
		
		single_key->key_level = 0;
		single_key->is_long = 0;
		gpio_pin_intr_state_set(GPIO_ID_PIN(single_key->gpio_id), GPIO_PIN_INTR_POSEDGE);
		
		// PRESS callback
		if (single_key->press) {
			single_key->press();
		}
	} else {
		gpio_pin_intr_state_set(GPIO_ID_PIN(single_key->gpio_id), GPIO_PIN_INTR_NEGEDGE);
	}
}

/******************************************************************************
 * FunctionName : key_50ms_release
 * Description  : 50ms timer callback to check it's a real key push
 * Parameters   : single_key_param *single_key - single key parameter
 * Returns	  : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR key_50ms_release(struct single_key_param *single_key) {
	os_timer_disarm(&single_key->key_50ms);

	// high, then key is up
	if (1 == GPIO_INPUT_GET(GPIO_ID_PIN(single_key->gpio_id))) {
		os_timer_disarm(&single_key->key_5s);
		single_key->key_level = 1;
		gpio_pin_intr_state_set(GPIO_ID_PIN(single_key->gpio_id), GPIO_PIN_INTR_NEGEDGE);
		
		// RELEASE callback
		if (single_key->is_long) {
			if (single_key->long_release) {
				single_key->long_release();
			}
		} else {
			if (single_key->short_release) {
				single_key->short_release();
			}
		}
	} else {
		gpio_pin_intr_state_set(GPIO_ID_PIN(single_key->gpio_id), GPIO_PIN_INTR_POSEDGE);
	}
}

/******************************************************************************
 * FunctionName : key_intr_handler
 * Description  : key interrupt handler
 * Parameters   : key_param *keys - keys parameter, which inited by key_init_single
 * Returns	  : none
*******************************************************************************/
LOCAL void key_intr_handler(struct keys_param *keys) {
	uint8 i;
	uint32 gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);

	for (i = 0; i < keys->key_num; i++) {
		if (gpio_status & BIT(keys->single_key[i]->gpio_id)) {
			//disable interrupt
			gpio_pin_intr_state_set(GPIO_ID_PIN(keys->single_key[i]->gpio_id), GPIO_PIN_INTR_DISABLE);

			//clear interrupt status
			GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status & BIT(keys->single_key[i]->gpio_id));

			if (keys->single_key[i]->key_level == 1) {
				// 50ms check if this is a real key press
				os_timer_disarm(&keys->single_key[i]->key_press);
				os_timer_setfn(&keys->single_key[i]->key_press, (os_timer_func_t *)key_50ms_press, keys->single_key[i]);
				os_timer_arm(&keys->single_key[i]->key_press, 50, 0);
			} else {
				// 50ms check if this is a real key up
				os_timer_disarm(&keys->single_key[i]->key_50ms);
				os_timer_setfn(&keys->single_key[i]->key_50ms, (os_timer_func_t *)key_50ms_release, keys->single_key[i]);
				os_timer_arm(&keys->single_key[i]->key_50ms, 50, 0);
			}
		}
	}
}
