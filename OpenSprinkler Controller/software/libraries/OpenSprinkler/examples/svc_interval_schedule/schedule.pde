// Example code for OpenSprinkler

/* Schedule functions
   Creative Commons Attribution-ShareAlike 3.0 license
   Mar 2012 @ Rayshobby.net
*/

#include <OpenSprinkler.h>
// make sure these numbers are all multiples of 16
#define SC_BYTES_PER_BOARD			32	// total bytes required for each board
/* Explanation:
The external EEPROM stores the schedule data, including the interval value (down to minutes) and the duration value (down to seconds) of each station. Each value is an unsigned int (2 bytes), so the total number of bytes required per board is 2*2*8=32 bytes
*/

extern OpenSprinkler svc;

// clear the external eeprom data for a selected board
void schedule_clear(byte bidx) {
  unsigned int start = (SC_BYTES_PER_BOARD*(unsigned int)bidx);
  unsigned int end = start + SC_BYTES_PER_BOARD;
  svc.ext_eeprom_clear(start, end);
}

// read all schedule data from external eeprom
void schedule_read_buffer(byte bidx, byte *buf) {
  unsigned int addr = SC_BYTES_PER_BOARD*(unsigned int)bidx;
  byte i, b;
  for(b=0;b<SC_BYTES_PER_BOARD/EEPROM_BLOCK_SIZE;b++, addr+=EEPROM_BLOCK_SIZE, buf+=EEPROM_BLOCK_SIZE) {
  	svc.ext_eeprom_read_buffer(addr, buf, EEPROM_BLOCK_SIZE);
	  for(i=0;i<EEPROM_BLOCK_SIZE;i++) {
  	  buf[i] = ~buf[i]; 
  	}
  }
}

// write all schedule data to external eeprom
void schedule_write_buffer(byte bidx, byte *buf) {
  unsigned int addr = SC_BYTES_PER_BOARD*(unsigned int)bidx;
  byte i, b;
  for(b=0;b<SC_BYTES_PER_BOARD/EEPROM_BLOCK_SIZE;b++, addr+=EEPROM_BLOCK_SIZE, buf+=EEPROM_BLOCK_SIZE) {
	  for(i=0;i<EEPROM_BLOCK_SIZE;i++) {
  	  buf[i] = ~buf[i]; 
  	}
  	svc.ext_eeprom_write_buffer(addr, buf, EEPROM_BLOCK_SIZE);
  }  
}

byte check_schedule_match(unsigned int interval, unsigned int current_minute) {
	if (interval == 0) return 0;
	unsigned int start_minute = (unsigned int)svc.options[OPTION_DAY_START] * 60;
	unsigned int end_minute = (unsigned int)svc.options[OPTION_DAY_END] * 60;
	if (current_minute < start_minute || current_minute > end_minute) return 0;
	// integer division rule
	if (((current_minute - start_minute) / interval) * interval == (current_minute - start_minute))
		return 1;
	else
		return 0;
	
}


