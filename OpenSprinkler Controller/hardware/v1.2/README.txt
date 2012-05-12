OpenSprinkler v1.2 (minor updates)

Changes from v1.1
=================
- Added on-board USBtiny programmer (based on pre-programmed attiny45) and a USB connector. This allows OpenSprinkler to be programmed without the need of external FTDI programmer.

- Some components have been changed to adopt more common parts. Examples are LM2574N (now using MC34063), RJ45 Ethernet Jack (now using SparkFun part), 16x2 LCD (now using 1602K).

- Added a rain sensor terminal, and a pin header for sensing power loss.

- Changed a pin assignment for the LCD in order to free up analog pin A1. This analog pin can be used to connect external sensors.

- Some components are removed to make space for the above changes. These includes the power barrel, the FTDI header, and the reset button.

- Extension board connector has been changed to a 2x3 pin format. Also, there are now both an EXTIN and EXTOUT connector, to allow cascading arbitrary number of boards. 

