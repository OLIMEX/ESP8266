#ifndef __USER_PREFERENCES_H__
	#define __USER_PREFERENCES_H__
	
	#define PREFERENCES_DEBUG          1
	
	#define PREFERENCES_MAX_NAME      32
	#define PREFERENCES_MAX_DATA    1024

	typedef void (*preferences_callback)(char *data, uint16 data_len);
	
	void preferences_init();
	void preferences_reset();
	void preferences_get(const char *name, preferences_callback callback);
	void preferences_set(const char *name, char *data);
#endif