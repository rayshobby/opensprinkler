// Example code for OpenSprinkler

/* Program Data Structures and Functions
   Creative Commons Attribution-ShareAlike 3.0 license
   June 2012 @ Rayshobby.net
*/

#include "program.h"

// Declaure static data members
byte ProgramData::nprograms = 0;
LogStruct ProgramData::lastrun;
unsigned int ProgramData::remaining_time[(MAX_EXT_BOARDS+1)*8];
unsigned int ProgramData::scheduled_duration[(MAX_EXT_BOARDS+1)*8];
byte ProgramData::scheduled_program_index[(MAX_EXT_BOARDS+1)*8];
unsigned long ProgramData::scheduled_stop_time[(MAX_EXT_BOARDS+1)*8];

void ProgramData::init() {
	
  // init data members
  for (byte i=0; i<(MAX_EXT_BOARDS+1)*8; i++) {
    remaining_time[i] = 0;
    scheduled_stop_time[i] = 0;
    scheduled_program_index[i] = 0;
    scheduled_duration[i] = 0;
  }
  load_count();

  // init log variables
  lastrun.station = 0;
  lastrun.program = 0;
  lastrun.duration = 0;
  lastrun.endtime = 0;
 
}

// load program count from EEPROM
void ProgramData::load_count() {
  nprograms = eeprom_read_byte((unsigned char *) ADDR_PROGRAMCOUNTER);
}

// save program count to EEPROM
void ProgramData::save_count() {
  eeprom_write_byte((unsigned char *) ADDR_PROGRAMCOUNTER, nprograms);
}

// reset all program data
void ProgramData::clear() {
  byte zero = 0;
  for(unsigned int addr=ADDR_PROGRAMDATA; addr<ADDR_PROGRAMDATA+(nprograms*PROGRAMSTRUCT_SIZE); addr++)
    eeprom_write_block((const void*)&zero, (void *)addr, 1);
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

// Check if a given station and a given time matches any program
// Returns the duration and program index of the first program that matches
unsigned int ProgramData::check_match(byte sid, time_t t, byte *pid) {
  ProgramStruct prog; 
  byte i;
  byte bid = sid>>3;
  byte s = sid%8;
  unsigned int current_minute = (unsigned int)hour(t)*60+(unsigned int)minute(t);
  
  // return the duration of the first program that matches the station index,
  // the start_time, end_time, and interval
  for(i=0; i<nprograms; i++) {
    read(i, &prog);
    // check station index match
    if (!(prog.stations[bid]&(1<<s)))
      continue;

    // check day match
    // if special program bit is set, and interval is larger than 1
    if ((prog.days[0]&0x80)&&(prog.days[1]>1)) {
      // this is an inverval program
      byte dn   =prog.days[1];      // interval
      byte drem =prog.days[0]&0x7f; // remainder, relative to 1970-01-01
      if (((t/SECS_PER_DAY)%dn) != drem)  continue;
    } else {
      // this is a weekly program
      byte wd = ((byte)weekday(t)+5)%7;
      // weekday match
      if (!(prog.days[0] & (1<<wd)))
        continue;
      byte dt=day(t);
      if ((prog.days[0]&0x80)&&(prog.days[1]==0)) {
        // even day restriction
        if((dt%2)!=0)  continue;
      }
      if ((prog.days[0]&0x80)&&(prog.days[1]==1)) {
        // odd day restriction
        // skip 31st and Feb 29
        if(dt==31)  continue;
        else if (dt==29 && month(t)==2)  continue;
        else if ((dt%2)!=1)  continue;
      }
    }

    // check start and end time
    if (current_minute < prog.start_time || current_minute > prog.end_time)
      continue;
      
    // check interval match
    if (prog.interval == 0)  continue;
    if (((current_minute - prog.start_time) / prog.interval) * prog.interval ==
         (current_minute - prog.start_time)) {
      if (pid != NULL)  *pid = i;
      return prog.duration;
    }
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
