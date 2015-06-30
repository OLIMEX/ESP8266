/*
 * 
 * arduino_serial.h is part of esp_arduino_style.
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

#ifndef USER_ARDUINO_ARDUINO_SERIAL_H_
#define USER_ARDUINO_ARDUINO_SERIAL_H_

#include "driver/uart.h"

typedef struct
{
	void (*begin)(UartBautRate baud);
	void (*println)(char *buffer);
	void (*print)(char *buffer);
	void (*write)(char c);
	int  (*read)(void);
	int  (*available)(void);
} serial_t;


void uart0_init(UartBautRate baud);
void uart_print(char *buffer);
void uart_println(char *buffer);
void uart_write(char c);
int uart_available(void);
int uart_read(void);

extern serial_t Serial;


#endif /* USER_ARDUINO_ARDUINO_SERIAL_H_ */
