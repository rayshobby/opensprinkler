// Arduino library code for OpenSprinkler

/* OpenSprinkler Class Definition
   Creative Commons Attribution-ShareAlike 3.0 license
   Feb 2013 @ Rayshobby.net
*/

#ifndef _OpenSprinkler_h
#define _OpenSprinkler_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include <avr/eeprom.h>
#include "Wire.h"
#include "Time.h"
#include "DS1307RTC.h"
#include "LiquidCrystal.h"
#include "EtherCard.h"
#include "defines.h"

// Option Data Structure
struct OptionStruct{
  byte value; // each option is byte
  byte max;   // maximum value
  char* str;  // name string
  byte flag;  // flag
};

struct StatusBits {
  byte enabled:1;           // operation enable (when set, controller operation is enabled)
  byte rain_delayed:1;      // rain delay bit (when set, rain delay is applied)
  byte rain_sensed:1;       // rain sensor bit (when set, it indicates that rain is detected)
  byte program_busy:1;      // when set, a program is being executed currently
  byte manual_mode:1;       // when set, the controller is in manual mode
  byte has_rtc:1;           // when set, the controller has a DS1307 RTC
  byte dummy:2;             // unused, filler for the first 8-bit
  
  byte display_board:4;     // the board that is being displayed onto the lcd
  byte network_fails:4;     // number of network fails
}; 
  
class OpenSprinkler {
public:
  
  // ====== Data Members ======
  static LiquidCrystal lcd;
  static StatusBits status;
  static byte nboards, nstations;
  
  static OptionStruct options[];  // option values, max, name, and flag
    
  static char* days_str[];		// 3-letter name of each weekday
  static byte station_bits[]; // station activation bits. each byte corresponds to a board (8 stations)
                              // first byte-> master controller, second byte-> ext. board 1, and so on
  static byte masop_bits[];   // station master operation bits. each byte corresponds to a board (8 stations)
  static unsigned long raindelay_stop_time;   // time (in seconds) when raindelay is stopped

  // ====== Member Functions ======
  // -- Setup --
  static void reboot();   // reboot the microcontroller
  static void begin();    // initialization, must call this function before calling other functions
  static byte start_network(byte mymac[], int http_port);  // initialize network with the given mac and port
  static void self_test();  // self-test function
  static void get_station_name(byte sid, char buf[]); // get station name
  static void set_station_name(byte sid, char buf[]); // set station name
  static void masop_load();  // load station master operation bits
  static void masop_save();  // save station master operation bits
  // -- Options --
  static void options_setup();
  static void options_load();
  static void options_save();

  // -- Operation --
  static void enable();     // enable controller operation
  static void disable();    // disable controller operation, all stations will be closed immediately
  static void raindelay_start(byte rd);  // start raindelay for rd hours
  static void raindelay_stop(); // stop rain delay
  static void rainsensor_status(); // update rainsensor stateus
  static byte weekday_today();  // returns index of today's weekday (Monday is 0) 
  // -- Station schedules --
  // Call functions below to set station bits
  // Then call apply_station_bits() to activate/deactivate valves
  static void set_station_bit(byte sid, byte value); // set station bit of one station (sid->station index, value->0/1)
  static void clear_all_station_bits(); // clear all station bits
  static void apply_all_station_bits(); // apply all station bits (activate/deactive values)

  // -- String functions --
  //static void password_set(char *pw);     // save password to eeprom
  static byte password_verify(char *pw);  // verify password
  static void eeprom_string_set(int start_addr, char* buf);
  static void eeprom_string_get(int start_addr, char* buf);
    
  // -- LCD functions --
  static void lcd_print_pgm(PGM_P PROGMEM str);           // print a program memory string
  static void lcd_print_line_clear_pgm(PGM_P PROGMEM str, byte line);
  static void lcd_print_time(byte line);                  // print current time
  static void lcd_print_ip(const byte *ip, int http_port);// print ip and port number
  static void lcd_print_station(byte line, char c);       // print station bits of the board selected by display_board
 
  // -- Button and UI functions --
  static byte button_read(byte waitmode); // Read button value. options for 'waitmodes' are:
                                          // BUTTON_WAIT_NONE, BUTTON_WAIT_RELEASE, BUTTON_WAIT_HOLD
                                          // return values are 'OR'ed with flags
                                          // check defines.h for details

  // -- UI functions --
  static void ui_set_options(int oid);    // ui for setting options (oid-> starting option index)

private:
  static void lcd_print_option(int i);  // print an option to the lcd
  static void lcd_print_2digit(int v);  // print a integer in 2 digits
  static byte button_read_busy(int value, byte waitmode, byte butt, byte is_holding);
};

#endif
