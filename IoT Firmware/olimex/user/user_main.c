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

#include "user_flash.h"
#include "user_preferences.h"

#include "user_button.h"
#include "user_relay.h"
#include "user_adc.h"
#include "user_battery.h"

#include "user_plug.h"
#include "user_switch1.h"
#include "user_switch2.h"
#include "user_badge.h"
#include "user_dimmer.h"
#include "user_robko.h"

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
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABCCC
 *                A : rf cal
 *                B : rf init data
 *                C : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
*******************************************************************************/
uint32 ICACHE_FLASH_ATTR user_rf_cal_sector_set(void) {
	enum flash_size_map size_map = system_get_flash_size_map();
	uint32 rf_cal_sec = 0;
	
	switch (size_map) {
		case FLASH_SIZE_4M_MAP_256_256:
			rf_cal_sec = 128 - 5;
		break;
		
		case FLASH_SIZE_8M_MAP_512_512:
			rf_cal_sec = 256 - 5;
		break;
		
		case FLASH_SIZE_16M_MAP_512_512:
		case FLASH_SIZE_16M_MAP_1024_1024:
			rf_cal_sec = 512 - 5;
		break;
		
		case FLASH_SIZE_32M_MAP_512_512:
		case FLASH_SIZE_32M_MAP_1024_1024:
			rf_cal_sec = 1024 - 5;
		break;
		
		default:
			rf_cal_sec = 0;
		break;
	}
	
	return rf_cal_sec;
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns	    : none
 *******************************************************************************/
void ICACHE_FLASH_ATTR user_init(void) {
	system_init_done_cb(user_init_done);
	
	// Force 802.11g mode to prevent random 
	// hardware WDT resets on some routers
	wifi_set_phy_mode(PHY_MODE_11G);
	
#if UART0_SWAP
	stdout_disable();
	system_uart_swap();
#endif
#if UART1_ENABLE
	stdout_init(UART1);
#endif
	
	flash_region_register("boot.bin",     0x000, 0x001);
	flash_region_register("user1.bin",    0x001, 0x07B);
	flash_region_register("user2.bin",    0x081, 0x07B);
	flash_region_register("user-config",  0x100, 0x001);
	flash_region_register("PrivateKey",   0x101, 0x001);
	flash_region_register("Certificate",  0x102, 0x001);
	flash_region_register("Preferences",  0x103, 0x001);
	
	preferences_init();
	
	// UART Devices
#if DEVICE == PLUG
	user_plug_init();
#endif
#if DEVICE == SWITCH1
	user_switch1_init();
#endif
#if DEVICE == SWITCH2
	user_switch2_init();
#endif
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
#if DEVICE == DIMMER
	user_dimmer_init();
#endif
#if DEVICE == ROBKO
	user_robko_init();
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
