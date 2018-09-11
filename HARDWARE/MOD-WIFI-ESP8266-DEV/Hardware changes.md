MOD-WIFI-ESP8266-DEV

Designed with Eagle CAD

---

MOD-WIFI-ESP8266-DEV hardware revision B1 - latest hardware revision - notable changes:

1. Changed C1 and C2 from 10pF to 3.9pF, because of different quartz.

---

MOD-WIFI-ESP8266-DEV hardware revision B - latest hardware revision - notable changes:

1. Removed cream layer from UEXT connector, decreased the width of ESP8266's pads
2. Added a button and R11 = 200R
3. Added NA(2K) pads for pull up resistor over GPIO4 - this is done to be capable to use the I2C-to (if you use the board with ESP8266-EVB, since it is part of EVB board then such a resistor is already placed on the EVB part). However, you can still use it without EVB if you solder the resistor.
4. Changed jumper names for better consistency with other ESP8266 products
5. Added a test pad for the RTC pin

---

MOD-WIFI-ESP8266 hardware revision A - initial release