=======================
=== !!!IMPORTANT!!! ===
=======================

*DO NOT* download .hex files directly from GitHub page by using 'Save File As...' or 'Save Link As...'. Instead, download the entire package as a zip file, or alternatively download individual files using the 'Raw' link.

**************************************
**************************************
This firmware is for hardware 2.0 only
It will not work on hardware 1.x
**************************************
**************************************

=== Update since 2.0.1 ===
- Added code to return controller variables and program / station data in JSON:
  /jo: returns options
  /jc: returns controller variables
  /jp: returns program data
  /jn: returns station names and data
- Stores controller operation enable bit, manual mode bit, rain delay time in EEPROM
- More options are made editable through the web interface
- Added support to change time manually (enabled when NTP sync is turned off).
===========================

This firmware (2.0.2) uses the microSD card available on OpenSprinkler 2.0 to store and serve Javascripts required for rendering OpenSprinkler webpages. To use this firmware, please read the instructions below:

* A microSD (uSD) card of 2.0 GB or below (NOTE: cards with more than 2.0 GB capacity are not supported)
  Here is an inexpensive uSD card that comes with an adapter and makes it easy to copy file in the next step:
  http://www.amazon.com/gp/offer-listing/B000PC62O6/
  
* Copy all the files in the 'copy_to_sd' folder to your uSD card's root directory. 
  It's important that these files are in the root directory, not in a subdirectory!
  
* Before inserting the uSD card to OpenSprinkler, flash the new firmware first, by following the README.txt in the folder one level above.
  =======================
  === !!!IMPORTANT!!! ===
  =======================
  - Flashing a new firmware to your OpenSprinkler will erase all settings and program data.
    Make sure you record or back up your settings and data before re-flashing.

  - There is a design flaw in the current OpenSprinkler 2.0 hardware which causes the uSD
    card to potentially interfere with the flashing process. If you encounter a 'verification
    error' during the flashing process, it is caused by the interference. The quick solution
    is to remove the uSD card during flashing, and insert it back in afterwards.
    The better solution is to add a 10K pullup resistor (to Vcc) on the uSD card slot's CS pin.


* Once the controller starts, if the uSD card is detected successfully, you will see an SD card
  icon displayed at the right end of the LCD screen. It looks like this:
  ┌────┐
  ├────┤
  │   ┌┘
  └───┘  If you don't see this icon, check to make sure your uSD card is inserted securely.
