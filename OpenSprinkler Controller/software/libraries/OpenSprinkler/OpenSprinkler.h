// Arduino library code for OpenSprinkler

/* OpenSprinkler Class Definition
   Creative Commons Attribution-ShareAlike 3.0 license
   June 2012 @ Rayshobby.net
*/

#ifndef _OpenSprinkler_h
#define _OpenSprinkler_h

#include <WProgram.h>
#include <avr/eeprom.h>
#include "Wire.h"
#include "Time.h"
#include "LiquidCrystal.h"
#include "EtherCard.h"
#include "defines.h"

struct StatusBits {
  byte enabled:1;           // operation enable (when set, controller operation is enabled)
  byte rain_delayed:1;      // rain delay bit (when set, rain delay is applied)
  byte rain_sensed:1;       // rain sensor bit (when set, it indicates that rain is detected)
  byte network_failed:1;    // network status bit (when set, network failure is detected)
  byte program_busy:1;      // when set, a program is being executed currently
  byte manual_mode:1;       // when set, the controller is in manual mode
  byte display_board:2;     // the board that is being displayed onto the lcd
}; 
  
class OpenSprinkler {
public:
  
  // ====== Data Members ======
  static LiquidCrystal lcd;
  static StatusBits status;
  
  static byte options[];			// option values, each option takes one byte
  static char* options_str[];	// display string of each option
  static char* days_str[];		// string of each weekday
  static byte station_bits[]; // station activation bits. each byte corresponds to a board (8 stations)
                              // first byte-> master controller, second byte-> ext. board 1, and so on

  ///static unsigned long time_second_counter;   // counts number of seconds since program starts (system time)
  static unsigned long raindelay_stop_time;   // time (in seconds) when raindelay is stopped
  
  // ====== Member Functions ======
  // -- Setup --
  static void reboot();
  static void begin();    // initialization, must call this function before calling other functions
  static byte start_network(byte mymac[], int http_port=80);  // initialize network with the given mac and port
  static void self_test();  // self-test function
  
  // -- Options --
  static void options_setup();
  static void options_load();
  static void options_save();
  static byte option_get_flag(int i);
  static byte option_get_max(int i);

  // -- Operation --
  static void enable();     // enable controller operation
  static void disable();    // disable controller operation, all stations will be closed
  static void raindelay_start(byte rd);  // start raindelay for rd hours
  static void raindelay_stop(); // stop rain delay
  static byte weekday_today();  // returns index of today's weekday (Monday is 0) 
  static void manual_mode_on();  // switch controller to manual mode
  static void manual_mode_off(); // switch controller to program mode
  // -- Station schedules --
  // Call functions below to set station bits
  // Then call apply_station_bits() to activate/deactivate valves
  static void set_board_bits(byte bid, byte value); // set station bits of one board (bid->board index)
  static void set_station_bit(byte sid, byte value); // set station bit of one station (sid->station index, value->0/1)
  static void clear_all_station_bits(); // clear all station bits
  static void apply_all_station_bits(); // apply all station bits (activate/deactive values)

  // -- Weather --
  static void location_get(char* loc);  // read location string from eeprom
  static void location_set(char* loc);  // write location string to eeprom
  
  // -- LCD functions --
  static void lcd_print_pgm(PGM_P PROGMEM str);           // print a program memory string
  static void lcd_print_line_clear_pgm(PGM_P PROGMEM str, byte line);
  static void lcd_print_lines_clear_pgm(PGM_P PROGMEM str1, PGM_P PROGMEM str2);
  static void lcd_print_time(byte line);                  // print current time
  static void lcd_print_ip(const byte *ip, int http_port);// print ip and port number
  static void lcd_print_station(byte line, char c);       // print station bits of the board selected by display_board
  static void lcd_print_status(); // print selected status bits
 
  // -- Button and UI functions --
  static byte button_read(byte waitmode); // Read button value. options for 'waitmodes' are:
                                          // BUTTON_WAIT_NONE, BUTTON_WAIT_RELEASE, BUTTON_WAIT_HOLD
                                          // return values are 'OR'ed with flags
                                          // check defines.h for details

  // -- UI functions --
  static void ui_set_options(int oid);    // ui for setting options (oid-> starting option index)

  // -- Password functions --
  static void password_set(char *pw);     // save password to eeprom
  static byte password_verify(char *pw);  // verify password

private:
  static void set_master_station_bit();  // set master station
  static byte options_flag[];   // editable flag of each option
  static byte options_max[];    // max value of each option
  static void lcd_print_option(int i);  // print an option to the lcd
  static byte button_read_busy(int value, byte waitmode, byte butt, byte is_holding);

};

#endif
