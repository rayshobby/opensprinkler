#!/usr/bin/python

import time
import sys
import string
import datetime
import RPi.GPIO as GPIO
from signal import signal, SIGTERM
from sys import exit
import atexit
import smtplib
from email.mime.text import MIMEText
import os
import httplib2
from apiclient.discovery import build

try:
  from xml.etree import ElementTree # for Python 2.5 users
except ImportError:
  from elementtree import ElementTree
import gdata.calendar
import gdata.calendar.service

import logging
import logging.handlers
import sys

LOG_LEVEL = os.getenv('OSPI_LOG_LEVEL');
if LOG_LEVEL is None:
   LOG_LEVEL = 'INFO';

# assuming loglevel is bound to the string value obtained from the
# command line argument. Convert to upper case to allow the user to
# specify --log=DEBUG or --log=debug
numeric_level = getattr(logging, LOG_LEVEL.upper(), None)
if not isinstance(numeric_level, int):
    numeric_level = getattr(logging, 'INFO');
 
mylogger = logging.getLogger("ospi_gc");
mylogger.setLevel(numeric_level);
handler = logging.handlers.TimedRotatingFileHandler(
   "ospi_gc.log", 
   when='midnight', 
   backupCount=14
);

formatter = logging.Formatter('%(asctime)s:%(levelname)s: %(message)s');
handler.setFormatter(formatter);
mylogger.addHandler(handler);

mylogger.info("Logging configured: numeric_level=%d LOG_LEVEL=%s", numeric_level, LOG_LEVEL);

EMAIL_FROM = os.getenv('OSPI_EMAIL_FROM');
EMAIL_TO = os.getenv('OSPI_EMAIL_TO');
mylogger.info("Will send email from %s to %s", EMAIL_FROM, EMAIL_TO);

def sendemail(subject, msgtext):
  if EMAIL_FROM is None or EMAIL_TO is None:
     mylogger.debug("Email not configured. Not Sending Message with Subject: %s", subject);
     return;

  msg = MIMEText(msgtext);
  msg['Subject'] = subject;

  s = smtplib.SMTP('localhost');
  s.sendmail(EMAIL_FROM, EMAIL_TO, msg.as_string());
  s.quit();

CALENDAR_ID = os.getenv('OSPI_CALENDAR_ID');
if (CALENDAR_ID is not None):
   mylogger.info("Will use calendar %s", CALENDAR_ID);
else:
   mylogger.error("OSPI_CALENDAR_ID environment variable not set. ABORTING");
   sendemail("OSPI GC Startup Aborted", "OSPI_CALENDAR_ID environment variable not set");
   exit(1);

API_KEY = os.getenv('OSPI_API_KEY');
if (API_KEY is not None):
   mylogger.debug("Will use Google API key %s", API_KEY);
else:
   mylogger.error("OSPI_API_KEY environment variable not set. ABORTING");
   sendemail("OSPI GC Startup Aborted", "OSPI_API_KEY environment variable not set");
   exit(1);

MAX_STATION_ENV = os.getenv('OSPI_MAX_STATION');
if (MAX_STATION_ENV is not None):
   MAX_STATION = int(MAX_STATION_ENV);
   mylogger.debug("Will look for OSPI_STATION_0 to OSPI_STATION_%d", MAX_STATION);
else:
   mylogger.error("MAX_STATION environment variable not set. ABORTING");
   sendemail("OSPI GC Startup Aborted", "MAX_STATION environment variable not set");
   exit(1);

STATIONS = {};

for i in range(0, MAX_STATION+1):
   STATION_ENV = "OSPI_STATION_%d" % (i);
   stations_names = os.getenv(STATION_ENV);
   if stations_names is None:
      continue;
   mylogger.info("Will reference station %d as %s", i, stations_names);
   for name in stations_names.split(','):
      STATIONS[name] = i;

if (len(STATIONS) == 0):
   mylogger.ERROR("No Stations Configured. ABORTING");
   sendemail("OSPI GC Startup Aborted", "OSPI_STATION_* variables not set or incorrect format");
   exit(1);

try:
   calendar_service = build('calendar', 'v3');
except:
   mylogger.exception("Error while building calendar service. ABORTING");
   sendemail("OSPI GC Startup Aborted", "Error while building calendar service");
   exit(1);

   
# ======================================================

# MAXIMUM NUMBER OF STATIONS
MAX_NSTATIONS = 64

# OSPI PIN DEFINES
pin_sr_clk =  4
pin_sr_noe = 17
pin_sr_dat = 27 # NOTE: if you have RPi rev.1, change this to 21
pin_sr_lat = 22

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
  try:
     global station_bits
     now = datetime.datetime.utcnow();
     removems = datetime.timedelta(microseconds=now.microsecond);
     now = now - removems
     nextminute = now + datetime.timedelta(minutes=1)

     TIME_MIN = now.isoformat() + "Z";
     TIME_MAX = nextminute.isoformat() + "Z";

     station_bits = [0]*MAX_NSTATIONS;
  except:
     mylogger.exception("ERROR during time setup");
  try:
    mylogger.debug("checking calendar...");
    calendar_events = calendar_service.events().list(
       calendarId=CALENDAR_ID, 
       key=API_KEY,
       timeMin=TIME_MIN,
       timeMax=TIME_MAX,
       orderBy='startTime',
       singleEvents='true'
    ).execute();
    stations = "";
    for event in calendar_events['items']:
      mylogger.debug("%s from %s to %s", 
         event['summary'], 
         event['start']['dateTime'],
         event['end']['dateTime']
      );
      if (stations != ""):
        stations += ',';
      try:
        stations += event['summary'];
        station_bits[STATIONS[event['summary']]] = 1;
      except:
        stations += "-> #name not found#",
    if (len(stations) > 0):
      mylogger.info("stations found: %s", stations);
    else:
      mylogger.debug("no stations found");
  except:
    mylogger.exception("#error getting calendar data#");
  try:
    shiftOut(station_bits)
  except:
    mylogger.error("#shiftOut error#");


      
def main():
      
  mylogger.info('OpenSprinkler Pi has started...')

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

  sendemail('OpenSprinkler GC Startup', 'Initialization complete. Starting main loop')

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
  mylogger.info('OpenSprinkler Pi shutdown... All station bits cleared')
  sendemail('OpenSprinkler GC Shutdown', 'All station bits cleared');

if __name__ == "__main__":
  atexit.register(progexit)
  # Normal exit when killed
  signal(SIGTERM, lambda signum, stack_frame: exit(1))
  main()
