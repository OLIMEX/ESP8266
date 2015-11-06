#include "ets_sys.h"
#include "osapi.h"
#include "stdout.h"

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
