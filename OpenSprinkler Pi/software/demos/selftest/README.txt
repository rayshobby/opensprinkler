======================================
OpenSprinkler Pi (OSPi) Self-Test Demo
Feb 2013, http://rayshobby.net
======================================

Description
-----------
This demo turns on each station for 10 seconds in turn one after another. It is written in C++ using the WiringPi framework.

Before running this demo, you need to install WiringPi, available at:
https://projects.drogon.net/raspberry-pi/wiringpi/download-and-install/ 

To run the demo, type in command line:
> sudo ./ospi_selftesti

Note that if a station is running when the user presses Ctrl+C to terminate the program, that station will not reset. This is because the program does not capture the Ctrl+C event.


Modification
------------
The source code for this program is in ospi_selftest.c
