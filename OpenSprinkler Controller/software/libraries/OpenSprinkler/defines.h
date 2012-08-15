// Arduino library code for OpenSprinkler

/* Macro definitions and Arduino pin assignments
   Creative Commons Attribution-ShareAlike 3.0 license
   June 2012 @ Rayshobby.net
*/

#ifndef _Defines_h
#define _Defines_h

// Firmware version
#define SVC_FW_VERSION  17  // firmware version. when this is different from the version number
                            // stored in eeprom, an eeprom reset will be automatically triggered

#define MAX_EXT_BOARDS  3   // maximum number of ext. boards (each consists of 8 stations)
                            // total number of stations: (1+MAX_EXT_BOARDS) * 8

#define STATION_NAME_SIZE  12 // size of each station name, default is 12 letters long

// External I2C EEPROM
//#define I2C_EEPROM_DEVICE_ADDR  0x50
//#define I2C_EEPROM_SIZE         16384 // external eeprom size (in bytes)
//#define EEPROM_BLOCK_SIZE       16    // eeprom block size, DO NOT CHANGE

// Internal EEPROM Defines
#define INT_EEPROM_SIZE         1024    // ATmega328 eeprom size
#define ADDR_EEPROM_OPTIONS     0x0000  // address where options are stored, 24 bytes reserved
#define ADDR_EEPROM_PASSWORD    0x0018	// address where password is stored, 16 bytes reserved
#define ADDR_EEPROM_LOCATION    0x0028  // address where location is stored, 32 bytes reserved
#define ADDR_EEPROM_SNAMES      0x0048  // address where station names are stored
#define ADDR_EEPROM_USER        (ADDR_EEPROM_SNAMES+(MAX_EXT_BOARDS+1)*8*STATION_NAME_SIZE)
                                        // address where user data is stored

#define DEFAULT_PASSWORD        "opendoor"
#define DEFAULT_LOCATION        "Boston,MA" // zip code, city name or any google supported location strings
                                            // IMPORTANT: use , or + in place of space
                                            // So instead of 'New York', use 'New,York' or 'New+York'
// macro define of each option
// See OpenSprinkler.h for details
typedef enum {
  OPTION_FW_VERSION = 0,
  OPTION_TIMEZONE,
  OPTION_USE_DHCP,
  OPTION_STATIC_IP1,
  OPTION_STATIC_IP2,
  OPTION_STATIC_IP3,
  OPTION_STATIC_IP4,
  OPTION_GATEWAY_IP1,
  OPTION_GATEWAY_IP2,
  OPTION_GATEWAY_IP3,
  OPTION_GATEWAY_IP4,
  OPTION_EXT_BOARDS,
  OPTION_MASTER_STATION,
  OPTION_SEQUENTIAL,
  OPTION_STATION_DELAY,
  OPTION_USE_RAINSENSOR,
  OPTION_RS_NORMALLY_OPEN,
  OPTION_USE_RTC,
  OPTION_RESET,
  NUM_OPTIONS	// total number of options
} OS_OPTION_t;

// Option flags
#define OPFLAG_DEFAULT      0x00
#define OPFLAG_WEB_EDIT     0x01    // the option is editable on webpage
#define OPFLAG_BOOL         0x02    // the option is a boolean variable

// =====================================
// ====== Arduino Pin Assignments ======
// =====================================

// ------ Define hardware version here ------
// Since each hardware version may use different pin assignments,
// and there is currently no mechanism for software to figure that out,
// you must manually provide the hardware version number here
// Uncomment only one line below

//#define SVC_HW_VERSION 14
//#define SVC_HW_VERSION 13
//#define SVC_HW_VERSION 12
//#define SVC_HW_VERSION 11   // OpenSprinkler v1.0 use the same pinouts as v1.1

#ifndef SVC_HW_VERSION
#error "==This error is intentional==: you must define SVC_HW_VERSION in arduino-xxxx/libraries/OpenSprnikler/defines.h"
#endif

#if SVC_HW_VERSION == 14

  #define PIN_READ_BUTTON    0    // analog pin assigned for button reading
  #define PIN_SR_LATCH       7    // shift register latch pin
  #define PIN_SR_DATA        5    // shift register data pin
  #define PIN_SR_CLOCK       6    // shift register clock pin
  #define PIN_SR_OE         15    // shift register output enable pin
  #define PIN_LCD_RS         1    // LCD rs pin
  #define PIN_LCD_EN         0    // LCD enable pin
  #define PIN_LCD_D4         4    // LCD d4 pin
  #define PIN_LCD_D5         5    // LCD d5 pin
  #define PIN_LCD_D6         6    // LCD d6 pin
  #define PIN_LCD_D7         9    // LCD d7 pin
  #define PIN_RAINSENSOR     3    // by default rain sensor is connected to pin D3

#elif SVC_HW_VERSION == 13

  #define PIN_READ_BUTTON    0    // analog pin assigned for button reading
  #define PIN_SR_LATCH       7    // shift register latch pin
  #define PIN_SR_DATA        5    // shift register data pin
  #define PIN_SR_CLOCK       6    // shift register clock pin
  #define PIN_SR_OE          3    // shift register output enable pin
  #define PIN_LCD_RS         0    // LCD rs pin
  #define PIN_LCD_EN         4    // LCD enable pin
  #define PIN_LCD_D4         1    // LCD d4 pin
  #define PIN_LCD_D5         5    // LCD d5 pin
  #define PIN_LCD_D6         6    // LCD d6 pin
  #define PIN_LCD_D7         9    // LCD d7 pin
  #define PIN_RAINSENSOR    15    // by default rain sensor is connected to pin A1
                                  // change if you need
#elif SVC_HW_VERSION == 12

  #define PIN_READ_BUTTON    0    // analog pin assigned for button reading
  #define PIN_SR_LATCH       7    // shift register latch pin
  #define PIN_SR_DATA        5    // shift register data pin
  #define PIN_SR_CLOCK       6    // shift register clock pin
  #define PIN_LCD_RS         5    // LCD rs pin
  #define PIN_LCD_EN         4    // LCD enable pin
  #define PIN_LCD_D4         6    // LCD d4 pin
  #define PIN_LCD_D5         9    // LCD d5 pin
  #define PIN_LCD_D6        16    // LCD d6 pin
  #define PIN_LCD_D7        17    // LCD d7 pin
  #define PIN_RAINSENSOR     3    // by default rain sensor pin is connected to pin D3
                                  // change if you need	
#else

  #define PIN_READ_BUTTON    0    // analog pin assigned for button reading
  #define PIN_SR_LATCH       7    // shift register latch pin
  #define PIN_SR_DATA        5    // shift register data pin
  #define PIN_SR_CLOCK       6    // shift register clock pin
  #define PIN_LCD_RS         5    // LCD rs pin
  #define PIN_LCD_EN         4    // LCD enable pin
  #define PIN_LCD_D4         6    // LCD d4 pin
  #define PIN_LCD_D5        15    // LCD d5 pin
  #define PIN_LCD_D6        16    // LCD d6 pin
  #define PIN_LCD_D7        17    // LCD d7 pin
  #define PIN_ETHER_RESET    9    // Ethernet reset pin
  #define PIN_RAINSENSOR     3    // by default rain sensor is connected to pin D3
                                  // change if you need  
#endif

// ====== Button Defines ======
#define BUTTON_1          0x01
#define BUTTON_2          0x02
#define BUTTON_3          0x04

// button status values
#define BUTTON_NONE         0x00  // no button pressed
#define BUTTON_MASK         0x0F  // button status mask
#define BUTTON_FLAG_HOLD    0x80  // long hold flag
#define BUTTON_FLAG_DOWN    0x40  // down flag
#define BUTTON_FLAG_UP      0x20  // up flag

// button timing values
#define BUTTON_DELAY_MS        1  // short delay (milliseconds)
#define BUTTON_HOLD_MS       800  // long hold expiration time (milliseconds)
#define BUTTON_IDLE_TIMEOUT    8  // timeout if no button is pressed within certain number of seconds

// button mode values
#define BUTTON_WAIT_NONE       0  // do not wait, return value immediately
#define BUTTON_WAIT_RELEASE    1  // wait until button is release
#define BUTTON_WAIT_HOLD       2  // wait until button hold time expires

// ====== Timing Defines ======
#define DISPLAY_MSG_MS      2000  // message display time (milliseconds)

// ====== Ethernet Defines ======
#define ETHER_BUFFER_SIZE     700	// increase if webpage becomes too large,
                                  // but do not exceed 1000
#define TMP_BUFFER_SIZE        30 // scratch buffer size

#endif


