#include "user_config.h"
#if DEVICE == PLUG

#include "ets_sys.h"
#include "osapi.h"
#include "stdout.h"
#include "gpio.h"

#include "user_relay.h"
#include "user_plug.h"

LOCAL bool plug_wifi_blink = false;

LOCAL plug_led_config plug_led_def[PLUG_LED_COUNT] = {
	{ 
		.gpio_id    = 0,
		.gpio_name  = PERIPHS_IO_MUX_GPIO0_U,
		.gpio_func  = FUNC_GPIO0
	}, 
	{ 
		.gpio_id    = 2,
		.gpio_name  = PERIPHS_IO_MUX_GPIO2_U,
		.gpio_func  = FUNC_GPIO2
	}
};

void ICACHE_FLASH_ATTR plug_led(plug_led_type led, uint8 state) {
	GPIO_OUTPUT_SET(GPIO_ID_PIN(plug_led_def[led].gpio_id), state);
}

LOCAL void plug_wifi_blink_cycle() {
	LOCAL bool state = false;
	
	if (!plug_wifi_blink) {
		return;
	}
	
	state = !state;
	plug_led(PLUG_LED2, state);
	
	setTimeout(plug_wifi_blink_cycle, NULL, 500);
}

void ICACHE_FLASH_ATTR plug_wifi_blink_start() {
	if (plug_wifi_blink) {
		return;
	}
	
	plug_wifi_blink = true;
	plug_wifi_blink_cycle();
}

void ICACHE_FLASH_ATTR plug_wifi_blink_stop() {
	plug_wifi_blink = false;
	plug_led(PLUG_LED2, user_relay_get());
}

void ICACHE_FLASH_ATTR user_plug_init() {
	uint8 i;
	
	for (i=0; i<PLUG_LED_COUNT; i++) {
		PIN_FUNC_SELECT(plug_led_def[i].gpio_name, plug_led_def[i].gpio_func);
	}
	
	plug_led(PLUG_LED1, 1);
	plug_led(PLUG_LED2, 0);
	
	plug_wifi_blink_start();
}

#endif
