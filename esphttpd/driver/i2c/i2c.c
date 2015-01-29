/*
    I2C driver for the ESP8266
    Copyright (C) 2014 Rudy Hardeman (zarya)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "i2c/i2c.h"

/**
 * Set SDA to state
 */
LOCAL void ICACHE_FLASH_ATTR
i2c_sda(uint8 state)
{
    state &= 0x01;
    //Set SDA line to state
    if (state)
        gpio_output_set(1 << I2C_SDA_PIN, 0, 1 << I2C_SDA_PIN, 0);
    else
        gpio_output_set(0, 1 << I2C_SDA_PIN, 1 << I2C_SDA_PIN, 0);
}

/**
 * Set SCK to state
 */
LOCAL void ICACHE_FLASH_ATTR
i2c_sck(uint8 state)
{
    //Set SCK line to state
    if (state)
        gpio_output_set(1 << I2C_SCK_PIN, 0, 1 << I2C_SCK_PIN, 0);
    else
        gpio_output_set(0, 1 << I2C_SCK_PIN, 1 << I2C_SCK_PIN, 0);
}

/**
 * I2C init function
 * This sets up the GPIO io
 */
void ICACHE_FLASH_ATTR
i2c_init(void)
{
    //Disable interrupts
    ETS_GPIO_INTR_DISABLE();

    //Set pin functions
    PIN_FUNC_SELECT(I2C_SDA_MUX, I2C_SDA_FUNC);
    PIN_FUNC_SELECT(I2C_SCK_MUX, I2C_SCK_FUNC);

    //Set SDA as open drain
    GPIO_REG_WRITE(
        GPIO_PIN_ADDR(GPIO_ID_PIN(I2C_SDA_PIN)),
        GPIO_REG_READ(GPIO_PIN_ADDR(GPIO_ID_PIN(I2C_SDA_PIN))) |
        GPIO_PIN_PAD_DRIVER_SET(GPIO_PAD_DRIVER_ENABLE)
    );

    GPIO_REG_WRITE(GPIO_ENABLE_ADDRESS, GPIO_REG_READ(GPIO_ENABLE_ADDRESS) | (1 << I2C_SDA_PIN));

    //Set SCK as open drain
    GPIO_REG_WRITE(
        GPIO_PIN_ADDR(GPIO_ID_PIN(I2C_SCK_PIN)),
        GPIO_REG_READ(GPIO_PIN_ADDR(GPIO_ID_PIN(I2C_SCK_PIN))) |
        GPIO_PIN_PAD_DRIVER_SET(GPIO_PAD_DRIVER_ENABLE)
    );

    GPIO_REG_WRITE(GPIO_ENABLE_ADDRESS, GPIO_REG_READ(GPIO_ENABLE_ADDRESS) | (1 << I2C_SCK_PIN));

    //Turn interrupt back on
    ETS_GPIO_INTR_ENABLE();

    i2c_sda(1);
    i2c_sck(1);
    return;
}

/**
 * I2C Start signal
 */
void ICACHE_FLASH_ATTR
i2c_start(void)
{
    i2c_sda(1);
    i2c_sck(1);
    os_delay_us(I2C_SLEEP_TIME);
    i2c_sda(0);
    os_delay_us(I2C_SLEEP_TIME);
    i2c_sck(0);
    os_delay_us(I2C_SLEEP_TIME);
}

/**
 * I2C Stop signal
 */
void ICACHE_FLASH_ATTR
i2c_stop(void)
{
    os_delay_us(I2C_SLEEP_TIME);
    i2c_sck(1);
    os_delay_us(I2C_SLEEP_TIME);
    i2c_sda(1);
    os_delay_us(I2C_SLEEP_TIME);
}

/**
 * Send I2C ACK to the bus
 * uint8 state 1 or 0
 *  1 for ACK
 *  0 for NACK
 */
void ICACHE_FLASH_ATTR
i2c_send_ack(uint8 state)
{
    i2c_sck(0);
    os_delay_us(I2C_SLEEP_TIME);
    //Set SDA
    //  HIGH for NACK
    //  LOW  for ACK
    i2c_sda((state?0:1));

    //Pulse the SCK
    i2c_sck(0);
    os_delay_us(I2C_SLEEP_TIME);
    i2c_sck(1);
    os_delay_us(I2C_SLEEP_TIME);
    i2c_sck(0);
    os_delay_us(I2C_SLEEP_TIME);

    i2c_sda(1);
    os_delay_us(I2C_SLEEP_TIME);
}

/**
 * Receive I2C ACK from the bus
 * returns 1 or 0
 *  1 for ACK
 *  0 for NACK
 */
uint8 ICACHE_FLASH_ATTR
i2c_check_ack(void)
{
    uint8 ack;
    i2c_sda(1);
    os_delay_us(I2C_SLEEP_TIME);
    i2c_sck(0);
    os_delay_us(I2C_SLEEP_TIME);
    i2c_sck(1);
    os_delay_us(I2C_SLEEP_TIME);

    //Get SDA pin status
    ack = i2c_read();

    os_delay_us(I2C_SLEEP_TIME);
    i2c_sck(0);
    os_delay_us(I2C_SLEEP_TIME);
    i2c_sda(0);
    os_delay_us(I2C_SLEEP_TIME);

    return (ack?0:1);
}

/**
 * Receive byte from the I2C bus
 * returns the byte
 */
uint8 ICACHE_FLASH_ATTR
i2c_readByte(void)
{
    uint8 data = 0;
    uint8 data_bit;
    uint8 i;

    i2c_sda(1);

    for (i = 0; i < 8; i++)
    {
        os_delay_us(I2C_SLEEP_TIME);
        i2c_sck(0);
        os_delay_us(I2C_SLEEP_TIME);

        i2c_sck(1);
        os_delay_us(I2C_SLEEP_TIME);

        data_bit = i2c_read();
        os_delay_us(I2C_SLEEP_TIME);

        data_bit <<= (7 - i);
        data |= data_bit;
    }
    i2c_sck(0);
    os_delay_us(I2C_SLEEP_TIME);

    return data;
}

/**
 * Write byte to I2C bus
 * uint8 data: to byte to be writen
 */
void ICACHE_FLASH_ATTR
i2c_writeByte(uint8 data)
{
    uint8 data_bit;
    sint8 i;

    os_delay_us(I2C_SLEEP_TIME);

    for (i = 7; i >= 0; i--) {
        data_bit = data >> i;
        i2c_sda(data_bit);
        os_delay_us(I2C_SLEEP_TIME);
        i2c_sck(1);
        os_delay_us(I2C_SLEEP_TIME);
        i2c_sck(0);
        os_delay_us(I2C_SLEEP_TIME);
    }
}
