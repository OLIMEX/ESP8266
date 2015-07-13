#ifndef __MOD_LED_8x8_RGB_H__
	#define __MOD_LED_8x8_RGB_H__
	
	#define SPI_LED_8x8_DATA_WAIT      50
	
	// One module requires 8x8x3 bits = 24 bytes buffer
	#define LED_8x8_BUFF_SIZE          24
	#define LED_8x8_MAX_MODULES        48
	
	typedef void (*led_8x8_rgb_done_callback)();
	
	void spi_led_8x8_rgb_init();
	
	void spi_led_8x8_rgb_start();
	void spi_led_8x8_rgb_write(uint8 data);
	void spi_led_8x8_rgb_stop();

	bool   led_8x8_rgb_set_dimensions(uint8 cols, uint8 rows);
	uint8  led_8x8_rgb_get_rows();
	uint8  led_8x8_rgb_get_cols();
	
	void   led_8x8_rgb_show();
	void   led_8x8_rgb_cls();
	
	bool   led_8x8_rgb_set(uint16 x, uint16 y, uint8 r, uint8 g, uint8 b);
	
	uint16 led_8x8_rgb_size_x(char *c);
	bool   led_8x8_rgb_print(uint16 x, uint16 y, uint8 r, uint8 g, uint8 b, char *c);
	bool   led_8x8_rgb_scroll(uint8 r, uint8 g, uint8 b, char *c, uint8 speed, led_8x8_rgb_done_callback done);
#endif