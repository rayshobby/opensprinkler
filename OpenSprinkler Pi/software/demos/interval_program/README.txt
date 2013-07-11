=============================================
OpenSprinkler Pi (OSPi) Interval Program Demo
May 2013, http://rayshobby.net
=============================================


**********
**UPDATES**
**********

-----------------------------------------------------------------------------------------
* July 10 2013
Bug fixes and additions:
1. Fixed a bug that prevented zones 9+ from running.
2. The Run once program was not observing the station delay setting - Fixed
3. Made the sd variable an attribute of the gv module. All references to sd... are now gv.sd... This should potentially fix several bugs, Specifically the Rain delay seems to be working properly now.
4. The Graph Programs time marker was not recognizing the time zone setting from the Options page - fixed.
5. Time displayed on the last run line of the main page was not correct - fixed.
6. Added a faveicon which will help distinguish the OpenSprinkler tabs on the browser.
7. Added an import statement and file which provide a stub for adding user written custom functions to the interval program without modifying the program itself.

* Jun 26 2013
1. Last run logging is now working for manual mode when an optional time value is selected, even if more that one station is started.
2. Fixed a bug that prevented the home page display from updating when running irrigation programs.
3. Includes a fix from Samer that allows the program preview time marker to update properly.

* Jun 20, 2013
This upsate includes:
1. Changed the way ospi.py handles time. It now uses the time zone setting from the OS options page. It also eliminates the auto daylight savings time adjustment that was causing problems for some users.
2. Fixes a bug mentioned on the forum that caused Samer's app to not update in program mode.
3. Fixes a bug that caused a program to re-start after the "Stop all stations" button was clicked.
4. A partial fix for the "last run" problems. Still need to get manual mode with an optional time setting working.
5. Added a docstring at the top of the ospi.py file with the date for version tracking.

* Jun 19, 2013
  Applied Samer Albahra's patch so that the program will work with Samer's mobile web app.
  Per forum discussion: http://rayshobby.net/phpBB3/viewtopic.php?f=2&t=154&start=40#p781
-----------------------------------------------------------------------------------------  

NOTE
----
This folder contains the interval program demo for OpenSprinkler Pi written by Dan Kimberling. It is compatible with the microcontroller-based OpenSprinkler firmware 1.8, the instructions of which can be found at:
  
  http://rayshobby.net/?page_id=730

The program makes use of web.py (http://webpy.org/) for the web interface. 

******************************************************
Full credit goes to Dan for his generous contributions
in porting the microcontroller firmware to Python.
******************************************************


PREPARATION
-----------
The demo requires RPi's GPIO library (which is included in the latest raspbian installation).

Depending on the RPi revision you have, you need to make the following changes in ospi.py:

* If you have RPi rev.1 (no screw holes), use the following GPIO defines:
  pin_sr_clk =  4
  pin_sr_noe = 17
  pin_sr_dat = 21
  pin_sr_lat = 22

* If you have RPi rev.2 (which has two PCB screw holes), change the pin_sr_dat to:
  pin_sr_dat = 27

The program assumes you have rev. 2 by default.


INSTRUCTIONS
------------
This demo is a full-featured sprinkler controller program for OpenSprinkler Pi. It replicates the microcontroller-based OpenSprinkler firmware 1.8, and makes use of web.py to run the HTTP web interface.

Before running this demo, please read the PREPARATION section above. Then follow the instructions:

1. Have an SD card with the latest Raspbian OS.
2. Download/copy the ospi.tar.gz file to the pi directory and unpack it with "tar -xvf ospi.tar.gz".
3. Set the Python code for the Pi rev you are using (see PREPARATION sectino above)
4. To run the demo, type in command line:

   > sudo python ospi.py

   This will start the Python HTTP server. Next, open a browser, and type into the following url:

   http://raspberrypi

   or alternatively:
 
   http://x.x.x.x

   where x.x.x.x is the ip address of your Raspi.

5. User manual for the full set of program features can be found at:

   http://rayshobby.net/?page_id=730

If you want the RPi to automatically run this program when it starts, please see the following:

Iinstructions copied directly from Dan's email:
================================================================
It is important to modify the /etc/rc.local file on the Pi.
Use the command "sudo nano /etc/rc.local" and add the following 4 lines just before the line "exit 0':

    host=$(hostname -I | sed 's/ *$//g')
    port=:80
    cd /home/pi/OSPi/
    ## If you aren't sure that the path to python on your Pi is /usr/bin/, use whereis python to determine the location
    /usr/bin/python ospi.py $host$port

Then use ctrl + o to save and ctrl x to exit the editor.

Reboot the pi.

This will cause the ospi.py program to launch automatically on boot so that it will re-start even if there is a power outage.

Be sure the rc.local file is executable (sudo chmod 755 /etc/rc.local). This should be default I think.

Because the automatic launch is done as root, the program runs like a daemon. You can log in to the pi as usual and do other stuff. You will not see an indication the program is running.
================================================================

