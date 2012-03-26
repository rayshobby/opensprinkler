// Example code for OpenSprinkler

/* Schedule functions
   Creative Commons Attribution-ShareAlike 3.0 license
   Mar 2012 @ Rayshobby.net
*/

#include <OpenSprinkler.h>
#include "schedule.h"

extern OpenSprinkler svc;

// convert time value to slot index
uint16_t schedule_time_to_slot(uint16_t h, uint16_t m)
{
  return h*SC_SLOTS_PER_HOUR + m/(SC_SLOT_LENGTH);
}


// clear a day's schedule for a selected board
void schedule_clear_day(byte bidx, byte day) {
  unsigned int start = (SC_BYTES_PER_BOARD*(unsigned int)bidx) + ((unsigned int)day*SC_NUM_SLOTS_PER_DAY);
  unsigned int end = start + SC_NUM_SLOTS_PER_DAY;
  svc.ext_eeprom_clear(start, end);
}

// clear the schedule of a selected board
void schedule_clear_all(byte bidx) {
  for(byte d=0;d<7;d++) {
    schedule_clear_day(bidx, d);
  }
}


// read a block of schedule values from eeprom
void schedule_read_slots(byte bidx, byte day, int idx, byte *buf) {
  unsigned int addr = (SC_BYTES_PER_BOARD*(unsigned int)bidx) + ((unsigned int)day*SC_NUM_SLOTS_PER_DAY+idx);
  svc.ext_eeprom_read_buffer(addr, buf, EEPROM_BLOCK_SIZE);
  for(byte i=0;i<EEPROM_BLOCK_SIZE;i++) {
    buf[i] = ~buf[i]; 
  }
}

// read the schedule value of a given day and index
byte schedule_read_slot(byte bidx, byte day, int idx) {
  unsigned int addr = (SC_BYTES_PER_BOARD*(unsigned int)bidx) + ((unsigned int)day*SC_NUM_SLOTS_PER_DAY+idx);
  byte value = ~(svc.ext_eeprom_read_byte(addr));
  return value;
}

// read the schedule value of a given day and given hour + minute
byte schedule_read(byte bidx, byte day, uint16_t h, uint16_t m) {
  return schedule_read_slot(bidx, day, schedule_time_to_slot(h, m));  
}

// write the schedule value of a given day and index
void schedule_write_slot(byte bidx, byte day, uint16_t idx, byte value) {
  unsigned int addr = (SC_BYTES_PER_BOARD*(unsigned int)bidx) + ((unsigned int)day*SC_NUM_SLOTS_PER_DAY+idx);
  value = ~value;
  svc.ext_eeprom_write_buffer(addr, &value, 1);
}


