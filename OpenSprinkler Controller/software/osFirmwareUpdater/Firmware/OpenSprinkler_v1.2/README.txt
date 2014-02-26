=======================
=== !!!IMPORTANT!!! ===
=======================

*DO NOT* download .hex files directly from GitHub page by using 'Save File As...' or 'Save Link As...'. Instead, download the entire package as a zip file, or alternatively download individual files using the 'Raw' file link.

*********************************
*********************************
This firmware is for hardware 1.2
*********************************
*********************************

-------- This folder contains compiled .hex binary files for OpenSprinkler v1.2u --------

OpenSprinkler v1.2u has on-board USB TinyISP programmer. Basic instructions are as follows:

- Insert a USB cable to OpenSprinkler's USB port.

- To flash, use command:
: avrdude -c usbtiny -p m328p -F -U flash:w:xxx.hex

where xxx.hex is the name of the firmware file you want to upload.

Additional details (including how to install avrdude and USBtiny driver) can be found at:
http://rayshobby.net/?page_id=732


