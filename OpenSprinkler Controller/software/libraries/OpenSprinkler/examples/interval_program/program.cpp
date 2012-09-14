// Example code for OpenSprinkler

/* Program Data Structures and Functions
   Creative Commons Attribution-ShareAlike 3.0 license
   Sep 2012 @ Rayshobby.net
*/

#include <limits.h>
#include "program.h"

// Declaure static data members
byte ProgramData::nprograms = 0;
LogStruct ProgramData::lastrun;
unsigned long ProgramData::scheduled_start_time[(MAX_EXT_BOARDS+1)*8];
unsigned long ProgramData::scheduled_stop_time[(MAX_EXT_BOARDS+1)*8];
byte ProgramData::scheduled_program_index[(MAX_EXT_BOARDS+1)*8];

void ProgramData::init() {
	reset_runtime();
  load_count();
  // reset log variables
  lastrun.station = 0;
  lastrun.program = 0;
  lastrun.duration = 0;
  lastrun.endtime = 0;  
}

void ProgramData::reset_runtime() {
  for (byte i=0; i<(MAX_EXT_BOARDS+1)*8; i++) {
    scheduled_start_time[i] = 0;
    scheduled_stop_time[i] = 0;
    scheduled_program_index[i] = 0;
  }
}

// load program count from EEPROM
void ProgramData::load_count() {
  nprograms = eeprom_read_byte((unsigned char *) ADDR_PROGRAMCOUNTER);
}

// save program count to EEPROM
void ProgramData::save_count() {
  eeprom_write_byte((unsigned char *) ADDR_PROGRAMCOUNTER, nprograms);
}

// erase all program data
void ProgramData::erase() {
  /*byte zero = 0;
  for(unsigned int addr=ADDR_PROGRAMDATA; addr<ADDR_PROGRAMDATA+(nprograms*PROGRAMSTRUCT_SIZE); addr++)
    eeprom_write_block((const void*)&zero, (void *)addr, 1);*/
  // no need to wipe data, just set count to 0
  nprograms = 0;
  save_count();
}

// read a program
void ProgramData::read(byte pid, ProgramStruct *buf) {
  if (pid >= nprograms) return;
  unsigned int addr = ADDR_PROGRAMDATA + (unsigned int)pid * PROGRAMSTRUCT_SIZE;
  eeprom_read_block((void*)buf, (const void *)addr, PROGRAMSTRUCT_SIZE);  
}

// add a program
void ProgramData::add(ProgramStruct *buf) {
  if (nprograms >= MAX_NUMBER_PROGRAMS)  return;
  unsigned int addr = ADDR_PROGRAMDATA + (unsigned int)nprograms * PROGRAMSTRUCT_SIZE;
  eeprom_write_block((const void*)buf, (void *)addr, PROGRAMSTRUCT_SIZE);
  nprograms ++;
  save_count();
}

// modify a program
void ProgramData::modify(byte pid, ProgramStruct *buf) {
  if (pid >= nprograms)  return;
  unsigned int addr = ADDR_PROGRAMDATA + (unsigned int)pid * PROGRAMSTRUCT_SIZE;
  eeprom_write_block((const void*)buf, (void *)addr, PROGRAMSTRUCT_SIZE);
}

// delete program(s)
void ProgramData::del(byte pid) {
  if (pid >= nprograms)  return;
  if (nprograms == 0) return;
  ProgramStruct copy;
  unsigned int addr = ADDR_PROGRAMDATA + (unsigned int)(pid+1) * PROGRAMSTRUCT_SIZE;
  // erase by copying backward
  for (; addr < ADDR_PROGRAMDATA + nprograms * PROGRAMSTRUCT_SIZE; addr += PROGRAMSTRUCT_SIZE) {
    eeprom_read_block((void*)&copy, (const void *)addr, PROGRAMSTRUCT_SIZE);  
    eeprom_write_block((const void*)&copy, (void *)(addr-PROGRAMSTRUCT_SIZE), PROGRAMSTRUCT_SIZE);
  }
  nprograms --;
  save_count();
}

// Check if a given time matches program schedule
byte ProgramStruct::check_match(time_t t) {

  unsigned int current_minute = (unsigned int)hour(t)*60+(unsigned int)minute(t);
  
  // check program enable status
  if (enabled == 0) return 0;
  
  // check day match
  // if special program bit is set, and interval is larger than 1
  if ((days[0]&0x80)&&(days[1]>1)) {
    // this is an inverval program
    byte dn   =days[1];      // interval
    byte drem =days[0]&0x7f; // remainder, relative to 1970-01-01
    if (((t/SECS_PER_DAY)%dn) != drem)  return 0;
  } else {
    // this is a weekly program
    byte wd = ((byte)weekday(t)+5)%7;
    // weekday match
    if (!(days[0] & (1<<wd)))
      return 0;
    byte dt=day(t);
    if ((days[0]&0x80)&&(days[1]==0)) {
      // even day restriction
      if((dt%2)!=0)  return 0;
    }
    if ((days[0]&0x80)&&(days[1]==1)) {
      // odd day restriction
      // skip 31st and Feb 29
      if(dt==31)  return 0;
      else if (dt==29 && month(t)==2)  return 0;
      else if ((dt%2)!=1)  return 0;
    }
  }

  // check start and end time
  if (current_minute < start_time || current_minute > end_time)
    return 0;
      
  // check interval match
  if (interval == 0)  return 0;
  if (((current_minute - start_time) / interval) * interval ==
       (current_minute - start_time)) {
    // program matched
    return 1;
  }
  return 0;
}

// convert absolute remainder (reference time 1970 01-01) to relative remainder (reference time today)
// absolute remainder is stored in eeprom, relative remainder is presented to web
void ProgramData::drem_to_relative(byte days[2]) {
  byte rem_abs=days[0]&0x7f;
  byte inv=days[1];
  days[0] = (days[0]&0x80) | (byte)((rem_abs + inv - (now()/SECS_PER_DAY) % inv) % inv);
}

// relative remainder -> absolute remainder
void ProgramData::drem_to_absolute(byte days[2]) {
  byte rem_rel=days[0]&0x7f;
  byte inv=days[1];
  days[0] = (days[0]&0x80) | (byte)(((now()/SECS_PER_DAY) + rem_rel) % inv);
}
