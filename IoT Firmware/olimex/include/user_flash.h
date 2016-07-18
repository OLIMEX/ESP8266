#ifndef __USER_FLASH_H__
	#define __USER_FLASH_H__
	
	#define FLASH_SIMULATE          false
	#define FLASH_DEBUG             0

	
	#define FLASH_STREAM_MAX_META   50
	#define FLASH_STREAM_DELIMITER  ':'
	
	typedef struct _flash_region_ {
		char     name[FLASH_STREAM_MAX_META];
		uint16   start_sector;
		uint16   sectors;
		bool     protect;
		STAILQ_ENTRY(_flash_region_) entries;
	} flash_region;
	
	typedef enum  {
		FLASH_MODE_NONE = 0,
		FLASH_MODE_READ,
		FLASH_MODE_WRITE,
		FLASH_MODE_ERASE,
	} flash_region_mode;
	
	typedef enum  {
		FLASH_STREAM_ERROR = 0,
		FLASH_STREAM_NAME,
		FLASH_STREAM_SIZE,
		FLASH_STREAM_DATA,
		FLASH_STREAM_DONE,
	} flash_stream_states;
	
	char *flash_error();
	
	bool flash_region_register(char *name, uint16 start_sector, uint16 sectors);
	bool flash_region_protect(char *name, bool state);
	
	bool   flash_region_open(char *name);
	bool   flash_region_write(uint8 *data, uint16 data_len, bool raise_event);
	uint8 *flash_region_read();
	bool   flash_region_close();
	
	bool   flash_region_erase(char *name);
	
	bool flash_stream_init();
	bool flash_stream_data(uint8 *data, uint16 data_len, bool raise_event);
#endif