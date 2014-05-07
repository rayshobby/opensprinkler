// Arduino library code for OpenSprinkler Generation 2

/* Macro definitions and Arduino pin assignments
   Creative Commons Attribution-ShareAlike 3.0 license
   Dec 2013 @ Rayshobby.net
*/

#ifndef _Defines_h
#define _Defines_h

// Firmware version
#define SVC_FW_VERSION  205 // firmware version (205 means 2.0.5 etc)
                            // if this number is different from stored in EEPROM,
                            // an EEPROM reset will be automatically triggered

#define MAX_EXT_BOARDS   5 // maximum number of ext. boards (each expands 8 stations)
                            // total number of stations: (1+MAX_EXT_BOARDS) * 8

#define MAX_NUM_STATIONS  ((1+MAX_EXT_BOARDS)*8)
#define STATION_NAME_SIZE 16 // size of each station name, default is 16 letters max

// Internal EEPROM Defines
#define INT_EEPROM_SIZE         2048    // ATmega644 eeprom size

#define ADDR_EEPROM_OPTIONS     0x0000  // address where options are stored, 48 bytes reserved
#define ADDR_EEPROM_CONSTATUS   0x0030  // address where controller status data are stored, 16 bytes reserved
#define ADDR_EEPROM_PASSWORD    0x0040	// address where password is stored, 32 bytes reserved
#define ADDR_EEPROM_LOCATION    0x0060  // address where location is stored, 32 bytes reserved
#define ADDR_EEPROM_SCRIPTURL   0x0080	// address where javascript url is stored, 128 bytes reserved
#define ADDR_EEPROM_STN_NAMES   0x0100  // address where station names are stored

#define ADDR_EEPROM_RUNONCE     (ADDR_EEPROM_STN_NAMES+(MAX_EXT_BOARDS+1)*8*STATION_NAME_SIZE)
                                        // address where run-once data is stored
#define ADDR_EEPROM_MAS_OP      (ADDR_EEPROM_RUNONCE+(MAX_EXT_BOARDS+1)*8*2)
                                        // address where master operation bits are stored
#define ADDR_EEPROM_IGNRAIN     (ADDR_EEPROM_MAS_OP+(MAX_EXT_BOARDS+1))
#define ADDR_EEPROM_USER        (ADDR_EEPROM_IGNRAIN+(MAX_EXT_BOARDS+1))
                                        // address where program schedule data is stored

#define DEFAULT_PASSWORD        "opendoor"
#define DEFAULT_LOCATION        "Boston,MA" // zip code, city name or any google supported location strings
                                            // IMPORTANT: use , or + in place of 'space'
                                            // So instead of 'New York', use 'New,York' or 'New+York'

#define DEFAULT_JAVASCRIPT_URL  "http://rayshobby.net/scripts/java/svc2.0.5"

// macro define of each option
// See OpenSprinkler.cpp for details on each option
typedef enum {
  OPTION_FW_VERSION = 0,
  OPTION_TIMEZONE,
  OPTION_USE_NTP,
  OPTION_USE_DHCP,
  OPTION_STATIC_IP1,
  OPTION_STATIC_IP2,
  OPTION_STATIC_IP3,
  OPTION_STATIC_IP4,
  OPTION_GATEWAY_IP1,
  OPTION_GATEWAY_IP2,
  OPTION_GATEWAY_IP3,
  OPTION_GATEWAY_IP4,
  OPTION_HTTPPORT_0,
  OPTION_HTTPPORT_1,
  OPTION_NETFAIL_RECONNECT,
  OPTION_EXT_BOARDS,
  OPTION_SEQUENTIAL,
  OPTION_STATION_DELAY_TIME,
  OPTION_MASTER_STATION,
  OPTION_MASTER_ON_ADJ,
  OPTION_MASTER_OFF_ADJ,
  OPTION_USE_RAINSENSOR,
  OPTION_RAINSENSOR_TYPE,
  OPTION_WATER_PERCENTAGE,
  OPTION_SELFTEST_TIME,
  OPTION_IGNORE_PASSWORD,
  OPTION_DEVICE_ID,
  OPTION_LCD_CONTRAST,
  OPTION_LCD_BACKLIGHT,
  OPTION_LCD_DIMMING,
  OPTION_NTP_IP1,
  OPTION_NTP_IP2,
  OPTION_NTP_IP3,
  OPTION_NTP_IP4,
  OPTION_RESET,
  NUM_OPTIONS	// total number of options
} OS_OPTION_t;

// Option Flags
#define OPFLAG_NONE        0x00  // default flag, this option is not editable
#define OPFLAG_SETUP_EDIT  0x01  // this option is editable during startup
#define OPFLAG_WEB_EDIT    0x02  // this option is editable on the Options webpage

// =====================================
// ====== Arduino Pin Assignments ======
// =====================================

// ------ Define hardware version here ------
#define SVC_HW_VERSION 20

#ifndef SVC_HW_VERSION
#error "==This error is intentional==: you must define SVC_HW_VERSION in arduino-xxxx/libraries/OpenSprinklerGen2/defines.h"
#endif

#if SVC_HW_VERSION == 20 || SVC_HW_VERSION == 21

  #define PIN_BUTTON_1      31    // button 1
  #define PIN_BUTTON_2      30    // button 2
  #define PIN_BUTTON_3      29    // button 3 
  #define PIN_RF_DATA       28    // RF data pin 
  #define PIN_SR_LATCH       3    // shift register latch pin
  #define PIN_SR_DATA       21    // shift register data pin
  #define PIN_SR_CLOCK      22    // shift register clock pin
  #define PIN_SR_OE          1    // shift register output enable pin
  #define PIN_LCD_RS        19    // LCD rs pin
  #define PIN_LCD_EN        18    // LCD enable pin
  #define PIN_LCD_D4        20    // LCD d4 pin
  #define PIN_LCD_D5        21    // LCD d5 pin
  #define PIN_LCD_D6        22    // LCD d6 pin
  #define PIN_LCD_D7        23    // LCD d7 pin
  #define PIN_LCD_BACKLIGHT 12    // LCD backlight pin
  #define PIN_LCD_CONTRAST  13    // LCD contrast pin
  #define PIN_ETHER_CS       4    // Ethernet controller chip select pin
  #define PIN_SD_CS          0    // SD card chip select pin
  #define PIN_RAINSENSOR    11    // rain sensor is connected to pin D3
  #define PIN_RELAY         14    // mini relay is connected to pin D14
  
#endif 

// ====== Button Defines ======
#define BUTTON_1            0x01
#define BUTTON_2            0x02
#define BUTTON_3            0x04

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
#define ETHER_BUFFER_SIZE   900  // if buffer size is increased, you must check the total RAM consumption
                                  // otherwise it may cause the program to crash
#define TMP_BUFFER_SIZE       48  // scratch buffer size

#endif


