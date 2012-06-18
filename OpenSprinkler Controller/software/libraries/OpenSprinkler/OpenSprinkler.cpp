// Arduino library code for OpenSprinkler

/* OpenSprinkler Class Implementation
   Creative Commons Attribution-ShareAlike 3.0 license
   June 2012 @ Rayshobby.net
*/

#include "OpenSprinkler.h"

// Declare static data members
LiquidCrystal OpenSprinkler::lcd(PIN_LCD_RS, PIN_LCD_EN, PIN_LCD_D4, PIN_LCD_D5, PIN_LCD_D6, PIN_LCD_D7);
StatusBits OpenSprinkler::status;
byte OpenSprinkler::station_bits[MAX_EXT_BOARDS+1];

unsigned long OpenSprinkler::raindelay_stop_time = 0;
int16_t OpenSprinkler::tm2_ov_cnt = TM2_OVCNT_LOAD_VALUE;
  
// Default option values
byte OpenSprinkler::options[NUM_OPTIONS] = {
  SVC_FW_VERSION , // firmware version
  (-4+12), // default time zone: GMT-4
  1,  // 0: use static ip, 1: use dhcp
  192,// static ip
  168, 
  1,   
  22,  
  192,// static gateway ip
  168,
  1,
  1,
  0,  // number of extension board. 0-> no extension boards
  0,	// index of master station. 0-> no master station
  1,  // sequential bit. 0-> stations can run concurrently; 1-> stations run sequentially
  0,	// rain sensor control bit. 0-> ignore rain sensor; 1-> use rain sensor
  0,  // real-time clock (RTC) bit. 0-> do not use RTC; 1-> use RTC (not available on all hardware)
  0   // reset all settings to default
};


// Option maximum values
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
  MAX_EXT_BOARDS, // ext
  8,		// mas
  1,    // seq
  1,		// urs
  1,		// rtc
  1     // reset
};

// Option display strings
prog_char _str_fwv [] PROGMEM = "Firmware ver.";
prog_char _str_tz  [] PROGMEM = "Time zone (GMT):";
prog_char _str_dhcp[] PROGMEM = "Use DHCP:";
prog_char _str_ip1 [] PROGMEM = "Static.ip1:";
prog_char _str_ip2 [] PROGMEM = "Static.ip2:";
prog_char _str_ip3 [] PROGMEM = "Static.ip3:";
prog_char _str_ip4 [] PROGMEM = "Static.ip4:";
prog_char _str_gw1 [] PROGMEM = "Gateway.ip1:";
prog_char _str_gw2 [] PROGMEM = "Gateway.ip2:";
prog_char _str_gw3 [] PROGMEM = "Gateway.ip3:";
prog_char _str_gw4 [] PROGMEM = "Gateway.ip4:";
prog_char _str_ext [] PROGMEM = "Ext. boards:";
prog_char _str_mas [] PROGMEM = "Master station:";
prog_char _str_seq [] PROGMEM = "Sequential:";
prog_char _str_urs [] PROGMEM = "Use rain sensor:";
prog_char _str_rtc [] PROGMEM = "Use RTC:";
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
  _str_ext,
  _str_mas,
  _str_seq,
  _str_urs,
  _str_rtc,
  _str_reset
};

// Option editable flags
prog_uchar OpenSprinkler::options_flag[NUM_OPTIONS] PROGMEM={
  OPFLAG_DEFAULT, // fw ver
  OPFLAG_WEB_EDIT,// tz
  OPFLAG_BOOL,    // dhcp
  OPFLAG_DEFAULT, // ip
  OPFLAG_DEFAULT,
  OPFLAG_DEFAULT,
  OPFLAG_DEFAULT,
  OPFLAG_DEFAULT, // gw
  OPFLAG_DEFAULT,
  OPFLAG_DEFAULT,
  OPFLAG_DEFAULT,
  OPFLAG_WEB_EDIT, // ext
  OPFLAG_WEB_EDIT, // mas
  OPFLAG_WEB_EDIT | OPFLAG_BOOL,// seq
  OPFLAG_WEB_EDIT | OPFLAG_BOOL,// urs    
  OPFLAG_BOOL,    // rtc
  OPFLAG_BOOL     // reset
};  

// Weekday display strings
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

// ===============
// Setup Functions
// ===============

// Arduino software reset function
void(* resetFunc) (void) = 0;

// timer2 overflow interrupt service routine
// configured to trigger at 125Hz (every 8ms)
ISR(TIMER2_OVF_vect) {  
  TCNT2 = TCNT2_LOAD_VALUE;
  if ((--OpenSprinkler::tm2_ov_cnt)!=0)  return;
  OpenSprinkler::tm2_ov_cnt = TM2_OVCNT_LOAD_VALUE;

  // time_second_counter increments every second
  // this does't have to be extremely precise
  // it is mainly used for time-keeping of tasks
  ///OpenSprinkler::time_second_counter ++;
  wdt_reset();  // resets watch dog timer every second
}

// Initialize network with given mac and http port
byte OpenSprinkler::start_network(byte mymac[], int http_port) {

  lcd_print_lines_clear_pgm(PSTR("Connecting to"), PSTR(" the network..."));

  ether.hisport = http_port;    
  
  if(!ether.begin(ETHER_BUFFER_SIZE, mymac))  return 0;
  
  if (options[OPTION_USE_DHCP]) {
    if (!ether.dhcpSetup()) return 0;
  } else {
    byte staticip[] = {
      options[OPTION_STATIC_IP1],
      options[OPTION_STATIC_IP2],
      options[OPTION_STATIC_IP3],
      options[OPTION_STATIC_IP4]    };

    byte gateway[] = {
      options[OPTION_GATEWAY_IP1],
      options[OPTION_GATEWAY_IP2],
      options[OPTION_GATEWAY_IP3],
      options[OPTION_GATEWAY_IP4]    };
    if (!ether.staticSetup(staticip, gateway))  return 0;
  }
  return 1;
}

// Reboot controller
void OpenSprinkler::reboot() {
  resetFunc();
}

// OpenSprinkler init function
void OpenSprinkler::begin() {

  // shift register setup
  pinMode(PIN_SR_LATCH, OUTPUT);
#ifdef PIN_SR_OE  
  pinMode(PIN_SR_OE, OUTPUT);
#endif
  pinMode(PIN_SR_CLOCK, OUTPUT);
  pinMode(PIN_SR_DATA,  OUTPUT);
  
  digitalWrite(PIN_SR_LATCH, HIGH);

#ifdef PIN_SR_OE
  // pull shift register OE high to disable output
  digitalWrite(PIN_SR_OE, HIGH);
#endif
 
	// Reset all stations
  clear_all_station_bits();
  apply_all_station_bits();
  
#ifdef PIN_SR_OE  
  // pull shift register OE low to enable output
  digitalWrite(PIN_SR_OE, LOW);
#endif
 
#ifdef PIN_RAINSENSOR  
  // Rain sensor port set up
  pinMode(PIN_RAINSENSOR, INPUT);
  digitalWrite(PIN_RAINSENSOR, HIGH); // enabled internal pullup
#endif
  
  // Init I2C
  Wire.begin();
  
  // Reset status variables
	status.enabled = 0;
	status.rain_delayed = 0;
	status.rain_sensed = 0;
	status.network_failed = 0;
	status.program_busy = 0;
	status.display_board = 0;

  // begin lcd
  lcd.begin(16, 2);
  
  // Atmega328 timer2 setup
  TCCR2A = 0x00;                  // normal operation
  TCCR2B |= (1<<CS22)|(1<<CS21);  // 256 pre-scalar, generating 31250Hz on 8MHz CPU
                                  // when TCNT2 overflow is set to 250,
                                  // timer2 overflow ISR will trigger at 125Hz
}

// Self_test function
void OpenSprinkler::self_test() {
	byte sid;
	while(1) {
		for(sid=0; sid<(1+options[OPTION_EXT_BOARDS])*8; sid++) {
			lcd_print_line_clear_pgm(PSTR(""), 1);
			lcd.setCursor(0, 1);
			lcd.print((int)sid+1);
			clear_all_station_bits();
			set_station_bit(sid, 1);
			apply_all_station_bits();
			delay(5000);	// run each station for 5 seconds
		}
	}
}

// ==================
// Schedule Functions
// ==================

// Index of today's weekday (Monday is 0)
byte OpenSprinkler::weekday_today() {
  return ((byte)weekday()+5)%7; // Timer::weekday() assumes Sunday is 1
}

// Set station bit
void OpenSprinkler::set_station_bit(byte sid, byte value) {
  byte bid = (sid>>3);  // board index
  byte s = sid % 8;       // station bit index
  if (value) {
    station_bits[bid] = station_bits[bid] | ((byte)1<<s);
  } 
  else {
    station_bits[bid] = station_bits[bid] &~((byte)1<<s);
  }
 	set_master_station_bit();
}		

// Reset and turn off all stations
/*void OpenSprinkler::reset_allstations() {
  schedule_clear_allstations();
  schedule_apply();
}*/

// Clear all station bits
void OpenSprinkler::clear_all_station_bits() {
  byte bid;
  for(bid=0;bid<=options[OPTION_EXT_BOARDS];bid++) {
    station_bits[bid] = 0;
  }
  set_master_station_bit();
}

// Apply all station bits
// !!! This will activate/deactivate valves !!!
void OpenSprinkler::apply_all_station_bits() {
  digitalWrite(PIN_SR_LATCH, LOW);

  byte bid, s;
  byte bitvalue;

  // Shift out all station bit values
  // from the highest bit to the lowest
  for(bid=0;bid<=MAX_EXT_BOARDS;bid++) {
    bitvalue = 0x00;
    if (status.enabled && (!status.rain_delayed) && !(options[OPTION_USE_RAINSENSOR] && status.rain_sensed))
      bitvalue = station_bits[MAX_EXT_BOARDS-bid];
    for(s=0;s<8;s++) {
      digitalWrite(PIN_SR_CLOCK, LOW);
      digitalWrite(PIN_SR_DATA, (bitvalue & ((byte)1<<(7-s))) ? HIGH : LOW );
      digitalWrite(PIN_SR_CLOCK, HIGH);          
    }
  }
  digitalWrite(PIN_SR_LATCH, HIGH);
}		

// Set master station bit
void OpenSprinkler::set_master_station_bit() {
	if (options[OPTION_MASTER_STATION] != 0) {
		// start by turning the master station's bit off (in case it's set for some reason)
		station_bits[0] = station_bits[0] & ~((byte)1<<(options[OPTION_MASTER_STATION]-1));
		for (byte bid=0;bid<=options[OPTION_EXT_BOARDS];bid++) {
			if (station_bits[bid] != 0) {
				// if any station is schedule to open, set the master station bit as well
				station_bits[0] = station_bits[0] | ((byte)1<<(options[OPTION_MASTER_STATION]-1));
				break;
			}
		}
	}
}

// =================
// Options Functions
// =================

void OpenSprinkler::options_setup() {

  if (eeprom_read_byte((unsigned char*)(ADDR_EEPROM_OPTIONS+OPTION_FW_VERSION))!=SVC_FW_VERSION ||
      eeprom_read_byte((unsigned char*)(ADDR_EEPROM_OPTIONS+OPTION_RESET))) {
    options_save(); // write default option values
    password_set(DEFAULT_PASSWORD); // write default password
    location_set(DEFAULT_LOCATION); // write default location
    
    // reset internal eeprom
    lcd_print_lines_clear_pgm(PSTR("Resetting EEPROM"), PSTR("Please wait..."));
    for(int i=ADDR_EEPROM_USER; i<INT_EEPROM_SIZE; i++) {
      eeprom_write_byte((unsigned char *) i, 0);      
    }
    // also need to reset external eeprom
    // ray: todo
  } 
  else {
    options_load();
  }

	byte button = button_read(BUTTON_WAIT_NONE);
	
	switch(button & BUTTON_MASK) {
	case BUTTON_1:
  	// if BUTTON_1 is pressed during startup, perform self-test
	  lcd_print_line_clear_pgm(PSTR("Self testing..."), 0);
	  self_test();
		break;
		
  case BUTTON_2:
  	// if BUTTON_2 is pressed during startup, jump to 'reset all options'
		ui_set_options(OPTION_RESET);
		if (options[OPTION_RESET]) {
			resetFunc();
		}
		break;
		
	case BUTTON_3:
  	// if BUTTON_3 is pressed during startup, enter Setup option mode
    lcd_print_line_clear_pgm(PSTR("==Set Options=="), 0);
    delay(DISPLAY_MSG_MS);
    lcd_print_lines_clear_pgm(PSTR("B3:sel B1/B2:chg"), PSTR("B3:hold to save"));
    delay(DISPLAY_MSG_MS+DISPLAY_MSG_MS);
    lcd.clear();
    ui_set_options(0);
    if (options[OPTION_RESET]) {
      resetFunc(); 
    }
  }
}

// Load options from internal eeprom
void OpenSprinkler::options_load() {
  for (int i=0; i<NUM_OPTIONS; i++) {
    options[i] = eeprom_read_byte((unsigned char *)(ADDR_EEPROM_OPTIONS + i));
  }
}

// Save options to internal eeprom
void OpenSprinkler::options_save() {
  // save in reverse order so SVC_FW_VERSION is written last
  for (int i=NUM_OPTIONS-1; i>=0; i--) {
    eeprom_write_byte((unsigned char *) (ADDR_EEPROM_OPTIONS + i), options[i]);
  }
}

byte OpenSprinkler::option_get_flag(int i) {
  return pgm_read_byte(options_flag+i);
}

byte OpenSprinkler::option_get_max(int i) {
  return pgm_read_byte(options_max+i);
}

// ==============================
// Controller Operation Functions
// ==============================

// Enable controller operation
void OpenSprinkler::enable() {
  status.enabled = 1;
  apply_all_station_bits();
}

// Disable controller operation
void OpenSprinkler::disable() {
  status.enabled = 0;
  apply_all_station_bits();
}

// Enable timer2 overflow interrupt
void OpenSprinkler::timer_start() {
  ///time_second_counter = 0;
  tm2_ov_cnt = TM2_OVCNT_LOAD_VALUE;
  TCNT2 = TCNT2_LOAD_VALUE;
  TIMSK2 |= (1<<TOIE2);    // turn on interrupt bit
}

// disable timer2 overflow interrupt
void OpenSprinkler::timer_stop() {
  TIMSK2 &=~(1<<TOIE2);    // turn off interrupt bit
}

void OpenSprinkler::raindelay_start(byte rd) {
  if(rd == 0) return;
  raindelay_stop_time = now() + (unsigned long) rd * 3600;
  status.rain_delayed = 1;
  apply_all_station_bits();
}

void OpenSprinkler::raindelay_stop() {
  status.rain_delayed = 0;
  apply_all_station_bits();
}

// =================
// Weather Functions
// =================
void OpenSprinkler::location_set(char* loc) {
  byte i=0;
  for (; (*loc)!=0; loc++, i++) {
    eeprom_write_byte((unsigned char*)(ADDR_EEPROM_LOCATION+i), *(loc));
  }
  eeprom_write_byte((unsigned char*)(ADDR_EEPROM_LOCATION+i), 0);  
}

void OpenSprinkler::location_get(char *buf) {
  byte c;
  byte i = 0;
  do {
    c = eeprom_read_byte((unsigned char*)(ADDR_EEPROM_LOCATION+i));
    if (c==' ') c='+';
    *(buf++) = c;
    i ++;
  } while (c != 0);
}

// =============
// LCD Functions
// =============

// Print a program memory string
void OpenSprinkler::lcd_print_pgm(PGM_P PROGMEM str) {
  uint8_t c;
  while((c=pgm_read_byte(str++))!= '\0') {
    lcd.print((char)c);
  }
}

// Print a program memory string to a given line with clearing
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

// Print two program memory strings with clearing
void OpenSprinkler::lcd_print_lines_clear_pgm(PGM_P PROGMEM str1, PGM_P PROGMEM str2) {
  lcd_print_line_clear_pgm(str1, 0);
  lcd_print_line_clear_pgm(str2, 1);
}

// Print time to a given line
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
  *p++ = ' ';
  PGM_P PROGMEM pt = days_str[weekday_today()];
  uint8_t c;
  while((c=pgm_read_byte(pt++))!= '\0') {
    *p++=c;
  }
  *p++ = ' ';
  *p++ = (month(t)/10) + '0';
  *p++ = (month(t)%10) + '0';
  *p++ = '-';
  *p++ = (day(t)/10) + '0';
  *p++ = (day(t)%10) + '0';
  *p++ = '\0';

  lcd.setCursor(0, line);
  lcd.print(timestr);
}

// print ip address and port
void OpenSprinkler::lcd_print_ip(const byte *ip, int http_port) {
  lcd.clear();
  lcd.setCursor(0, 0);
  for (byte i=0; i<3; i++) {
    lcd.print(ip[i], DEC); 
    lcd.print('.');
  }   
  lcd.print(ip[3], DEC);
  lcd.setCursor(0, 1);
  lcd.print("Port: ");
  lcd.print(http_port);
}

void OpenSprinkler::lcd_print_status() {

}

// Print station bits and controller status
void OpenSprinkler::lcd_print_station(byte line, char c) {
  lcd.setCursor(0, line);
  if (status.display_board == 0) {
    lcd.print("MC:");  // Master controller is display as 'MC'
  }
  else {
    lcd.print("E");
    lcd.print((int)status.display_board);
    lcd.print(":");   // extension boards are displayed as E1, E2...
  }
  
  if (!status.enabled) {
  	lcd_print_line_clear_pgm(PSTR("-- Disabled! -- "), 1);
  }
  else if (status.rain_delayed || (status.rain_sensed && options[OPTION_USE_RAINSENSOR])) {
    lcd_print_line_clear_pgm(PSTR("-Rain Delayed!- "), 1);
  }
  else {
	  byte bitvalue = station_bits[status.display_board];
	  for (byte s=0; s<8; s++) {
	    if (status.display_board == 0 &&(s+1) == options[OPTION_MASTER_STATION]) {
	      lcd.print('M'); // mark master station as 'M'
	    } else {
	      lcd.print((bitvalue&1) ? (char)c : '_');
	    }
  	  bitvalue >>= 1;
	  }
	}
	lcd_print_pgm(PSTR("     "));
}

// Print an option value
void OpenSprinkler::lcd_print_option(int i) {
  lcd_print_line_clear_pgm(options_str[i], 0);  
  lcd.setCursor(0, 1);
  int tz;
  switch(i) {
  case OPTION_TIMEZONE: // if this is the time zone option, do some conversion
    tz = (int)options[i]-12;
    if (tz >= 0) lcd.print('+');
    lcd.print(tz);
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
  lcd_print_pgm(PSTR("     "));
}


// ================
// Button Functions
// ================

// Wait for button
byte OpenSprinkler::button_read_busy(int value, byte waitmode, byte butt, byte is_holding) {

  int read_value;
  int hold_time = 0;

  read_value = analogRead(PIN_READ_BUTTON);

  if (waitmode==BUTTON_WAIT_NONE || (waitmode == BUTTON_WAIT_HOLD && is_holding)) {
    if (read_value <= value) return BUTTON_NONE;
    return butt | (is_holding ? BUTTON_FLAG_HOLD : 0);
  }

  while (read_value>value &&
         (waitmode == BUTTON_WAIT_RELEASE || (waitmode == BUTTON_WAIT_HOLD && hold_time<BUTTON_HOLD_MS))) {
    read_value = analogRead(PIN_READ_BUTTON);
    delay(BUTTON_DELAY_MS);
    hold_time += BUTTON_DELAY_MS;      
  };
  if (is_holding || hold_time >= BUTTON_HOLD_MS)
    butt |= BUTTON_FLAG_HOLD;
  return butt;

}

// Read button and returns button value 'OR'ed with flag bits
byte OpenSprinkler::button_read(byte waitmode)
{
  static byte old = BUTTON_NONE;
  byte curr = BUTTON_NONE;
  byte is_holding = (old&BUTTON_FLAG_HOLD);

  int read_value = analogRead(PIN_READ_BUTTON);
  delay(BUTTON_DELAY_MS);

#if SVC_HW_VERSION >= 12
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

  /* set flags in return value */
  byte ret = curr;
  if (!(old&BUTTON_MASK) && (curr&BUTTON_MASK))
    ret |= BUTTON_FLAG_DOWN;
  if ((old&BUTTON_MASK) && !(curr&BUTTON_MASK))
    ret |= BUTTON_FLAG_UP;

  old = curr;
  return ret;
}


// user interface for setting options during startup
void OpenSprinkler::ui_set_options(int oid)
{
  boolean finished = false;
  byte button;
  int i=oid;

  lcd_print_option(i);
  while(!finished) {
    button = button_read(BUTTON_WAIT_HOLD);

    switch (button & BUTTON_MASK) {
    case BUTTON_1:
      if (i==0) break;  // the first option is always firmware version, non-editable
      if (option_get_max(i) != options[i]) options[i] ++;
      break;

    case BUTTON_2:
      if (i==0) break;  // the first option is always firmware version, non-editable
      if (options[i] != 0) options[i] --;
      break;

    case BUTTON_3:
      if (!(button & BUTTON_FLAG_DOWN)) break; 
      if (button & BUTTON_FLAG_HOLD) {
        // long press, save options
        options_save();
        finished = true;
      } 
      else {
        // click, move to the next option
        if (i==OPTION_USE_DHCP && options[i]) i += 9; // if use DHCP, skip static ip set
        else  i = (i+1) % NUM_OPTIONS;
      }
      break;
    }

    if (button != BUTTON_NONE) {
      lcd_print_option(i);
    }
  }
  lcd.noBlink();
}

// User interface for setting time manually
/*
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
*/

// User interface for setting rain delay
/*
int OpenSprinkler::ui_set_raindelay()
{
  boolean finished = false;
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
  else return rd;
  return 0;
}
*/

// ==================
// Password Functions
// ==================

// store password to internal eeprom
void OpenSprinkler::password_set(char *pw) {
  byte i=0;
  for (; (*pw)!=0; pw++, i++) {
    eeprom_write_byte((unsigned char*)(ADDR_EEPROM_PASSWORD+i), *pw);
  }
  eeprom_write_byte((unsigned char*)(ADDR_EEPROM_PASSWORD+i), 0);
}

// verify if a string matches password
byte OpenSprinkler::password_verify(char *pw) {
  byte i = 0;
  byte c1, c2;
  while(1) {
    c1 = eeprom_read_byte((unsigned char*)(ADDR_EEPROM_PASSWORD+i));
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

