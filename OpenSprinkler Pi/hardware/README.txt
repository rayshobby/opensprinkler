OpenSprinkler Pi (OSPi) Eagle Design Files
==========================================
Note: requires Eagle CAD v6.0 or above

Part lists of all versions can be found at:
http://goo.gl/4Nrvb

Release Notes
=============
New in v1.4 / v1.4 B+
* New layout to fit the OpenSprinkler injection-molded enclosure
* Changed on-board relay to 250V / 3A type

New in v1.3
* Added per-station bidirectional TVS (SMBJ48CA)
* Added 2A solder-on fuse
* Added on-board mini-relay (120V / 2A)
* Added rain sensor terminal
* 24V AC terminal block changed to orange 3.96 spacing type

New in v1.2
* Added PCF8591T 8-bit 4-channel ADC (analog-digital converter)
* Added pin headers for ADC and also i2C pins.

New in v1.1
-----------
* Adopt LM2596S-5.0 as switching regulator, less peripheral elements, more output current.
* Improved locations of the separation pillars to fit both RPi rev. 1 and rev. 2.
  There are now two pillars that match the screw hole locations on rev. 2 .
* Changed 32.768kHz crystal to SMT package.
* Changed the value of D1 to S1B type.


Original Release v1.0
---------------------
- MC34063 switching regulator.
- DS1307 RTC with CR1220 backup battery.
- Shift register, BT1308W triac solenoid driver.


