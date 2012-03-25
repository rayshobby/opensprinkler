// Example code for Sprinkler Valve Controller (SVC)
// Defines and global variables
// Licensed under GPL V2
// Mar 2012 @Rayshobby

// Firmware version
// !!! Note: firmware version should be changed whenever *type*
//     or *number* of options or schedule formats are modified !!!
#define FW_VERSION             13

// Hardware version
#define HW_VERSION             "1.1"

// ====== Hardware Defines ======
#define MAX_EXT_BOARDS         3	// maximum number of ext boards (8 additional stations per ext board)
                                        // this is mainly limited by the external eeprom size
                                        // total storage required are (1+MAX_EXT_BOARDS) * 16 bytes

// ====== Internal EEPROM Defines ======
#define ADDR_EEPROM_BASE       0x0000
#define ADDR_EEPROM_PASSWORD   0x0080
#define DEFAULT_PASSWORD       "opendoor"

// index of each option
#define OPTION_FW_VERSION      0
#define OPTION_TIMEZONE        1
#define OPTION_DHCP            2
#define OPTION_STATIC_IP1      3
#define OPTION_STATIC_IP2      4
#define OPTION_STATIC_IP3      5
#define OPTION_STATIC_IP4      6
#define OPTION_GATEWAY_IP1     7
#define OPTION_GATEWAY_IP2     8
#define OPTION_GATEWAY_IP3     9
#define OPTION_GATEWAY_IP4     10
#define OPTION_NTP_SYNC        11
#define OPTION_SHOW_MSG        12
#define OPTION_MULTIVALVE      13
#define OPTION_EXT_BOARDS      14
#define OPTION_RESET           15
#define NUM_OPTIONS            16

// Option flags
#define OPFLAG_NONE         0x00
#define OPFLAG_EDITABLE     0x01    // the option is editable
#define OPFLAG_WEB_EDIT     0x02    // the option is editable through the web
#define OPFLAG_BOOL         0x10    // the option takes bool values

// External I2C EEPROM
#define I2C_EEPROM_DEVICE_ADDR 0x50
#define EEPROM_BLOCK_SIZE      16

// ====== Arduino Pin Assignments ======
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
#define ETHER_BUFFER_SIZE    950
#define TMP_BUFFER_SIZE        20

// ====== Global Variables ======
extern LiquidCrystal lcd;
extern byte options[];         // stores all options
extern byte options_max[];     // max value of each option
extern char *options_str[];
extern char* days_str[];

extern byte time_display_mode;

extern char tmp_buffer[];
extern BufferFiller bfill;

extern byte valve_bitvalues[];    // scheduled open/close value of each bit, maximum 16 stations supported
extern unsigned int remaining_minutes[];
extern unsigned long time_second_counter;
extern byte lcd_display_board;
