esphttpd
========

ESP8266 webserver + Olimex DEMO page


How to build project and upload:

* Install toolchain
* Build the project
* Connect the board to pc
* Upload the project

### Install the toolchain
Follow the instructions on this site:
https://github.com/esp8266/esp8266-wiki/wiki

### Build the project
Clone this repository into Espressif install directory

```shell
cd /opt/Espressif
git clone https://github.com/OLIMEX/ESP8266.git
cd ESP8266/esphttpd
git submodule init
git submodule update
make
```
Once you make the project the next step is to upload it to [ESP8266-EVB](https://www.olimex.com/Products/IoT/ESP8266-EVB/open-source-hardware/ "ESP8266-EVB") board.

### Connect the board to pc

You will need 3.3V cable like [USB-SERIAL-CABLE-F](https://www.olimex.com/Products/Components/Cables/USB-Serial-Cable/USB-Serial-Cable-F/ "USB-Serial-Cable-F").

Connect cable as follows:
* blue --> uext pin-2
* green --> uext pin-3
* red --> ext pin-4

Connect the cable to USB port. You should see the new device in /dev/tty*, for example /dev/ttyUSB0 if you see other update makefile properly.

To go into bootloader mode, simply hold the big white button on the board, apply power and release the button.

### Upload the project

If you have build the project without errors you can upload the code to ESP8266 with these commands:

```shell
make flash
```
The upload takes 2 cycles. 

To upload demo webpages go again into bootloader mode and run:

```shell
make htmlflash
```

Now everything is complete.

Use computer with WIFI and scan for WIFI networks the name of ESP8266-EVB WIFI will be ESP_xxxxx where xxxxx is the ESP MAC address.
Then launch browser and type http://192.168.4.1
You will see this home page:

![Home page](https://www.olimex.com/Products/IoT/ESP8266-EVB/resources/HOME.png)

if you click on the RELAY button you will be re-directed to the page:

![Home page](https://www.olimex.com/Products/IoT/ESP8266-EVB/resources/RELAY.png)

on this page you can change on-board RELAY to ON and OFF

if you click on the BUTTON button you will be re-directed to the page:

![Home page](https://www.olimex.com/Products/IoT/ESP8266-EVB/resources/RELAY.png)

where you can scan the button status.

This is very basic demo code which can be used as template for your own projects!

