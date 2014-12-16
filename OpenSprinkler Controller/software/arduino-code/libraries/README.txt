====== Updates ======
The list here documents updates to the OpenSprinkler library. Updates to specific programs are documented in each program folder.

===============================
* Dec 15, 2014
- Check in firmware 2.1.2 source code. This firmware fixed the following bugs in 2.1.1: 1) program data output (/jp) is incomplete when the number of programs is a multiple of 3 (likely a compiler bug); 2) incorrect handling of the SPACE character in program names. For details please refer to the development thread at https://github.com/OpenSprinkler/OpenSprinklerGen2.

* Dec 2,  2014
- Check in firmware 2.1.1 source code. This firmware fixed a few bugs in 2.1.0, and added support for radio frequency (RF) stations, support for per-station 'sequential' flag, water percentage logging, negative station delay time, hardware MAC address, and 'enable logging' option. For intermediate changes please refer to the development thread at https://github.com/OpenSprinkler/OpenSprinklerGen2.


* Oct 22, 2014
- Check in firmware 2.1.0 source code. This firmware is a major upgrade, adding features such as per-station water time, automatic weather-based water time adjustment, auto-detection of time zone and DST. For intermediate changes please refer to the development thread at https://github.com/OpenSprinkler/OpenSprinklerGen2.

* Sep 3, 2014
- Check in firmware 2.0.9 source code. This firmware fixes a major bug in 2.0.8 which causes issues when a SPACE character appears in the station name or location. This firmware also refined cache directives in the HTTP headers.

* Aug  25, 2014
- Check in firmware 2.0.8 source code. This firmware adds support for relay, logging rain sensor / rain delay status, and the default UI has been changed to Samer's mobile UI.

* July 23, 2014
- Check in firmware 2.0.7 source code. To compile the source code, you need to download the SdFat library (https://code.google.com/p/sdfatlib/) and install it to your arduino\libraries folder.

* July 5, 2014
- Check in firmware 2.0.7. This firmware improves SD card support by switching to use the Arduino SdFat lbirary, which eliminates the restriction of 2GB card and FAT16 formatting. With this firmware, you can use microSD card of any size, formatted to either FAT16 or FAT32. The SdFat library also supports creating sub-diectories.
- Added watchdog timer to automatically restart the controller after a certain timeout (2 minutes by default).
- Added support to auto-detect the number of expansion boards. Specifically, in the 'Options' page, the '# of exp boards' option now shows the number of auto-detected expansion boards.

* June 13, 2014
- Check in firmware 2.0.6. This firmware adds logging capability (requires a microSD card, 2GB or less, formatted to FAT16). The logging is supported by opensprinkler mobile app.
  Technically, a log record is written to SD card every time a valve is closed. The record is written to a file named xxxx.txt where xxxx is the epoch time in days.
  For example, all records generated on June 13, 2014 are written into 16234.txt. You can directly check the log file by http://x.x.x.x/16234.txt.
- Minor improvement on the auto-reconnect feature to avoid controller getting stuck.

* May 7, 2014
- Check in firmware 2.0.5. This firmware adds per-station 'ignore rain' bit, which allows a station (including Master) to ignore rain delay / rain sensing. Note that this does not affect manual mode or run-once program -- in other words, when the controller is in manual mode or executing a run-once program, stations will run regardless of rain delay or ignore rain bit.

* Feb 24, 2014
- Check in firmware 2.0.4. This firmware is for hardware v2.0 and v2.1 only.

* Jan 5, 2014
- Check in firmware 2.0.3. This firmware is for hardware v2.0 and v2.1 only.

* Jun 18, 2013
- Check in firmware 2.0.0. This firmware (for hardware v2.0 only) is equivalent to firmware 1.8.3 (for hardware v1.x).

* Feb 09, 2013
- Check in firmware 1.8.3. This firmware adds back the 'sequential' option (which was available in firmware 1.7 but disabled in 1.8). It sets the controller to run in sequential or concurrent mode. 
- Added a new 'device_id' option, so that multiple controllers can be used on the same network. This is done by assigning 'device_id' as the last byte of the controller's mac address.

* September 26, 2012
- Removed support for parallel (concurrent) running mode due to a temporarily unsolvable bug.

* September 13, 2012

- Update OpenSprinkler library. Firmware number changed to 1.8. 
- Added support for custom station names. The maximum length of a name is 12 characters
- Added full range of time zones, such as GMT+5:30GMT, GMT+5:45
- Added per-station master operation bit (i.e. whether a station will activate master station or not)
- Added support for Run-Once program. A Run-Once program will interrupt the current program, turn on each station for a specified amount of time and return back to normal program mode after it's completed.
- Added DS1307 RTC support, and automatic detection of DS1307.
- Reworked options data structure to save space and make it easy to add new options.
- The following options are added:
  * USE_NTP (auto time sync using NTP)
  * HTTPPORT (http port)
  * NETFAIL_RECONNECT (auto reconnect upon network failure)
  * STATION_DELAY_TIME (a delay time between two consecutive station runs, up to 240 seconds)
  * MASTER_ON_ADJ (adjusted time to turn on master after a station opens, 0 to 60 seconds)
  * MASTER_OFF_ADJ (adjusted time to turn off master after a station closes, -60 to 60 seconds)
  * WATER_LEVEL (scaling factor for water duration time, 0 to 250%)
  * SELFTEST_TIME (time to turn on each station during self-test, up to 255 seconds)
  * IGNORE_PASSWORD (ignore web password)
  Refer to OpenSprinkler.cpp and online instructions for the meaning of these options
- Changed to use the latest EtherCard library, with name support. Now you can access the controller webpage by 'http://opensprinkler'
- Added 'network_fails' status and lcd_print_status function (which prints out the number of network failures so far).

===============================
* August 14, 2012
- Update OpenSprinkler library. Firmware version number is now 1.7. Major changes are added support for DS1307 RTC, two new options (station delay time and rain sensor type).
- Updated code to work with Arduino 1.0 and above. Arduino 0023 is still recommended because it produces binary code that's 1KB smaller than Arudino 1.0.
- Modified internal EEPROM layout to make space for storing station names (not implemented yet).
- Fixed clear_station_bits to be consistent with apply_station_bits.

===============================
* June 19, 2012
 - Added manual_mode_on and manual_mode_off functions.

===============================  
* June 17, 2012
 - Updated OpenSprinkler library. Version number is now 1.6. Major changes are added data structures, modified options (including a sequential option, rainsensor, and rtc option), added location string to prepare for the weather feature.
 - Modified EtherCard::emit_p to print unsigned long values.

=============================== 
* May 25, 2012
 - Update the EtherCard files to use JeeLab's latest library code. Improved DHCP robustness.
 - Added boards.txt for v1.2u on-board USBtiny programmer.

===============================
* May 17, 2012
 - Update the library with new Wire class
 - Added hardware definition macro. Before using the library, please define your hardware version in 'defines.h'
 - Added svc_interval_schedule program. Work still in progress.
 - Moved example programs into the 'examples' folder, making it easier to load an example program.
 
===============================
* Apr 8, 2012
 - added support for master station: the master station will be turned on if there any other station is on. any station on the master controller (stations 1-8) can be assigned as a master station. the default value is 0, which means no master station is assigned. 

===============================
* Mar 25, 2012 
 -Merged all required libraries to the same folder.
 -Crated 'OpenSprinkler' class.
 -Modified button control during powering up: hold B1 during powerup actives a self-test program (each station is turned on for 5 seconds, including all extended stations); B2 activates reset; B3 activates the default option setup.
 -changed reset value of external EEPROM to 0xFF so that it's consistent with un-initialized EEPROM.


