#include "osapi.h"
#include "os_type.h"
#include "ip_addr.h"

#include "user_misc.h"

/******************************************************************************
 * FunctionName : str_match
 * Description  : checks if two given strings match. The first string may contain wildcards
 * Parameters   : pattern
 *                string
 * Returns      : true if strings match
*******************************************************************************/
bool ICACHE_FLASH_ATTR str_match(char *pattern, char *string) {
	// If we reach at the end of both strings, we are done
	if (*pattern == '\0' && *string == '\0')
		return true;

	// Make sure that the characters after '*' are present in string string.
	// This function assumes that the pattern string will not contain two
	// consecutive '*' 
	if (*pattern == '*' && *(pattern+1) != '\0' && *string == '\0')
		return false;

	// If the pattern string contains '?', or current characters of both 
	// strings match
	if (*pattern == '?' || *pattern == *string)
		return str_match(pattern+1, string+1);

	// If there is *, then there are two possibilities
	// a) We consider current character of string string
	// b) We ignore current character of string string.
	if (*pattern == '*')
		return str_match(pattern+1, string) || str_match(pattern, string+1);
	return false;
}

/******************************************************************************
 * FunctionName : strncpy_null
 * Description  : copies up to n characters from the string pointed to by src to dest
 * Parameters   : dest
 *                src
 * Returns      : dest
*******************************************************************************/
char ICACHE_FLASH_ATTR *strncpy_null(char *dest, char *src, uint16 n) {
	os_strncpy(dest, src, n);
	*(dest+n) = '\0';
	return dest;
}

/******************************************************************************
 * FunctionName : strstr_end
 * Description  : finds the first occurrence of the substring needle in the string haystack
 * Parameters   : haystack
 *                needle
 * Returns      : pointer to the end of the first occurrence
*******************************************************************************/
void ICACHE_FLASH_ATTR *strstr_end(char *haystack, char *needle) {
	char *pStart;
	pStart = (char *)os_strstr(haystack, needle);
	if (pStart == NULL) {
		return NULL;
	}
	
	return pStart + os_strlen(needle);
}

/******************************************************************************
 * FunctionName : itob
 * Description  : convert up to 32 bit integer to string
 * Parameters   : integer to convert
 *                string
 *                string len
 * Returns      : pointer to the end of the first occurrence
*******************************************************************************/
char ICACHE_FLASH_ATTR *itob(uint32 i, char *b, uint8 l) {
	uint8 j=0;
	for (j=0; j < l; j++) {
		b[l-j-1] = (((i>>j) & 1) != 0 ? '1' : '0');
	}
	
	return b;
}

/******************************************************************************
 * FunctionName : ip4_addr_parse
 * Description  : parse IP v4 string address in HEX representation to uint32
 * Parameters   : addr
 * Returns      : uint32
*******************************************************************************/
uint32 ICACHE_FLASH_ATTR ip4_addr_parse(char *addr) {
	struct ip_addr result;
	uint8 a[4] = {0,0,0,0};
	uint8 i=0;
	
	while (*addr) {
		if (isdigit((unsigned char)*addr)) {
			a[i] *= 10;
			a[i] += *addr - '0';
		} else if (*addr == '.') {
			i++;
			if (i > 3) {
				return 0;
			}
		} else {
			return 0;
        }
        addr++;
	}
	
	IP4_ADDR(&result, a[0],a[1],a[2],a[3]);
	return result.addr;
}

/******************************************************************************
 * FunctionName : wifi_auth_mode_str
 * Description  : convert wifi authmode to string
 * Parameters   : mode
*******************************************************************************/
const char ICACHE_FLASH_ATTR *wifi_auth_mode_str(AUTH_MODE mode) {
	switch (mode) {
		case AUTH_OPEN         : return "Open";
		case AUTH_WEP          : return "WEP";
		case AUTH_WPA_PSK      : return "WPA PSK";
		case AUTH_WPA2_PSK     : return "WPA2 PSK";
		case AUTH_WPA_WPA2_PSK : return "WPA WPA2 PSK";
	}
	return "UNKNOWN";
}

/******************************************************************************
 * FunctionName : wifi_op_mode_str
 * Description  : convert wifi op_mode to string
 * Parameters   : mode
*******************************************************************************/
const char ICACHE_FLASH_ATTR *wifi_op_mode_str(uint8 mode) {
	switch (mode) {
		case STATION_MODE   : return "Station";
		case SOFTAP_MODE    : return "Access Point";
		case STATIONAP_MODE : return "Access Point and Station";
	}
}

/******************************************************************************
 * FunctionName : sha1
 * Description  : 
 * Parameters   : 
*******************************************************************************/
void ICACHE_FLASH_ATTR sha1(char *msg, int len, char *digest) {
	SHA1_CTX context;
	
	SHA1_Init(&context);
	SHA1_Update(&context, msg, len);
	SHA1_Final(digest, &context);
}

