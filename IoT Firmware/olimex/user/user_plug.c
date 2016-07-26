#include "user_config.h"
#if DEVICE == PLUG

#include "ets_sys.h"
#include "osapi.h"
#include "stdout.h"
#include "gpio.h"
#include "gpio_config.h"

#include "user_relay.h"
#include "user_plug.h"

LOCAL bool plug_wifi_blink   = false;
LOCAL bool plug_server_blink = false;

LOCAL gpio_config plug_led_hardware[PLUG_LED_COUNT] = {
	{ 
		.gpio_id    = 14,
		.gpio_name  = PERIPHS_IO_MUX_MTMS_U,
		.gpio_func  = FUNC_GPIO14
	}, 
	{ 
		.gpio_id    = 12,
		.gpio_name  = PERIPHS_IO_MUX_MTDI_U,
		.gpio_func  = FUNC_GPIO12
	}
};

LOCAL gpio_config plug_reset_hardware = {
	.gpio_id    = 15,
	.gpio_name  = PERIPHS_IO_MUX_MTDO_U,
	.gpio_func  = FUNC_GPIO15
};

void ICACHE_FLASH_ATTR plug_led(plug_led_type led, uint8 state) {
	GPIO_OUTPUT_SET(GPIO_ID_PIN(plug_led_hardware[led].gpio_id), state);
}

LOCAL void plug_wifi_blink_cycle() {
	LOCAL bool state = false;
	LOCAL uint32 timer = 0;
	
	if (!plug_wifi_blink) {
		state = false;
		return;
	}
	
	state = !state;
	plug_led(PLUG_LED1, state);
	plug_led(PLUG_LED2, !state && user_relay_get());
	
	clearTimeout(timer);
	timer = setTimeout(plug_wifi_blink_cycle, NULL, 500);
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
	plug_led(PLUG_LED1, 1);
	plug_led(PLUG_LED2, user_relay_get());
}

LOCAL void plug_server_blink_cycle() {
	LOCAL uint8  state = 0;
	LOCAL uint32 timer = 0;
	
	if (!plug_server_blink) {
		state = 0;
		return;
	}
	
	if (plug_wifi_blink) {
		state = 0;
	} else {
		// change led state only if wifi not blinking
		state++;
		if (state > 7) {
			state = 0;
		}
		
		uint8 led_state = (state & 0x01) && (state < 5);
		
		plug_led(PLUG_LED1, led_state);
		plug_led(PLUG_LED2, !led_state && user_relay_get());
	}
	
	clearTimeout(timer);
	timer = setTimeout(plug_server_blink_cycle, NULL, 250);
}

void ICACHE_FLASH_ATTR plug_server_blink_start() {
	if (plug_server_blink) {
		return;
	}
	
	plug_server_blink = true;
	plug_server_blink_cycle();
}

void ICACHE_FLASH_ATTR plug_server_blink_stop() {
	plug_server_blink = false;
	plug_led(PLUG_LED1, 1);
	plug_led(PLUG_LED2, user_relay_get());
}

void ICACHE_FLASH_ATTR plug_down() {
#if PLUG_DEBUG
	debug("PLUG: Down\n");
#endif
	plug_led(PLUG_LED1, 0);
	plug_led(PLUG_LED2, 0);
	
	GPIO_OUTPUT_SET(GPIO_ID_PIN(plug_reset_hardware.gpio_id), 0);
}

void ICACHE_FLASH_ATTR plug_init() {
#if PLUG_DEBUG
	debug("PLUG: Init\n");
#endif
	plug_led(PLUG_LED1, 1);
	plug_led(PLUG_LED2, user_relay_get());
	
	GPIO_OUTPUT_SET(GPIO_ID_PIN(plug_reset_hardware.gpio_id), 0);
	os_delay_us(10000);
	GPIO_OUTPUT_SET(GPIO_ID_PIN(plug_reset_hardware.gpio_id), 1);
}

void ICACHE_FLASH_ATTR user_plug_init() {
	uint8 i;
	
	for (i=0; i<PLUG_LED_COUNT; i++) {
		PIN_FUNC_SELECT(plug_led_hardware[i].gpio_name, plug_led_hardware[i].gpio_func);
	}
	PIN_FUNC_SELECT(plug_reset_hardware.gpio_name, plug_reset_hardware.gpio_func);
	
	plug_init();
	plug_wifi_blink_start();
}

#endif
