// Arduino library code for OpenSprinkler

/* OpenSprinkler Class Implementation
   Creative Commons Attribution-ShareAlike 3.0 license
   Feb 2013 @ Rayshobby.net
*/

#include "OpenSprinkler.h"

// Declare static data members
LiquidCrystal OpenSprinkler::lcd(PIN_LCD_RS, PIN_LCD_EN, PIN_LCD_D4, PIN_LCD_D5, PIN_LCD_D6, PIN_LCD_D7);
StatusBits OpenSprinkler::status;
byte OpenSprinkler::nboards;
byte OpenSprinkler::nstations;
byte OpenSprinkler::station_bits[MAX_EXT_BOARDS+1];
byte OpenSprinkler::masop_bits[MAX_EXT_BOARDS+1];
unsigned long OpenSprinkler::raindelay_stop_time;

// Option names
prog_char _str_fwv [] PROGMEM = "Firmware ver.";
prog_char _str_tz  [] PROGMEM = "Time zone:";
prog_char _str_ntp [] PROGMEM = "NTP Sync:";
prog_char _str_dhcp[] PROGMEM = "Use DHCP:";
prog_char _str_ip1 [] PROGMEM = "Static.ip1:";
prog_char _str_ip2 [] PROGMEM = "ip2:";
prog_char _str_ip3 [] PROGMEM = "ip3:";
prog_char _str_ip4 [] PROGMEM = "ip4:";
prog_char _str_gw1 [] PROGMEM = "Gateway.ip1:";
prog_char _str_hp0 [] PROGMEM = "HTTP port:";
prog_char _str_hp1 [] PROGMEM = "";
prog_char _str_ar  [] PROGMEM = "Auto reconnect:";
prog_char _str_ext [] PROGMEM = "Ext. boards:";
prog_char _str_seq [] PROGMEM = "Sequential:";
prog_char _str_sdt [] PROGMEM = "Station delay:";
prog_char _str_mas [] PROGMEM = "Master station:";
prog_char _str_mton[] PROGMEM = "Mas. on adj.:";
prog_char _str_mtof[] PROGMEM = "Mas. off adj.:";
prog_char _str_urs [] PROGMEM = "Use rain sensor:";
prog_char _str_rso [] PROGMEM = "Normally open:";
prog_char _str_wl  [] PROGMEM = "Water level (%):";
prog_char _str_stt [] PROGMEM = "Selftest time:";
prog_char _str_ipas[] PROGMEM = "Ignore password:";
prog_char _str_devid[] PROGMEM = "Device ID:";
prog_char _str_reset[] PROGMEM = "Reset all?";


OptionStruct OpenSprinkler::options[NUM_OPTIONS] = {
  {SVC_FW_VERSION, 0, _str_fwv, OPFLAG_NONE}, // firmware version
  {32,  108, _str_tz,   OPFLAG_WEB_EDIT | OPFLAG_SETUP_EDIT},     // default time zone: GMT-4
  {1,   1,   _str_ntp,  OPFLAG_SETUP_EDIT},   // use NTP sync
  {1,   1,   _str_dhcp, OPFLAG_SETUP_EDIT},   // 0: use static ip, 1: use dhcp
  {192, 255, _str_ip1,  OPFLAG_SETUP_EDIT},   // this and next 3 bytes define static ip
  {168, 255, _str_ip2,  OPFLAG_SETUP_EDIT},
  {1,   255, _str_ip3,  OPFLAG_SETUP_EDIT},
  {22,  255, _str_ip4,  OPFLAG_SETUP_EDIT},
  {192, 255, _str_gw1,  OPFLAG_SETUP_EDIT},   // this and next 3 bytes define static gateway ip
  {168, 255, _str_ip2,  OPFLAG_SETUP_EDIT},
  {1,   255, _str_ip3,  OPFLAG_SETUP_EDIT},
  {1,   255, _str_ip4,  OPFLAG_SETUP_EDIT},
  {80,  255, _str_hp0,  OPFLAG_WEB_EDIT},     // this and next byte define http port number
  {0,   255, _str_hp1,  OPFLAG_WEB_EDIT},
  {1,   1,   _str_ar,   OPFLAG_SETUP_EDIT},   // network auto reconnect
  {0,   MAX_EXT_BOARDS, _str_ext, OPFLAG_SETUP_EDIT | OPFLAG_WEB_EDIT}, // number of extension board. 0: no extension boards
  {1,   1,   _str_seq,  OPFLAG_SETUP_EDIT | OPFLAG_WEB_EDIT}, // sequential mode. 1: stations run sequentially; 0:concurrently
  {0,   240, _str_sdt,  OPFLAG_SETUP_EDIT | OPFLAG_WEB_EDIT}, // station delay time (0 to 240 seconds).
  {0,   8,   _str_mas,  OPFLAG_SETUP_EDIT | OPFLAG_WEB_EDIT}, // index of master station. 0: no master station
  {0,   60,  _str_mton, OPFLAG_SETUP_EDIT | OPFLAG_WEB_EDIT}, // master on time [0,60] seconds
  {60,  120, _str_mtof, OPFLAG_SETUP_EDIT | OPFLAG_WEB_EDIT}, // master off time [-60,60] seconds
  {0,   1,   _str_urs,  OPFLAG_SETUP_EDIT | OPFLAG_WEB_EDIT}, // rain sensor control bit. 1: use rain sensor input; 0: ignore
  {1,   1,   _str_rso,  OPFLAG_SETUP_EDIT | OPFLAG_WEB_EDIT}, // rain sensor type. 0: normally closed; 1: normally open.
  {100, 250, _str_wl,   OPFLAG_SETUP_EDIT | OPFLAG_WEB_EDIT}, // water level (default 100%),
  {10,  240, _str_stt,  OPFLAG_SETUP_EDIT},                   // self-test time (in seconds)
  {0,   1,   _str_ipas, OPFLAG_SETUP_EDIT | OPFLAG_WEB_EDIT}, // 1: ignore password; 0: use password
  {0,   255, _str_devid,OPFLAG_SETUP_EDIT},                   // device id
  {0,   1,   _str_reset,OPFLAG_SETUP_EDIT}
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

// Initialize network with the given mac address and http port
byte OpenSprinkler::start_network(byte mymac[], int http_port) {

  mymac[5] = options[OPTION_DEVICE_ID].value;
  if(!ether.begin(ETHER_BUFFER_SIZE, mymac))  return 0;
  ether.hisport = http_port;    
  
  if (options[OPTION_USE_DHCP].value) {
    // register with domain name "opensprinkler"
    if (!ether.dhcpSetup("opensprinkler")) return 0;
  } else {
    byte staticip[] = {
      options[OPTION_STATIC_IP1].value,
      options[OPTION_STATIC_IP2].value,
      options[OPTION_STATIC_IP3].value,
      options[OPTION_STATIC_IP4].value};

    byte gateway[] = {
      options[OPTION_GATEWAY_IP1].value,
      options[OPTION_GATEWAY_IP2].value,
      options[OPTION_GATEWAY_IP3].value,
      options[OPTION_GATEWAY_IP4].value};
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
  // shift register output enable
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
  status.enabled = 1;
  status.rain_delayed = 0;
  status.rain_sensed = 0;
  status.program_busy = 0;
  status.manual_mode = 0;
  status.has_rtc = 0;
  status.display_board = 0;
  status.network_fails = 0;

  nboards = 1;
  nstations = 8;
  raindelay_stop_time = 0;

  // begin lcd
  lcd.begin(16, 2);
  
  // detect if DS1307 RTC exists
  if (RTC.testerr()==0) {
    status.has_rtc = 1;
  }
}

// Self_test function
void OpenSprinkler::self_test() {
	byte sid;
	while(1) {
		for(sid=0; sid<nstations; sid++) {
			lcd.clear();
			lcd.setCursor(0, 0);
			lcd.print((int)sid+1);
			clear_all_station_bits();
			set_station_bit(sid, 1);
			apply_all_station_bits();
			// run each station for designated amount of time
			delay((unsigned long)options[OPTION_SELFTEST_TIME].value*1000);
      //delay(3000);  // 3 seconds delay between every two stations			
		}
	}
}

// Get station name from eeprom
void OpenSprinkler::get_station_name(byte sid, char tmp[]) {
  int i=0;
  int start = ADDR_EEPROM_STN_NAMES + (int)sid * STATION_NAME_SIZE;
  tmp[STATION_NAME_SIZE]=0;
  while(1) {
    tmp[i] = eeprom_read_byte((unsigned char *)(start+i));
    if (tmp[i]==0 || i==(STATION_NAME_SIZE-1)) break;
    i++;
  }
  return;
}

// Set station name to eeprom
void OpenSprinkler::set_station_name(byte sid, char tmp[]) {
  int i=0;
  int start = ADDR_EEPROM_STN_NAMES + (int)sid * STATION_NAME_SIZE;
  tmp[STATION_NAME_SIZE]=0;
  while(1) {
    eeprom_write_byte((unsigned char *)(start+i), tmp[i]);
    if (tmp[i]==0 || i==(STATION_NAME_SIZE-1)) break;
    i++;
  }
  return;  
}

// Save station master operation bits to eeprom
void OpenSprinkler::masop_save() {
  byte i;
  for(i=0;i<=MAX_EXT_BOARDS;i++) {
    eeprom_write_byte((unsigned char *)ADDR_EEPROM_MAS_OP+i, masop_bits[i]);
  }
}

// Load station master operation bits from eeprom
void OpenSprinkler::masop_load() {
  byte i;
  for(i=0;i<=MAX_EXT_BOARDS;i++) {
    masop_bits[i] = eeprom_read_byte((unsigned char *)ADDR_EEPROM_MAS_OP+i);
  }
}

// ==================
// Schedule Functions
// ==================

// Index of today's weekday (Monday is 0)
byte OpenSprinkler::weekday_today() {
  return ((byte)weekday()+5)%7; // Time::weekday() assumes Sunday is 1
}

// Set station bit
void OpenSprinkler::set_station_bit(byte sid, byte value) {
  byte bid = (sid>>3);  // board index
  byte s = sid % 8;     // station bit index
  if (value) {
    station_bits[bid] = station_bits[bid] | ((byte)1<<s);
  } 
  else {
    station_bits[bid] = station_bits[bid] &~((byte)1<<s);
  }
}	

// Clear all station bits
void OpenSprinkler::clear_all_station_bits() {
  byte bid;
  for(bid=0;bid<=MAX_EXT_BOARDS;bid++) {
    station_bits[bid] = 0;
  }
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
    bitvalue = 0;
    if (status.enabled && (!status.rain_delayed) && !(options[OPTION_USE_RAINSENSOR].value && status.rain_sensed))
      bitvalue = station_bits[MAX_EXT_BOARDS-bid];
    for(s=0;s<8;s++) {
      digitalWrite(PIN_SR_CLOCK, LOW);
      digitalWrite(PIN_SR_DATA, (bitvalue & ((byte)1<<(7-s))) ? HIGH : LOW );
      digitalWrite(PIN_SR_CLOCK, HIGH);          
    }
  }
  digitalWrite(PIN_SR_LATCH, HIGH);
}		

// =================
// Options Functions
// =================

void OpenSprinkler::options_setup() {

  // add 0.5 second delay to allow EEPROM to stablize
  delay(500);
  
  // check reset condition: either firmware version has changed, or reset flag is up
  byte curr_ver = eeprom_read_byte((unsigned char*)(ADDR_EEPROM_OPTIONS+OPTION_FW_VERSION));
  if (curr_ver<100) curr_ver = curr_ver*10; // adding a default 0 if version number is the old type
  if (curr_ver != SVC_FW_VERSION || eeprom_read_byte((unsigned char*)(ADDR_EEPROM_OPTIONS+OPTION_RESET))==0xAA) {
      
    //======== Reset EEPROM data ========
    options_save(); // write default option values
    eeprom_string_set(ADDR_EEPROM_PASSWORD, DEFAULT_PASSWORD);  // write default password
    eeprom_string_set(ADDR_EEPROM_LOCATION, DEFAULT_LOCATION);  // write default location
    
    lcd_print_line_clear_pgm(PSTR("Reset EEPROM"), 0);
    lcd_print_line_clear_pgm(PSTR("Please Wait..."), 1);  
      
    int i, sn;
    for(i=ADDR_EEPROM_STN_NAMES; i<INT_EEPROM_SIZE; i++) {
      eeprom_write_byte((unsigned char *) i, 0);      
    }

    // reset station names
    for(i=ADDR_EEPROM_STN_NAMES, sn=1; i<ADDR_EEPROM_MAS_OP; i+=STATION_NAME_SIZE, sn++) {
      eeprom_write_byte((unsigned char *)i    ,'S');
      eeprom_write_byte((unsigned char *)(i+1),'0'+(sn/10));
      eeprom_write_byte((unsigned char *)(i+2),'0'+(sn%10)); 
    }
    
    // reset master operation bits
    for(i=ADDR_EEPROM_MAS_OP; i<ADDR_EEPROM_MAS_OP+(MAX_EXT_BOARDS+1); i++) {
      // default master operation bits on
      eeprom_write_byte((unsigned char *)i, 0xff);
    }
    //======== END OF EEPROM RESET CODE ========
    
    // restart after resetting EEPROM.
    delay(500);
    reboot();
  } 
  else {
    options_load(); // load option values
    masop_load();   // load master operation bits
  }

	byte button = button_read(BUTTON_WAIT_NONE);
	
	switch(button & BUTTON_MASK) {
	case BUTTON_1:
  	// if BUTTON_1 is pressed during startup, go to self-test
	  //lcd_print_line_clear_pgm(PSTR("Self test:"), 0);
	  self_test();
		break;
		
  case BUTTON_2:
  	// if BUTTON_2 is pressed during startup, go to 'reset all options'
		ui_set_options(OPTION_RESET);
		if (options[OPTION_RESET].value) {
			resetFunc();
		}
		break;
		
	case BUTTON_3:
  	// if BUTTON_3 is pressed during startup, enter Setup option mode
    lcd_print_line_clear_pgm(PSTR("==Set Options=="), 0);
    delay(DISPLAY_MSG_MS);
    lcd_print_line_clear_pgm(PSTR("B3:sel B1/B2:chg"), 0);
    lcd_print_line_clear_pgm(PSTR("B3:hold to save"), 1);
    do {
      button = button_read(BUTTON_WAIT_NONE);
    } while (!(button & BUTTON_FLAG_DOWN));
    lcd.clear();
    ui_set_options(0);
    if (options[OPTION_RESET].value) {
      resetFunc(); 
    }
    break;
  }
}

// Load options from internal eeprom
void OpenSprinkler::options_load() {
  for (byte i=0; i<NUM_OPTIONS; i++) {
    options[i].value = eeprom_read_byte((unsigned char *)(ADDR_EEPROM_OPTIONS + i));
  }
  nboards = options[OPTION_EXT_BOARDS].value+1;
  nstations = nboards * 8;
}

// Save options to internal eeprom
void OpenSprinkler::options_save() {
  // save options in reverse order so version number is saved the last
  for (int i=NUM_OPTIONS-1; i>=0; i--) {
    eeprom_write_byte((unsigned char *) (ADDR_EEPROM_OPTIONS + i), options[i].value);
  }
  nboards = options[OPTION_EXT_BOARDS].value+1;
  nstations = nboards * 8;
}

// ==============================
// Controller Operation Functions
// ==============================

// Enable controller operation
void OpenSprinkler::enable() {
  status.enabled = 1;
  apply_all_station_bits();
  // write enable bit to eeprom
  options_save();
}

// Disable controller operation
void OpenSprinkler::disable() {
  status.enabled = 0;
  apply_all_station_bits();
  // write enable bit to eeprom
  options_save();
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

void OpenSprinkler::rainsensor_status() {
  // options[OPTION_RS_TYPE]: 0 if normally closed, 1 if normally open
  status.rain_sensed = (digitalRead(PIN_RAINSENSOR) == options[OPTION_RAINSENSOR_TYPE].value ? 0 : 1);
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
  for(; (16-cnt) >= 0; cnt ++) lcd_print_pgm(PSTR(" "));  
}

void OpenSprinkler::lcd_print_2digit(int v)
{
  lcd.print((int)(v/10));
  lcd.print((int)(v%10));
}

// Print time to a given line
void OpenSprinkler::lcd_print_time(byte line)
{
  time_t t=now();
  lcd.setCursor(0, line);
  lcd_print_2digit(hour(t));
  lcd_print_pgm(PSTR(":"));
  lcd_print_2digit(minute(t));
  lcd_print_pgm(PSTR("  "));
  lcd_print_pgm(days_str[weekday_today()]);
  lcd_print_pgm(PSTR(" "));
  lcd_print_2digit(month(t));
  lcd_print_pgm(PSTR("-"));
  lcd_print_2digit(day(t));
}

// print ip address and port
void OpenSprinkler::lcd_print_ip(const byte *ip, int http_port) {
  lcd.clear();
  lcd.setCursor(0, 0);
  for (byte i=0; i<3; i++) {
    lcd.print((int)ip[i]); 
    lcd_print_pgm(PSTR("."));
  }   
  lcd.print((int)ip[3]);
  lcd.setCursor(0, 1);
  lcd_print_pgm(PSTR(":"));
  lcd.print(http_port);
}

// Print station bits
void OpenSprinkler::lcd_print_station(byte line, char c) {
  //lcd_print_line_clear_pgm(PSTR(""), line);
  lcd.setCursor(0, line);
  if (status.display_board == 0) {
    lcd_print_pgm(PSTR("MC:"));  // Master controller is display as 'MC'
  }
  else {
    lcd_print_pgm(PSTR("E"));
    lcd.print((int)status.display_board);
    lcd_print_pgm(PSTR(":"));   // extension boards are displayed as E1, E2...
  }
  
  if (!status.enabled) {
  	lcd_print_line_clear_pgm(PSTR("-Disabled!-"), 1);
  }
  else if (status.rain_delayed || (status.rain_sensed && options[OPTION_USE_RAINSENSOR].value)) {
    lcd_print_line_clear_pgm(PSTR("-Rain Stop-"), 1);
  }
  else {
	  byte bitvalue = station_bits[status.display_board];
	  for (byte s=0; s<8; s++) {
	    if (status.display_board == 0 &&(s+1) == options[OPTION_MASTER_STATION].value) {
	      lcd.print((bitvalue&1) ? (char)c : 'M'); // print master station
	    } else {
	      lcd.print((bitvalue&1) ? (char)c : '_');
	    }
  	  bitvalue >>= 1;
	  }
	}
	lcd_print_pgm(PSTR("    "));
	lcd.setCursor(15, 1);
	if(status.network_fails>0)
		lcd.print('?');
}

// Print an option value
void OpenSprinkler::lcd_print_option(int i) {
  lcd_print_line_clear_pgm(options[i].str, 0);  
  lcd_print_line_clear_pgm(PSTR(""), 1);
  lcd.setCursor(0, 1);
  if(options[i].flag&OPFLAG_SETUP_EDIT) lcd.blink();
  else lcd.noBlink();
  int tz;
  switch(i) {
  case OPTION_TIMEZONE: // if this is the time zone option, do some conversion
    tz = (int)options[i].value-48;
    if (tz>=0) lcd_print_pgm(PSTR("+"));
    else {lcd_print_pgm(PSTR("-")); tz=-tz;}
    lcd.print(tz/4); // print integer portion
    lcd_print_pgm(PSTR(":"));
    tz = (tz%4)*15;
    if (tz==0)  lcd_print_pgm(PSTR("00"));
    else {
      lcd.print(tz);  // print fractional portion
    }
    lcd_print_pgm(PSTR(" GMT"));    
    break;
  case OPTION_MASTER_ON_ADJ:
    lcd_print_pgm(PSTR("+"));
    lcd.print((int)options[i].value);
    break;
  case OPTION_MASTER_OFF_ADJ:
    if(options[i].value>=60)  lcd_print_pgm(PSTR("+"));
    lcd.print((int)options[i].value-60);
    break;
  case OPTION_HTTPPORT_0:
    lcd.print((int)(options[i+1].value<<8)+options[i].value);
    break;
  default:
    // if this is a boolean option
    if (options[i].max==1)
      lcd_print_pgm(options[i].value ? PSTR("Yes") : PSTR("No"));
    else
      lcd.print((int)options[i].value);
    break;
  }
  if (i==OPTION_WATER_LEVEL)  lcd_print_pgm(PSTR("%"));
  else if (i==OPTION_MASTER_ON_ADJ || i==OPTION_MASTER_OFF_ADJ ||
      i==OPTION_SELFTEST_TIME || i==OPTION_STATION_DELAY_TIME)
    lcd_print_pgm(PSTR(" sec"));
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
      if (!(options[i].flag&OPFLAG_SETUP_EDIT)) break; // ignore non-editable options
      if (options[i].max != options[i].value) options[i].value ++;
      break;

    case BUTTON_2:
      if (!(options[i].flag&OPFLAG_SETUP_EDIT)) break; // ignore non-editable options
      if (options[i].value != 0) options[i].value --;
      break;

    case BUTTON_3:
      if (!(button & BUTTON_FLAG_DOWN)) break; 
      if (button & BUTTON_FLAG_HOLD) {
        // if OPTION_RESET is set to nonzero, change it to reset condition value
        if (options[OPTION_RESET].value) {
          options[OPTION_RESET].value = 0xAA;
        }
        // long press, save options
        options_save();
        finished = true;
      } 
      else {
        // click, move to the next option
        if (i==OPTION_USE_DHCP && options[i].value) i += 9; // if use DHCP, skip static ip set
        else if(i==OPTION_HTTPPORT_0) i+=2; // skip OPTION_HTTPPORT_1
        else if(i==OPTION_USE_RAINSENSOR && options[i].value==0) i+=2; // if not using rain sensor, skip rain sensor type
        else if(i==OPTION_MASTER_STATION && options[i].value==0) i+=3; // if not using master station, skip master on/off adjust
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

// ==================
// String Functions
// ==================
void OpenSprinkler::eeprom_string_set(int start_addr, char* buf) {
  byte i=0;
  for (; (*buf)!=0; buf++, i++) {
    eeprom_write_byte((unsigned char*)(start_addr+i), *(buf));
  }
  eeprom_write_byte((unsigned char*)(start_addr+i), 0);  
}

void OpenSprinkler::eeprom_string_get(int start_addr, char *buf) {
  byte c;
  byte i = 0;
  do {
    c = eeprom_read_byte((unsigned char*)(start_addr+i));
    //if (c==' ') c='+';
    *(buf++) = c;
    i ++;
  } while (c != 0);
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

