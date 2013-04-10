OpenSprinkler v1.42 (minor update)
Changes from v1.4

- Added one 5.6V zener diode and removed the PTC fuse.
- Changed capacitor CT to 100pF in order to reduce switching regulator noise.
- Combined four 10K resistors from 1.4u to one 10Kx4resistor network.
- Switching positions of rain sensor terminal and zone expansion connector. Expansion connector now uses 2×4 right-angle male header.

=================================

OpenSprinkler v1.4 (minor update)
Changes from v1.3:

- Added DS1307 real-time clock (RTC) and button cell battery (CR1220). Removed 24LC128 external EEPROM due to PCB space constraint.
- Changed a couple of pin connections for the shift register and LCD. D3 is now used for rain sensor (can be used for other purpose if not using a rain sensor). Analog pins A2 and A3 are free; also, if not using RFM12B transceiver, digital pins D2 and D10 are free.
- Added SMT (surface mount) version 1.4s. The SMT version has two additional analog pins A6 and A7 due to the surface mount version of ATmega328. The RJ45 Ethernet jack is changed to use HanRun H911105A.

