/*
 * File	: uart.c
 * Copyright (C) 2013 - 2016, Espressif Systems
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of version 3 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "ets_sys.h"
#include "osapi.h"
#include "driver/uart.h"
#include "osapi.h"
#include "driver/uart_register.h"
#include "mem.h"
#include "os_type.h"

// UartDev is defined and initialized in rom code.
extern UartDevice UartDev;

#define UART_TASK_PRIORITY		0
#define UART_TASK_QUEUE_LEN	    10

LOCAL os_event_t uart_recv_queue[UART_TASK_QUEUE_LEN];
LOCAL uart_char_in_callback uart_char_in = NULL;

LOCAL void uart_rx_intr_handler(void *para);

/******************************************************************************
 * FunctionName : uart_config
 * Description  : Configure UART interface
 * Returns	    : NONE
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR uart_config(
	uint8            uart,
	UartBaudRate     baud_rate, 
	UartBitsNum4Char data_bits, 
	UartParityMode   parity, 
	UartStopBitsNum  stop_bits
) {
	if (uart == UART0) {
		/* rcv_buff size if 0x100 */
		ETS_UART_INTR_ATTACH(uart_rx_intr_handler,  NULL);
		PIN_PULLUP_DIS(PERIPHS_IO_MUX_U0TXD_U);
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0TXD_U, FUNC_U0TXD);
		
		#if UART_HW_RTS
		// HW FLOW CONTROL RTS PIN
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_U0RTS); 
		#endif
		#if UART_HW_CTS
		//HW FLOW CONTROL CTS PIN
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_U0CTS); 
		#endif
	} else {
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_U1TXD_BK);
	}
	
	//SET BAUDRATE
	uart_div_modify(uart, UART_CLK_FREQ / baud_rate); 
	
	// SET BIT AND PARITY MODE
	UartExistParity exist_parity = (parity == NONE_BITS ? STICK_PARITY_DIS : STICK_PARITY_EN);
	WRITE_PERI_REG(
		UART_CONF0(uart), 
		((exist_parity & UART_PARITY_EN_M)  << UART_PARITY_EN_S) | 
		((parity       & UART_PARITY_M)     << UART_PARITY_S) |
		((stop_bits    & UART_STOP_BIT_NUM) << UART_STOP_BIT_NUM_S) |
		((data_bits    & UART_BIT_NUM)      << UART_BIT_NUM_S)
	);
	
	// clear rx and tx fifo,not ready
	SET_PERI_REG_MASK(UART_CONF0(uart),   UART_RXFIFO_RST | UART_TXFIFO_RST);	//RESET FIFO
	CLEAR_PERI_REG_MASK(UART_CONF0(uart), UART_RXFIFO_RST | UART_TXFIFO_RST);
	
	if (uart == UART0) {
		// set rx fifo trigger
		WRITE_PERI_REG(
			UART_CONF1(uart),
			((100 & UART_RXFIFO_FULL_THRHD) << UART_RXFIFO_FULL_THRHD_S) |
			#if UART_HW_RTS
			((110 & UART_RX_FLOW_THRHD) << UART_RX_FLOW_THRHD_S) |
			UART_RX_FLOW_EN |   // enable rx flow control
			#endif
			((0x02 & UART_RX_TOUT_THRHD) << UART_RX_TOUT_THRHD_S) |
			UART_RX_TOUT_EN |
			((0x10 & UART_TXFIFO_EMPTY_THRHD) << UART_TXFIFO_EMPTY_THRHD_S)
		); //wjl
	
		#if UART_HW_CTS
		SET_PERI_REG_MASK(UART_CONF0(uart), UART_TX_FLOW_EN);  //add this sentence to add a tx flow control via MTCK (CTS)
		#endif
		SET_PERI_REG_MASK(UART_INT_ENA(uart), UART_RXFIFO_TOUT_INT_ENA | UART_FRM_ERR_INT_ENA);
	} else {
		WRITE_PERI_REG(
			UART_CONF1(uart),
			((UartDev.rcv_buff.TrigLvl & UART_RXFIFO_FULL_THRHD) << UART_RXFIFO_FULL_THRHD_S)
		); //TrigLvl default val == 1
	}
		
	//clear all interrupt
	WRITE_PERI_REG(UART_INT_CLR(uart), 0xffff);
	
	//enable rx_interrupt
	SET_PERI_REG_MASK(UART_INT_ENA(uart), UART_RXFIFO_FULL_INT_ENA | UART_RXFIFO_OVF_INT_ENA);
}

void uart_rx_intr_disable(uint8 uart) {
	CLEAR_PERI_REG_MASK(UART_INT_ENA(uart), UART_RXFIFO_FULL_INT_ENA | UART_RXFIFO_TOUT_INT_ENA);
}

void uart_rx_intr_enable(uint8 uart) {
	WRITE_PERI_REG(UART_INT_CLR(uart), UART_RXFIFO_FULL_INT_CLR | UART_RXFIFO_TOUT_INT_CLR);
	SET_PERI_REG_MASK(UART_INT_ENA(uart), UART_RXFIFO_FULL_INT_ENA | UART_RXFIFO_TOUT_INT_ENA);
}

/******************************************************************************
 * FunctionName : uart0_rx_intr_handler
 * Description  : Internal used function
 *				  UART0 interrupt handler, add self handle code inside
 * Parameters   : void *param - point to ETS_UART_INTR_ATTACH's arg
 * Returns	    : NONE
*******************************************************************************/
LOCAL void uart_rx_intr_handler(void *param) {
	/* ATTENTION: */
	/* IN NON-OS VERSION SDK, DO NOT USE "ICACHE_FLASH_ATTR" FUNCTIONS IN THE WHOLE HANDLER PROCESS */
	/* ALL THE FUNCTIONS CALLED IN INTERRUPT HANDLER MUST BE DECLARED IN RAM */
	/* IF NOT, POST AN EVENT AND PROCESS IN SYSTEM TASK */
	if (UART_FRM_ERR_INT_ST == (READ_PERI_REG(UART_INT_ST(UART0)) & UART_FRM_ERR_INT_ST)) {
		WRITE_PERI_REG(UART_INT_CLR(UART0), UART_FRM_ERR_INT_CLR);
	} else if (UART_RXFIFO_FULL_INT_ST == (READ_PERI_REG(UART_INT_ST(UART0)) & UART_RXFIFO_FULL_INT_ST)) {
		uart_rx_intr_disable(UART0);
		WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_FULL_INT_CLR);
		system_os_post(UART_TASK_PRIORITY, 0, 0);
	} else if (UART_RXFIFO_TOUT_INT_ST == (READ_PERI_REG(UART_INT_ST(UART0)) & UART_RXFIFO_TOUT_INT_ST)) {
		uart_rx_intr_disable(UART0);
		WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_TOUT_INT_CLR);
		system_os_post(UART_TASK_PRIORITY, 0, 0);
	} else if (UART_TXFIFO_EMPTY_INT_ST == (READ_PERI_REG(UART_INT_ST(UART0)) & UART_TXFIFO_EMPTY_INT_ST)) {
		CLEAR_PERI_REG_MASK(UART_INT_ENA(UART0), UART_TXFIFO_EMPTY_INT_ENA);
		WRITE_PERI_REG(UART_INT_CLR(UART0), UART_TXFIFO_EMPTY_INT_CLR);
	} else if (UART_RXFIFO_OVF_INT_ST  == (READ_PERI_REG(UART_INT_ST(UART0)) & UART_RXFIFO_OVF_INT_ST)) {
		WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_OVF_INT_CLR);
	}
}

LOCAL void ICACHE_FLASH_ATTR uart_recv(os_event_t *event) {
	if (event->sig == 0) {
		if (uart_char_in == NULL) {
			uart_rx_intr_enable(UART0);
			return;
		}
		
		uint8 fifo_len = (READ_PERI_REG(UART_STATUS(UART0)) >> UART_RXFIFO_CNT_S) & UART_RXFIFO_CNT;
		uint8 i;
		for (i = 0; i<fifo_len; i++) {
			(*uart_char_in)(READ_PERI_REG(UART_FIFO(UART0)) & 0xFF);
		}
		uart_rx_intr_enable(UART0);
	}
}

void ICACHE_FLASH_ATTR uart_char_in_set(uart_char_in_callback func) {
	uart_char_in = func;
}


/******************************************************************************
 * FunctionName : uart_init
 * Description  : user interface for init uart
 * Parameters   : UartBaudRate baud - uart0 baud rate
 * Returns	  : NONE
*******************************************************************************/
void ICACHE_FLASH_ATTR uart_init(
	uint8            uart,
	UartBaudRate     baud_rate, 
	UartBitsNum4Char data_bits, 
	UartParityMode   parity, 
	UartStopBitsNum  stop_bits
) {
	if (uart == UART0) {
		system_os_task(uart_recv, UART_TASK_PRIORITY, uart_recv_queue, UART_TASK_QUEUE_LEN);
		uart_config(uart, baud_rate, data_bits, parity, stop_bits);
		ETS_UART_INTR_ENABLE();
	} else {
		uart_config(uart, baud_rate, data_bits, parity, stop_bits);
	}
}

/******************************************************************************
 * FunctionName : uart_tx_one_char
 * Description  : Use uart interface to transfer one char
 * Parameters   : uint8 TxChar - character to tx
 * Returns	    : OK
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR uart_tx_one_char(uint8 uart, uint8 TxChar) {
	while (((READ_PERI_REG(UART_STATUS(uart)) >> UART_TXFIFO_CNT_S) & UART_TXFIFO_CNT) >= 126) ;
	WRITE_PERI_REG(UART_FIFO(uart), TxChar);
}

/******************************************************************************
 * FunctionName : uart_tx_one_char_direct
 * Description  : Use uart interface to transfer one char
 * Parameters   : uint8 TxChar - character to tx
 * Returns	    : OK
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR uart_tx_one_char_direct(uint8 uart, uint8 TxChar) {
	while (((READ_PERI_REG(UART_STATUS(uart)) >> UART_TXFIFO_CNT_S) & UART_TXFIFO_CNT) != 0) ;
	WRITE_PERI_REG(UART_FIFO(uart), TxChar);
}

/******************************************************************************
 * FunctionName : uart_tx_one_char_no_wait
 * Description  : uart tx a single char without waiting for fifo 
 * Parameters   : uint8 uart - uart port
 *				  uint8 TxChar - char to tx
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR uart_tx_one_char_no_wait(uint8 uart, uint8 TxChar) {
	uint8 fifo_cnt = ((READ_PERI_REG(UART_STATUS(uart)) >> UART_TXFIFO_CNT_S) & UART_TXFIFO_CNT);
	if (fifo_cnt < 126) {
		WRITE_PERI_REG(UART_FIFO(uart) , TxChar);
	}
}

LOCAL void ICACHE_FLASH_ATTR _uart_write_char_(uint8 uart, char c) {
LOCAL char last;
	if (c == '\n') {
		if (last == '\r') {
			return;
		}
		uart_tx_one_char(uart, '\r');
	}
	
	uart_tx_one_char(uart, c);
	
	last = c;
	if (c == '\r') {
		uart_tx_one_char(uart, '\n');
	}
}

void ICACHE_FLASH_ATTR uart_write_char(char c) {
	_uart_write_char_(UART0, c);
}

void ICACHE_FLASH_ATTR uart1_write_char(char c) {
	_uart_write_char_(UART1, c);
}

void ICACHE_FLASH_ATTR uart_write_char_no_wait(char c) {
LOCAL char last;
	if (c == '\n') {
		if (last == '\r') {
			return;
		}
		uart_tx_one_char_no_wait(UART0, '\r');
	}
	
	uart_tx_one_char_no_wait(UART0, c);
	
	last = c;
	if (c == '\r') {
		uart_tx_one_char_no_wait(UART0, '\n');
	}
}

/******************************************************************************
 * FunctionName : uart_write_str
 * Description  : use uart to transfer string
 * Parameters   : uint8 *str - point to the string
 * Returns	  :
*******************************************************************************/
void ICACHE_FLASH_ATTR uart_write_str(const char *str) {
	while (*str) {
		uart_tx_one_char(UART0, *str);
		str++;
	}
}

void at_port_print(const char *str) __attribute__((alias("uart_write_str")));


void ICACHE_FLASH_ATTR uart_write_byte(uint8 b) {
	uart_tx_one_char(UART0, b);
}

void ICACHE_FLASH_ATTR uart_write_buff(uint8 *buff, uint32 len) {
	uint32 i;
	for (i=0; i<len; i++) {
		uart_tx_one_char(UART0, buff[i]);
	}
}
void ICACHE_FLASH_ATTR uart_write_buff_direct(uint8 *buff, uint32 len) {
	uint32 i;
	for (i=0; i<len; i++) {
		uart_tx_one_char_direct(UART0, buff[i]);
	}
}
