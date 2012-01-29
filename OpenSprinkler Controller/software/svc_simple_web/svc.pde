// Example code for Sprinkler Valve Controller (SVC)
// SVC library functions
// Licensed under GPL V2
// Dec 2011 @Rayshobby

#include <Wire.h>
#include <EEPROM.h>
#include <Time.h>
#include <LiquidCrystal.h>
#include <EtherCard.h>
#include "defines.h"

LiquidCrystal lcd(PIN_LCD_RS, PIN_LCD_EN, PIN_LCD_D4, PIN_LCD_D5, PIN_LCD_D6, PIN_LCD_D7);

unsigned int valve_bitvalue;    // scheduled open/close value of each bit, maximum 32 stations supported

byte time_display_mode = 0;

unsigned long remaining_seconds[(MAX_EXT_BOARDS+1)*8];
unsigned long scheduled_seconds[(MAX_EXT_BOARDS+1)*8];
unsigned long scheduled_stop_time[(MAX_EXT_BOARDS+1)*8];

// Option defaults values
byte options[NUM_OPTIONS] = {
  FW_VERSION,
  (-5+12), // default time zone: UTC-5
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
  1,  // 0: don't show, 1: show
  2,  // 0: disable multi valve protection; other: max# valves allowed to open at the same time
  0,  // 0: no extension boards; other: number of extension boards
  0,  // reset all settings to default
};


// Maximum value of each option
prog_uchar options_max[NUM_OPTIONS] PROGMEM = {
  0, 24, 1, 255, 255, 255, 255, 255, 255, 255, 255,
  1,  1, (MAX_EXT_BOARDS+1)*8, MAX_EXT_BOARDS, 1 };

// Option title strings
prog_char str_op0 [] PROGMEM = "FW";
prog_char str_op1 [] PROGMEM = "Time zone:";
prog_char str_op2 [] PROGMEM = "Enable DHCP: ";
prog_char str_op3 [] PROGMEM = "Static.ip1: ";
prog_char str_op4 [] PROGMEM = "Static.ip2: ";
prog_char str_op5 [] PROGMEM = "Static.ip3: ";
prog_char str_op6 [] PROGMEM = "Static.ip4: ";
prog_char str_op7 [] PROGMEM = "Gateway.ip1:";
prog_char str_op8 [] PROGMEM = "Gateway.ip2:";
prog_char str_op9 [] PROGMEM = "Gateway.ip3:";
prog_char str_op10[] PROGMEM = "Gateway.ip4:";
prog_char str_op11[] PROGMEM = "NTP sync: ";
prog_char str_op12[] PROGMEM = "Startup msg: ";
prog_char str_op13[] PROGMEM = "Multi valve: ";
prog_char str_op14[]  PROGMEM = "Ext. Boards: ";
prog_char str_op15[] PROGMEM = "Reset all? ";

// Array of option title strings
char *options_str[NUM_OPTIONS]  = {
  str_op0, str_op1, str_op2, str_op3, str_op4, str_op5, str_op6, str_op7, str_op8, str_op9,
  str_op10,str_op11,str_op12,str_op13,str_op14,str_op15 };

// Flag of each option
prog_uchar options_flag[NUM_OPTIONS] PROGMEM={
  OPFLAG_NONE,
  OPFLAG_EDITABLE | OPFLAG_WEB_EDIT,
  OPFLAG_EDITABLE | OPFLAG_BOOL,
  OPFLAG_EDITABLE,
  OPFLAG_EDITABLE,
  OPFLAG_EDITABLE,
  OPFLAG_EDITABLE,
  OPFLAG_EDITABLE,
  OPFLAG_EDITABLE,
  OPFLAG_EDITABLE,
  OPFLAG_EDITABLE, // gw.ip4
  OPFLAG_EDITABLE | OPFLAG_BOOL,
  OPFLAG_EDITABLE | OPFLAG_BOOL,
  OPFLAG_EDITABLE | OPFLAG_WEB_EDIT,
  OPFLAG_EDITABLE,
  OPFLAG_EDITABLE | OPFLAG_BOOL
};  

// Name abbrev of each weekday
prog_char str_day0[] PROGMEM = "Mon";
prog_char str_day1[] PROGMEM = "Tue";
prog_char str_day2[] PROGMEM = "Wed";
prog_char str_day3[] PROGMEM = "Thu";
prog_char str_day4[] PROGMEM = "Fri";
prog_char str_day5[] PROGMEM = "Sat";
prog_char str_day6[] PROGMEM = "Sun";

char *days_str[7] = {
  str_day0, str_day1, str_day2, str_day3, str_day4, str_day5, str_day6
};

// ==========================
// Controller Setup Functions
// ==========================

// Software reset function
void(* resetFunc) (void) = 0;

void reset_ethernet() {
  digitalWrite(PIN_ETHER_RESET, LOW);
  delay(50);
  digitalWrite(PIN_ETHER_RESET, HIGH);
}

void svc_reboot() {
  resetFunc();
}

void svc_setup() {

  // shift register setup
  pinMode(PIN_SR_LATCH, OUTPUT);
  digitalWrite(PIN_SR_LATCH, HIGH);
  pinMode(PIN_SR_CLOCK, OUTPUT);
  pinMode(PIN_SR_DATA,  OUTPUT);
  pinMode(PIN_ETHER_RESET, OUTPUT);

  // turn off valves
  valve_reset();

  // initialize variables
  time_display_mode = 1;

  // reset Ethernet module momentarily
  reset_ethernet();

  for (byte i=0; i<(MAX_EXT_BOARDS+1)*8; i++) {
    scheduled_seconds[i] = 0;
    remaining_seconds[i] = 0;
  }
  // start lcd
  lcd.begin(16, 2);
}

// =================
// Options Functions
// =================

void options_setup() {

  if (EEPROM.read(OPTION_FW_VERSION)!=FW_VERSION || EEPROM.read(OPTION_RESET)) {
    options_save();
    password_set(DEFAULT_PASSWORD);
  } 
  else {
    options_load();
  }

  // if BUTTON_3 is pressed during setup, enter option edit mode
  if ((button_read(BUTTON_WAIT_NONE) & BUTTON_MASK) == BUTTON_3) {
    lcd_print_line_clear_pgm(PSTR("==Option Setup=="), 0);
    delay(DISPLAY_MSG_MS);
    ui_set_options();
    if (options[OPTION_RESET]) {
      resetFunc(); 
    }
  }

}


// Load options from internal eeprom
void options_load() {
  for (byte i=0; i<NUM_OPTIONS; i++) {
    options[i] = EEPROM.read(ADDR_EEPROM_BASE + i);
  }
}

// Save options to internal eeprom
void options_save() {
  for (int i=NUM_OPTIONS-1; i>=0; i--) {
    EEPROM.write(ADDR_EEPROM_BASE + i, options[i]);
  }
}

byte option_get_flag(int i)
{
  return pgm_read_byte(options_flag+i);
}

byte option_get_max(int i)
{
  return pgm_read_byte(options_max+i);
}

// =======================
// Valve Control Functions
// =======================

// schedule one station
void valve_schedule(byte index, byte value) {
  if (value) {
    valve_bitvalue = valve_bitvalue | ((unsigned int)1<<index);
  } 
  else {
    valve_bitvalue = valve_bitvalue &~((unsigned int)1<<index);
  }
}		

// reset (shut down) all valves
void valve_reset() {
  valve_bitvalue = 0;
  valve_apply();
}
// apply scheduled valve values
// !!! This will activate the valves !!!
void valve_apply() {
  digitalWrite(PIN_SR_LATCH, LOW);

  for (byte i = 0; i < (MAX_EXT_BOARDS+1) * 8; i++)  {
    digitalWrite(PIN_SR_CLOCK, LOW);
    unsigned int idx = ((MAX_EXT_BOARDS+1) * 8 - 1 - i);
    digitalWrite(PIN_SR_DATA, (valve_bitvalue & ((unsigned int)1<<idx)) ? HIGH : LOW );
    digitalWrite(PIN_SR_CLOCK, HIGH);
  }

  digitalWrite(PIN_SR_LATCH, HIGH);
}		

// get schdule value for one station
byte valve_get(byte index) {
  return (valve_bitvalue>>index)&1;
}

// =============
// LCD Functions
// =============

// print a string stored in program memory space
void lcd_print_pgm(PGM_P PROGMEM str) {
  uint8_t c;
  while((c=pgm_read_byte(str++))!= '\0') {
    lcd.print(c);
  }
}

// print string to a cleared line
void lcd_print_line_clear_pgm(PGM_P PROGMEM str, byte line) {
  lcd.setCursor(0, line);
  uint8_t c;
  int8_t cnt = 0;
  while((c=pgm_read_byte(str++))!= '\0') {
    lcd.print(c);
    cnt++;
  }
  for(; (16-cnt) >= 0; cnt ++) lcd.print(' ');  
}

// print two strings on clear lines
void lcd_print_lines_clear_pgm(PGM_P PROGMEM str1, PGM_P PROGMEM str2) {
  lcd_print_line_clear_pgm(str1, 0);
  lcd_print_line_clear_pgm(str2, 1);
}

// print time to a given LCD line
void lcd_print_time(byte line)
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
void lcd_print_ip(const byte *ip, byte line)
{
  lcd.setCursor(0, line);
  for (byte i=0; i<3; i++) {
    lcd.print(ip[i], DEC); 
    lcd.print('.');
  }   
  lcd.print(ip[3], DEC);
  lcd_print_pgm(PSTR("    "));
}

void lcd_print_raindelay(byte rd, byte line)
{
  lcd.setCursor(0, line);
  lcd.print((int)rd);
  lcd_print_pgm(PSTR(" hours  "));
}

// print valve values onto lcd
byte lcd_print_valve(byte line, char c)
{
  lcd.setCursor(0, line);
  lcd.print("S:");
  byte value = valve_bitvalue & 0xFF;
  for (byte i=0; i<8; i++) {
    lcd.print((value&1) ? c : '_');
    value >>= 1;
  }
  lcd.print(' ');
  lcd_print_pgm(PSTR("(web)"));
  return 2; // strlen("Stn:");
}

// print an option value to lcd
void lcd_print_option(int i)
{
  //xxxlcd_print_line_clear_pgm(options_aux_str[i], 1);
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

// ================
// Button Functions
// ================

// busy wait for button
byte button_read_busy(int value, byte waitmode, byte butt, byte is_holding) {

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
byte button_read(byte waitmode)
{
  static byte old = BUTTON_NONE;
  byte curr = BUTTON_NONE;
  byte is_holding = (old&BUTTON_FLAG_HOLD);

  int read_value = analogRead(PIN_READ_BUTTON);
  delay(BUTTON_DELAY_MS);

  if (read_value > 750) {
    curr = button_read_busy(750, waitmode, BUTTON_1, is_holding);
  }
  else if (read_value > 300) {
    curr = button_read_busy(300, waitmode, BUTTON_2, is_holding);
  }
  else if (read_value > 100) {
    curr = button_read_busy(100, waitmode, BUTTON_3, is_holding);
  } 

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
void ui_set_options()
{
  boolean finished = false;
  byte button;
  int i=2;

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
void ui_toggle_time_display() {
  time_display_mode = 1-time_display_mode;
}

// user interface for setting time manually
void ui_set_time() {

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

// ==================
// Password Functions
// ==================

// store password to eeprom
void password_set(char *pw) {
  byte i=0;
  for (; (*pw)!=0; pw++, i++) {
    EEPROM.write(ADDR_EEPROM_PASSWORD+i, *pw);
  }
  EEPROM.write(ADDR_EEPROM_PASSWORD+i, 0);
}

// verify a string matches password
byte password_verify(char *pw) {
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
byte weekday_today() {
  return ((byte)weekday()+5)%7;  
}



