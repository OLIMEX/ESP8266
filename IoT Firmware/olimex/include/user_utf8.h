#ifndef __USER_UTF8_H__
	#define __USER_UTF8_H__

	uint16 utf8_length(char *str);
	uint32 utf8_decode_char(char *str);
	char  *utf8_char_at(char *str, uint16 i);
#endif
