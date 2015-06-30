/*
 * 
 * arduino_gpio.c is part of esp_arduino_style.
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

#include "ets_sys.h"
#include "arduino_gpio.h"

gpio_reg_t gpio[] =
{
	{ PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0 },	//gpio0
	{ PERIPHS_IO_MUX_U0TXD_U, FUNC_GPIO1 },	//gpio1
	{ PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2 },	//gpio2
	{ PERIPHS_IO_MUX_U0RXD_U, FUNC_GPIO3 },	//gpio3
	{ PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4 },	//gpio4
	{ PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5 },	//gpio5
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12 },	//gpio12
	{ PERIPHS_IO_MUX_MTCK_U, FUNC_GPIO13 },	//gpio13
	{ PERIPHS_IO_MUX_MTMS_U, FUNC_GPIO14 },	//gpio14
	{ PERIPHS_IO_MUX_MTDO_U, FUNC_GPIO15 },	//gpio15
};

/**
 * Return if gpio is available.
 *
 * @param pin The gpio to be used
 * @return 0 if gpio is valid, 1 for invalid.
 */
uint8_t pin_is_valid(uint8 pin)
{
	/* Check if pin number is valid */
	if ((pin > 5 && pin < 12) || pin > 15)
	{
		return 0;
	}
	return 1;
}

/**
 * Set data direction of GPIO.
 *
 * @param pin GPIO to be configured.
 * @param mode The data direction
 * @see pinmode_t
 * @return None
 */
void pinMode(uint8 pin, pinmode_t mode)
{
	/* Check if pin number is valid */
	if (!pin_is_valid(pin))
	{
		return;
	}

	/* Mux to gpio */
	PIN_FUNC_SELECT(gpio[pin].reg, gpio[pin].gpio_func);
	PIN_PULLUP_DIS(gpio[pin].reg);
	PIN_PULLDWN_DIS(gpio[pin].reg);

	/* Set direction */
	switch (mode)
	{

	case INPUT:
		WRITE_PERI_REG(GPIO_ENABLE_W1TC, 0x1 << pin);
		break;

	case OUTPUT:
		WRITE_PERI_REG(GPIO_ENABLE_W1TS, 0x1 << pin);
		break;

	case INPUT_PULLUP:
		WRITE_PERI_REG(GPIO_ENABLE_W1TC, 0x1 << pin);
		PIN_PULLUP_EN(gpio[pin].reg);
		break;

	default:
		return;
		break;
	}

	return;
}

/**
 * Set output value of GPIO.
 *
 * @param pin GPIO to be configured.
 * @param value The output value. If configured
 * as input 1 enables pull-up.
 * @see pinvalue_t
 * @return None
 */
void digitalWrite(uint8 pin, pinvalue_t value)
{
	/* Check if pin number is valid */
	if (!pin_is_valid(pin))
	{
		return;
	}

	/* Read current confgiguration */
	if (READ_PERI_REG(GPIO_ENABLE) & ((uint32) 0x1 << pin))
	{
		/* Pin is output */
		WRITE_PERI_REG(GPIO_OUT,
				(READ_PERI_REG(GPIO_OUT) & (uint32) (~(0x01 << pin)))
						| (value << pin));
	}
	else
	{
		/* Pin is input */
		if (value)
		{
			PIN_PULLUP_EN(gpio[pin].reg);
		}
		else
		{
			PIN_PULLUP_DIS(gpio[pin].reg);
		}
	}
}

/**
 * Read current value of GPIO.
 *
 * @param pin GPIO to be read.
 * @return If configured as input return value at GPIO.
 * Otherwise return the output value.
 */
uint8 digitalRead(uint8 pin)
{
	/* Check if pin number is valid */
	if (!pin_is_valid(pin))
	{
		return 0;
	}

	if (READ_PERI_REG(GPIO_ENABLE) & ((uint32) 0x1 << pin))
	{
		return !!((READ_PERI_REG(GPIO_OUT) & ((uint32) 0x1 << pin)));
	}
	else
	{
		return !!((READ_PERI_REG(GPIO_IN) & ((uint32) 0x1 << pin)));
	}

}
