// Example code for OpenSprinkler Generation 2

/* Program Data Structures and Functions
   Creative Commons Attribution-ShareAlike 3.0 license
   Apr 2013 @ Rayshobby.net
*/

#ifndef PROGRAM_STRUCT_H
#define PROGRAM_STRUCT_H

#include <OpenSprinklerGen2.h>

// Program data structure
class ProgramStruct {
public:
  // 'days' are formatted as follows:
  // if(days[0].bits[7]==0), this is a standard weekly program:
  //    days[0].bits[0..6] correspond to Monday to Sunday
  // if(days[0].bits[7]==1), this is a special program:
  //   if(days[1]==1), even day restriction
  //   if(days[1]==2), odd day restriction (except 31st and Feb 29th)
  //   if(days[1]>=2), interval program, days[1] stores interval
  //     days[0].bits[0..6] stores starting day remainder (reference time 1970-01-01)
  byte days[2];
  uint16_t start_time;  // start time in minutes
  uint16_t end_time;    // end time in minutes
  uint16_t interval;    // interval in minutes
  uint16_t duration;    // duration in seconds
  byte stations[MAX_EXT_BOARDS+1];  // station bit
  byte enabled;         // program enable
  
  byte check_match(time_t t);
};

// Log data structure
struct LogStruct {
  byte station;
  byte program;
  unsigned int duration;
  unsigned long endtime;
};

// program structure size
#define PROGRAMSTRUCT_SIZE   (sizeof(ProgramStruct))
#define ADDR_PROGRAMCOUNTER  ADDR_EEPROM_USER
#define ADDR_PROGRAMDATA     (ADDR_EEPROM_USER+2)
// maximum number of programs, restricted by internal EEPROM size, 32 default
#define MAX_NUMBER_PROGRAMS  ((INT_EEPROM_SIZE-ADDR_EEPROM_USER-2)/PROGRAMSTRUCT_SIZE)

extern OpenSprinkler svc;

class ProgramData {
public:  
  static unsigned long scheduled_start_time[];// scheduled start time for each station
  static unsigned long scheduled_stop_time[]; // scheduled stop time for each station
  static byte scheduled_program_index[]; // scheduled program index
  static byte  nprograms;     // number of programs
  static LogStruct lastrun;   // last run log
  
  static void init();
  static void reset_runtime();
  static void erase();
  static void read(byte pid, ProgramStruct *buf);
  static void add(ProgramStruct *buf);
  static void modify(byte pid, ProgramStruct *buf);
  static void del(byte pid);
  static void drem_to_relative(byte days[2]); // absolute to relative reminder conversion
  static void drem_to_absolute(byte days[2]);
private:  
  static void load_count();
  static void save_count();
};

#endif
