=============================================
OpenSprinkler Pi (OSPi) Interval Program Demo
May 2013, http://rayshobby.net
=============================================


**********
**UPDATE**
**********

-----------------------------------------------------------------------------------------
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

