// Arduino library code for OpenSprinkler

/* Class definition of OpenSprinkler
   Creative Commons Attribution-ShareAlike 3.0 license
   Mar 2012 @ Rayshobby.net
*/

#ifndef _OpenSprinkler_h
#define _OpenSprinkler_h

#include "Wire.h"
#include "EEPROM.h"
#include "Time.h"
#include "LiquidCrystal.h"
#include "EtherCard.h"
#include "defines.h"

class OpenSprinkler {
public:
	static LiquidCrystal lcd;
  static byte options[];			// option values, each option is a byte
  static char* options_str[];	// display string of each option
  static char* days_str[];		// string of each weekday

  static byte time_display_mode;	// time display mode
  static byte lcd_display_board;	// which ext board to display on the lcd
  static byte station_bitvalues[];  // scheduled open/close bit value of each station
  static unsigned int remaining_minutes[];	// remaining minutes of an open station

  static void reboot();
  static void reset_ethernet();
  static void begin();
  static void self_test();
  static void options_setup();
	static void options_load();
	static void options_save();
  static void station_reset();													// reset (close) all stations
	static void station_schedule(byte index, byte value);	// call this function to schedule a station
	static void station_schedule_clear();									// clear the schedule of all stations
  static void station_apply();													// call this function to apply the current schedule

  static void lcd_print_pgm(PGM_P PROGMEM str);				// print a program memory string to the lcd
  static void lcd_print_line_clear_pgm(PGM_P PROGMEM str, byte line);
  static void lcd_print_lines_clear_pgm(PGM_P PROGMEM str1, PGM_P PROGMEM str2);
  
  static void lcd_print_time(byte line);						  // print the current time to the lcd
  static void lcd_print_ip(const byte *ip, byte line);// print the ip address to the lcd
  static byte lcd_print_station(byte line, char c);			// print the station values of the ext board selected by lcd_display_board

	// read butto. options for 'waitmodes' are:
	// BUTTON_WAIT_NONE, BUTTON_WAIT_RELEASE, BUTTON_WAIT_HOLD
	// return values are BUTTON_1, BUTTON_2, or BUTTON_3 ORed with button flags
  // check defines.h for details
  static byte button_read(byte waitmode);
  
  static void ui_set_options(int which_option);				// ui for setting options
  static void ui_toggle_time_display();
  static void ui_set_time();												  // ui for manually set time
  
  static void password_set(char *pw);
  static byte password_verify(char *pw);
  static byte weekday_today();
  
  static unsigned long get_station_scheduled_seconds(byte i);
  static void set_station_scheduled_seconds(byte i, unsigned long value);
  static unsigned long get_station_scheduled_stop_time(byte i);
  static void set_station_scheduled_stop_time(byte i, unsigned long value);
  
	static void int_eeprom_write_buffer(unsigned int address, byte* buffer, byte length);
	static void int_eeprom_read_buffer (unsigned int address, byte* buffer, byte length);
	
  static void ext_eeprom_write_byte(unsigned int eeaddress, byte data);
  static byte ext_eeprom_read_byte (unsigned int eeaddress);
  
	// Warning: max 16 bytes per write
  static void ext_eeprom_write_buffer(unsigned int eeaddresspage, byte* buffer, byte length);
  static void ext_eeprom_read_buffer (unsigned int eeaddress, byte *buffer, int length);

private:
  static byte options_flag[];		// flag of each option
  static byte options_max[];    // max value of each option
  static byte option_get_flag(int i);
  static byte option_get_max(int i);
  
  static void lcd_print_option(int i);							// print an option to the lcd
  static byte button_read_busy(int value, byte waitmode, byte butt, byte is_holding);

};

extern BufferFiller bfill;

#endif
