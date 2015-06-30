#include "ets_sys.h"

static const char base64enc_tab[64]= "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static void ICACHE_FLASH_ATTR base64encode(const unsigned char in[3], unsigned char out[4], int count) {
	out[0]=base64enc_tab[(in[0]>>2)];
	out[1]=base64enc_tab[((in[0]&3)<<4)|(in[1]>>4)];
	out[2]=count<2 ? '=' : base64enc_tab[((in[1]&15)<<2)|(in[2]>>6)];
	out[3]=count<3 ? '=' : base64enc_tab[(in[2]&63)];
}

int ICACHE_FLASH_ATTR base64_encode(char *in, uint16 in_len, char *out, uint16 out_len) {
	unsigned ii, io;
	uint32 v;
	unsigned rem;
	for(io = 0, ii = 0, v = 0, rem = 0; ii < in_len; ii++) {
		unsigned char ch;
		ch = in[ii];
		v = (v<<8) | ch;
		rem += 8;
		while(rem >= 6) {
			rem -= 6;
			if(io >= out_len) {
				return -1; /* truncation is failure */
			}
			out[io++] = base64enc_tab[(v>>rem) & 63];
		}
	}
	
	if (rem) {
		v <<= (6-rem);
		if (io >= out_len) {
			return -1; /* truncation is failure */
		}
		out[io++] = base64enc_tab[v&63];
	}
	
	while (io&3) {
		if (io>=out_len) {
			return -1; /* truncation is failure */
		}
		out[io++] = '=';
	}
	
	if (io >= out_len) {
		return -1; /* no room for null terminator */
	}
	
	out[io] = 0;
	return io;
}
