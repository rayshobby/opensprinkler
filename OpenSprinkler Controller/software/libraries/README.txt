====== Updates ======

--- Mar 25, 2012 ---
-Merged all required libraries to the same folder.
-Crated 'OpenSprinkler' class.
-Improved button control during startup: press B1 during startup actives a self-test program; B2 activates reset; B3 activates the default option setup.
-Added a self-test program (press B1 during startup): each station will open for 5 seconds in turn.
-changed reset value of external EEPROM to 0xFF so that it's consistent with un-initialized EEPROM.

====== How to use the code ======
Copy this folder to the Arduino's libraries directory (or make a symbolic link there).

