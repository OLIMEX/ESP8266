README
======

These apps accomplish the following:

-- ESP8266-EVB-blinkLED : Blink by green LED on ESP8266-EVB

-- arduino_style :

-- esphttpd : Advanced web-server for ESP8266-EVB (Config, Rele, Button).

## Build toolchain

https://github.com/pfalcon/esp-open-sdk is easy to use, it basically does all on its own.

- Clone the repository to ```/opt/Espressif``` (or somewhere else, this path seems to be preferred by some people)
- Install dependencies: ```sudo apt-get install make unrar autoconf automake libtool gcc g++ gperf flex bison texinfo gawk ncurses-dev libexpat-dev python sed```
- ```make```, wait a few minutes, done.
- Add ```export PATH="${PATH}:/opt/Espressif/xtensa-lx106-elf/bin``` to your .bashrc, .profile, .zshrc, whatever. Or just run it every time you want to use the toolchain.

In some versions of the SDK there seems to be a problem with some type definitions. If you have problems compiling with errors that some types are not defined try opening ```xtensa-lx106-elf/xtensa-lx106-elf/sysroot/usr/include/c_types.h``` and replace ```#if 0``` near the top with ```#if 1```.

## Get esptool

Some projects use ```esptool```, which is something completely different as ```esptool.py``` and which is not installed by ```esp-open-sdk```. You can get ```esptool``` here: 
* https://github.com/tommie/esptool-ck 

or here: 
* https://github.com/themadinventor/esptool
