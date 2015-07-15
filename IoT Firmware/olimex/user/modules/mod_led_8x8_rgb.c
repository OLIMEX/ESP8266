#include "ets_sys.h"
#include "osapi.h"
#include "mem.h"
#include "gpio.h"
#include "stdout.h"

#include "driver/spi_register.h"

#include "user_font.h"
#include "mod_led_8x8_rgb.h"

#define SPI 			0
#define HSPI			1

// Here we allocate buffer for up to 48 linked modules (8x6)
LOCAL uint8 *led_8x8_rgb_buffer = NULL;

// How many modules are connected by rows and columns
LOCAL uint8 led_8x8_rgb_rows = 1;
LOCAL uint8 led_8x8_rgb_cols = 1;

void ICACHE_FLASH_ATTR spi_led_8x8_rgb_init() {
	// SPI Registers Initialization 
	WRITE_PERI_REG(PERIPHS_IO_MUX, 0x105); 
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, 2); // GPIO12 is HSPI MISO pin (Master Data In)
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, 2); // GPIO13 is HSPI MOSI pin (Master Data Out)
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, 2); // GPIO14 is HSPI CLK pin (Clock)
	
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_GPIO15); // Custom GPIO15 as CS pin (Chip Select / Slave Select)

	SET_PERI_REG_MASK(SPI_USER(HSPI), SPI_CS_SETUP | SPI_CS_HOLD | SPI_USR_COMMAND | SPI_USR_MOSI);
	CLEAR_PERI_REG_MASK(SPI_USER(HSPI), SPI_FLASH_MODE);

	//clear Daul or Quad lines transmission mode
	CLEAR_PERI_REG_MASK(SPI_CTRL(HSPI), SPI_QIO_MODE | SPI_DIO_MODE | SPI_DOUT_MODE | SPI_QOUT_MODE);
	
	WRITE_PERI_REG(
		SPI_CLOCK(HSPI), 
		((31 & SPI_CLKDIV_PRE) << SPI_CLKDIV_PRE_S) |
		((9  & SPI_CLKCNT_N  ) << SPI_CLKCNT_N_S) |
		((1  & SPI_CLKCNT_H  ) << SPI_CLKCNT_H_S) |
		((6  & SPI_CLKCNT_L  ) << SPI_CLKCNT_L_S)
	); // clear bit 31, set SPI clock

	// set 8bit output buffer length, the buffer is the low 8bit of register "SPI_FLASH_C0"
	WRITE_PERI_REG(
		SPI_USER1(HSPI), 
		((7 & SPI_USR_MOSI_BITLEN) << SPI_USR_MOSI_BITLEN_S) |
		((7 & SPI_USR_MISO_BITLEN) << SPI_USR_MISO_BITLEN_S)
	);
	
	// Set CS high
	spi_led_8x8_rgb_stop();
}

void ICACHE_FLASH_ATTR spi_led_8x8_rgb_stop() {
	os_delay_us(SPI_LED_8x8_DATA_WAIT);
	GPIO_OUTPUT_SET(15, 1);
}

void ICACHE_FLASH_ATTR spi_led_8x8_rgb_start() {
	GPIO_OUTPUT_SET(15, 0);
	os_delay_us(SPI_LED_8x8_DATA_WAIT);
}

void ICACHE_FLASH_ATTR spi_led_8x8_rgb_write(uint8 data) {
	while(READ_PERI_REG(SPI_CMD(HSPI))&SPI_USR);
	CLEAR_PERI_REG_MASK(SPI_USER(HSPI), SPI_USR_MOSI|SPI_USR_MISO);

	// SPI_FLASH_USER2 bit28-31 is cmd length,cmd bit length is value(0-15)+1,
	// bit15-0 is cmd value.
	WRITE_PERI_REG(
		SPI_USER2(HSPI), 
		((7 & SPI_USR_COMMAND_BITLEN) << SPI_USR_COMMAND_BITLEN_S) | ((uint32)data)
	);
	SET_PERI_REG_MASK(SPI_CMD(HSPI), SPI_USR);
	os_delay_us(SPI_LED_8x8_DATA_WAIT);
}  

bool ICACHE_FLASH_ATTR led_8x8_rgb_set_dimensions(uint8 cols, uint8 rows) {
	if (led_8x8_rgb_buffer != NULL && cols == led_8x8_rgb_cols && rows == led_8x8_rgb_rows) {
		return true;
	}
	
	if (rows * cols > LED_8x8_MAX_MODULES) {
		debug("MOD-LED-8x8-RGB: Exceed maximum modules count [%d x %d = %d]\n", cols, rows, cols * rows);
		return false;
	}
	
	if (led_8x8_rgb_buffer)	os_free(led_8x8_rgb_buffer);
	
	led_8x8_rgb_cols = cols;
	led_8x8_rgb_rows = rows;
	
	led_8x8_rgb_buffer = (uint8 *)os_zalloc(led_8x8_rgb_rows * led_8x8_rgb_cols * LED_8x8_BUFF_SIZE);
	return led_8x8_rgb_buffer != NULL;
}

uint8 ICACHE_FLASH_ATTR led_8x8_rgb_get_rows() {
	return led_8x8_rgb_rows;
}

uint8 ICACHE_FLASH_ATTR led_8x8_rgb_get_cols() {
	return led_8x8_rgb_cols;
}

void ICACHE_FLASH_ATTR led_8x8_rgb_show() {
	if (led_8x8_rgb_buffer == NULL) {
		return;
	}
	uint8 cnt = led_8x8_rgb_rows * led_8x8_rgb_cols;
	
	uint8 i;
	// Send buffer
	spi_led_8x8_rgb_start();
	for (i = 0; i<cnt; i++) {
		uint16 b = (cnt-i-1) * LED_8x8_BUFF_SIZE;
		uint8 o;
		for (o=0; o < LED_8x8_BUFF_SIZE; o++) {
			spi_led_8x8_rgb_write(led_8x8_rgb_buffer[b+o]);
		}
	}
	spi_led_8x8_rgb_stop();
}

void ICACHE_FLASH_ATTR led_8x8_rgb_cls() {
	if (led_8x8_rgb_buffer == NULL) {
		return;
	}
	
	// Clear buffer
	os_memset(led_8x8_rgb_buffer, 0, led_8x8_rgb_rows * led_8x8_rgb_cols * LED_8x8_BUFF_SIZE);
	led_8x8_rgb_show();
}

bool ICACHE_FLASH_ATTR led_8x8_rgb_set(uint16 x, uint16 y, uint8 r, uint8 g, uint8 b) {
	if (led_8x8_rgb_buffer == NULL) {
		return false;
	}
	
	uint8 mX = x / 8; // module X
	uint8 oX = x % 8; // X offset in module
	if (mX >= led_8x8_rgb_cols) {
		return false;
	}
	
	uint8 mY = y / 8; // module Y
	uint8 oY = y % 8; // Y offset in module
	if (mY >= led_8x8_rgb_rows) {
		return false;
	}
	
	uint8 m = mX + mY * led_8x8_rgb_cols; // Linear module
	uint16 mO = m * LED_8x8_BUFF_SIZE;    // Linear byte offset
	
	uint8 rO = oY * 3;
	uint8 gO = oY * 3 + 1;
	uint8 bO = oY * 3 + 2;
	
	uint8 orMask  = 1 << oX;
	uint8 andMask = ~orMask;
	
	led_8x8_rgb_buffer[mO + rO] = (r != 0) ? led_8x8_rgb_buffer[mO + rO] | orMask : led_8x8_rgb_buffer[mO + rO] & andMask;
	led_8x8_rgb_buffer[mO + gO] = (g != 0) ? led_8x8_rgb_buffer[mO + gO] | orMask : led_8x8_rgb_buffer[mO + gO] & andMask;
	led_8x8_rgb_buffer[mO + bO] = (b != 0) ? led_8x8_rgb_buffer[mO + bO] | orMask : led_8x8_rgb_buffer[mO + bO] & andMask;
	
	return true;
}

uint16 ICACHE_FLASH_ATTR led_8x8_rgb_size_x(char *c) {
	return os_strlen(c) * (FONT_COLUMNS + 1);
}

uint8 ICACHE_FLASH_ATTR led_8x8_rgb_column(char *c, uint16 x) {
	if (x >= led_8x8_rgb_size_x(c)) {
		// outside string
		return 0;
	}
	
	uint16 ci = x / (FONT_COLUMNS + 1);
	uint16 cc = x % (FONT_COLUMNS + 1);
	
	c += ci;
	if (*c < 0x20 || *c > 0x7F) {
		// invalid char
		return 0;
	}
	
	if (cc == FONT_COLUMNS) {
		// spacer column
		return 0;
	}
	
	return FONT[*c - 0x20][cc];
}

bool ICACHE_FLASH_ATTR led_8x8_rgb_print(uint16 x, uint16 y, uint8 r, uint8 g, uint8 b, char *c) {
	if (led_8x8_rgb_buffer == NULL) {
		return false;
	}
	
	uint8 col, j;
	uint16 i;
	uint16 size = led_8x8_rgb_size_x(c);
	for (i=0; i<size; i++) {
		col = led_8x8_rgb_column(c, i);
		for (j=0; j<8; j++) {
			if (col & 0x80) {
				led_8x8_rgb_set(x, y+j, r, g, b);
			} else {
				led_8x8_rgb_set(x, y+j, 0, 0, 0);
			}
			col = col << 1;
		}
		x++;
	}
	
	return true;
}

LOCAL bool ICACHE_FLASH_ATTR led_8x8_rgb_shift_left_mod(uint8 row, uint8 col, uint8 *r, uint8 *g, uint8 *b) {
	if (led_8x8_rgb_buffer == NULL) {
		return false;
	}
	
	uint8 nR, nG, nB;
	
	uint8 m   = col + row * led_8x8_rgb_cols; // Linear module
	uint16 mO = m * LED_8x8_BUFF_SIZE;        // Linear byte offset
	
	nR = 0;
	nG = 0;
	nB = 0;
	
	uint8 i;
	for (i=0; i<8; i++) {
		uint8 oR = i*3;
		uint8 oG = i*3 + 1;
		uint8 oB = i*3 + 2;
		
		nR |= ((led_8x8_rgb_buffer[mO + oR] & 0x01) != 0);
		nG |= ((led_8x8_rgb_buffer[mO + oG] & 0x01) != 0);
		nB |= ((led_8x8_rgb_buffer[mO + oB] & 0x01) != 0);
		
		led_8x8_rgb_buffer[mO + oR] = (led_8x8_rgb_buffer[mO + oR] >> 1) | (((*r & 0x80) != 0) << 7);
		led_8x8_rgb_buffer[mO + oG] = (led_8x8_rgb_buffer[mO + oG] >> 1) | (((*g & 0x80) != 0) << 7);
		led_8x8_rgb_buffer[mO + oB] = (led_8x8_rgb_buffer[mO + oB] >> 1) | (((*b & 0x80) != 0) << 7);
		
		*r = *r << 1;
		*g = *g << 1;
		*b = *b << 1;
		
		if (i != 7) {
			nR = nR << 1;
			nG = nG << 1;
			nB = nB << 1;
		}
	}
	
	*r = nR;
	*g = nG;
	*b = nB;
	
	return true;
}

bool ICACHE_FLASH_ATTR led_8x8_rgb_shift_left() {
	uint8 col, row, r, g, b;
	for (row=0; row<led_8x8_rgb_rows; row++) {
		r = 0;
		g = 0;
		b = 0;
		for (col=0; col<led_8x8_rgb_cols; col++) {
			led_8x8_rgb_shift_left_mod(row, led_8x8_rgb_cols - col - 1, &r, &g, &b);
		}
	}
}

LOCAL uint16 scroll_in_progress = false;
LOCAL char  *scroll_c = NULL;
LOCAL uint16 scroll_i = 0;

LOCAL uint8  scroll_r = 0;
LOCAL uint8  scroll_g = 0;
LOCAL uint8  scroll_b = 0;

LOCAL uint16 scroll_x = 0;
LOCAL uint16 scroll_y = 0;
LOCAL uint16 scroll_size = 0;
LOCAL uint8  scroll_delay = 0;
LOCAL led_8x8_rgb_done_callback scroll_done = NULL;

void ICACHE_FLASH_ATTR _led_8x8_rgb_scroll_() {
	uint8 col, j;
	
	led_8x8_rgb_shift_left();
	
	col = led_8x8_rgb_column(scroll_c, scroll_i);
	for (j=0; j<8; j++) {
		if (col & 0x80) {
			led_8x8_rgb_set(scroll_x, scroll_y + j, scroll_r, scroll_g, scroll_b);
		} else {
			led_8x8_rgb_set(scroll_x, scroll_y + j, 0, 0, 0);
		}
		col = col << 1;
	}
	
	led_8x8_rgb_show();
	scroll_i++;
	
	if (scroll_i >= scroll_size + scroll_x) {
		scroll_in_progress = false;
		scroll_x = 0;
		scroll_y = 0;
		
		scroll_i = 0;
		scroll_size = 0;
		scroll_delay = 0;
		
		scroll_r = 0;
		scroll_g = 0;
		scroll_b = 0;
		scroll_c = NULL;
		
		if (scroll_done) {
			(*scroll_done)();
		}
		scroll_done = NULL;
	} else {
		setTimeout(_led_8x8_rgb_scroll_, NULL, scroll_delay);
	}
}

bool ICACHE_FLASH_ATTR led_8x8_rgb_scroll(uint8 r, uint8 g, uint8 b, char *c, uint8 delay, led_8x8_rgb_done_callback done) {
	if (scroll_in_progress) {
		return false;
	}
	
	scroll_in_progress = true;
	
	scroll_x = led_8x8_rgb_cols * 8 - 1;
	scroll_y = led_8x8_rgb_rows * 8 / 2 - FONT_ROWS / 2;
	
	scroll_i = 0;
	scroll_size = led_8x8_rgb_size_x(c);
	scroll_delay = delay;

	scroll_r = r;
	scroll_g = g;
	scroll_b = b;
	scroll_c = c;
	
	scroll_done = done;
	
	setTimeout(_led_8x8_rgb_scroll_, NULL, scroll_delay);
	
	return true;
}
