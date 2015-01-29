#ifndef RELAY_H
#define RELAY_H

#include "httpd.h"

int cgiRelay(HttpdConnData *connData);
void ICACHE_FLASH_ATTR ioRelay(int value);

#endif
