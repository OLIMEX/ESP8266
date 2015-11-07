#include "user_config.h"
#if DEVICE == BADGE

#include "ets_sys.h"
#include "osapi.h"
#include "stdout.h"
#include "gpio.h"
#include "mem.h"
#include "driver/spi_register.h"

#include "driver/key.h"
#include "json/jsonparse.h"

#include "user_json.h"
#include "user_webserver.h"
#include "user_badge.h"
#include "user_devices.h"

#include "user_utf8.h"
#include "user_battery.h"

#define SPI 			 0
#define HSPI			 1
#define BADGE_ANIMATION  100

LOCAL uint8 badge_screen[BADGE_BUFF_SIZE];
LOCAL uint8 badge_buffer[BADGE_BUFF_SIZE];

LOCAL bool  badge_battery_update = true;

LOCAL bool  wifi_animation = false;
const LOCAL uint8 badge_wifi_buff[BADGE_BUFF_SIZE] = {0x02, 0x09, 0x25, 0x95, 0x25, 0x09, 0x02};

LOCAL bool  server_animation = false;
const LOCAL uint8 badge_server_buff[BADGE_BUFF_SIZE] = {0x3E, 0x00, 0x18, 0x24, 0x18, 0x02, 0x3E, 0x02};

void ICACHE_FLASH_ATTR spi_badge_init() {
	// SPI Registers Initialization 
	WRITE_PERI_REG(PERIPHS_IO_MUX, 0x105); 
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, 2); // GPIO12 is HSPI MISO pin (Master Data In)
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, 2); // GPIO13 is HSPI MOSI pin (Master Data Out)
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, 2); // GPIO14 is HSPI CLK pin (Clock)
	
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5); // Custom GPIO5 as Latch

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
	
	// Set Latch low
	spi_badge_start();
}

void ICACHE_FLASH_ATTR spi_badge_start() {
	GPIO_OUTPUT_SET(5, 0);
}

void ICACHE_FLASH_ATTR spi_badge_stop() {
	GPIO_OUTPUT_SET(5, 1);
}

void ICACHE_FLASH_ATTR spi_badge_write(uint8 data) {
	while(READ_PERI_REG(SPI_CMD(HSPI))&SPI_USR);
	CLEAR_PERI_REG_MASK(SPI_USER(HSPI), SPI_USR_MOSI|SPI_USR_MISO);

	// SPI_FLASH_USER2 bit28-31 is cmd length,cmd bit length is value(0-15)+1,
	// bit15-0 is cmd value.
	WRITE_PERI_REG(
		SPI_USER2(HSPI), 
		((7 & SPI_USR_COMMAND_BITLEN) << SPI_USR_COMMAND_BITLEN_S) | ((uint32)data)
	);
	SET_PERI_REG_MASK(SPI_CMD(HSPI), SPI_USR);
	os_delay_us(SPI_BADGE_DATA_WAIT);
}  

LOCAL void ICACHE_FLASH_ATTR badge_refresh() {
LOCAL uint8 y=0;
	spi_badge_write(~(1 << y));
	spi_badge_write(badge_screen[y]);
	spi_badge_stop();
	spi_badge_start();
	
	y++;
	if (y == BADGE_BUFF_SIZE) {
		y = 0;
	}
}

void ICACHE_FLASH_ATTR badge_show() {
	// Send buffer
	os_memcpy(badge_screen, badge_buffer, BADGE_BUFF_SIZE);
}

void ICACHE_FLASH_ATTR badge_cls() {
	// Clear buffer
	os_memset(badge_buffer, 0, BADGE_BUFF_SIZE);
	badge_show();
}

bool ICACHE_FLASH_ATTR badge_set(uint8 x, uint8 y, uint8 l) {
	if (x >= 8) {
		return false;
	}
	
	if (y >= 8) {
		return false;
	}
	
	uint8 tx = 7 - y;
	uint8 ty = x;
	
	uint8 orMask  = 1 << tx;
	uint8 andMask = ~orMask;
	
	badge_buffer[ty] = (l != 0) ? 
		badge_buffer[ty] | orMask 
		: 
		badge_buffer[ty] & andMask
	;
	
	return true;
}

bool ICACHE_FLASH_ATTR badge_print(uint8 x, uint8 y, char *c) {
	uint8 col, j;
	uint16 i;
	uint16 size = utf8_columns_count(c);
	for (i=0; i<size; i++) {
		col = utf8_column(c, i);
		for (j=0; j<8; j++) {
			if (col & 0x80) {
				badge_set(x, y+j, 1);
			} else {
				badge_set(x, y+j, 0);
			}
			col = col << 1;
		}
		x++;
	}
	
	return true;
}

bool ICACHE_FLASH_ATTR badge_shift_left() {
	uint8 i;
	for (i=1; i<8; i++) {
		badge_buffer[i-1] = badge_buffer[i];
	}
	
	return true;
}

/* ================================================================================== */

LOCAL void badge_set_response(char *response, uint8 speed, char *text) {
	char data_str[WEBSERVER_MAX_VALUE * 2];
	char *escaped = json_escape_str(text, BADGE_MAX_TEXT);
	
	json_data(
		response, BADGE_STR, OK_STR,
		json_sprintf(
			data_str,
			"\"Speed\" : %d, "
			"\"Text\" : \"%s\"",
			speed,
			escaped
		),
		NULL
	);
	
	os_free(escaped);
}

LOCAL void ICACHE_FLASH_ATTR badge_scroll_done() {
	char status[WEBSERVER_MAX_VALUE];
	user_event_raise(BADGE_URL, json_status(status, BADGE_STR, DONE, NULL));
}

LOCAL void ICACHE_FLASH_ATTR badge_battery() {
	LOCAL char text[20];
	uint8 speed = 40;
	
	os_sprintf(text, "Battery: %d\%", battery_percent_get());
	badge_battery_update = badge_scroll(text, BADGE_MAX_SPEED - speed + 1, badge_scroll_done);
	
	if (badge_battery_update) {
		char response[WEBSERVER_MAX_VALUE];
		badge_set_response(response, speed, text);
		user_event_raise(BADGE_URL, response);
	}
}

LOCAL bool   scroll_in_progress = false;
LOCAL char  *scroll_c = NULL;
LOCAL uint16 scroll_i = 0;

LOCAL uint16 scroll_x = 0;
LOCAL uint16 scroll_y = 0;
LOCAL uint16 scroll_size = 0;
LOCAL uint8  scroll_delay = 0;
LOCAL badge_done_callback scroll_done = NULL;

void ICACHE_FLASH_ATTR _badge_scroll_() {
	uint8 col, j;
	
	badge_shift_left();
	
	col = utf8_column(scroll_c, scroll_i);
	for (j=0; j<8; j++) {
		if (col & 0x80) {
			badge_set(scroll_x, scroll_y + j, 1);
		} else {
			badge_set(scroll_x, scroll_y + j, 0);
		}
		col = col << 1;
	}
	
	badge_show();
	scroll_i++;
	
	if (scroll_i >= scroll_size + scroll_x) {
		scroll_in_progress = false;
		scroll_x = 0;
		scroll_y = 0;
		
		scroll_i = 0;
		scroll_size = 0;
		scroll_delay = 0;
		
		scroll_c = NULL;
		
		if (scroll_done) {
			(*scroll_done)();
		}
		scroll_done = NULL;
		
		if (!badge_battery_update) {
			badge_battery();
		}
	} else {
		setTimeout(_badge_scroll_, NULL, scroll_delay);
	}
}

bool ICACHE_FLASH_ATTR badge_scroll(char *c, uint8 delay, badge_done_callback done) {
	if (scroll_in_progress) {
		return false;
	}
	
	scroll_in_progress = true;
	
	scroll_x = 7;
	scroll_y = 0;
	
	scroll_i = 0;
	scroll_size = utf8_columns_count(c);
	scroll_delay = delay;
	
	scroll_c = c;
	
	scroll_done = done;
	
	badge_cls();
	setTimeout(_badge_scroll_, NULL, scroll_delay);
	
	return true;
}

LOCAL void ICACHE_FLASH_ATTR badge_wifi_animation() {
LOCAL uint32 timeout = 0;
	if (!wifi_animation) {
		badge_cls();
		return;
	}
	
	if (scroll_in_progress) {
		goto next;
	}
	
	LOCAL uint8 y = 0;
	uint8 i;
	uint8 mask = ~(0xFF >> y);
	
	os_memcpy(badge_buffer, badge_wifi_buff, BADGE_BUFF_SIZE);
	for (i=0; i<BADGE_BUFF_SIZE; i++) {
		badge_buffer[i] = badge_buffer[i] & mask;
	}
	badge_show();
	
	y++;
	if (y > 8) {
		y = 0;
	}
next:
	setTimeout(badge_wifi_animation, NULL, BADGE_ANIMATION);
}

void ICACHE_FLASH_ATTR badge_wifi_animation_start() {
	if (wifi_animation) {
		return;
	}
	wifi_animation = true;
	badge_wifi_animation();
}

void ICACHE_FLASH_ATTR badge_wifi_animation_stop() {
	wifi_animation = false;
}

LOCAL void ICACHE_FLASH_ATTR badge_server_animation() {
	if (!server_animation) {
		badge_cls();
		return;
	}
	
	if (wifi_animation || scroll_in_progress) {
		goto next;
	}
	
	LOCAL bool x = true;
	
	if (x) {
		os_memcpy(badge_screen, badge_server_buff, BADGE_BUFF_SIZE);
	} else {
		badge_cls();
	}
	x = x ? false : true;

next:
	setTimeout(badge_server_animation, NULL, BADGE_ANIMATION * 4);
}

void ICACHE_FLASH_ATTR badge_server_animation_start() {
	if (server_animation) {
		return;
	}
	server_animation = true;
	badge_server_animation();
}

void ICACHE_FLASH_ATTR badge_server_animation_stop() {
	server_animation = false;
}

LOCAL void ICACHE_FLASH_ATTR badge_test() {
	LOCAL uint8 i = 0;
	
	if (scroll_in_progress || wifi_animation || server_animation) {
		return;
	}
	
	badge_cls();
	
	uint8 x, y;
	if (i < 8) {
		y = i;
		for (x=0; x<8; x++) {
			badge_set(x, y, 1);
		}
	} else {
		x = i-8;
		for (y=0; y<8; y++) {
			badge_set(x, y, 1);
		}
	}
	badge_show();
	
	i++;
	if (i < 16) {
		setTimeout(badge_test, NULL, BADGE_ANIMATION);
	} else {
		setTimeout(badge_cls,  NULL, BADGE_ANIMATION);
		setTimeout(badge_wifi_animation_start, NULL, BADGE_ANIMATION*10);
	}
}

void ICACHE_FLASH_ATTR badge_handler(
	struct espconn *pConnection, 
	request_method method, 
	char *url, 
	char *data, 
	uint16 data_len, 
	uint32 content_len, 
	char *response,
	uint16 response_len
) {
	struct jsonparse_state parser;
	int type;
	
	LOCAL uint8  speed = 40;
	LOCAL char  *text = NULL;
	
	if (text == NULL) {
		text = (char *)os_zalloc(BADGE_MAX_TEXT);
	}
	
	if (method == POST && data != NULL && data_len != 0) {
		if (scroll_in_progress) {
			json_error(response, BADGE_STR, BUSY_STR, NULL);
			return;
		}
		
		jsonparse_setup(&parser, data, data_len);

		while ((type = jsonparse_next(&parser)) != 0) {
			if (type == JSON_TYPE_PAIR_NAME) {
				if (jsonparse_strcmp_value(&parser, "Speed") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					speed = jsonparse_get_value_as_int(&parser);
					if (speed > BADGE_MAX_SPEED) {
						speed = BADGE_MAX_SPEED;
					}
				} else if (jsonparse_strcmp_value(&parser, "Text") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					jsonparse_copy_value(&parser, text, BADGE_MAX_TEXT);
				}
			}
		}
		
		if (!badge_scroll(text, BADGE_MAX_SPEED - speed + 1, badge_scroll_done)) {
			json_error(response, BADGE_STR, BUSY_STR, NULL);
			return;
		}
	}
	
	badge_set_response(response, speed, text);
}

void ICACHE_FLASH_ATTR badge_init() {
	spi_badge_init();
	setInterval(badge_refresh, NULL, 1);
	setInterval(badge_battery, NULL, BADGE_BATTERY_UPDATE);
	
	badge_cls();
	badge_test();
	
	webserver_register_handler_callback(BADGE_URL, badge_handler);
	device_register(SPI, 0, BADGE_URL, NULL, NULL);
}

#endif
