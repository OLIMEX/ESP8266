#include "ets_sys.h"
#include "osapi.h"
#include "mem.h"
#include "queue.h"
#include "stdout.h"

#include "user_timer.h"
#include "modules/mod_led_8x8_rgb.h"

STAILQ_HEAD(timers, _timer_) timers = STAILQ_HEAD_INITIALIZER(timers);

LOCAL uint32 timer_last_handle = 1;

void ICACHE_FLASH_ATTR clearTimeout(uint32 handle) {
	timer *t;
	STAILQ_FOREACH(t, &timers, entries) {
		if (t->handle == handle) {
			os_timer_disarm(&(t->timer));
			STAILQ_REMOVE(&timers, t, _timer_, entries);
			os_free(t);
		}
	}
}

void ICACHE_FLASH_ATTR clearInterval(uint32 handle) __attribute__((alias("clearTimeout")));

void ICACHE_FLASH_ATTR clearAllTimers() {
	timer *t;
	
	STAILQ_FOREACH(t, &timers, entries) {
		os_timer_disarm(&(t->timer));
		STAILQ_REMOVE(&timers, t, _timer_, entries);
		os_free(t);
	}
}

LOCAL void ICACHE_FLASH_ATTR timer_handler(void *param) {
	timer *t = param;
	(*t->func)(t->param);
	clearTimeout(t->handle);
}

LOCAL uint32 ICACHE_FLASH_ATTR timer_init(os_timer_func_t func, void *param, uint32 interval, bool repeat) {
	timer *t = (timer *)os_zalloc(sizeof(timer));
	STAILQ_INSERT_TAIL(&timers, t, entries);
	
	t->handle = timer_last_handle++;
	t->func   = func;
	t->param  = param;
	
	if (timer_last_handle == 0) {
		timer_last_handle = 1;
	}
	
	os_timer_disarm(&(t->timer));
	if (repeat) {
		os_timer_setfn(&(t->timer), func, param);
	} else {
		os_timer_setfn(&(t->timer), timer_handler, t);
	}
	os_timer_arm(&(t->timer), interval, repeat);
	
	return t->handle;
}

uint32 ICACHE_FLASH_ATTR setTimeout(os_timer_func_t func, void *param, uint32 interval) {
	return timer_init(func, param, interval, false);
}

uint32 ICACHE_FLASH_ATTR setInterval(os_timer_func_t func, void *param, uint32 interval) {
	return timer_init(func, param, interval, true);
}

uint32 ICACHE_FLASH_ATTR timersCount() {
	uint32 count = 0;
	timer *t;
	
	STAILQ_FOREACH(t, &timers, entries) {
		count++;
	}
	
	return count;
}

void ICACHE_FLASH_ATTR timers_info() {
	debug("Active timers: %d\n", timersCount());
	debug("   Last timer handle: %d\n", timer_last_handle);
}
