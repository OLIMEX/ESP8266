/******************************************************************************
 * Copyright 2013-2014 Espressif Systems (Wuxi)
 *
 * FileName: user_main.c
 *
 * Description: entry file of user application
 *
 * Modification history:
 *	 2014/1/1, v1.0 create this file.
 *******************************************************************************/
#include "ets_sys.h"
#include "osapi.h"
#include "user_interface.h"

#include "stdout.h"
#include "user_timer.h"

#include "user_config.h"

#if SSL_ENABLE
	#include "ssl/private_key.h"
	#include "ssl/cert.h"
#else
	unsigned char *default_certificate = NULL;
	unsigned int   default_certificate_len = 0;
	unsigned char *default_private_key = NULL;
	unsigned int   default_private_key_len = 0;
#endif

#include "driver/key.h"
#include "driver/uart.h"

#include "user_button.h"
#include "user_relay.h"
#include "user_adc.h"
#include "user_battery.h"

#include "user_switch.h"
#include "user_switch2.h"
#include "user_badge.h"

#include "user_events.h"
#include "user_webserver.h"

#include "user_devices.h"
#include "user_i2c_scan.h"
#include "user_wifi_scan.h"

#include "user_mod_rfid.h"
#include "user_mod_finger.h"

#include "user_mod_rgb.h"
#include "user_mod_tc_mk2.h"
#include "user_mod_io2.h"
#include "user_mod_irda+.h"

#include "user_mod_led_8x8_rgb.h"

void ICACHE_FLASH_ATTR user_rf_pre_init(void) {
}

void ICACHE_FLASH_ATTR user_init_done() {
	debug("\nINIT: Done\n\n");
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns	    : none
 *******************************************************************************/
void ICACHE_FLASH_ATTR user_init(void) {
	system_init_done_cb(user_init_done);
	
//	wifi_set_phy_mode(PHY_MODE_11N);
//	wifi_set_sleep_type(MODEM_SLEEP_T);
	
#if UART0_SWAP
	system_uart_swap();
#endif
#if UART1_ENABLE
	stdout_init(UART1);
#endif
	
	// UART Devices
#if MOD_RFID_ENABLE
	mod_rfid_init();
#endif
#if MOD_FINGER_ENABLE
	mod_finger_init();
#endif
#if MOD_EMTR_ENABLE
	mod_emtr_init();
#endif
	devices_init();
	
	debug("\n\nSDK version: %s\n", system_get_sdk_version());
	debug("Firmware: %s\n", config_firmware_bin());
	memory_info();
	
	user_config_init();
	user_config_load();
#if SSL_ENABLE
	user_config_load_private_key();
	user_config_load_certificate();
#endif
	
	user_events_init();
	
	// Scan
	webserver_register_handler_callback(DEVICES_URL,   devices_handler);
	webserver_register_handler_callback(WIFI_SCAN_URL, wifi_scan_handler);
	
	// Native Devices
#if BUTTON_ENABLE
	user_button_init();
#endif
#if RELAY_ENABLE
	user_relay_init();
#endif
#if ADC_ENABLE
	user_adc_init();
#endif
#if BATTERY_ENABLE
	user_battery_init();
#endif
#if DEVICE == SWITCH
	user_switch_init();
#endif
#if DEVICE == SWITCH2
	user_switch2_init();
#endif
	
#if I2C_ENABLE	
	// I2C Devices
	i2c_master_gpio_init();
#if MOD_RGB_ENABLE
	mod_rgb_init();
#endif
#if MOD_TC_MK2_ENABLE
	mod_tc_mk2_init();
#endif
#if MOD_IO2_ENABLE
	mod_io2_init();
#endif
#if MOD_IRDA_ENABLE
	mod_irda_init();
#endif
#endif
	
	// SPI Devices
#if MOD_LED_8x8_RGB_ENABLE
	mod_led_8x8_rgb_init();
#endif
#if DEVICE == BADGE
	badge_init();
#endif
	
	key_init();
	webserver_init();
}
