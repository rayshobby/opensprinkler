OpenSprinkler v1.4 (minor update)

Changes from v1.3
=================
- Added DS1307 real-time clock (RTC) and button cell battery (CR1220).

- Changed a couple of pin connections for the shift register and LCD. D3 is now used for rain sensor (can be used for other purpose if not using a rain sensor). Analog pins A2 and A3 are free; also, if not using RFM12B transceiver, digital pins D2 and D10 are free.

- Added SMT (surface mount) version 1.4s. The SMT version has two additional analog pins A6 and A7 due to the surface mount version of ATmega328. The RJ45 Ethernet jack is changed to use HanRun H911105A.


