// Example code for Sprinkler Valve Controller (SVC)
// Support both manual mode and web mode
// Licensed under GPL V2
// Sep 2011 @Rayshobby

#include <Wire.h>
#include <EEPROM.h>
#include <Time.h>
#include <LiquidCrystal.h>
#include <EtherCard.h>
#include <avr/wdt.h>
#include "defines.h"

// ====== UI defines ======
static char ui_anim_chars[3] = {
  '.', 'o', 'O'};

// ====== Web defines ======
static byte mymac[] = { 
  0x00,0x69,0x69,0x2D,0x30,0x30 };  // ethernet mac address

byte Ethernet::buffer[ETHER_BUFFER_SIZE];    // Ehternet packet buffer


int ntp_failure = 0;

// ===============
// Timer Functions
// ===============

// tcnt2 load value = 256 - overflow (250 in this case)
// overflow interrupt triggers when tcnt2 reaches overflow
#define TCNT2_LOAD_VALUE    6

#define TM2_OVCNT_LOAD_VALUE 125

static int16_t tm2_ov_cnt = TM2_OVCNT_LOAD_VALUE;  // timer2 overflow counter
unsigned long time_second_counter;  // counts the seconds since timer interrupt enabled

// enable timer2 overflow interrupt
void timer2_int_enable() {
  time_second_counter = 0;
  tm2_ov_cnt = TM2_OVCNT_LOAD_VALUE;
  TCNT2 = TCNT2_LOAD_VALUE;
  TIMSK2 |= (1<<TOIE2);    // turn on interrupt bit
}

// disable timer2 overflow interrupt
void timer2_int_disable() {
  TIMSK2 &=~(1<<TOIE2);    // turn off interrupt bit
}

// timer2 overflow interrupt handler (set to trigger at 125Hz)
ISR(TIMER2_OVF_vect) {  
  TCNT2 = TCNT2_LOAD_VALUE;
  if ((--tm2_ov_cnt)!=0)  return;
  tm2_ov_cnt = TM2_OVCNT_LOAD_VALUE;

  time_second_counter ++;    // increments every second
  wdt_reset();
}

// ==================
// Web Mode Functions
// ==================

// read schedule of the current time from eeprom
byte web_mode_get_schedule_now() {

  uint16_t tick=0;    // time out in 5 seconds, prevents dead loop
  while(schedule_can_read()==0 && tick<1000) {
    delay(1); 
    tick++;
  }
  if (tick==1000) return 0;
  time_t t = now();
  return schedule_read(weekday_today(), hour(t), minute(t));
}

// web mode main loop
void web_mode_loop()
{
  static unsigned long mytime = 0;
  static int16_t old_tm2_ov_cnt = -1;

  static word pos;
  pos = ether.packetLoop(ether.packetReceive());
  if (pos) {  // package received

    bfill = ether.tcpOffset();
    analyze_get_url((char*)Ethernet::buffer+pos);
    //Serial.println(bfill.position());  // print buffer size, must be less than ETHER_BUFFER_SIZE
    ether.httpServerReply(bfill.position());   
  }

  // tm2_ov_cnt changes every 8ms
  if (old_tm2_ov_cnt == tm2_ov_cnt) return;
  old_tm2_ov_cnt = tm2_ov_cnt;  
  if (old_tm2_ov_cnt%5) return;
  // the following code runs every 8ms*5=40ms

  web_mode_button_poll();    // check button press

//  Serial.println("alive");
  // if 1 second has passed
  if (mytime != time_second_counter) {
    mytime = time_second_counter;

    lcd_print_time(0);       // print time
    
    if (ntp_failure > 10)
      svc_reboot();
      
    // check raindelay status
    if (valve_raindelayed) {
       // divide by 256 before comparing,
       // so we don't miss detection within a 1 minute window
      if ((time_second_counter>>6) == (raindelay_stop_time>>6)) 
        web_mode_stop_raindelay();
    }
    
    // get schedule slot
    valve_schedule(web_mode_get_schedule_now());
    byte err = 0;
    if (valve_multi_check(valve_get_schedule()) == false) {
      err = 1;
    }

    if (!err) valve_apply();

    // display animation
    if (valve_enabled && !valve_raindelayed) {
      if (err)
        lcd_print_valve(1, mytime%2 ? '!' : ' ');
      else
        lcd_print_valve(1, ui_anim_chars[mytime%3]);
    } 
    else {
      lcd_print_valve(1, 'x');
    }
  }
}

// poll button press
void web_mode_button_poll() {

  // read button, if something is pressed, wait till release
  byte button = button_read(BUTTON_WAIT_HOLD);

  if (!(button & BUTTON_FLAG_DOWN)) return;  // repond only to button down event

  switch (button & BUTTON_MASK) {
  case BUTTON_1:
    // long hold of button 1, start operation
    if (button & BUTTON_FLAG_HOLD) {
      web_mode_start_operation();
    } 
    else {
      lcd_print_ip(ether.myip, 0);
      delay(1000);
    }
    break;

  case BUTTON_2:
    // long hold of button 2, stop operation
    if (button & BUTTON_FLAG_HOLD) {
      web_mode_stop_operation();
    } else {
      int rd = ui_set_raindelay();
      if (rd>0) {
        web_mode_start_raindelay(rd);        
      } else if (rd==0) {
        web_mode_stop_raindelay();
      }
    }
    break;

  case BUTTON_3:
    if (button & BUTTON_FLAG_HOLD) {
      if (options[OPTION_NTP_SYNC]) {
        lcd_print_line_clear_pgm(PSTR("NTP enabled."), 0);
        delay(1000);
      } 
      else {
        ui_set_time();
      }
    } 
    else {
      ui_toggle_time_display();
    }
    break;
  }
}

void web_mode_init_failed()
{
  lcd_print_lines_clear_pgm(PSTR("Network init"), PSTR(" failed!"));
  delay(DISPLAY_MSG_MS);
  svc_reboot();
}

void web_mode_setup()
{
  lcd_print_lines_clear_pgm(PSTR("Connecting to"), PSTR(" the network..."));

  // Ethernet init
  if (!ether.begin(sizeof Ethernet::buffer, mymac)) {
    web_mode_init_failed();
  }

  if (options[OPTION_DHCP]) {
    // DHCP
    if (!ether.dhcpSetup()) {
      web_mode_init_failed();
    }
  }
  else {
    // static ip
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

    if (!ether.staticSetup(staticip, gateway)) {
      web_mode_init_failed();
    }
  }

  setTime(12, 0, 0, 1, 1, 2011);  // set initial time

  // setup NTP sync if its option is enabled
  if (options[OPTION_NTP_SYNC] == 1) {
    setSyncInterval(3600);
    setSyncProvider(getNtpTime);
  }

  timer2_int_enable();    // enabled timer2

  web_mode_start_operation();  // start valve operation

  lcd_print_time(0);

  
  // we are online, start watchdog timer now
  wdt_enable(WDTO_4S);
  
}

void web_mode_start_raindelay(byte rd) {
  raindelay_stop_time = time_second_counter + (unsigned long) rd * 3600;
  time_t t = now() + (unsigned long) rd * 3600;
  raindelay_stop_clocktime[0] = hour(t);
  raindelay_stop_clocktime[1] = minute(t);
  raindelay_stop_clocktime[2] = month(t);
  raindelay_stop_clocktime[3] = day(t);
  valve_raindelayed = 1;
}

void web_mode_stop_raindelay() {
  valve_raindelayed = 0;
}

void web_mode_start_operation() {
  valve_enabled = 1;
  if (valve_multi_check(valve_get_schedule()) == true) {
    valve_apply();
    lcd_print_valve(1, 'o');  
  } 
}

void web_mode_stop_operation() {
  valve_enabled = 0;
  valve_apply();
  lcd_print_valve(1, 'x');  
}

// =====================
// Manual Mode Functions
// =====================

void manual_mode_setup()
{
  // disabling operation in manual mode
  valve_enabled = 0;
  
  lcd.clear();
  lcd_print_pgm(PSTR("==Manual Mode=="));
  delay(DISPLAY_MSG_MS);

  setTime(12, 0, 0, 1, 1, 2011);
  valve_schedule(options[OPTION_MANUAL_STATIONS]);
  
  manual_scheduled_minutes = options[OPTION_MANUAL_HRS] * 60 + options[OPTION_MANUAL_MINS];
  if (manual_scheduled_minutes == 0)  manual_scheduled_minutes=1;
}


void manual_mode_start_operation() {

  // check to see if all scheduled valves can open at the same time
  if (valve_multi_check(valve_get_schedule()) == false) {
    lcd_print_line_clear_pgm(PSTR("Multi valve err!"), 1);
    delay(DISPLAY_MSG_MS);
    return;
  }
  valve_enabled = 1;
  valve_apply();
  lcd_print_valve(1, 'o');

  // set up timer interrupt and stop timer
  manual_stop_time = (unsigned long)manual_scheduled_minutes * 60 - 1;
  manual_running_seconds = 0;
  timer2_int_enable();
}


void manual_mode_stop_operation()
{
  timer2_int_disable();
  valve_enabled = 0;
  valve_apply();
  lcd_print_valve(1, 'x');
}

void manual_mode_button_poll()
{
  // read button, if something is pressed, wait till release
  byte button = button_read(BUTTON_WAIT_HOLD);

  if (!(button & BUTTON_FLAG_DOWN)) return;  // repond only to button down event

  switch (button & BUTTON_MASK) {
  case BUTTON_1:
    // long hold of button 1, start valves
    if (valve_enabled) break;
    if (button & BUTTON_FLAG_HOLD) {
      manual_mode_start_operation();
    } 
    else {
      // short hold of button 1, set schedule
      ui_manual_mode_set_schedule();
    }
    break;

  case BUTTON_2:
    // long hold of button 2, stop valves
    if (button & BUTTON_FLAG_HOLD) {
      manual_mode_stop_operation();
    } 
    break;

  case BUTTON_3:
    if ((button & BUTTON_FLAG_HOLD)) {
      if (!valve_enabled)
        ui_set_time();
    } 
    else {
      ui_toggle_time_display(); 
    }
    break;
  }  
}

void manual_mode_loop()
{
  lcd_print_time(0);

  manual_mode_button_poll();  

  // display flashing message
  if (valve_enabled) {
    // if 1 second time expired
    if (manual_running_seconds != time_second_counter) {
      manual_running_seconds = time_second_counter;  // update time

      // check if we should stop valves
      if ((manual_running_seconds >= manual_stop_time)) {
        manual_mode_stop_operation();
      }

      if (manual_running_seconds%12 >= 6) {
        lcd_print_valve(1, ui_anim_chars[manual_running_seconds%3]);
      } 
      else {
        ui_manual_mode_print_remaining_time(1); 
      }
    }
  } 
  else {
    lcd_print_valve(1, 'x');
  }
}


// ======================
// Arduino Main Functions
// ======================

void setup() { 

  // sprinkler valve controller setup
  svc_setup();

  // initialize arduino modules
//  Serial.begin(9600);
  Wire.begin();

  // load and set up options
  options_setup();

  // display startup message
  if (options[OPTION_SHOW_MSG]) {
    lcd_print_lines_clear_pgm(PSTR("Sprinkler Valve"), PSTR("Controller " HW_VERSION));
    delay(DISPLAY_MSG_MS);
  }

  // normal operation
  TCCR2A = 0x00;
  // 256 pre-scalar --> 31250Hz clock assuming 8MHz CPU freq.
  TCCR2B |= (1<<CS22)|(1<<CS21);
  // if TCNT2 overflows at 250, this will generate a 125Hz overflow cloc

  if (running_mode == 0) {
    manual_mode_setup(); 
  } 
  else  {
    web_mode_setup();
  }
}

void loop() {

  (running_mode == 1) ?  web_mode_loop() : manual_mode_loop();

}

// ============
// UI Functions
// ============

// print remaining time 
void ui_manual_mode_print_remaining_time(byte line)
{
  lcd.setCursor(0, line);
  long x = manual_stop_time - manual_running_seconds;
  if (x>5 && x<86400)
  {  
    lcd_print_pgm(PSTR("Rem: "));
    lcd.print(x/60);
    lcd_print_pgm(PSTR("m "));
    lcd.print(x%60);
    lcd_print_pgm(PSTR("s.      "));  
  }
}

void ui_manual_mode_set_schedule() {

  byte valve_selected = 0;
  // load previous station selection from eeprom
  lcd_print_line_clear_pgm(PSTR("Set stations:"), 0);

  byte lcd_offset = lcd_print_valve(1, 'x'); 
  lcd.blink();
  lcd.setCursor(lcd_offset+valve_selected, 1);

  boolean finished = false;

  // select stations
  byte button;
  while (!finished) {
    button = button_read(BUTTON_WAIT_HOLD);
    if (!(button & BUTTON_FLAG_DOWN))  continue;

    if ((button & BUTTON_MASK) == BUTTON_3) {
      if (button & BUTTON_FLAG_HOLD) {
        options[OPTION_MANUAL_STATIONS] = valve_get_schedule();
        options_save();
        finished = true;
      } 
      else {
        valve_selected = (valve_selected + 1) % 8;
        lcd.setCursor(lcd_offset+valve_selected, 1);
      }
    } 
    else {
      valve_schedule(valve_selected, ((button & BUTTON_MASK) == BUTTON_1) ? 1 : 0);
      lcd_print_valve(1, 'x');
      lcd.setCursor(lcd_offset+valve_selected, 1);
    }
  }

  lcd_print_line_clear_pgm(PSTR("Set duration:"), 0);

  int max_minutes = 1440;  // 24 hours

  finished = false;
  while (!finished) {
    button = button_read(BUTTON_WAIT_HOLD);

    switch (button & BUTTON_MASK) {
    case BUTTON_1:
      if (max_minutes != manual_scheduled_minutes) manual_scheduled_minutes ++;
      break;

    case BUTTON_2:
      if (manual_scheduled_minutes != 1) manual_scheduled_minutes --;
      break;

    case BUTTON_3:
      if (!(button & BUTTON_FLAG_DOWN)) break;
      if (button & BUTTON_FLAG_HOLD) {
        options[OPTION_MANUAL_HRS] = manual_scheduled_minutes / 60;
        options[OPTION_MANUAL_MINS]= manual_scheduled_minutes % 60;
        options_save();
        finished = true;
      }
      break;
    }
    if (button != BUTTON_NONE) {
      lcd.setCursor(0, 1);
      lcd.print(manual_scheduled_minutes);
      lcd_print_pgm(PSTR(" minutes.      "));
      lcd.setCursor(0, 1);
    }    
  }  
  lcd.noBlink();
}


