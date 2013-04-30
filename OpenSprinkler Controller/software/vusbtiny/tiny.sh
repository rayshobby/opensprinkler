#!/bin/sh
avrdude -c usbtiny -p t45 -F -U flash:w:vusbtiny.hex
avrdude -c usbtiny -p t45 -U lfuse:w:0xe1:m -U hfuse:w:0x5d:m -U efuse:w:0xff:m
