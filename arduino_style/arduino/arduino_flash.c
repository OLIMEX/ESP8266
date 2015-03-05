/*
 * 
 * arduino_flash.c is part of esp_arduino_style.
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

#include "osapi.h"	//TODO: remove me after debug
#include "arduino_flash.h"
#include "spi_flash.h"

flash_t flash =
{ .erase = flash_erase, .read = flash_read, .write = flash_write, };

/**
 * Erase sectors in given range.
 *
 * @param startAddress Start address.
 * @param endAddress End address.
 * @return None.
 */
void flash_erase(uint32 startAddress, uint32 endAddress)
{

	uint32 currentAddress = startAddress & ~(0x00000FFF);

	do
	{
		if (spi_flash_erase_sector(currentAddress / 0x1000)
				!= SPI_FLASH_RESULT_OK)
		{
			return;
		}

		currentAddress += 0x1000;
	} while (currentAddress <= endAddress);
}

/**
 * Write data at address.
 *
 * @param address Address in the memory.
 * @param data Data to be written.
 * @return None
 */
void flash_write(uint32 address, uint32 data)
{

	uint32 buffer[] =
	{ data };

	if (spi_flash_write(address, (uint32 *) buffer, sizeof(buffer))
			!= SPI_FLASH_RESULT_OK)
		return;
}

/**
 * Read data at address.
 * @param address Address in the memory.
 * @return Value in the memory cell.
 */
uint32 flash_read(uint32 address)
{
	uint32 buffer[1];

	spi_flash_read(address, (uint32 *) buffer, sizeof(buffer));
	return buffer[0];
}
