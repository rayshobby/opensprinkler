// Example code for Sprinkler Valve Controller (SVC)
// SVC library functions
// Licensed under GPL V2
// Sep 2011 @Rayshobby

#include <Wire.h>
#include <EEPROM.h>
#include <Time.h>
#include <LiquidCrystal.h>
#include <EtherCard.h>
#include "defines.h"

LiquidCrystal lcd(PIN_LCD_RS, PIN_LCD_EN, PIN_LCD_D4, PIN_LCD_D5, PIN_LCD_D6, PIN_LCD_D7);

byte valve_bitvalue = 0;    // scheduled open/close value of each bit
byte valve_enabled = 0;     // operation enable status
byte valve_raindelayed = 0; // raindelay status

byte running_mode = 0;
byte time_display_mode = 0;
byte eeprom_busy = 0;

int  manual_scheduled_minutes;
unsigned long manual_stop_time;
unsigned long manual_running_seconds;
unsigned long raindelay_stop_time;
byte raindelay_stop_clocktime[4];

// Option defaults values
byte options[NUM_OPTIONS] = {
    FW_VERSION,
    1,  // 0: manual, 1: web
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
    1,  // 0: don't show, 1: show
    2,  // 0: disable multi valve protection; other: max# valves allowed to open at the same time
    0,  // station selection (manual mode)
    0,  // duration in hours (manual mode)
    30, // duration in minuts(manual mode)
    0,  // reset all settings to default
};


// Maximum value of each option
prog_uchar options_max[NUM_OPTIONS] PROGMEM = {
  0, 1, 24, 1, 255, 255, 255, 255, 255, 255,
  255, 255, 1, 12, 24, 1, 8, 0, 23, 59, 1 };
  
// Option title strings
prog_char str_op0 [] PROGMEM = "FW";
prog_char str_op1 [] PROGMEM = "Web mode: ";
prog_char str_op2 [] PROGMEM = "Time zone:";
prog_char str_op3 [] PROGMEM = "Enable DHCP: ";
prog_char str_op4 [] PROGMEM = "Static.ip1: ";
prog_char str_op5 [] PROGMEM = "Static.ip2: ";
prog_char str_op6 [] PROGMEM = "Static.ip3: ";
prog_char str_op7 [] PROGMEM = "Static.ip4: ";
prog_char str_op8 [] PROGMEM = "Gateway.ip1:";
prog_char str_op9 [] PROGMEM = "Gateway.ip2:";
prog_char str_op10[] PROGMEM = "Gateway.ip3:";
prog_char str_op11[] PROGMEM = "Gateway.ip4:";
prog_char str_op12[] PROGMEM = "NTP sync: ";
prog_char str_op13[] PROGMEM = "Start hour:";
prog_char str_op14[] PROGMEM = "End hour  :";
prog_char str_op15[] PROGMEM = "Startup msg: ";
prog_char str_op16[] PROGMEM = "Multi valve: ";
prog_char str_op17[] PROGMEM = "MS";
prog_char str_op18[] PROGMEM = "MH";
prog_char str_op19[] PROGMEM = "MM";
prog_char str_op20[] PROGMEM = "Reset all? ";

// Array of option title strings
char *options_str[NUM_OPTIONS]  = {
  str_op0, str_op1, str_op2, str_op3, str_op4, str_op5, str_op6, str_op7, str_op8, str_op9,
  str_op10,str_op11,str_op12,str_op13,str_op14,str_op15,str_op16,str_op17,str_op18,str_op19,
  str_op20 };

// Flag of each option
prog_uchar options_flag[NUM_OPTIONS] PROGMEM={
  OPFLAG_NONE,
  OPFLAG_EDITABLE | OPFLAG_BOOL,
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
  OPFLAG_EDITABLE | OPFLAG_WEB_EDIT,
  OPFLAG_EDITABLE | OPFLAG_WEB_EDIT,
  OPFLAG_EDITABLE | OPFLAG_BOOL,
  OPFLAG_EDITABLE | OPFLAG_WEB_EDIT,
  OPFLAG_NONE,
  OPFLAG_NONE,
  OPFLAG_NONE,
  OPFLAG_EDITABLE | OPFLAG_BOOL
};  
  
/*
boolean options_setup_editable[NUM_OPTIONS]={
  false, true, true, true, true, true, true, true, true, true,
  true,  true, true, true, true, true, true, false,true, true,
  true };
*/


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
  valve_schedule(0);
  valve_apply();
  
  // initialize variables
  time_display_mode = 1;
  eeprom_busy = 0;
  
  // reset Ethernet module momentarily
  reset_ethernet();

  // start lcd
  lcd.begin(16, 2);
}

// =================
// Options Functions
// =================

void options_setup() {
  
  if (EEPROM.read(OPTION_FW_VERSION)!=FW_VERSION || EEPROM.read(OPTION_RESET)) {
    options_save();
    schedule_clear_all();
    password_set(DEFAULT_PASSWORD);
    
    // flash debug light to indicate that initialization is ready
    pinMode(PIN_DEBUG, OUTPUT);
    digitalWrite(PIN_DEBUG, HIGH);
    delay(500);
    digitalWrite(PIN_DEBUG, LOW);
    pinMode(PIN_DEBUG, INPUT);
    digitalWrite(PIN_DEBUG, HIGH);
  } else {
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
  
  running_mode = options[OPTION_RUNNING_MODE];
  // correct option errors
  if (options[OPTION_DAY_END] <= options[OPTION_DAY_START])
    options[OPTION_DAY_END] = options[OPTION_DAY_START]+4;  
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

// schedule all stations according to the given bit values
void valve_schedule(byte value) {
  valve_bitvalue = value;
}

// schedule one station
void valve_schedule(byte index, byte value) {
  if (value) {
    valve_bitvalue = valve_bitvalue | (1<<index);
  } else {
    valve_bitvalue = valve_bitvalue &~(1<<index);
  }
}		

// apply scheduled valve values
// !!! This will activate the valves !!!
void valve_apply() {
  digitalWrite(PIN_SR_LATCH, LOW);
  digitalWrite(PIN_SR_CLOCK, LOW);  // add this line due to a minor bug of the Arduino's shiftOut function
  shiftOut(PIN_SR_DATA, PIN_SR_CLOCK, MSBFIRST,
    (valve_enabled && !valve_raindelayed) ? valve_bitvalue : 0);  
  digitalWrite(PIN_SR_LATCH, HIGH);
}		

// get scheduled values
byte valve_get_schedule() {
   return valve_bitvalue;
}

// get schdule value for one station
byte valve_get(byte index) {
  return (valve_bitvalue>>index)&1;
}

// verify multiple valve protection
// if the number of scheduled valves is larger
// than allowed, return false
boolean valve_multi_check(byte value) {
  byte total = 0;
  for (byte i=0; i<8; i++) {
    if(value&1) total ++;
    value >>= 1;
  }
  if (options[OPTION_MULTIVALVE] && (total > options[OPTION_MULTIVALVE])) return false;
  else return true;
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
  } else {
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
  if (running_mode==1 && !valve_enabled)
    lcd_print_pgm(PSTR("-STOPPED-"));
  else if (running_mode==1 && valve_raindelayed)
    lcd_print_pgm(PSTR("-DELAYED-"));
  else {
    byte value = valve_get_schedule();
    for (byte i=0; i<8; i++) {
      lcd.print((value&1) ? c : '_');
      value >>= 1;
    }
    lcd.print(' ');
  }
  lcd_print_pgm(running_mode ? PSTR("(web)") : PSTR("(man)"));
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
      // no break here!!
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

// ============
// UI Functions
// ============

// user interface for seting rain delay
int ui_set_raindelay()
{
  boolean finished = false;
  boolean timeout = false;
  unsigned long t;
  byte button;
  
  byte rd = 0;
  
  lcd.clear();
  lcd_print_line_clear_pgm(PSTR("Set rain delay"), 1);
  lcd_print_raindelay(rd, 0);
  lcd.blink();
  
  t = time_second_counter + BUTTON_IDLE_TIMEOUT;

  while (!finished && !timeout) {
    button = button_read(BUTTON_WAIT_HOLD);
   
    if (t == time_second_counter)
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
      t = time_second_counter + BUTTON_IDLE_TIMEOUT;  // time out in 10 seconds
    }
  }
  
  lcd.noBlink();
  if (timeout)  return -1;
  else return rd;
}

// user interface for setting options during startup
void ui_set_options()
{
  boolean finished = false;
  byte button;
  int i=1;

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
        } else {
          do {
            i = (i+1) % NUM_OPTIONS;
          } while((option_get_flag(i)&OPFLAG_EDITABLE)==0);  // skip non-editable options
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
  int cursor_move[5] = {1, 4, 9, 12, 15};
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

// lock write flag
byte schedule_start_write() {
  if (eeprom_busy)  return 0;
  else {eeprom_busy = 1; return 1;}
}

// clear write flag
void schedule_end_write() {
  eeprom_busy = 0;
}

// check if write flag is on
byte schedule_can_read() {
  return eeprom_busy ? 0 : 1;
}

// convert time value to slot index
uint16_t schedule_time_to_slot(uint16_t h, uint16_t m)
{
  return h*SC_SLOTS_PER_HOUR + m/(SC_SLOT_LENGTH);
}

// weekday index of today
byte weekday_today() {
  return ((byte)weekday()+5)%7;  
}

// clear the entire schedule
void schedule_clear_all() {
  for(byte d=0;d<7;d++) {
    schedule_clear_day(d);
  }
}

// clear the schedule of one day
void schedule_clear_day(unsigned int d) {
  byte cc[]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  for(unsigned int a=0;a<SC_NUM_SLOTS_PER_DAY;a+=EEPROM_BLOCK_SIZE) {
    ext_eeprom_write_page(d*SC_NUM_SLOTS_PER_DAY+a, cc, EEPROM_BLOCK_SIZE);
  }
}

// read a block of schedule values from eeprom
void schedule_read_slots(uint16_t day, int idx, byte *buf) {
  return ext_eeprom_read_buffer(day*SC_NUM_SLOTS_PER_DAY+idx, buf, EEPROM_BLOCK_SIZE); 
}

// read the schedule value of a given day and index
byte schedule_read_slot(uint16_t day, int idx) {
  return ext_eeprom_read_byte(day*SC_NUM_SLOTS_PER_DAY+idx);
}

// read the schedule value of a given day and given hour + minute
byte schedule_read(uint16_t day, uint16_t h, uint16_t m) {
  return schedule_read_slot(day, schedule_time_to_slot(h, m));  
}


// write the schedule value of a given day and index
void schedule_write_slot(uint16_t day, uint16_t idx, byte value) {
  ext_eeprom_write_page(day*SC_NUM_SLOTS_PER_DAY+idx, &value, 1);
}

// ================
// EEPROM Functions
// ================

// ++++++ modified from Arduino EEPROM library ++++++

void ext_eeprom_write_byte(unsigned int eeaddress, byte data) {
  int rdata = data;
  Wire.beginTransmission(I2C_EEPROM_DEVICE_ADDR);
  Wire.send((int)(eeaddress >> 8)); // MSB
  Wire.send((int)(eeaddress & 0xFF)); // LSB
  Wire.send(rdata);
  Wire.endTransmission();
  delay(5);
}

// WARNING: address is a page address, 6-bit end will wrap around
// also, data can be maximum of about 30 bytes, because the Wire library has a buffer of 32 bytes
void ext_eeprom_write_page(unsigned int eeaddresspage, byte* data, byte length) {
  Wire.beginTransmission(I2C_EEPROM_DEVICE_ADDR);
  Wire.send((int)(eeaddresspage >> 8)); // MSB
  Wire.send((int)(eeaddresspage & 0xFF)); // LSB
  byte c;
  for ( c = 0; c < length; c++)
    Wire.send(data[c]);
  Wire.endTransmission();
  delay(15);
}

byte ext_eeprom_read_byte(unsigned int eeaddress) {
  byte rdata = 0xFF;
  Wire.beginTransmission(I2C_EEPROM_DEVICE_ADDR);
  Wire.send((int)(eeaddress >> 8)); // MSB
  Wire.send((int)(eeaddress & 0xFF)); // LSB
  Wire.endTransmission();
  Wire.requestFrom(I2C_EEPROM_DEVICE_ADDR,1);
  if (Wire.available()) rdata = Wire.receive();
  delay(1);
  return rdata;
}

// maybe let's not read more than 30 or 32 bytes at a time!
void ext_eeprom_read_buffer(unsigned int eeaddress, byte *buffer, int length) {
  Wire.beginTransmission(I2C_EEPROM_DEVICE_ADDR);
  Wire.send((int)(eeaddress >> 8)); // MSB
  Wire.send((int)(eeaddress & 0xFF)); // LSB
  Wire.endTransmission();
  Wire.requestFrom(I2C_EEPROM_DEVICE_ADDR,length);
  int c = 0;
  for ( c = 0; c < length; c++ )
    if (Wire.available()) buffer[c] = Wire.receive();
  delay(1);
}


