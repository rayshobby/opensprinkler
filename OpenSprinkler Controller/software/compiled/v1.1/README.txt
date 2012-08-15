=======================
=== !!!IMPORTANT!!! ===
=======================
*DO NOT* download .hex files directly from GitHub page by using 'Save File As...' or 'Save Link As...'. Instead, download the entire package as a zip file, or alternatively download individual files using the 'Raw' link.

-------- This folder contains compilex .hex files for OpenSprinkler v1.0/v1.1 --------

OpenSprinkler v1.1/v1.0 does not have on-board USB programmer. If you want to directly flash an hex file, you need an external ISP programmer, such as USBtiny, or USBasp. Instructions are as follows:

- To begin, you need to solder the 2x3 ISP pinheader to the PCB (on v1.1 it is located close to the Ethernet connector, on v1.0 it is at the center of the PCB). 

- Plug in the ISP programmer connector. Make sure the red strip on the ribbon cable matches the white stripe on the PCB.

- To flash, use  command (if you have USBtiny programmer):
: avrdude -c usbtiny -p m328p -F -U flash:w:xxxxx.hex

or (if you have USBasp programmer)

: avrdude -c usbasp -p m328p -F -U flash:w:xxxxx.hex

