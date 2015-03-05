/*
 * 
 * arduino_i2c.h is part of esp_arduino_style.
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

#ifndef USER_ARDUINO_ARDUINO_I2C_H_
#define USER_ARDUINO_ARDUINO_I2C_H_

#define I2C_MASTER_SDA_MUX PERIPHS_IO_MUX_GPIO2_U
#define I2C_MASTER_SCL_MUX PERIPHS_IO_MUX_GPIO4_U
#define I2C_MASTER_SDA_GPIO 2
#define I2C_MASTER_SCL_GPIO 4
#define I2C_MASTER_SDA_FUNC FUNC_GPIO2
#define I2C_MASTER_SCL_FUNC FUNC_GPIO4

#define I2C_MASTER_SDA_HIGH_SCL_HIGH()  \
    gpio_output_set(1<<I2C_MASTER_SDA_GPIO | 1<<I2C_MASTER_SCL_GPIO, 0, 1<<I2C_MASTER_SDA_GPIO | 1<<I2C_MASTER_SCL_GPIO, 0)

#define I2C_MASTER_SDA_HIGH_SCL_LOW()  \
    gpio_output_set(1<<I2C_MASTER_SDA_GPIO, 1<<I2C_MASTER_SCL_GPIO, 1<<I2C_MASTER_SDA_GPIO | 1<<I2C_MASTER_SCL_GPIO, 0)

#define I2C_MASTER_SDA_LOW_SCL_HIGH()  \
    gpio_output_set(1<<I2C_MASTER_SCL_GPIO, 1<<I2C_MASTER_SDA_GPIO, 1<<I2C_MASTER_SDA_GPIO | 1<<I2C_MASTER_SCL_GPIO, 0)

#define I2C_MASTER_SDA_LOW_SCL_LOW()  \
    gpio_output_set(0, 1<<I2C_MASTER_SDA_GPIO | 1<<I2C_MASTER_SCL_GPIO, 1<<I2C_MASTER_SDA_GPIO | 1<<I2C_MASTER_SCL_GPIO, 0)

#define i2c_master_wait    os_delay_us

typedef struct
{
	bool AddressNack;
	bool DataNack;
	uint8 address;
	uint8 read_buffer[64];
	uint8 data_available;
	uint8 read_buffer_len;
} i2c_data_t;

typedef enum
{
	I2C_SUCCESS,
	I2C_DATA_TOO_LONG,
	I2C_ADDRESS_NACK,
	I2C_DATA_NACK,
	I2C_OTHER_ERROR
} i2cTransmissionStatus_t;

typedef struct
{
	void (*begin)(void);
	void (*beginTransmission)(uint8 address);
	i2cTransmissionStatus_t (*endTransmission)(void);
	void (*write)(uint8 data);
	uint8 (*read)(void);
	uint8 (*requestFrom)(uint8 address, uint8 bytes);
	uint8 (*available)(void);
} i2c_t;

void i2c_begin();
void i2c_begin_transmission(uint8 address);
i2cTransmissionStatus_t i2c_end_transmission();
void i2c_write(uint8 data);
uint8 i2c_requestFrom(uint8 address, uint8 bytes);
uint8 i2c_available(void);
uint8 i2c_read(void);


void i2c_master_gpio_init(void);
void i2c_master_init(void);


void i2c_master_stop(void);
void i2c_master_start(void);
void i2c_master_setAck(uint8 level);
uint8 i2c_master_getAck(void);
uint8 i2c_master_readByte(void);
void i2c_master_writeByte(uint8 wrdata);

bool i2c_master_checkAck(void);
void i2c_master_send_ack(void);
void i2c_master_send_nack(void);

extern i2c_t Wire;

#endif /* USER_ARDUINO_ARDUINO_I2C_H_ */
