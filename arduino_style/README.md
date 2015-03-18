# Arduino style programming for ESP8266


## Description

This is simple code library that uses Espressif drivers without any modification, so you can use it with fresh copy of the SDK. The point of this library is to ease the use of the gpio, i2c, spi, uart and flash memory. It's written with **esp_iot_sdk_v0.9.5**. We don't guarantee compatibility with older SDK.


## Installation

In this file we will not discuss how to install working toolchain. If you don't know how, we recommend using [this](http://www.esp8266.com/wiki/doku.php?id=toolchain).

Clone this repository:

``` bash
# mkdir -p /home/foo/checkout 
# cd /home/foo/checkout
# svn checkout https://github.com/OLIMEX/ESP8266/trunk/arduino_style
```

Let's say that you've previously copied sdk to **/home/foo/sdk/**. You should see the this tree:

``` bash
# cd /home/foo/sdk/
# tree -d
.
├── app
├── bin
│   ├── at
│   └── upgrade
├── document
│   ├── Chinese
│   └── English
├── examples
│   ├── at
│   │   ├── include
│   │   └── user
│   ├── IoT_Demo
│   │   ├── driver
│   │   ├── include
│   │   │   ├── driver
│   │   │   └── ssl
│   │   └── user
│   └── smart_config
│       ├── include
│       └── user
├── include
│   └── json
├── ld
├── lib
└── tools
	
```
Let's build IoT_Demo. Copy example folder to the root folder of the SDK:

``` bash
# cp -r examples/IoT_Demo .
```
	
If you installed the toolchain correctly the build should work perfect:

``` bash
# make COMPILE=gcc

!!!
No boot needed.
Generate eagle.flash.bin and eagle.irom0text.bin successfully in folder bin.
eagle.flash.bin-------->0x00000
eagle.irom0text.bin---->0x40000
!!!
```
	
Now to use arduino_style library copy **arduino** folder and **arduinostyle.h** from this repository into **user** directory:

``` bash
# cp -r /home/foo/checkout/arduino /home/foo/sdk/IoT_Demo/user/
# cp /home/foo/checkout/arduinostyle.h /home/foo/sdk/IoT_Demo/user/
```
	
Now what's left is to make a little modification to the **Makefile**:

``` bash
# nano Makefile
```
	
Find **COMPONENTS_eagle.app.v6** and edit it to looks like this:

``` c
COMPONENTS_eagle.app.v6 = \
user/libuser.a  \
user/arduino/libarduino.a \
driver/libdriver.a 
```	

Type again *make* and everything should be fine:

``` bash
# make COMPILE=gcc
```
	
Now to use this library simply include *arduinostyle.h* in your project

``` c
#include "arduinostyle.h
```	

## Available functions

### GPIO

The following functions are available for using gpio:

#### Set direction

``` c
pinMode(uint8 pin, pinmode_t mode)	
```

* pin - Can be between 0 and 5 and between 12 and 15. The other pins are used for the spi flash.

* mode - Can be *INPUT*, *OUTPUT*, *INPUT_PULLUP*. You can also use 0 for input, 1 - output and 2 - input with pull-up resistor.

#### Set output level

```c
digitalWrite(uint8 pin, pinvalue_t value)
```
	
* pin - Can be between 0 and 5 and between 12 and 15. The other pins are used for the spi flash.

* value - Can be *OUTPUT*, *INPUT* or 0 and 1. In pin is set as input it enables/disables pull-up resistor.

#### Read input level

``` c
digitalRead(uint8 pin)
```

* pin - Can be between 0 and 5 and between 12 and 15. The other pins are used for the spi flash.

Function return the input level, if pin is configured as input. Otherwise it returns output level.


### UART

For printing messages on the serial interface we recommends to use the build-in function *os_printf* because is much more flexible than *Serial.println*. 

#### Setup communication

``` c
Serial.begin(buad)
```
	
* baud - Communication speed. Available speeds are: **9600**, **19200**, **38400**, **57600**, **74880**, **115200**, **230400**, **460800**, **921600**.

This will only initialize *UART1* (the one on the UEXT connector).

#### Print line with CR/LF

``` c
Serial.println(char *buffer)
```
	
* buffer - Message to be printed. Note that if you want to print number you should previously create buffer with *sprintf*.

#### Print line without CR/LF

``` c
Serial.print(char *buffer)
```
	
#### Print single character

``` c
Serial.write(char c)
```	

#### Reads incoming serial data.

``` c
i = Serial.read()
```

* Function returns the first byte of incoming serial data available (or -1 if no data is available).

	
### I2C	

I2C pins are pins 5 and 6 in the UEXT connector. The following functions are available.

#### Setup communication

``` c
Wire.begin()
```
	
Setup SDA and SCL pins.

#### Send slave address with W flag

``` c
Wire.beginTransmission(uint8 address)
```
	
* address - The address of the slave device **without** W/R flag at the end.

#### Send one byte of data

``` c
Wire.write(uint8 data)
```
	
* data - One byte of data to be sent to the slave device.

#### Send Stop condition and check for status of previously commands

``` c
Wire.endTransmission()
```
	
The function check for address NACK or data NACK and returns:

* I2C_SUCCESS - 0

* I2C_DATA_TOO_LONG - 1

* I2C_ADDRESS_NACK - 2

* I2C_DATA_NACK - 3

* I2C_OTHER_ERROR  - 4


#### Read bytes from slave device

``` c
Wire.requestFrom(uint8 address, uint8 bytes)
```
	
* address - The address of the slave device **without** W/R flag at the end.

* bytes - Number of bytes to read.

#### Check if there are any available data for reading

``` c
Wire.available()
```
	
Check for available data in the buffer and returns their count.

#### Read single byte of data

``` c
Wire.read()
```
	
Return byte of data from the data buffer.


### Flash

You could use the spi flash memory for storing user data. **Make sure that the place where you are writing is not used!** The memory is divided to sectors by 4096 bytes. This is the minimum erase size.

#### Erase sectors in range

``` c
flash.erase(uint32 startAddress, uint32 endAddress)
```
	
* startAddress - Start address of the segment
* endAddress - End address of the segment

Note that this function will erase the sectors where start and end addresses are. If start and end address are the same, it will erase single sector.


#### Read data from address

``` c
flash.read(uint32 address)
```
	
* address - The address of the data field.

This function returns 4bytes of data.


#### Write data to address

``` c
flash.write(uint32 address, uint32 data)
```
	
* address - The address of the data field.

* data - 4 bytes of data to be written.



### SPI

There is hardware SPI, but in this library we make it bit-banging. This way you can have more control. 
If you want to increase speed change the delay value in *arduino/arduino_spi.c*:

``` c
spi_config.delay = 10;
```
		
#### Setup SPI

``` c
spi.begin(uint8 pin)
```
	
* pin - Chip select pin to be used. For UEXT connector it should be **15**.

#### Set data order

``` c
spi.setBitOrder(spiOrder_t order)
```
	
* order - can be MSBFIRST (default) or LSBFIRST.

#### Set communication mode

``` c
spi.setDataMode(spiMode_t mode)
```

* mode - Can be *SPI_MODE0*, *SPI_MODE1*, *SPI_MODE2* or  *SPI_MODE3*.

#### Transfer data with slave device

``` c
spi.transfer(uint8 pin, uint8 val, spiTransferMode_t transferMode)
```
	
* pin - Chip-select pin.

* val - Data to be transmitted to the slave device

* transferMode - Can be *SPI_CONTINUE* or *SPI_LAST*. The first one will hold chip-select active.

The function return one byte of data from slave to master.


## Contact

*support@olimex.com*

## Revision

5 MAR 2015 - Initial release
