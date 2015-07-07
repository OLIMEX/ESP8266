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

#include "user_button.h"
#include "user_relay.h"
#include "user_adc.h"

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
	
	// UART Devices
	mod_rfid_init();
	mod_finger_init();
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
	user_button_init();
	user_relay_init();
	user_adc_init();
	
	// I2C Devices
	i2c_master_gpio_init();
	mod_rgb_init();
	mod_tc_mk2_init();
	mod_io2_init();
	mod_irda_init();
	
	// SPI Devices
	mod_led_8x8_rgb_init();
	
	webserver_init();
}
