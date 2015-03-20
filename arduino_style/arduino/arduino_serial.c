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
#include "eagle_soc.h"
#include "ets_sys.h"
#include "osapi.h"
#include "driver/uart.h"
#include "arduino_serial.h"

#define UART0   0

// UartDev is defined and initialized in rom code.
extern UartDevice UartDev;

LOCAL void uart0_rx_intr_handler(void *para);

serial_t Serial =
{
		.begin = uart0_init,
		.print = uart_print,
		.println = uart_println,
		.write = uart_write,
		.available = uart_available,
		.read = uart_read
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

	ETS_UART_INTR_DISABLE();

	// Attach RX interrupt handler. Rcv_buff size if 0x100
	ETS_UART_INTR_ATTACH(uart0_rx_intr_handler,  &(UartDev.rcv_buff));
	//Enable TxD pin
	PIN_PULLUP_DIS(PERIPHS_IO_MUX_U0TXD_U);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0TXD_U, FUNC_U0TXD);
	//Enable RxD pin
	PIN_PULLUP_EN(PERIPHS_IO_MUX_U0RXD_U);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0RXD_U, FUNC_U0RXD);

	//Set baud rate and other serial parameters to 115200,n,8,1
	uart_div_modify(UART0, UART_CLK_FREQ / baud);
	WRITE_PERI_REG(UART_CONF0(UART0),
			(STICK_PARITY_DIS) | (ONE_STOP_BIT << UART_STOP_BIT_NUM_S) | (EIGHT_BITS << UART_BIT_NUM_S));

	//Reset tx & rx fifo
	SET_PERI_REG_MASK(UART_CONF0(UART0), UART_RXFIFO_RST | UART_TXFIFO_RST);
	CLEAR_PERI_REG_MASK(UART_CONF0(UART0), UART_RXFIFO_RST | UART_TXFIFO_RST);

	//set rx fifo trigger
	WRITE_PERI_REG(UART_CONF1(UART0), (UartDev.rcv_buff.TrigLvl & UART_RXFIFO_FULL_THRHD) << UART_RXFIFO_FULL_THRHD_S);

	//Clear pending interrupts
	WRITE_PERI_REG(UART_INT_CLR(UART0), 0xffff);
	//enable rx_interrupt
	SET_PERI_REG_MASK(UART_INT_ENA(UART0), UART_RXFIFO_FULL_INT_ENA);

	//Install our own putchar handler
	os_install_putc1((void *) uart_put_char);

	ETS_UART_INTR_ENABLE();
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

/**
 * Internal UART0 interrupt handler
 *
 * @param   void *para - point to ETS_UART_INTR_ATTACH's arg
 * @return  None
*******************************************************************************/
LOCAL void
uart0_rx_intr_handler(void *para)
{
    /* uart0 and uart1 intr combine together, when interrupt occur, see reg 0x3ff20020, bit2, bit0 represents
     * uart1 and uart0 respectively
     */
    RcvMsgBuff *pRxBuff = (RcvMsgBuff *)para;
    uint8 RcvChar;

    if (UART_RXFIFO_FULL_INT_ST != (READ_PERI_REG(UART_INT_ST(UART0)) & UART_RXFIFO_FULL_INT_ST)) {
        return;
    }

    WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_FULL_INT_CLR);

    while (READ_PERI_REG(UART_STATUS(UART0)) & (UART_RXFIFO_CNT << UART_RXFIFO_CNT_S)) {
        RcvChar = READ_PERI_REG(UART_FIFO(UART0)) & 0xFF;

        /* you can add your handle code below.*/

        *(pRxBuff->pWritePos) = RcvChar;

        // insert here for get one command line from uart
        if (RcvChar == '\r' || RcvChar == '\n' ) {
            pRxBuff->BuffState = WRITE_OVER;
        }

        if (pRxBuff->pWritePos == (pRxBuff->pRcvMsgBuff + RX_BUFF_SIZE)) {
            // overflow ...we may need more error handle here.
            pRxBuff->pWritePos = pRxBuff->pRcvMsgBuff ;
        } else {
            pRxBuff->pWritePos++;
        }

        if (pRxBuff->pWritePos == pRxBuff->pReadPos){   // overflow one byte, need push pReadPos one byte ahead
            if (pRxBuff->pReadPos == (pRxBuff->pRcvMsgBuff + RX_BUFF_SIZE)) {
                pRxBuff->pReadPos = pRxBuff->pRcvMsgBuff ;
            } else {
                pRxBuff->pReadPos++;
            }
        }
    }
}

/**
 * Get the number of bytes (characters) available for reading from the serial port.
 * This is data that's already arrived and stored in the serial receive buffer
 *
 * @param None
 * @return int the number of bytes available to read
 */
int uart_available(void)
{
	RcvMsgBuff *pRxBuff = &(UartDev.rcv_buff);
	return (pRxBuff->pWritePos - pRxBuff->pReadPos);
}

/**
 * Reads incoming serial data.
 *
 * @param None
 * @return int the first byte of incoming serial data available (or -1 if no data is available)
 */
int uart_read(void)
{
	RcvMsgBuff *pRxBuff = &(UartDev.rcv_buff);
	if(pRxBuff->pWritePos == pRxBuff->pReadPos){   // empty
		return -1;
	}

	int firstByte;

	// ETS_UART_INTR_DISABLE();
	ETS_INTR_LOCK();

	firstByte = (int)(*pRxBuff->pReadPos);
	if (pRxBuff->pReadPos == (pRxBuff->pRcvMsgBuff + RX_BUFF_SIZE)) {
		pRxBuff->pReadPos = pRxBuff->pRcvMsgBuff ;
	} else {
		pRxBuff->pReadPos++;
	}
	// ETS_UART_INTR_ENABLE();
	ETS_INTR_UNLOCK();
	return firstByte;
}
