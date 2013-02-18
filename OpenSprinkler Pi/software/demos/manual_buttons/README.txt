===========================================
OpenSprinkler Pi (OSPi) Manual_Buttons Demo
Feb 2013, http://rayshobby.net
===========================================

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


