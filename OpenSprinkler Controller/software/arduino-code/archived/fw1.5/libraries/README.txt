====== Updates ======
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

====== How to use the code ======
Copy this folder to the Arduino's libraries directory (or make a symbolic link there).

