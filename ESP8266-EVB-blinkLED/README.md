ESP8266-EVB Hello World Blinking LED code
=========================================

Will blink TX Green LED on the MOD-WIFI-ESP8266-DEV module with 1Hz.

How to build project and upload:

* Install toolchain
* Build the project
* Connect the board to pc
* Upload the project

### Install the toolchain
Follow the instructions on this site:
https://github.com/esp8266/esp8266-wiki/wiki

### Build the project  

```shell
$ make
```

### Connect the board to pc

You will need 3.3V cable like [USB-SERIAL-CABLE-F](https://www.olimex.com/Products/Components/Cables/USB-Serial-Cable/USB-Serial-Cable-F/ "USB-Serial-Cable-F").

Connect cable as follows:
* blue --> uext pin-2
* green --> uext pin-3
* red --> ext pin-4

Connect the cable to USB port. You should see the new device in /dev/tty*, for example /dev/ttyUSB0 if you see other update makefile properly.

### Upload the project

If you have build the project without errors you can upload the code to ESP8266 with these commands:

```shell
make flash
```
The upload takes 2 cycles. 

Now everything is complete and you will see the board GREEN LED to flash slowly.
