#ifndef __USER_TIMER_H__
	#define __USER_TIMER_H__
	
	#include "os_type.h"
	#include "queue.h"
	
	typedef struct _timer_ {
		uint32           handle;
		
		os_timer_t       timer;
		os_timer_func_t *func;
		void            *param;
		
		STAILQ_ENTRY(_timer_) entries;
	} timer;
	
	uint32 setTimeout (os_timer_func_t func, void *param, uint32 interval);
	uint32 setInterval(os_timer_func_t func, void *param, uint32 interval);
	
	void clearTimeout (uint32 handle);
	void clearInterval(uint32 handle);
	
	void clearAllTimers();
	
	uint32 timersCount();
#endif