// Arduino library code for OpenSprinkler

/* Class definition of OpenSprinkler
   Creative Commons Attribution-ShareAlike 3.0 license
   Mar 2012 @ Rayshobby.net
*/

#include "OpenSprinkler.h"

// define class data members
LiquidCrystal OpenSprinkler::lcd(PIN_LCD_RS, PIN_LCD_EN, PIN_LCD_D4, PIN_LCD_D5, PIN_LCD_D6, PIN_LCD_D7);
byte OpenSprinkler::lcd_display_board = 0;
byte OpenSprinkler::time_display_mode = 1;
byte OpenSprinkler::enabled = 1;
byte OpenSprinkler::raindelayed = 0;
byte OpenSprinkler::ext_eeprom_busy = 0;
byte OpenSprinkler::station_bitvalues[MAX_EXT_BOARDS+1];
unsigned int OpenSprinkler::remaining_time[(MAX_EXT_BOARDS+1)*8];

// Option defaults values
byte OpenSprinkler::options[NUM_OPTIONS] = {
  SVC_FW_VERSION , // firmware version
  (-4+12), // default time zone: UTC-4
  1,  // 0: static ip, 1: dhcp
  192,// static ip
  168, 
  1,   
  22,  
  192,// static gateway ip
  168,
  1,
  1,
  1,  // 0: disable NTP sync, 1: enable
  6,  // start of a schedule day (in hours)
  21, // end of a schedule day (in hours)  
  2,  // 0: disable multi station protection; other: max# stations allowed to open simultaneously.
  0,  // 0: no extension boards; other: number of extension boards
  1,	// 0: continue when network init fails; 1: reboot when network init fails
	0,	// index of the master station. 0 means no master station
	0,	// 0: do not integrate rain sensor; 1: integrate rain sensor
  0   // reset all settings to default
};


// Maximum value of each option
prog_uchar OpenSprinkler::options_max[NUM_OPTIONS] PROGMEM = {
  0,		// fw_ver
  24,		// tz
  1,		// dhcp
  255,	// ip
  255,
  255,
  255,
  255,	// gw
  255,
  255,
  255,
  1,		// ntp
  12,		// ds
  24,		// de
  (MAX_EXT_BOARDS+1)*8,	// ms
  MAX_EXT_BOARDS,				// ext
  1,		// rn
  8,		// ma
  1,		// rs
  1											// reset
};

// Display string of each option
prog_char _str_fwv [] PROGMEM = "FW";
prog_char _str_tz  [] PROGMEM = "Time zone:";
prog_char _str_dhcp[] PROGMEM = "Use DHCP: ";
prog_char _str_ip1 [] PROGMEM = "Static.ip1: ";
prog_char _str_ip2 [] PROGMEM = "Static.ip2: ";
prog_char _str_ip3 [] PROGMEM = "Static.ip3: ";
prog_char _str_ip4 [] PROGMEM = "Static.ip4: ";
prog_char _str_gw1 [] PROGMEM = "Gateway.ip1:";
prog_char _str_gw2 [] PROGMEM = "Gateway.ip2:";
prog_char _str_gw3 [] PROGMEM = "Gateway.ip3:";
prog_char _str_gw4 [] PROGMEM = "Gateway.ip4:";
prog_char _str_ntp [] PROGMEM = "NTP sync: ";
prog_char _str_ds  [] PROGMEM = "Start hour:";
prog_char _str_de  [] PROGMEM = "End hour  :";
prog_char _str_ms  [] PROGMEM = "Multi station:";
prog_char _str_ext [] PROGMEM = "Ext. Boards: ";
prog_char _str_rn  [] PROGMEM = "Require net: ";
prog_char _str_ma  [] PROGMEM = "Master stn:";
prog_char _str_rs  [] PROGMEM = "Rain sensor:";
prog_char _str_reset[] PROGMEM = "Reset all? ";

char* OpenSprinkler::options_str[NUM_OPTIONS]  = {
  _str_fwv,
  _str_tz,
  _str_dhcp,
  _str_ip1,
  _str_ip2,
  _str_ip3,
  _str_ip4,
  _str_gw1,
  _str_gw2,
  _str_gw3,
  _str_gw4,
  _str_ntp,
  _str_ds,
  _str_de,
  _str_ms,
  _str_ext,
  _str_rn,
  _str_ma,
  _str_rs,
  _str_reset
};

// Flag of each option
prog_uchar OpenSprinkler::options_flag[NUM_OPTIONS] PROGMEM={
  OPFLAG_NONE,	// fw ver
  OPFLAG_EDITABLE | OPFLAG_WEB_EDIT,	// tz
  OPFLAG_EDITABLE | OPFLAG_BOOL,			// dhcp
  OPFLAG_EDITABLE,	// ip
  OPFLAG_EDITABLE,
  OPFLAG_EDITABLE,
  OPFLAG_EDITABLE,
  OPFLAG_EDITABLE,	// gw
  OPFLAG_EDITABLE,
  OPFLAG_EDITABLE,
  OPFLAG_EDITABLE,
  OPFLAG_EDITABLE | OPFLAG_BOOL,		 // ntp
  OPFLAG_EDITABLE | OPFLAG_WEB_EDIT, // ds
  OPFLAG_EDITABLE | OPFLAG_WEB_EDIT, // de
  OPFLAG_EDITABLE | OPFLAG_WEB_EDIT, // ms
  OPFLAG_EDITABLE | OPFLAG_WEB_EDIT, // ext
  OPFLAG_EDITABLE | OPFLAG_BOOL,		 // rn
  OPFLAG_EDITABLE, // ma
  OPFLAG_EDITABLE | OPFLAG_BOOL, // rs    
  OPFLAG_EDITABLE | OPFLAG_BOOL      // reset
};  

// Name abbrev of each weekday
prog_char str_day0[] PROGMEM = "Mon";
prog_char str_day1[] PROGMEM = "Tue";
prog_char str_day2[] PROGMEM = "Wed";
prog_char str_day3[] PROGMEM = "Thu";
prog_char str_day4[] PROGMEM = "Fri";
prog_char str_day5[] PROGMEM = "Sat";
prog_char str_day6[] PROGMEM = "Sun";

char* OpenSprinkler::days_str[7] = {
  str_day0,
  str_day1,
  str_day2,
  str_day3,
  str_day4,
  str_day5,
  str_day6
};

// ==========================
// Controller Setup Functions
// ==========================

// Software reset function
void(* resetFunc) (void) = 0;

// Reset ethernet controller
void OpenSprinkler::reset_ethernet() {
#if SVC_HW_VERSION == 12

#else
  pinMode(PIN_ETHER_RESET, OUTPUT);
  digitalWrite(PIN_ETHER_RESET, LOW);
  delay(50);
  digitalWrite(PIN_ETHER_RESET, HIGH);
#endif
}

// Reboot controller
void OpenSprinkler::reboot() {
  resetFunc();
}

// Main setup function
void OpenSprinkler::begin() {

  // shift register setup
  pinMode(PIN_SR_LATCH, OUTPUT);
  digitalWrite(PIN_SR_LATCH, HIGH);
  pinMode(PIN_SR_CLOCK, OUTPUT);
  pinMode(PIN_SR_DATA,  OUTPUT);

	// Reset all stations
  station_reset();
  
  // Init I2C (used for external eeprom)
  Wire.begin();
  
  // initialize variables
	lcd_display_board = 0;
	time_display_mode = 1;
	ext_eeprom_busy = 0;
	enabled = 1;
	raindelayed = 0;
	
  // reset Ethernet module
  reset_ethernet();
  
  for (byte i=0; i<(MAX_EXT_BOARDS+1)*8; i++) {
    set_station_scheduled_seconds(i, 0);
    remaining_time[i] = 0;
  }
  // start lcd
  lcd.begin(16, 2);
}

// perform self_test
void OpenSprinkler::self_test() {
	byte i;
	while(1) {
		for(i=0; i<(1+options[OPTION_EXT_BOARDS])*8; i++) {
			lcd_print_line_clear_pgm(PSTR(""), 1);
			lcd.setCursor(0, 1);
			lcd.print((int)i+1);
			station_schedule_clear();
			station_schedule(i, 1);
			station_apply();
			delay(5000);	// open each station for 5 seconds
		}
	}
}

// =================
// Options Functions
// =================

void OpenSprinkler::options_setup() {

  if (EEPROM.read(OPTION_FW_VERSION)!=SVC_FW_VERSION || EEPROM.read(OPTION_RESET)) {
    options_save();
    password_set(DEFAULT_PASSWORD);
    
    // erase external eeprom
    lcd_print_lines_clear_pgm(PSTR("Resetting EEPROM"), PSTR("Please wait..."));
    ext_eeprom_clear(0, I2C_EEPROM_SIZE);
    
    // flash debug light to indicate that initialization is ready
    pinMode(PIN_DEBUG, OUTPUT);
    digitalWrite(PIN_DEBUG, HIGH);
    delay(500);
    digitalWrite(PIN_DEBUG, LOW);
    pinMode(PIN_DEBUG, INPUT);
    digitalWrite(PIN_DEBUG, HIGH);    
  } 
  else {
    options_load();
  }

	byte button = button_read(BUTTON_WAIT_NONE);
	
	switch(button & BUTTON_MASK) {
	// if BUTTON_1 is pressed during startup
	case BUTTON_1:
	  lcd_print_line_clear_pgm(PSTR("Self testing..."), 0);
	  self_test();
		break;
		
	// if BUTTON_2 is pressed during startup, reset all options
	case BUTTON_2:
		ui_set_options(OPTION_RESET);
		if (options[OPTION_RESET]) {
			resetFunc();
		}
		break;
	// if BUTTON_3 is pressed during startup, enter option setup mode
	case BUTTON_3:
    lcd_print_line_clear_pgm(PSTR("==Option Setup=="), 0);
    delay(DISPLAY_MSG_MS);
    ui_set_options(1);
    if (options[OPTION_RESET]) {
      resetFunc(); 
    }
  }
}

// Load options from internal eeprom
void OpenSprinkler::options_load() {
  for (byte i=0; i<NUM_OPTIONS; i++) {
    options[i] = EEPROM.read(ADDR_EEPROM_BASE + i);
  }
}

// Save options to internal eeprom
void OpenSprinkler::options_save() {
  for (int i=NUM_OPTIONS-1; i>=0; i--) {
    EEPROM.write(ADDR_EEPROM_BASE + i, options[i]);
  }
}

byte OpenSprinkler::option_get_flag(int i)
{
  return pgm_read_byte(options_flag+i);
}

byte OpenSprinkler::option_get_max(int i)
{
  return pgm_read_byte(options_max+i);
}

// =======================
// Station Control Functions
// =======================

// schedule the master station
void OpenSprinkler::master_schedule() {
	if (options[OPTION_MASTER_STATION] != 0) {
		// begin by turning the master station off
		station_bitvalues[0] = station_bitvalues[0] & ~((byte)1<<(options[OPTION_MASTER_STATION]-1));
		for (byte b=0;b<=options[OPTION_EXT_BOARDS];b++) {
			if (station_bitvalues[b] != 0) {
				// if any station is schedule to open, then schedule the master station
				station_bitvalues[0] = station_bitvalues[0] | ((byte)1<<(options[OPTION_MASTER_STATION]-1));
				break;
			}
		}
	}
}

// schedule all stations on a single board
void OpenSprinkler::board_schedule(byte bidx, byte value) {
	station_bitvalues[bidx] = value;
	master_schedule();
}

// schedule one station
void OpenSprinkler::station_schedule(byte index, byte value) {
  byte b = (index>>3);
  byte i = index % 8;
  if (value) {
    station_bitvalues[b] = station_bitvalues[b] | ((byte)1<<i);
  } 
  else {
    station_bitvalues[b] = station_bitvalues[b] &~((byte)1<<i);
  }
 	master_schedule();
}		

// reset (shut down) all stations
void OpenSprinkler::station_reset() {
  byte b;
  for(b=0;b<=options[OPTION_EXT_BOARDS];b++) {
    station_bitvalues[b] = 0;
  }
  master_schedule();
  station_apply();
}

// clear the schedules of all stations
void OpenSprinkler::station_schedule_clear() {
  byte b;
  for(b=0;b<=options[OPTION_EXT_BOARDS];b++) {
    station_bitvalues[b] = 0;
  }
  master_schedule();
}

// apply scheduled station values
// !!! This will activate the stations !!!
void OpenSprinkler::station_apply() {
  digitalWrite(PIN_SR_LATCH, LOW);

  byte b, i;
  byte bitvalue;

  // Shift out all station bit values
  // from the highest bit to the lowest
  for(b=0;b<=MAX_EXT_BOARDS;b++) {
    bitvalue = (enabled && !raindelayed) ? station_bitvalues[MAX_EXT_BOARDS-b] : 0x00;
    for(i=0;i<8;i++) {
      digitalWrite(PIN_SR_CLOCK, LOW);
      digitalWrite(PIN_SR_DATA, (bitvalue & ((byte)1<<(7-i))) ? HIGH : LOW );
      digitalWrite(PIN_SR_CLOCK, HIGH);          
    }
  }
  digitalWrite(PIN_SR_LATCH, HIGH);
}		

boolean OpenSprinkler::multistation_check() {
	byte total = 0;
	byte i, s, bitvalue;
	for(i=0; i<=options[OPTION_EXT_BOARDS]; i++) {
		bitvalue = station_bitvalues[i];
		for(s=0; s<8; s++) {
			if (bitvalue&1) total++;
			bitvalue >>= 1;
		}
	}
	if (options[OPTION_MULTISTATION] && (total > options[OPTION_MULTISTATION]))
		return false;
	else
		return true;
}

// =============
// LCD Functions
// =============

// print a string stored in program memory space
void OpenSprinkler::lcd_print_pgm(PGM_P PROGMEM str) {
  uint8_t c;
  while((c=pgm_read_byte(str++))!= '\0') {
    lcd.print((char)c);
  }
}

// print string to a cleared line
void OpenSprinkler::lcd_print_line_clear_pgm(PGM_P PROGMEM str, byte line) {
  lcd.setCursor(0, line);
  uint8_t c;
  int8_t cnt = 0;
  while((c=pgm_read_byte(str++))!= '\0') {
    lcd.print((char)c);
    cnt++;
  }
  for(; (16-cnt) >= 0; cnt ++) lcd.print(' ');  
}

// print two strings on clear lines
void OpenSprinkler::lcd_print_lines_clear_pgm(PGM_P PROGMEM str1, PGM_P PROGMEM str2) {
  lcd_print_line_clear_pgm(str1, 0);
  lcd_print_line_clear_pgm(str2, 1);
}

// print time to a given LCD line
void OpenSprinkler::lcd_print_time(byte line)
{
  char timestr[17];
  char *p = timestr;
  time_t t = now();
  *p++ = (hour(t)/10) + '0';
  *p++ = (hour(t)%10) + '0';
  *p++ = ':';
  *p++ = (minute(t)/10) + '0';
  *p++ = (minute(t)%10) + '0';
  *p++ = ' ';
  if (time_display_mode == 0) {
    *p++ = (year(t)/1000) + '0';
    *p++ = ((year(t)/100)%10) + '0';
    *p++ = ((year(t)/10)%10) + '0';
    *p++ = (year(t)%10) + '0';
    *p++ = '-';
  } 
  else {
    *p++ = ' ';
    PGM_P PROGMEM pt = days_str[weekday_today()];
    uint8_t c;
    while((c=pgm_read_byte(pt++))!= '\0') {
      *p++=c;
    }
    *p++ = ' ';
  }
  *p++ = (month(t)/10) + '0';
  *p++ = (month(t)%10) + '0';
  *p++ = '-';
  *p++ = (day(t)/10) + '0';
  *p++ = (day(t)%10) + '0';
  *p++ = '\0';

  lcd.setCursor(0, line);
  lcd.print(timestr);
}

// print ip address onto lcd
void OpenSprinkler::lcd_print_ip(const byte *ip, byte line)
{
  lcd.setCursor(0, line);
  for (byte i=0; i<3; i++) {
    lcd.print(ip[i], DEC); 
    lcd.print('.');
  }   
  lcd.print(ip[3], DEC);
  lcd_print_pgm(PSTR("    "));
}

// print station values onto lcd
byte OpenSprinkler::lcd_print_station(byte line, char c)
{
  lcd.setCursor(0, line);
  if (lcd_display_board == 0) {
    lcd.print("MC:");  // display master controller
  }
  else {
    lcd.print("E");
    lcd.print((int)lcd_display_board);
    lcd.print(":");
  }
  
  if (!enabled) {
  	lcd_print_pgm(PSTR("-STOPPED-"));
  }
  else if (raindelayed) {
    lcd_print_pgm(PSTR("-DELAYED-"));
  }
  else {
	  byte bitvalue = station_bitvalues[lcd_display_board];
	  for (byte i=0; i<8; i++) {
  	  lcd.print((bitvalue&1) ? (char)c : '_');
  	  bitvalue >>= 1;
	  }
	}
	lcd_print_pgm(PSTR("     "));
  return 3;
}

// print an option value to lcd
void OpenSprinkler::lcd_print_option(int i)
{
  lcd.setCursor(0, 0);
  lcd_print_pgm(options_str[i]);  
  int tz;
  switch(i) {
  case OPTION_TIMEZONE: // if this is the time zone option, do conversion
    tz = (int)options[i]-12;
    if (tz >= 0) lcd.print('+');
    lcd.print(tz);
    lcd_print_pgm(PSTR(":00"));
    break;
  case OPTION_DAY_START: // if these are the day start and day end options
  case OPTION_DAY_END:
    lcd.print((int)options[i]);
    lcd_print_pgm(PSTR(":00"));
    break;  
  default:
    // if this is a boolean option
    if (option_get_flag(i) & OPFLAG_BOOL)
      lcd_print_pgm(options[i] ? PSTR("Yes") : PSTR("No"));
    else
      lcd.print((int)options[i]);
    break;
  }
  lcd_print_pgm(PSTR("    "));
  lcd.setCursor(15, 0);
}

void OpenSprinkler::lcd_print_raindelay(byte rd, byte line) {
  lcd.setCursor(0, line);
  lcd.print((int)rd);
  lcd_print_pgm(PSTR(" hours  "));
}

// ================
// Button Functions
// ================

// busy wait for button
byte OpenSprinkler::button_read_busy(int value, byte waitmode, byte butt, byte is_holding) {

  int read_value;
  int hold_time = 0;

  read_value = analogRead(PIN_READ_BUTTON);
  boolean hold_mask = (waitmode == BUTTON_WAIT_HOLD) ? true : false;

  if ((waitmode==BUTTON_WAIT_NONE) || (hold_mask && is_holding)) {
    if (read_value <= value) return BUTTON_NONE;
    return butt | (is_holding ? BUTTON_FLAG_HOLD : 0);
  }

  while (read_value>value && (hold_mask && (hold_time<BUTTON_HOLD_MS))) {
    read_value = analogRead(PIN_READ_BUTTON);
    delay(BUTTON_DELAY_MS);
    hold_time += BUTTON_DELAY_MS;      
  };
  if (is_holding || hold_time >= BUTTON_HOLD_MS)
    butt |= BUTTON_FLAG_HOLD;
  return butt;

}

// read button and returns button value and status
byte OpenSprinkler::button_read(byte waitmode)
{
  static byte old = BUTTON_NONE;
  byte curr = BUTTON_NONE;
  byte is_holding = (old&BUTTON_FLAG_HOLD);

  int read_value = analogRead(PIN_READ_BUTTON);
  delay(BUTTON_DELAY_MS);

#if SVC_HW_VERSION == 12
  if (read_value > 450) {
    curr = button_read_busy(450, waitmode, BUTTON_1, is_holding);
  }
  else if (read_value > 300) {
    curr = button_read_busy(300, waitmode, BUTTON_2, is_holding);
  }
  else if (read_value > 150) {
    curr = button_read_busy(150, waitmode, BUTTON_3, is_holding);
  } 
#else
  if (read_value > 750) {
    curr = button_read_busy(750, waitmode, BUTTON_1, is_holding);
  }
  else if (read_value > 300) {
    curr = button_read_busy(300, waitmode, BUTTON_2, is_holding);
  }
  else if (read_value > 100) {
    curr = button_read_busy(100, waitmode, BUTTON_3, is_holding);
  } 
#endif

  /* set button flag in return value */
  byte ret = curr;
  if (!(old&BUTTON_MASK) && (curr&BUTTON_MASK))
    ret |= BUTTON_FLAG_DOWN;
  if ((old&BUTTON_MASK) && !(curr&BUTTON_MASK))
    ret |= BUTTON_FLAG_UP;

  old = curr;
  return ret;
}


// user interface for setting options during startup
void OpenSprinkler::ui_set_options(int which_option)
{
  boolean finished = false;
  byte button;
  int i=which_option;

  lcd_print_option(i);
  while(!finished) {
    button = button_read(BUTTON_WAIT_HOLD);

    switch (button & BUTTON_MASK) {
    case BUTTON_1:
      if (option_get_max(i) != options[i]) options[i] ++;
      break;

    case BUTTON_2:
      if (options[i] != 0) options[i] --;
      break;

    case BUTTON_3:
      if (!(button & BUTTON_FLAG_DOWN)) break; 
      if (button & BUTTON_FLAG_HOLD) {
        options_save();
        finished = true;
      } 
      else {
        do {
          i = (i+1) % NUM_OPTIONS;
        } 
        while((option_get_flag(i)&OPFLAG_EDITABLE)==0);  // skip non-editable options
      }

      break;
    }

    if (button != BUTTON_NONE) {
      (option_get_flag(i) & OPFLAG_EDITABLE) ? lcd.blink() : lcd.noBlink();
      lcd_print_option(i);
    }
  }
  lcd.noBlink();
}

// toggle time display mode
void OpenSprinkler::ui_toggle_time_display() {
  time_display_mode = 1-time_display_mode;
}

// on-board user interface for setting time manually
void OpenSprinkler::ui_set_time() {
  byte odm = time_display_mode;

  time_display_mode = 0;  // turn to full time display
  lcd_print_time(0);  

  lcd_print_line_clear_pgm(PSTR("Set current time"), 1);
  lcd.blink();
  lcd.setCursor(0,0);

  int field = 0;
  int cursor_move[5] = {
    1, 4, 9, 12, 15  };
  lcd.setCursor(cursor_move[0], 0);

  time_t t = now();
  boolean finished = false;
  boolean timeout = false;
  byte button;

  time_t te = ((millis()/1000)) + BUTTON_IDLE_TIMEOUT;

  while (!finished && !timeout)
  {
    if ((millis()/1000) == te) timeout = true;
    button = button_read(BUTTON_WAIT_HOLD);
    switch(button & BUTTON_MASK) {

    case BUTTON_1:
      if (field == 0)  setTime((hour(t)+1)%24,minute(t),second(t),day(t),month(t),year(t));  // try to increment field
      else if (field == 1) setTime(hour(t),(minute(t)+1)%60,second(t),day(t),month(t),year(t));
      else if (field == 2)  setTime(hour(t),minute(t),second(t),day(t),month(t),year(t)+1);
      else if (field == 3)  setTime(hour(t),minute(t),second(t),day(t),((month(t)-1+1)%12)+1,year(t));
      else  setTime(hour(t),minute(t),second(t),((day(t)-1+1)%31)+1,month(t),year(t));
      break;

    case BUTTON_2:
      if (field == 0)  setTime((hour(t)+24-1)%24,minute(t),second(t),day(t),month(t),year(t));  // try to increment field
      else if (field == 1) setTime(hour(t),(minute(t)+60-1)%60,second(t),day(t),month(t),year(t));
      else if (field == 2) setTime(hour(t),minute(t),second(t),day(t),month(t),year(t)-1);
      else if (field == 3) setTime(hour(t),minute(t),second(t),day(t),((month(t)+11-1)%12)+1,year(t));
      else  setTime(hour(t),minute(t),second(t),((day(t)+30-1)%31)+1,month(t),year(t));
      break;

    case BUTTON_3:
      if (!(button & BUTTON_FLAG_DOWN)) break; 
      if (button & BUTTON_FLAG_HOLD)  finished = true;
      else
      {
        field = (field + 1) % 5;
        lcd.setCursor(cursor_move[field],0);
      }
      break;
    }

    if (button != BUTTON_NONE) {
      t = now();
      lcd_print_time(0);          
      lcd.setCursor(cursor_move[field],0);
      te = (millis() / 1000) + BUTTON_IDLE_TIMEOUT;
    }
  }
  lcd.noBlink();
  lcd.clear();

  time_display_mode = odm;
}

// on-board user interface for seting rain delay
int OpenSprinkler::ui_set_raindelay()
{
	// ======> remove the comments if you need this feature
  /*boolean finished = false;
  boolean timeout = false;
  byte button;
  
  byte rd = 0;
  
  lcd.clear();
  lcd_print_line_clear_pgm(PSTR("Set rain delay"), 1);
  lcd_print_raindelay(rd, 0);
  lcd.blink();
  
  time_t te = ((millis()/1000)) + BUTTON_IDLE_TIMEOUT;

  while (!finished && !timeout) {
    button = button_read(BUTTON_WAIT_HOLD);
   
    if ((millis()/1000) == te)
      timeout = true;   
    if (!(button & BUTTON_FLAG_DOWN)) continue;  // repond only to button down event
    
   switch(button & BUTTON_MASK) {
    case BUTTON_2:
      rd = (rd+24) % SC_RAINDELAY_MAX;    // longest rain delay time is a week: 5 days
      break;
    case BUTTON_3:
      if (!(button & BUTTON_FLAG_DOWN)) break;
      if (button & BUTTON_FLAG_HOLD) {
        finished = true; 
      }
    }
    
    if (button != BUTTON_NONE) {
      lcd_print_raindelay(rd, 0);
      te = (millis() / 1000) + BUTTON_IDLE_TIMEOUT;  // time out in 10 seconds
    }
  }
  
  lcd.noBlink();
  if (timeout)  return -1;
  else return rd;*/
  return 0;
}

// ==================
// Password Functions
// ==================

// store password to eeprom
void OpenSprinkler::password_set(char *pw) {
  byte i=0;
  for (; (*pw)!=0; pw++, i++) {
    EEPROM.write(ADDR_EEPROM_PASSWORD+i, *pw);
  }
  EEPROM.write(ADDR_EEPROM_PASSWORD+i, 0);
}

// verify a string matches password
byte OpenSprinkler::password_verify(char *pw) {
  byte i = 0;
  byte c1, c2;
  while(1) {
    c1 = EEPROM.read(ADDR_EEPROM_PASSWORD+i);
    c2 = *pw;
    if (c1==0 || c2==0)
      break;
    if (c1!=c2) {
      return 0;
    }
    i++;
    pw++;
  }
  return (c1==c2) ? 1 : 0;

}

// ==================
// Schedule Functions
// ==================

// weekday index of today
byte OpenSprinkler::weekday_today() {
  return ((byte)weekday()+5)%7;  
}

// each station schedule data takes 2 unsigned longs
// get the scheduled seconds for station i
unsigned long OpenSprinkler::get_station_scheduled_seconds(byte i) {
  unsigned long value;
  int_eeprom_read_buffer(i*2*sizeof(unsigned long), (byte*)&value, sizeof(unsigned long));
  return value;  
}

void OpenSprinkler::set_station_scheduled_seconds(byte i, unsigned long value) {
  int_eeprom_write_buffer(i*2*sizeof(unsigned long), (byte*)&value, sizeof(unsigned long));
}

// get the scheduled stop time for station i
unsigned long OpenSprinkler::get_station_scheduled_stop_time(byte i) {
  unsigned long value;
  int_eeprom_read_buffer((i*2+1)*sizeof(unsigned long), (byte*)&value, sizeof(unsigned long));
  return value;  
}

void OpenSprinkler::set_station_scheduled_stop_time(byte i, unsigned long value) {
  int_eeprom_write_buffer((i*2+1)*sizeof(unsigned long), (byte*)&value, sizeof(unsigned long));
}

// ================
// EEPROM Functions
// ================

void OpenSprinkler::int_eeprom_read_buffer(unsigned int address, byte* buffer, byte length) {
	eeprom_read_block((void*)buffer, (const void *)(ADDR_EEPROM_USER+address), length);
}

void OpenSprinkler::int_eeprom_write_buffer (unsigned int address, byte* buffer, byte length) {
	eeprom_write_block((const void*)buffer, (void *)(ADDR_EEPROM_USER+address), length);
}
	
byte OpenSprinkler::ext_eeprom_write_lock() {
	if(ext_eeprom_busy) return 0;
	else {
		ext_eeprom_busy = 1;
		return 1;
	}
}

void OpenSprinkler::ext_eeprom_write_unlock() {
	ext_eeprom_busy = 0;
}

void OpenSprinkler::ext_eeprom_clear(unsigned int start, unsigned int end)
{
	byte cc[] = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255};
  for(unsigned int addr=start; addr<end; addr += EEPROM_BLOCK_SIZE) {
  	ext_eeprom_write_buffer(addr, cc, EEPROM_BLOCK_SIZE);
  }
}

void OpenSprinkler::ext_eeprom_write_byte(unsigned int eeaddress, byte data) {
  int rdata = data;
  Wire.beginTransmission(I2C_EEPROM_DEVICE_ADDR);
  Wire.write((int)(eeaddress >> 8)); // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  Wire.write(rdata);
  Wire.endTransmission();
}

// WARNING: address is a page address, 6-bit end will wrap around
// also, data can be maximum of about 30 bytes, because the Wire library has a buffer of 32 bytes
void OpenSprinkler::ext_eeprom_write_buffer(unsigned int eeaddresspage, byte* buffer, byte length) {
  Wire.beginTransmission(I2C_EEPROM_DEVICE_ADDR);
  Wire.write((int)(eeaddresspage >> 8)); // MSB
  Wire.write((int)(eeaddresspage & 0xFF)); // LSB
  byte c;
  for ( c = 0; c < length; c++)
    Wire.write(buffer[c]);
  Wire.endTransmission();
  delay(10);
}

byte OpenSprinkler::ext_eeprom_read_byte(unsigned int eeaddress) {
  byte rdata = 0xFF;
  Wire.beginTransmission(I2C_EEPROM_DEVICE_ADDR);
  Wire.write((int)(eeaddress >> 8)); // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  Wire.endTransmission();
  Wire.requestFrom(I2C_EEPROM_DEVICE_ADDR,1);
  if (Wire.available()) rdata = Wire.read();
  return rdata;
}

// maybe let's not read more than 30 or 32 bytes at a time!
void OpenSprinkler::ext_eeprom_read_buffer(unsigned int eeaddress, byte *buffer, int length) {
  Wire.beginTransmission(I2C_EEPROM_DEVICE_ADDR);
  Wire.write((int)(eeaddress >> 8)); // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  Wire.endTransmission();
  Wire.requestFrom(I2C_EEPROM_DEVICE_ADDR,length);
  int c = 0;
  for ( c = 0; c < length; c++ )
    if (Wire.available()) buffer[c] = Wire.read();
}

