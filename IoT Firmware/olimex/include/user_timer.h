#ifndef __USER_TIMER_H__
	#define __USER_TIMER_H__
	
	#include "os_type.h"
	#include "queue.h"
	
	typedef struct _timer_ {
		os_timer_t       timer;
		os_timer_func_t *func;
		void            *param;
		
		STAILQ_ENTRY(_timer_) entries;
	} timer;
	
	timer *setTimeout (os_timer_func_t func, void *param, uint32 interval);
	timer *setInterval(os_timer_func_t func, void *param, uint32 interval);
	
	void clearTimeout (timer *t);
	void clearInterval(timer *t);
	
	uint32 timersCount();
#endif