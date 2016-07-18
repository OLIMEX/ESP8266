#include "ets_sys.h"
#include "stdout.h"
#include "osapi.h"
#include "mem.h"

#include "json/jsonparse.h"

#include "user_json.h"
#include "user_interface.h"
#include "user_flash.h"
#include "user_preferences.h"

LOCAL char  *preferences_json     = NULL;
LOCAL uint32 preferences_json_len = 0;

LOCAL char PREFERENCES[] = "PREFERENCES";

LOCAL void ICACHE_FLASH_ATTR preferences_assing(char *data) {
	if (preferences_json) {
		os_free(preferences_json);
	}
	
	preferences_json_len = os_strlen(data);
	preferences_json = (char *)os_zalloc(preferences_json_len + 1);
	os_memcpy(preferences_json, data, preferences_json_len);
}

void ICACHE_FLASH_ATTR preferences_init() {
	flash_region_open("Preferences");
	uint8 *buff = flash_region_read();
	flash_region_close();
	
	if (os_strncmp(PREFERENCES, buff, os_strlen(PREFERENCES)) != 0) {
#if PREFERENCES_DEBUG
		debug("PREFERENCES: Not set\n");
#endif	
		return;
	}
	
	preferences_assing((char *)(buff + os_strlen(PREFERENCES)));
#if PREFERENCES_DEBUG
	debug("PREFERENCES: Loaded %s\n", preferences_json);
#endif
}

void ICACHE_FLASH_ATTR preferences_get(const char *name, preferences_callback callback) {
	struct jsonparse_state parser;
	int type;
	
	if (preferences_json == NULL || callback == NULL) {
		return;
	}
	
	jsonparse_setup(&parser, preferences_json, preferences_json_len);
	while ((type = jsonparse_next(&parser)) != 0) {
		if (type == JSON_TYPE_PAIR_NAME) {
			if (jsonparse_strcmp_value(&parser, name) == 0) {
				jsonparse_next(&parser);
				jsonparse_next(&parser);
				
				char *data = jsonparse_alloc_object_str(&parser);
				if (data) {
					(*callback)(data, os_strlen(data));
					os_free(data);
				}
				
				return;
			}
		}
	}
}

LOCAL void ICACHE_FLASH_ATTR preferences_add(char *buff, char *delimiter, const char *name, char *data, uint32 preferences_size) {
	uint32 buff_len = os_strlen(buff);
	uint32 delimiter_len = os_strlen(delimiter);
	uint32 name_len = os_strlen(name);
	uint32 data_len = os_strlen(data);
	
	if (buff_len + delimiter_len + name_len + 2 + data_len + 1 < preferences_size) {
		os_sprintf(buff + buff_len - 1, "%s\"%s\":%s}\0", delimiter, name, data);
	} else {
		debug("PREFERENCES: Exceeding max size\n");
	}
}

LOCAL void ICACHE_FLASH_ATTR preferences_store(char *buff) {
	flash_region_open("Preferences");
	flash_region_write(buff, os_strlen(buff), false);
	flash_region_close();
	
#if PREFERENCES_DEBUG
	debug("PREFERENCES: Stored %s\n", preferences_json);
#endif
}

void ICACHE_FLASH_ATTR preferences_reset() {
	preferences_store("PREFERENCES{}");
}

void ICACHE_FLASH_ATTR preferences_set(const char *name, char *data) {
	if (name == NULL || os_strlen(name) == 0) {
#if PREFERENCES_DEBUG
		debug("PREFERENCES: Empty name\n");
#endif
	}
	
	if (data == NULL || os_strlen(data) == 0) {
#if PREFERENCES_DEBUG
		debug("PREFERENCES: Nothing to set [%s]\n", name);
#endif
		return;
	}
	
	if (data[0] != '{' || data[os_strlen(data) - 1] != '}') {
#if PREFERENCES_DEBUG
		debug("PREFERENCES: JSON Object expected [%s]\n", name);
#endif
		return;
	}
	
	struct jsonparse_state parser;
	int type;
	
	char buff[PREFERENCES_MAX_DATA] = "PREFERENCES{}";
	char current_name[PREFERENCES_MAX_NAME] = "";
	char comma[] = ",";
	char empty[] = "";
	char *delimiter = empty;
	bool found = false;
	
	if (preferences_json) {
		jsonparse_setup(&parser, preferences_json, preferences_json_len);
		while ((type = jsonparse_next(&parser)) != 0) {
			if (type == JSON_TYPE_PAIR_NAME) {
				jsonparse_copy_value(&parser, current_name, PREFERENCES_MAX_NAME);
				
				jsonparse_next(&parser);
				jsonparse_next(&parser);
				
				char *current_data = jsonparse_alloc_object_str(&parser);
				
				if (os_strcmp(current_name, name) == 0) {
					preferences_add(buff, delimiter, current_name, data, PREFERENCES_MAX_DATA);
					found = true;
				} else {
					preferences_add(buff, delimiter, current_name, current_data, PREFERENCES_MAX_DATA);
				}
				
				os_free(current_data);
				delimiter = comma;
			}
		}
	}
	
	if (!found) {
		preferences_add(buff, delimiter, name, data, PREFERENCES_MAX_DATA);
	}
	
	preferences_assing(buff + os_strlen(PREFERENCES));
	preferences_store(buff);
}
