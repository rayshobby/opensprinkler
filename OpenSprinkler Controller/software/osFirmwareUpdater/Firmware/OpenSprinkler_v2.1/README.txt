=======================
=== !!!IMPORTANT!!! ===
=======================

*DO NOT* download .hex files directly from GitHub page by using 'Save File As...' or 'Save Link As...'. Instead, download the entire package as a zip file, or alternatively download individual files using the 'Raw' file link.

***************************************
***************************************
This firmware is for hardware 2.1 only.
It will NOT work on other versions.
***************************************
***************************************

OpenSprinkler v2.1 and v2.0 use the same injection-molded enclosure, and both have the ATmega644 microcontroller. However, the microcontrollers run at different frequencies:

* V2.1 runs at 12MHz with internal USBasp bootloader. Its 24VAC terminal block (on the left side of the controller) is orange colored.
* V2.0 runs at 8MHz with external USBtiny programmer. Its 24VAC terminal block (on the left side of the controller) is green colored.

Therefore you must use the firmware that corresponds to your hardware version. 

-------- This folder contains compiled .hex binary files for OpenSprinkler v2.1 --------

OpenSprinkler v2.1 has built-in USBasp programmer. Basic instructions are as follows:

- Power off the controller.

- Enter bootloader mode by pressing and holding the second pushbutton (B2) while inserting a USB cable to OpenSprinkler's USB port. Release button B2 after a few seconds.

- The LCD screen should remain off. If the LCD screen turns on, repeat the above step until the controller enters bootloader mode.

- To flash, use command:
: avrdude -c usbasp -p m644 -U flash:w:xxx.hex

where xxx.hex is the name of the firmware file you want to upload.

Additional details (including how to install avrdude and USBtiny driver) can be found at:
http://rayshobby.net/?page_id=732

