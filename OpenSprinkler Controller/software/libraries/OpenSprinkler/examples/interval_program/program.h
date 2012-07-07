// Example code for OpenSprinkler

/* Program Data Structures and Functions
   Creative Commons Attribution-ShareAlike 3.0 license
   June 2012 @ Rayshobby.net
*/

#include <Arduino.h>
#include <OpenSprinkler.h>

// Program data structure
struct ProgramStruct {
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
};

// Log data structure
struct LogStruct {
  byte station;
  byte program;
  unsigned int duration;
  unsigned long endtime;
};

#define MAX_NUMBER_PROGRAMS  64  // maximum number of programs, restricted by internal EEPROM size
#define PROGRAMSTRUCT_SIZE   (sizeof(ProgramStruct))
#define ADDR_PROGRAMCOUNTER  ADDR_EEPROM_USER
#define ADDR_PROGRAMDATA     (ADDR_EEPROM_USER+2)
extern OpenSprinkler svc;

class ProgramData {
public:  
  static unsigned int remaining_time[];	// remaining schedule time for each station
  static unsigned long scheduled_stop_time[]; // scheduled stop time for each station
  static byte scheduled_program_index[];  // scheduled program index
  static unsigned int scheduled_duration[]; // scheduled duration (in seconds)
  static byte  nprograms;     // number of programs
  static LogStruct lastrun;   // last run log
  
  static void init();
  static void reset_runtime();
  static void erase();
  static void read(byte pid, ProgramStruct *buf);
  static void add(ProgramStruct *buf);
  static void modify(byte pid, ProgramStruct *buf);
  static void del(byte pid);
  static unsigned int check_match(byte sid, time_t t, byte *pid);
  static void drem_to_relative(byte days[2]); // absolute to relative reminder conversion
  static void drem_to_absolute(byte days[2]);
private:  
  static void load_count();
  static void save_count();
};
