#include "user_config.h"
#if FONT_UTF8_ENABLE

#include "ets_sys.h"
#include "osapi.h"
#include "stdout.h"

#include "user_font.h"

uint16 ICACHE_FLASH_ATTR utf8_length(char *str) {
	uint16 i=0;
	uint16 len=0;
	while (str[i]) {
		if ((str[i] & 0xC0) != 0x80) {
			len++;
		}
		i++;
	}
	return len;
}

uint8 ICACHE_FLASH_ATTR utf8_char_len(char *str) {
	uint8  mask = 0xFE;
	uint8  bits = 0xFC;
	uint8  bytes = 6;
	
	if (*str <= 0x7F) {
		return 1;
	}
	
	while ((*str & mask) != bits) {
		mask = mask << 1;
		bits = bits << 1;
		bytes--;
		if (bytes < 2) {
			// Error
			return 0;
		}
	}
	
	return bytes;
}

char ICACHE_FLASH_ATTR *utf8_char_at(char *str, uint16 i) {
	uint16 c = 0;
	while (*str != 0 && c != i) {
		uint8 bytes = utf8_char_len(str);
		if (bytes == 0) {
			return NULL;
		}
		
		while (*str != 0 && bytes > 0) {
			str++;
			bytes--;
		}
		c++;
	}
	
	return str;
}

uint32 ICACHE_FLASH_ATTR utf8_decode_char(char *str) {
	uint8  mask = 0xFE;
	uint8  bits = 0xFC;
	uint8  bytes = 6;
	
	uint32 utf = *str;
	
	if (utf <= 0x7F) {
		// ASCII
		return utf;
	}
	
	while ((*str & mask) != bits) {
		mask = mask << 1;
		bits = bits << 1;
		bytes--;
		if (bytes < 2) {
			// Error
			return 0;
		}
	}
	
	utf = utf ^ bits;
	while (bytes > 1) {
		str++;
		if (*str == 0) {
			// Error
			return 0;
		}
		utf = (utf << 6) + (*str & 0x3F);
		bytes--;
	}
	
	return utf;
}

uint8 ICACHE_FLASH_ATTR utf8_font_rows() {
	return FONT_ROWS;
}

uint8 ICACHE_FLASH_ATTR utf8_font_columns() {
	return FONT_COLUMNS;
}

uint16 ICACHE_FLASH_ATTR utf8_columns_count(char *c) {
	return utf8_length(c) * (FONT_COLUMNS + 1);
}

uint8 ICACHE_FLASH_ATTR utf8_column(char *c, uint16 x) {
	if (x >= utf8_columns_count(c)) {
		// outside string
		return 0;
	}
	
	uint16 ci = x / (FONT_COLUMNS + 1);
	uint16 cc = x % (FONT_COLUMNS + 1);
	
	if (cc == FONT_COLUMNS) {
		// spacer column
		return 0;
	}
	
	c = utf8_char_at(c, ci);
	if (c == NULL) {
		return 0;
	}
	
	uint32 utf = utf8_decode_char(c);
	if (utf == 0) {
		return 0;
	}
	
	uint8 i;
	uint8 font = 0xFF;
	for (i=0; i<FONT_COUNT; i++) {
		if (
			utf >= FONT_DESCRIPTION[i].first && 
			utf <= FONT_DESCRIPTION[i].last
		) {
			font = i;
			break;
		}
	}
	
	if (font == 0xFF) {
		// char not in font range
		return 0;
	}
	
	if (font == LATIN) {
		return FONT_LATIN[utf - FONT_DESCRIPTION[font].first][cc];
	} else if (font == CYRILLIC) {
		return FONT_CYRILLIC[utf - FONT_DESCRIPTION[font].first][cc];
	} else if (font == LATIN_EXTRA) {
		return FONT_LATIN_EXTRA[utf - FONT_DESCRIPTION[font].first][cc];
	}
	
	return 0;
}

#endif