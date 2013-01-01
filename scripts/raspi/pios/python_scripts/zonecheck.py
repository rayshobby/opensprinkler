#!/usr/bin/env python
"""A daemon program for the Raspberry PI to theck the valve state of an 
OpenSprinkler irrigation controller. The output files are used by 
Sprinklers.php to display an irrigation log.
"""

import sys, os
import urllib2
import datetime
import time

local_datetime = datetime.datetime.now()
now = local_datetime.strftime("%Y-%m-%d %H:%M:%S")
changes = open("/var/www/irrlog/SprinklerChanges.txt", "a")
changes.write("System Restart--" + now + "\n")
changes.close()

def main():
    """ Daemon main routine """
    f_prev = open("/var/www/irrlog/SprinklerPrevious.txt", "r")
    old_state = f_prev.read
    f_prev.close

    while 1: #daemon main loop
        state = urllib2.urlopen("http://192.168.1.55/sn0").read()
        local_datetime = datetime.datetime.now()
        now = local_datetime.strftime("%Y-%m-%d %H:%M:%S")
        if state != old_state:
            changes = open("/var/www/irrlog/SprinklerChanges.txt", "a")
            changes.write(state + "--" + now + "\n")
            changes.close()
            old_state = state
        f = open("/var/www/irrlog/SprinklerPrevious.txt", "w")
        f.write(state + "\n") 
        f.close() 
        time.sleep(10)

if __name__ == "__main__":
    try: 
        pid = os.fork() 
        if pid > 0:
            # exit first parent
            sys.exit(0) 
    except OSError, e: 
        print >>sys.stderr, "fork #1 failed: %d (%s)" % (e.errno, e.strerror) 
        sys.exit(1)

    # decouple from parent environment
    os.chdir("/") 
    os.setsid() 
    os.umask(0) 

    # start the daemon main loop
    main()
