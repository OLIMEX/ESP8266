#ifndef __MOD_RFID_H__
	#define __MOD_RFID_H__
	
	#define RFID_BUFFER_LEN    50
	#define RFID_DEBUG         0
	
	typedef enum {
		RFID_ANY,
		RFID_NONE,
		MOD_RFID125,
		MOD_RFID1356,
	} rfid_module;
	
	typedef void (*rfid_tag_callback)(char *tag);
	
	const char *rfid_mod_str();
	rfid_module rfid_get_mod();
	
	void rfid_info();
	void rfid_init();
	
	void rfid_set_tag_callback(rfid_tag_callback func);
	void rfid_set(uint8 freq, uint8 led);
#endif