VUSBtiny Source Code for ATtiny45
=================================
This folder contains the source code for a USBtiny programmer implemented with an ATtiny45.

The original source was from the following two links:
http://tequals0.wordpress.com/2011/09/26/attiny45-based-usbtinyisp-programmer/
http://www.simpleavr.com/avr/vusbtiny/

The source code was slightly changed to use clock parameter '-B 1' by default with avrdude. This allows the USBtiny programmer to program a chip at the fastest clock speed when used with Arduino (there doesn't seem to be a way to set the -B parameter in Arduino).

=================================
Program ATtiny45:

vusbtiny.hex is the pre-compiled firmware. To burn it to a blank ATtiny45, you need an external AVRISP programmer. The following two batch files are examples of using Adafruit's USBtinyISP programmer:
in Linux, run
> sudo sh tiny.sh
or in Windows, run
> tiny.bat

****** Warning ******: once the firmware is burned, you will not be able to re-program the ATtiny45 with an AVRISP programmer anymore, since the above commands will change the fuse bits of ATtiny45 and disable the RESET pin. As a result, if you ever need to re-program the ATtiny45 again, you need to use a high-voltage parallel programmer, or a TinyRescue board to recover the fuse bits.

=================================
Program a target MCU:

Since the default transfer clock has been set to the fastest, it requires the target MCU to run at a minimum of 8MHz speed to be compatible, otherwise, you need to add the '-B 250' parameter in avrdude in order to slow down the clock. For example, a blank chip (such as ATmega328 or ATmega644) typically runs at 1MHz by factory default. In order to program a blank chip, you either need to add the '-B 250' paramter, or program the target mcu's fuse bits to increase its speed to 8MHz. For example:

Run:
> avrdude -c usbtiny -p m328p -B 250 -F -U lfuse:w:0xe2:m -U hfuse:w:0xd1:m -U efuse:w:0x06:m
to set a blank ATmega328 chip to run on 8MHz internal clock speed.
then:
> avrdude -c usbtiny -p m328p -B 1 -F -U flash:w:firmware1.8.3.hex
to upload a program (e.g. firmware1.8.3.hex)

=================================

