====== Updates ======
The list here documents updates to the OpenSprinkler library. Updates to specific programs are documented in each program folder.

* August 14, 2012
- Update OpenSprinkler library. Firmware version number is now 1.7. Major changes are added support for DS1307 RTC, two new options (station delay time and rain sensor type).
- Updated code to work with Arduino 1.0 and above. Arduino 0023 is still recommended because it produces binary code that's 1KB smaller than Arudino 1.0.
- Modified internal EEPROM layout to make space for storing station names (not implemented yet).
- Fixed clear_station_bits to be consistent with apply_station_bits.

* June 19, 2012
 - Added manual_mode_on and manual_mode_off functions.
  
* June 17, 2012
 - Updated OpenSprinkler library. Version number is now 1.6. Major changes are added data structures, modified options (including a sequential option, rainsensor, and rtc option), added location string to prepare for the weather feature.
 - Modified EtherCard::emit_p to print unsigned long values.
 
* May 25, 2012
 - Update the EtherCard files to use JeeLab's latest library code. Improved DHCP robustness.
 - Added boards.txt for v1.2u on-board USBtiny programmer.

* May 17, 2012
 - Update the library with new Wire class
 - Added hardware definition macro. Before using the library, please define your hardware version in 'defines.h'
 - Added svc_interval_schedule program. Work still in progress.
 - Moved example programs into the 'examples' folder, making it easier to load an example program.
 
* Apr 8, 2012
 - added support for master station: the master station will be turned on if there any other station is on. any station on the master controller (stations 1-8) can be assigned as a master station. the default value is 0, which means no master station is assigned. 

* Mar 25, 2012 
 -Merged all required libraries to the same folder.
 -Crated 'OpenSprinkler' class.
 -Modified button control during powering up: hold B1 during powerup actives a self-test program (each station is turned on for 5 seconds, including all extended stations); B2 activates reset; B3 activates the default option setup.
 -changed reset value of external EEPROM to 0xFF so that it's consistent with un-initialized EEPROM.


