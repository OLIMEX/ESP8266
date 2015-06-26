#include "ets_sys.h"
#include "stdout.h"
#include "osapi.h"
#include "mem.h"
#include "spi_flash.h"
#include "queue.h"
#include "user_interface.h"

#include "user_flash.h"
#include "user_webserver.h"
#include "user_events.h"

LOCAL uint8  flash_buffer[SPI_FLASH_SEC_SIZE];
LOCAL uint16 flash_buf_size = SPI_FLASH_SEC_SIZE;
LOCAL uint16 flash_buf_pos = 0;

LOCAL flash_region *flash_open_region = NULL;

LOCAL uint16 flash_sector = 0;
LOCAL uint16 flash_max_sector = 0;
LOCAL uint16 flash_mode = 0;

STAILQ_HEAD(flash_regions, _flash_region_) flash_regions = STAILQ_HEAD_INITIALIZER(flash_regions);

LOCAL char *flash_error_str = NULL;
LOCAL void ICACHE_FLASH_ATTR flash_error_set(char *error) {
	if (flash_error_str != NULL) {
		os_free(flash_error_str);
	}
	
	flash_error_str = NULL;
	
	if (error != NULL) {
		flash_error_str = (char *)os_zalloc(os_strlen(error)+1);
		os_strcpy(flash_error_str, error);
	}
}

char ICACHE_FLASH_ATTR *flash_error() {
	return flash_error_str;
}

LOCAL flash_region ICACHE_FLASH_ATTR *flash_region_collision(uint16 start_sector, uint16 sectors) {
	flash_region *region;
	uint16 end_sector = start_sector + sectors - 1;
	
	STAILQ_FOREACH(region, &flash_regions, entries) {
		uint16 region_end_sector = region->start_sector + region->sectors - 1;
		if (start_sector > region_end_sector) {
			continue;
		}
		
		if (end_sector < region->start_sector) {
			continue;
		}
		
		return region;
	}
	
	return NULL;
}

LOCAL flash_region ICACHE_FLASH_ATTR *flash_find_region(char *name) {
	flash_region *region;
	
	STAILQ_FOREACH(region, &flash_regions, entries) {
		if (os_strcmp(region->name, name) == 0) {
			return region;
		}
	}
	
	return NULL;
}

bool ICACHE_FLASH_ATTR flash_region_register(char *name, uint16 start_sector, uint16 sectors) {
	flash_region *region;
	
	flash_error_set(NULL);
	
	region = flash_region_collision(start_sector, sectors);
	if (region != NULL) {
		flash_error_set("Region collision detected");
		debug("FLASH: %s [%s] [%s]\n", flash_error(), region->name, name);
		
		return false;
	}
	
	region = flash_find_region(name);
	if (region != NULL) {
		flash_error_set("Region already registered");
		debug("FLASH: %s [%s]\n", flash_error(), name);
		return false;
	}
	
	region = (flash_region *)os_zalloc(sizeof(flash_region));
	
	os_strcpy(region->name, name);
	region->start_sector = start_sector;
	region->sectors = sectors;
	
	STAILQ_INSERT_TAIL(&flash_regions, region, entries);
	
	return true;
}

bool ICACHE_FLASH_ATTR flash_region_protect(char *name, bool state) {
	flash_region *region;
	
	flash_error_set(NULL);
	
	region = flash_find_region(name);
	if (region == NULL) {
		flash_error_set("Region not found");
		debug("FLASH: %s [%s]\n", flash_error(), name);
		return false;
	}
	
	region->protect = state;
	return true;
}

LOCAL void ICACHE_FLASH_ATTR flash_init_buffer() {
	flash_buf_pos = 0;
	os_memset(flash_buffer, 0, flash_buf_size);
}

LOCAL bool ICACHE_FLASH_ATTR flash_flush_buffer(bool raise_event) {
	bool result = false;
	flash_error_set(NULL);
	
	if (flash_buf_pos != 0 && flash_mode == FLASH_MODE_WRITE) {
		if (flash_sector > flash_max_sector) {
			flash_error_set("Region limit exceeded");
			debug("FLASH: %s [%s]\n", flash_error(), flash_open_region->name);
			goto exit;
		}
		
		if (flash_sector == 0 && flash_buffer[0] == 0xE9) {
			// Preserve sector 0 flash settings
			uint8 read_buf[4];
			spi_flash_read(0, (uint32 *)&read_buf, sizeof(read_buf));
			flash_buffer[2] = read_buf[2];
			flash_buffer[3] = read_buf[3];
		}
		
		if (FLASH_SIMULATE || spi_flash_erase_sector(flash_sector) == SPI_FLASH_RESULT_OK) {
#if FLASH_DEBUG
			debug("FLASH: Erased sector [%s][0x%03x]\n", flash_open_region->name, flash_sector);
#endif
		} else {
			flash_error_set("Can not erase sector");
			debug("FLASH: %s [%s][0x%03x]\n", flash_error(), flash_open_region->name, flash_sector);
			goto exit;
		}
		
		if (FLASH_SIMULATE || spi_flash_write(flash_sector * SPI_FLASH_SEC_SIZE, (uint32 *)&flash_buffer, flash_buf_size) == SPI_FLASH_RESULT_OK) {
#if FLASH_DEBUG
			debug("FLASH: Sector write [%s][0x%03x]\n", flash_open_region->name, flash_sector);
#endif
		} else {
			flash_error_set("Can not write sector");
			debug("FLASH: %s [%s][0x%03x]\n", flash_error(), flash_open_region->name, flash_sector);
			goto exit;
		}
		
		if (raise_event) {
			uint8 progress = (flash_sector - flash_open_region->start_sector + 1) * 100 / flash_open_region->sectors;
			user_event_progress(progress);
		}
		
		flash_sector++;
	}
	
	result = true;
	
exit:
	flash_init_buffer();
	return result;
}

bool ICACHE_FLASH_ATTR flash_region_open(char *name) {
	flash_error_set(NULL);
	
	if (flash_open_region != NULL) {
		flash_error_set("Can not open two regions");
		debug("FLASH: Close region [%s] before open [%s]\n", flash_open_region->name, name);
		flash_mode = FLASH_MODE_NONE;
		flash_region_close();
		return false;
	}
	
	flash_open_region = flash_find_region(name);
	if (flash_open_region == NULL) {
		flash_error_set("Region not found");
		debug("FLASH: %s [%s]\n", flash_error(), name);
		return false;
	}
	
	if (flash_open_region->protect) {
		flash_open_region = NULL;
		flash_error_set("Region is protected");
		debug("FLASH: %s [%s]\n", flash_error(), name);
		return false;
	}
	
	flash_mode = FLASH_MODE_NONE;
	flash_sector = flash_open_region->start_sector;
	flash_max_sector = flash_open_region->start_sector + flash_open_region->sectors - 1;
	flash_init_buffer();
	
	return true;
}

uint8 ICACHE_FLASH_ATTR *flash_region_read() {
	flash_error_set(NULL);
	
	if (flash_open_region == NULL) {
		flash_error_set("No open region to read");
		debug("FLASH: %s\n", flash_error());
		return false;
	}
	
	if (flash_mode == FLASH_MODE_WRITE) {
		flash_error_set("Region is set as WRITE");
		debug("FLASH: %s [%s]\n", flash_error(), flash_open_region->name);
		return NULL;
	}
	
	if (flash_sector > flash_max_sector) {
		flash_error_set("Region limit exceeded");
		debug("FLASH: %s [%s]\n", flash_error(), flash_open_region->name);
		return NULL;
	}
	
	flash_mode = FLASH_MODE_READ;
	
	if (spi_flash_read(flash_sector * SPI_FLASH_SEC_SIZE, (uint32 *)&flash_buffer, flash_buf_size) == SPI_FLASH_RESULT_OK) {
		debug("FLASH: Sector read [%s][0x%03x]\n", flash_open_region->name, flash_sector);
	} else {
		flash_error_set("Can not read sector");
		debug("FLASH: %s [%s][0x%03x]\n", flash_error(), flash_open_region->name, flash_sector);
		return NULL;
	}
	
	flash_sector++;
	
	return flash_buffer;
}

bool ICACHE_FLASH_ATTR flash_region_write(uint8 *data, uint16 data_len, bool raise_event) {
	flash_error_set(NULL);
	
	if (flash_open_region == NULL) {
		flash_error_set("No open region to write");
		debug("FLASH: %s\n", flash_error());
		return false;
	}
	
	if (flash_mode == FLASH_MODE_READ) {
		flash_error_set("Region is set as READ");
		debug("FLASH: %s [%s]\n", flash_error(), flash_open_region->name);
		return false;
	}
	
	flash_mode = FLASH_MODE_WRITE;
	
	bool result = true;
	uint16 available = flash_buf_size - flash_buf_pos;
	uint16 copy_len = (available >= data_len) ? 
		data_len 
		:
		available
	;
	
	os_memcpy(flash_buffer + flash_buf_pos, data, copy_len);
	flash_buf_pos += copy_len;
	
	if (flash_buf_pos == flash_buf_size) {
		result = flash_flush_buffer(raise_event);
	}
	
	if (result && copy_len < data_len) {
		result = flash_region_write(data + copy_len, data_len - copy_len, raise_event);
	}
	
	return result;
}

LOCAL void ICACHE_FLASH_ATTR flash_region_clear() {
	flash_open_region = NULL;
	flash_sector = 0;
	flash_max_sector = 0;
}

LOCAL void ICACHE_FLASH_ATTR flash_region_truncate() {
	if (flash_mode == FLASH_MODE_WRITE) {
		bool result = true;
		while (flash_sector <= flash_max_sector && result) {
			result = flash_flush_buffer(false);
			flash_buf_pos = flash_buf_size;
			
			// Feed the watchdog to prevent reset
			slop_wdt_feed();
		}
	}
	
	flash_region_clear();
}

bool ICACHE_FLASH_ATTR flash_region_close() {
	bool result = true;
	
	if (flash_mode == FLASH_MODE_WRITE) {
		result = flash_flush_buffer(false);
		flash_buf_pos = flash_buf_size;
		flash_region_truncate();
		return result;
	}
	
	flash_region_clear();
	return result;
}

LOCAL flash_stream_states flash_stream_state = FLASH_STREAM_DONE;

LOCAL char   flash_stream_name[FLASH_STREAM_MAX_META];
LOCAL char   flash_stream_size_str[FLASH_STREAM_MAX_META];
LOCAL uint32 flash_stream_size = 0;
LOCAL uint32 flash_stream_pos = 0;
LOCAL uint32 flash_stream_global_pos = 0;

bool ICACHE_FLASH_ATTR flash_stream_init() {
	flash_error_set(NULL);
	if (flash_stream_state != FLASH_STREAM_DONE && flash_stream_state != FLASH_STREAM_ERROR) {
		flash_error_set("Can not open two streams");
		debug("FLASH STREAM: Close current stream [%s] before open another [%d]\n", flash_stream_name, flash_stream_state);
		flash_stream_state = FLASH_STREAM_ERROR;
		return false;
	}
	
	flash_stream_state = FLASH_STREAM_NAME;
	os_memset(flash_stream_name, '\0', FLASH_STREAM_MAX_META);
	os_memset(flash_stream_size_str, '\0', FLASH_STREAM_MAX_META);

	flash_stream_size = 0;
	flash_stream_pos = 0;
	flash_stream_global_pos = 0;
	
	return true;
}

LOCAL bool ICACHE_FLASH_ATTR flash_stream_meta(char *meta, uint8 *data, uint16 data_len) {
	uint8 meta_pos = os_strlen(meta);
	flash_stream_states initial_state = flash_stream_state;
	
	flash_error_set(NULL);
	
	while (flash_stream_state == initial_state) {
		if (data[flash_stream_pos] == FLASH_STREAM_DELIMITER) {
			flash_stream_state++;
		} else {
			meta[meta_pos] = data[flash_stream_pos];
			meta_pos++;
			if (meta_pos >= FLASH_STREAM_MAX_META) {
				flash_error_set("Meta value too long");
				debug("FLASH STREAM: %s [%s]\n", flash_error(), meta);
				flash_stream_state = FLASH_STREAM_ERROR;
				return false;
			}
		}
		
		flash_stream_pos++;
		if (flash_stream_pos >= data_len) {
			flash_stream_pos = 0;
			return true;
		}
	}
	
	return true;
}

bool ICACHE_FLASH_ATTR flash_stream_data(uint8 *data, uint16 data_len, bool raise_event) {
	if (flash_stream_state == FLASH_STREAM_ERROR) {
		debug("FLASH STREAM: Error [%s] [%s]\n", flash_stream_name, flash_error());
		return false;
	}
	
	if (flash_stream_state == FLASH_STREAM_NAME) {
		if (!flash_stream_meta(flash_stream_name, data, data_len)) {
			return false;
		}
		
		if (flash_stream_pos == 0) {
			return true;
		}
		
		if (flash_stream_state != FLASH_STREAM_NAME) {
			debug("FLASH STREAM: Name [%s]\n", flash_stream_name);
		}
	}
	
	if (flash_stream_state == FLASH_STREAM_SIZE) {
		if (!flash_stream_meta(flash_stream_size_str, data, data_len)) {
			return false;
		}
		
		if (flash_stream_state != FLASH_STREAM_SIZE) {
			flash_stream_size = strtoul(flash_stream_size_str, NULL, 10);
			debug("FLASH STREAM: Size [%d]\n", flash_stream_size);
		}
		
		if (!flash_region_open(flash_stream_name)) {
			flash_stream_state = FLASH_STREAM_ERROR;
			return false;
		}
		
		if (flash_stream_pos == 0) {
			return true;
		}
	}
	
	if (flash_stream_state == FLASH_STREAM_DATA) {
		data += flash_stream_pos;
		data_len -= flash_stream_pos;
		
		uint16 write_len = (flash_stream_global_pos + data_len >= flash_stream_size) ?
			flash_stream_size - flash_stream_global_pos
			:
			data_len
		;
		
		if (!flash_region_write(data, write_len, false)) {
			flash_stream_state = FLASH_STREAM_ERROR;
			return false;
		}
		
		flash_stream_global_pos += write_len;
		flash_stream_pos = write_len;
		
		if (flash_stream_global_pos >= flash_stream_size) {
			if (!flash_region_close()) {
				flash_stream_state = FLASH_STREAM_ERROR;
				return false;
			}
			
			flash_stream_state = FLASH_STREAM_DONE;
			
			if (data_len > flash_stream_pos) {
				data += flash_stream_pos;
				data_len -= flash_stream_pos;
				
				if (!flash_stream_init()) {
					flash_stream_state = FLASH_STREAM_ERROR;
					return false;
				}
				
				return flash_stream_data(data, data_len, raise_event);
			}
			
			flash_stream_pos = 0;
			flash_stream_global_pos = 0;
			return true;
		} else if (raise_event) {
			uint8 progress = flash_stream_global_pos * 100 / flash_stream_size;
			user_event_progress(progress);
		}
		
		if (flash_stream_pos < data_len) {
			debug("FLASH STREAM: Assert\n");
		}
		
		flash_stream_pos = 0;
		return true;
	}
	
	debug("FLASH STREAM: Invalid state [%s] [%d]\n", flash_stream_name, flash_stream_state);
	return false;
}
