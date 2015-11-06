#ifndef __USER_UTF8_H__
	#define __USER_UTF8_H__

	#include "user_config.h"
	#if FONT_UTF8_ENABLE
		uint16 utf8_length(char *str);
		uint32 utf8_decode_char(char *str);
		char  *utf8_char_at(char *str, uint16 i);
		
		uint8  utf8_font_rows();
		uint8  utf8_font_columns();
		
		uint16 utf8_columns_count(char *c);
		uint8  utf8_column(char *c, uint16 x);
	#endif
#endif
