/*
 * 
 * arduino_spi.h is part of esp_arduino_style.
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

#ifndef USER_ARDUINO_ARDUINO_SPI_H_
#define USER_ARDUINO_ARDUINO_SPI_H_

/* SPI */
typedef enum
{
	SPI_MODE0, SPI_MODE1, SPI_MODE2, SPI_MODE3
} spiMode_t;

typedef enum
{
	MSBFIRST = 0, LSBFIRST
} spiOrder_t;

typedef enum
{
	SPI_CONTINUE, SPI_LAST
} spiTransferMode_t;

typedef struct
{
	spiOrder_t order;
	spiMode_t mode;
	uint16 delay;

} spi_config_t;

typedef struct
{
	void (*begin)(uint8 pin);
	void (*setClockDivider)(uint16 predivider, uint8 divider);
	void (*setDataMode)(spiMode_t mode);
	void (*setBitOrder)(spiOrder_t order);
	uint8 (*transfer)(uint8 pin, uint8 val, spiTransferMode_t transferMode);
} spi_t;

void spi_begin(uint8 pin);
void spi_set_clock_divider(uint16 predivider, uint8 divider);
void spi_set_data_mode(spiMode_t mode);
void spi_set_bit_order(spiOrder_t order);
uint8 spi_transfer(uint8 pin, uint8 val, spiTransferMode_t transferMode);

extern spi_t spi;

#endif /* USER_ARDUINO_ARDUINO_SPI_H_ */
