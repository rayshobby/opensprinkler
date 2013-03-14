#!/usr/bin/python

import time
import sys
import string
import datetime
import RPi.GPIO as GPIO
import atexit

try:
  from xml.etree import ElementTree # for Python 2.5 users
except ImportError:
  from elementtree import ElementTree
import gdata.calendar
import gdata.calendar.service

# ======================================================
# !!! MODIFY THE CALENDAR ID AND STATION NAMES BELOW !!!
# ======================================================

# PUBLIC GOOGLE CALENDAR ID
# - the calendar should be set as public
# - calendar id can be found in calendar settings
# - !!!!!!!! PLEASE CHANGE THIS TO YOUR OWN CALENDAR ID !!!!!!
CALENDAR_ID = 'ma2lg95i25jantdiciij85aq0s@group.calendar.google.com'

# STATION NAMES
# - specify the name : index for each station
# - station index starts from 0
# - station names are case sensitive
# - you can define multiple names for each station

STATIONS = {
  "master" 		: 0,

  "front yard"		: 1,  # you can map multiple common names
  "frontyard"		: 1,  # to the same station index

  "back yard"		: 2,
  "backyard"		: 2,

  "s04"	: 3,
  "s05" : 4,
  "s06" : 5,

  "s09" : 8,
  "s10" : 9,

  "s16" : 15
}

# ======================================================

# MAXIMUM NUMBER OF STATIONS
MAX_NSTATIONS = 64

# OSPI PIN DEFINES
pin_sr_clk =  4
pin_sr_noe = 17
pin_sr_dat = 21 # NOTE: if you have RPi rev.2, change this to 27
pin_sr_lat = 22

calendar_service = gdata.calendar.service.CalendarService()
query = gdata.calendar.service.CalendarEventQuery(CALENDAR_ID, 'public', 'full')
query.orderby = 'starttime'
query.singleevents = 'true'
query.sortorder = 'a'
station_bits = [0]*MAX_NSTATIONS

def enableShiftRegisterOutput():
    GPIO.output(pin_sr_noe, False)

def disableShiftRegisterOutput():
    GPIO.output(pin_sr_noe, True)

def shiftOut(station_bits):
    GPIO.output(pin_sr_clk, False)
    GPIO.output(pin_sr_lat, False)
    for s in range(0,MAX_NSTATIONS):
        GPIO.output(pin_sr_clk, False)
        GPIO.output(pin_sr_dat, 1 if (station_bits[MAX_NSTATIONS-1-s]==1) else 0)
        GPIO.output(pin_sr_clk, True)
    GPIO.output(pin_sr_lat, True)


def runOSPI():

  global station_bits
  now = datetime.datetime.utcnow();
  print datetime.datetime.now();
  nextminute = now + datetime.timedelta(minutes=1)

  query.start_min = now.isoformat()
  query.start_max = nextminute.isoformat()

  station_bits = [0]*MAX_NSTATIONS;
  try:
    feed = calendar_service.CalendarQuery(query)
    print '(',
    for i, an_event in enumerate(feed.entry):
      if (i!=0):
        print ',',
      try:
        print an_event.title.text,
        station_bits[STATIONS[an_event.title.text]] = 1;
      except:
        print "-> #name not found#",
#      print '%s' % (an_event.title.text)
    print ')'
  except:
    print "#error getting calendar data#"
  try:
    shiftOut(station_bits)
  except:
    print "#shiftOut error#"
      
def main():
  print('OpenSprinkler Pi has started...')

  GPIO.cleanup()
  # setup GPIO pins to interface with shift register
  GPIO.setmode(GPIO.BCM)
  GPIO.setup(pin_sr_clk, GPIO.OUT)
  GPIO.setup(pin_sr_noe, GPIO.OUT)
  disableShiftRegisterOutput()
  GPIO.setup(pin_sr_dat, GPIO.OUT)
  GPIO.setup(pin_sr_lat, GPIO.OUT)

  shiftOut(station_bits)
  enableShiftRegisterOutput()

  while True:
    try:
      runOSPI()
    except:
      pass
    time.sleep(60)  # check every 60 seconds
 
def progexit():
  global station_bits
  station_bits = [0]*MAX_NSTATIONS
  shiftOut(station_bits)
  GPIO.cleanup()

if __name__ == "__main__":
  atexit.register(progexit)
  main()
