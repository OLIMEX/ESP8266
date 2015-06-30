/*
 * 
 * arduino_flash.h is part of esp_arduino_style.
 *
 *  esp_arduino_style is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  esp_arduino_style is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with esp_arduino_style.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Created on: Mar 3, 2015
 *      Author: Stefan Mavrodiev
 *	   Company: Olimex LTD.
 *     Contact: support@olimex.com 
 */

#ifndef USER_ARDUINO_ARDUINO_FLASH_H_
#define USER_ARDUINO_ARDUINO_FLASH_H_

#include "c_types.h"

typedef struct
{
	void (*erase)(uint32 startAddress, uint32 endAddress);
	void (*write)(uint32 address, uint32 data);
	uint32 (*read)(uint32 address);
} flash_t;

void flash_erase(uint32 startAddress, uint32 endAddress);
uint32 flash_read(uint32 address);
void flash_write(uint32 address, uint32 data);

extern flash_t flash;
#endif /* USER_ARDUINO_ARDUINO_FLASH_H_ */
