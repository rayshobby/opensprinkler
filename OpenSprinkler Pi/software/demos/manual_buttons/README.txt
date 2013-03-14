===========================================
OpenSprinkler Pi (OSPi) Manual_Buttons Demo
Feb 2013, http://rayshobby.net
===========================================

SPECIAL NOTE
------------
This was discovered by Ric on the forum:

If you have RPi rev.1, use the following GPIO defines:
pin_sr_clk =  4
pin_sr_noe = 17
pin_sr_dat = 21
pin_sr_lat = 22

If you have RPi rev.2, change the pin_sr_dat to:
pin_sr_dat = 27

Please check the following post for details:
http://rayshobby.net/phpBB3/viewtopic.php?f=28&t=51

Also the pin names listed here:
http://elinux.org/RPi_Low-level_peripherals#General_Purpose_Input.2FOutput_.28GPIO.29

Description
-----------
This demo starts a Python HTTP server, which presents a simple webpage with a list of buttons, each corresponding to a stations. Clicking on each button to manually turn on/off a station.

Before running this demo, it is assumed that you have the RPi.GPIO Python module installed (which should come by default with the latest raspbian distribution).

To run the demo, type in command line:
> sudo python ospi_manual.py

This will start the Python HTTP server. Next, open a browser, and type into the following url:

http://raspberrypi:8080

or alternatively:

http://x.x.x.x:8080

where x.x.x.x is the ip address of your Raspi.

You should see a list of 16 buttons. Clicking on each button to toggle the corresponding station.


Modification
------------
You can change the number of stations by modifying the 'num_stations' variable in ospi_manual.py. 

The code consists of two files: ospi_manual.py runs the HTTP server and maintains station variables; the webpage is formatted using the Javascripts in manual.js. You can follow the examples to extend the functionality of this demo.


