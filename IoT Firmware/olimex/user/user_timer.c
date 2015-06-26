#include "ets_sys.h"
#include "osapi.h"
#include "mem.h"
#include "queue.h"

#include "user_timer.h"

STAILQ_HEAD(timers, _timer_) timers = STAILQ_HEAD_INITIALIZER(timers);

void ICACHE_FLASH_ATTR clearTimeout(timer *t) {
	timer *i;
	STAILQ_FOREACH(i, &timers, entries) {
		if (t == i) {
			os_timer_disarm(&(t->timer));
			STAILQ_REMOVE(&timers, t, _timer_, entries);
			os_free(t);
		}
	}
}

void ICACHE_FLASH_ATTR clearInterval(timer *t) __attribute__((alias("clearTimeout")));

LOCAL void ICACHE_FLASH_ATTR timer_handler(void *param) {
	timer *t = param;
	(*t->func)(t->param);
	clearTimeout(t);
}

LOCAL timer ICACHE_FLASH_ATTR *timer_init(os_timer_func_t func, void *param, uint32 interval, bool repeat) {
	timer *t = (timer *)os_zalloc(sizeof(timer));
	STAILQ_INSERT_TAIL(&timers, t, entries);
	
	t->func  = func;
	t->param = param;
	
	os_timer_disarm(&(t->timer));
	if (repeat) {
		os_timer_setfn(&(t->timer), func, param);
	} else {
		os_timer_setfn(&(t->timer), timer_handler, t);
	}
	os_timer_arm(&(t->timer), interval, repeat);
	
	return t;
}

timer ICACHE_FLASH_ATTR *setTimeout(os_timer_func_t func, void *param, uint32 interval) {
	return timer_init(func, param, interval, false);
}

timer ICACHE_FLASH_ATTR *setInterval(os_timer_func_t func, void *param, uint32 interval) {
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
