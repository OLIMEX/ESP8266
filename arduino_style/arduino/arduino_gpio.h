/*
 * 
 * arduino_gpio.h is part of esp_arduino_style.
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
 *  Created on: Mar 2, 2015
 *      Author: Stefan Mavrodiev
 *	   Company: Olimex LTD.
 *     Contact: support@olimex.com 
 */

#ifndef USER_ARDUINO_ARDUINO_GPIO_H_
#define USER_ARDUINO_ARDUINO_GPIO_H_

#include "c_types.h"

/*
 * Define GPIO related registers.
 */
#define PERIPHS_GPIO_BASEADDR	0x60000300

#define GPIO_OUT			(PERIPHS_GPIO_BASEADDR + 0x00)
#define GPIO_OUT_W1TS		(PERIPHS_GPIO_BASEADDR + 0x04)
#define GPIO_OUT_W1TC		(PERIPHS_GPIO_BASEADDR + 0x08)

#define GPIO_ENABLE			(PERIPHS_GPIO_BASEADDR + 0x0C)
#define GPIO_ENABLE_W1TS	(PERIPHS_GPIO_BASEADDR + 0x10)
#define GPIO_ENABLE_W1TC	(PERIPHS_GPIO_BASEADDR + 0x14)

#define GPIO_IN				(PERIPHS_GPIO_BASEADDR + 0x18)

/*
 * Define structure that describes register -> function
 * relation.
 */
typedef struct {
	uint32 reg;
	uint8 gpio_func;
} gpio_reg_t;

/*
 * Define possible pin modes.
 */
typedef enum {
	INPUT,
	OUTPUT,
	INPUT_PULLUP
} pinmode_t;

/*
 * Define output levels.
 */
typedef enum {
	LOW,
	HIGH
} pinvalue_t;

extern void pinMode(uint8 pin, pinmode_t mode);
extern void digitalWrite(uint8 pin, pinvalue_t value);
extern uint8 digitalRead(uint8 pin);

#endif /* USER_ARDUINO_ARDUINO_GPIO_H_ */
