============================================
OpenSprinkler Pi (OSPi) Google Calendar Demo
Feb 2013, http://rayshobby.net
============================================

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
This demo makes use of a public Google calendar to program water events. To do so, you need to first create a public Google calendar, set the calendar ID in the Python program, then the program will periodically queries the calendar to check which stations are scheduled to be on at the current time. Each calendar event is treated as a water program, where the title of the event is the station name.

Using the Google calendar provides the advantage that you can easily set recurring events. Also, you can access Google calendar on both desktop and mobile computers. It is a convenient and suitable interface for programming complex water schedules.


Preparation
-----------
1. Before running this demo, it is assumed that you have the RPi.GPIO Python module installed (which should come by default with the latest raspbian distribution).


2. In addition, you need to install the Google Data (gdata) API Python module, available at:
http://code.google.com/p/gdata-python-client/downloads/list


3. As mentioned above, you need to create a Google calendar, set it to be public, and copy the calendar ID (available in the calendar settings page) to the correpsonding location in ospi_gc.py. For exmaple:

CALENDAR_ID = 'ma2lg95i25jantdiciij85aq0s@group.calendar.google.com'

The reason to set the calendar public is to avoid the complexity of account authentication. If you need, you can also use a private calendar, but in that case you should follow the Google Data API for the appropriate authentication procedure.


4. Next, set desired station names in ospi_gc.py. For each station, provide the station name and the corresponding station index (the first station is indexed 0). The station name will be used to match the Google calendar event title, and if a match is found, the corresponding station will be set to open. For example, an event named 'Front Yard' scheduled for every Monday morning from 9am to 10am will trigger the Front Yard station weekly during the designated time.

Note that the station names are ***case sensitive***. So 'Front Yard' is different from 'Front yard'. Also, 'Front Yard' is different from 'FrontYard'. So be careful, otherwise the program will not be able to match the calendar events correctly.

Note that you can map multiple names to the same station. For example, map 'Front Yard', 'FrontYard', 'Front_Yard' all to index 1. This can help avoid mismatches if you don't remember the name exactly.


Running the Demo
----------------
To run the demo, type in command line:
> sudo python ospi_gc.py

The program will check the specified Google calendar once every minute (60 seconds). The frequency can be increased or decreased by modifying the sleep() time in the source code. All events that overlap with the current time will trigger the corresponding stations.


Error Handling
--------------
If the program cannot access Google calendar (e.g. due to lost connection), it will reset all stations to prevent them from potentially running for a long time. Also, if an event title has no matched station name, that event will be ignored and will not trigger any station. 

The program also outputs the time stamp and the detected stations. This can be used as an event log.

Because the program requires Internet connection to run properly, it is not suitable in cases that the network is unreliable. It is possible to introduce a data cache to pre-load the events whenever the network is available. 

