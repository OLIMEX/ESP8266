/*
 * 
 * arduino_spi.c is part of esp_arduino_style.
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

#include "driver/spi.h"
#include "arduino_spi.h"
#include "arduino_gpio.h"

#ifndef HW_SPI

#define MISO 12
#define MOSI 13
#define CLK	 14

#endif

spi_config_t spi_config;

spi_t spi =
{ .begin = spi_begin, .setClockDivider = spi_set_clock_divider, .setDataMode =
		spi_set_data_mode, .setBitOrder = spi_set_bit_order, .transfer =
		spi_transfer, };

/**
 * Initialize spi interface.
 *
 * @param pin The gpio number for chip-select.
 * @return None
 */
void spi_begin(uint8 pin)
{
	if (!pin_is_valid(pin))
	{
		return;
	}

	spi_config.mode = SPI_MODE0;
	spi_config.order = MSBFIRST;
	spi_config.delay = 10;

	/* Init SPI */
#ifdef HW_SPI
	spi_master_init(HSPI);
#else
	pinMode(MISO, INPUT);
	pinMode(MOSI, INPUT);
	pinMode(CLK, OUTPUT);
#endif
	/* Override CS pin to GPIO */
	pinMode(pin, OUTPUT);
	digitalWrite(pin, HIGH);

}
/**
 * Set pre-divider and divider for spi.
 * Frequency is calculated:
 * 80Mhz/(predivider+1)/(divider+1)
 *
 * @param predivider Pre-divider for spi clock - 0 to 8192.
 * @param divider Divider fo spi clock - 0 to 63.
 * @return None.
 */
void spi_set_clock_divider(uint16 predivider, uint8 divider)
{

	/* Check if predivider and divider are valid */
	if (divider > 63 || predivider > 8192)
	{
		return;
	}
	WRITE_PERI_REG(SPI_CLOCK(HSPI),
			((predivider & SPI_CLKDIV_PRE) << SPI_CLKDIV_PRE_S) | ((divider & SPI_CLKCNT_N) << SPI_CLKCNT_N_S) | ((((divider + 1) / 2 - 1) & SPI_CLKCNT_H) << SPI_CLKCNT_H_S) | ((divider & SPI_CLKCNT_L) << SPI_CLKCNT_L_S)); //clear bit 31,set SPI clock div
}

/**
 * Set polarity and phase. Currently not working.
 *
 * @param mode Mode to be configured.
 * @see spiMode_t.
 * @return None.
 */
void spi_set_data_mode(spiMode_t mode)
{
	spi_config.mode = mode;
}

/**
 * Set bit order.
 *
 * @param order MSB first or LSB first
 * @see spiOrder_t
 * @return None
 */
void spi_set_bit_order(spiOrder_t order)
{
#ifdef HW_SPI
	if (!order)
	{
		WRITE_PERI_REG(SPI_CTRL(HSPI),
				READ_PERI_REG(SPI_CTRL(HSPI)) & (~(SPI_WR_BIT_ORDER | SPI_RD_BIT_ORDER)));
	}
	else
	{
		WRITE_PERI_REG(SPI_CTRL(HSPI),
				READ_PERI_REG(SPI_CTRL(HSPI)) | (SPI_WR_BIT_ORDER | SPI_RD_BIT_ORDER));
	}
#else
	spi_config.order = order;
#endif
}

/**
 * Transfer byte to spi slave device.
 *
 * @param pin Chip-select gpio number.
 * @param val Data for transfer.
 * @param transferMode SPI_CONTINUE - the master hold CS pin active,
 * SPI_LAST - the master release CS after transmission.
 * @see spiTransferMode_t
 * @return data read from MISO line.
 */
uint8 spi_transfer(uint8 pin, uint8 val, spiTransferMode_t transferMode)
{
	if (!pin_is_valid(pin))
	{
		return 0;
	}

	uint8 i;
	uint8 pol = spi_config.mode >> 1;
	uint8 pha = spi_config.mode & 1;
	uint8 buffer = 0;
	uint8 mask = 0;

	mask = (spi_config.mode) ? 0x01 : 0x80;

	/* Set the clock */
	digitalWrite(CLK, pol);

	/* Pulldown chip select */
	digitalWrite(pin, LOW);

#ifdef HW_SPI
	spi_mast_byte_write(HSPI, val);

	if (transferMode == SPI_LAST)
	{
		digitalWrite(pin, HIGH);
	}
#else
	pinMode(MOSI, OUTPUT);

	if (pha == 0)
	{
		if(val & mask)
			digitalWrite(MOSI, HIGH);
		else
			digitalWrite(MOSI, LOW);

		if(!spi_config.order)
			val <<= 1;
		else
			val >>= 1;
	}

	os_delay_us(spi_config.delay);



	/* Begin loop */
	for (i = 0; i < 8; ++i) {

		digitalWrite(CLK, !pol);

		if(pha == 1)
		{
			if(val & mask)
				digitalWrite(MOSI, HIGH);
			else
				digitalWrite(MOSI, LOW);

			if(!spi_config.order)
				val <<= 1;
			else
				val >>= 1;
		}
		else
		{
			if(!spi_config.order)
			{
				buffer |= digitalRead(MISO);
				if (i != 7)
					buffer <<= 1;
			}
			else
			{
				buffer |= digitalRead(MISO) << (7-i);
			}
		}
		os_delay_us(spi_config.delay);

		digitalWrite(CLK, pol);

		if(pha == 0)
		{
			if(val & mask)
				digitalWrite(MOSI, HIGH);
			else
				digitalWrite(MOSI, LOW);
			if(!spi_config.order)
				val <<= 1;
			else
				val >>= 1;
		}
		else
		{
			if(!spi_config.order)
			{
				buffer |= digitalRead(MISO);
				if (i != 7)
					buffer <<= 1;
			}
			else
			{
				buffer |= digitalRead(MISO) << (7-i);
			}
		}
		os_delay_us(spi_config.delay);

	}

	if(transferMode == SPI_LAST)
	{
		pinMode(MOSI, INPUT);
		digitalWrite(pin, HIGH);
	}



#endif
	return buffer;
}

