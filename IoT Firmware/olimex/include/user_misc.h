#ifndef __USER_MISC_H__
	#define __USER_MISC_H__
	
	#include "user_interface.h"
	
	#define MAX_STATUS_LEN   50
	
	typedef unsigned long int _uint64_;
	typedef signed long int _sint64_;
	
	uint32 pow_int(uint32 num, uint8 pow);
	
	bool  str_match(char *pattern, char * string);
	char *strncpy_null(char *dest, char *src, uint16 n);
	void *strstr_end(char *haystack, char *needle);
	char *itob(uint32 i, char *b, uint8 l);
	
	const char *wifi_auth_mode_str(AUTH_MODE mode);
	const char *wifi_op_mode_str(uint8 mode);
	const char *wifi_phy_mode_str(uint8 mode);
	
	/**************************************************************************
	 * SHA1 declarations 
	 **************************************************************************/

	#define SHA1_SIZE   20

	/*
	 *  This structure will hold context information for the SHA-1
	 *  hashing operation
	 */
	typedef struct {
		uint32_t Intermediate_Hash[SHA1_SIZE/4];  /* Message Digest */
		uint32_t Length_Low;                      /* Message length in bits */
		uint32_t Length_High;                     /* Message length in bits */
		uint16_t Message_Block_Index;             /* Index into message block array   */
		uint8_t Message_Block[64];                /* 512-bit message blocks */
	} SHA1_CTX;
	
	void sha1(char *msg, int len, char *digest);
	
	uint16 crc16(uint8 *data, uint16 len);
#endif