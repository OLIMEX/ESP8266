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
Clone this repository into some directory, for example **~/olimex**
```shell
mkdir ~/olimex
cd ~/olimex
git clone https://github.com/SelfDestroyer/esphttpd.git -b demo
cd esphttpd
make
```
If you see errors than your **Makefile** is not set properly. Open it and check all paths. 

Once you make the project the next step is to upload it to **ESP8288-ESP** board.

### Connect the board to pc
To connect the board all you need is serial communication cable.
Like [this](https://www.olimex.com/Products/Components/Cables/USB-Serial-Cable/USB-Serial-Cable-F/ "USB-Serial-Cable-F") or others.

Connection is the following:
* blue --> uext pin-2
* green --> uext pin-3
* red --> ext pin-4

Connect the cable to USB port. You should see the new device in /dev/tty\*, for example **/dev/ttyUSB0**.

Tell the upload program the port:
```shell
export ESPPORT=/dev/ttyUSB0
```
To go into bootloader mode, simply hold the big white button on the board and then power it up.

### Upload the project
All that left is run the upload program:
```shell
make flash
```
The upload takes 2 cycles. Depending on your OS between then it's possible the board to exit bootloader mode. 

To upload demo webpages go again into bootloader mode and run:
```shell
make htmlflash
```

That's it!
Next connecto to the board (name should be ESP_.....) and begin switching relays.