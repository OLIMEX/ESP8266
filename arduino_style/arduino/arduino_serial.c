/*
 * 
 * arduino_serial.c is part of esp_arduino_style.
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

#include "ets_sys.h"
#include "osapi.h"

#include "driver/uart.h"
#include "arduino_serial.h"

serial_t Serial =
{
		.begin = uart0_init,
		.print = uart_print,
		.println = uart_println,
		.write = uart_write,
};

/**
 * Send on char
 *
 * @param c Char to be printed
 * @return None
 */
static void ICACHE_FLASH_ATTR uart_txd(char c)
{
	while (((READ_PERI_REG(UART_STATUS(0)) >> UART_TXFIFO_CNT_S)
			& UART_TXFIFO_CNT) >= 126);
	WRITE_PERI_REG(UART_FIFO(0), c);
}

/**
 * Append CR to LF.
 *
 * @param c Char to be printed.
 * @return None.
 */
static void ICACHE_FLASH_ATTR uart_put_char(char c)
{

	if (c == '\n')
	{
		uart_txd('\r');
	}

	uart_txd(c);
}


/**
 * Initialize baud rate for the serial communication.
 *
 * @param baud Desired baud rate
 * @see UartBautRate
 * @return None
 */
void uart0_init(UartBautRate baud)
{

	if (baud != BIT_RATE_9600 && baud != BIT_RATE_19200
			&& baud != BIT_RATE_38400 && baud != BIT_RATE_57600
			&& baud != BIT_RATE_74880 && baud != BIT_RATE_115200
			&& baud != BIT_RATE_230400 && baud != BIT_RATE_460800
			&& baud != BIT_RATE_921600)
		return;

	//Enable TxD pin
	PIN_PULLUP_DIS(PERIPHS_IO_MUX_U0TXD_U);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0TXD_U, FUNC_U0TXD);

	//Set baud rate and other serial parameters to 115200,n,8,1
	uart_div_modify(0, UART_CLK_FREQ / baud);
	WRITE_PERI_REG(UART_CONF0(0),
			(STICK_PARITY_DIS) | (ONE_STOP_BIT << UART_STOP_BIT_NUM_S) | (EIGHT_BITS << UART_BIT_NUM_S));

	//Reset tx & rx fifo
	SET_PERI_REG_MASK(UART_CONF0(0), UART_RXFIFO_RST | UART_TXFIFO_RST);
	CLEAR_PERI_REG_MASK(UART_CONF0(0), UART_RXFIFO_RST | UART_TXFIFO_RST);
	//Clear pending interrupts
	WRITE_PERI_REG(UART_INT_CLR(0), 0xffff);

	//Install our own putchar handler
	os_install_putc1((void *) uart_put_char);
}

/**
 * Print line to serial output with CR/LF.
 *
 * @param buffer Data to be displayed
 * @return None
 */
void uart_println(char *buffer)
{
	os_printf("%s\n", buffer);
}

/**
 * Print line to serial output without CR/LF.
 *
 * @param buffer Data to be displayed
 * @return None
 */
void uart_print(char *buffer)
{
	os_printf("%s", buffer);
}

/**
 * Print single character
 *
 * @param c Char to write
 * @return None
 */
void uart_write(char c)
{
	os_printf("%c", c);
}
