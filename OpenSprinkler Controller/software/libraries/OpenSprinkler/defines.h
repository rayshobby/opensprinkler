// Arduino library code for OpenSprinkler

/* Macro definitions and pin assignments
   Creative Commons Attribution-ShareAlike 3.0 license
   Mar 2012 @ Rayshobby.net
*/

#ifndef _Defines_h
#define _Defines_h

// Firmware version
#define SVC_FW_VERSION  15

// ====== Hardware Defines ======
#define MAX_EXT_BOARDS         3	// maximum number of ext. boards (each ext. board contains 8 stations), hence total number of stations: (1+MAX_EXT_BOARDS) * 8
													
// External I2C EEPROM
#define I2C_EEPROM_DEVICE_ADDR 0x50
#define I2C_EEPROM_SIZE			   16382 // external eeprom size (in bytes)
#define EEPROM_BLOCK_SIZE      16
	
// ====== Internal EEPROM Defines ======
#define ADDR_EEPROM_BASE       0x0000 // address where options are stored
#define ADDR_EEPROM_PASSWORD   0x0080	// address where password is stored
#define ADDR_EEPROM_USER			 0x00A0 // address where user parameters are stored
#define DEFAULT_PASSWORD       "opendoor"

// index of each option
typedef enum {
	OPTION_FW_VERSION = 0,
	OPTION_TIMEZONE,
	OPTION_DHCP,
	OPTION_STATIC_IP1,
	OPTION_STATIC_IP2,
	OPTION_STATIC_IP3,
	OPTION_STATIC_IP4,
	OPTION_GATEWAY_IP1,
	OPTION_GATEWAY_IP2,
	OPTION_GATEWAY_IP3,
	OPTION_GATEWAY_IP4,
	OPTION_NTP_SYNC,
	OPTION_DAY_START,
	OPTION_DAY_END,
	OPTION_MULTISTATION,
	OPTION_EXT_BOARDS,
	OPTION_REQUIRE_NETWORK,
	OPTION_MASTER_STATION,
	OPTION_USE_RAINSENSOR,
	OPTION_RESET,
	NUM_OPTIONS	// total number of options
} OS_OPTION_t;

// Option flags
#define OPFLAG_NONE         0x00
#define OPFLAG_EDITABLE     0x01    // the option is editable
#define OPFLAG_WEB_EDIT     0x02    // the option is editable through the web
#define OPFLAG_BOOL         0x10    // the option takes bool values

#define SC_RAINDELAY_MAX     144		// maximum number of rain delay hours

// ====== Arduino Pin Assignments ======
// Define hardware version here

//#define SVC_HW_VERSION 12
//#define SVC_HW_VERSION 11

#ifndef SVC_HW_VERSION
#error "You must define SVC_HW_VERSION in libraries/OpenSprnikler/defines.h"
#endif

#if SVC_HW_VERSION == 12

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
	#define PIN_DEBUG          3    // pin 3 is left unused
	
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
	#define PIN_DEBUG          3    // pin 3 is left unused
	#define PIN_ETHER_RESET    9    // Ethernet reset pin

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
#define BUTTON_DELAY_MS        1  // short delay
#define BUTTON_HOLD_MS       800  // long hold
#define BUTTON_IDLE_TIMEOUT    8  // timeout if no button is pressed within a few seconds
// button mode values
#define BUTTON_WAIT_NONE       0  // do not wait, return value immediately
#define BUTTON_WAIT_RELEASE    1  // wait till button release
#define BUTTON_WAIT_HOLD       2  // wait till long hold time

// ====== Timing Defines ======
#define DISPLAY_MSG_MS      2000  // message display delay time

// ====== Ethernet Defines ======
#define ETHER_BUFFER_SIZE     900	// this should not exceed 1100
#define TMP_BUFFER_SIZE        20

#endif


