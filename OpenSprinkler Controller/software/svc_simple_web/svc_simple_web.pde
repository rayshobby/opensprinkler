// Example code for Sprinkler Valve Controller (SVC)
// This example shows a simply button based interface
// Licensed under GPL V2
// Mar 2012 @Rayshobby

#include <Wire.h>
#include <EEPROM.h>
#include <Time.h>
#include <LiquidCrystal.h>
#include <EtherCard.h>
#include "defines.h"
#include <limits.h>

// ====== UI defines ======
static char ui_anim_chars[3] = {
  '.', 'o', 'O'};
  
// ====== Web defines ======
static byte mymac[] = { 
  0x00,0x69,0x69,0x2D,0x30,0x30 };  // ethernet mac address

byte Ethernet::buffer[ETHER_BUFFER_SIZE];    // Ehternet packet buffer

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
}

// web mode main loop
void web_mode_loop()
{
  static unsigned long mytime = 0;
  static int16_t old_tm2_ov_cnt = -1;
  
  static word pos;
  
  // infinite loop
  while(1) {
  // Ethernet controller packet loop
  pos = ether.packetLoop(ether.packetReceive());
  if (pos) {  // package received

    bfill = ether.tcpOffset();
    analyze_get_url((char*)Ethernet::buffer+pos);
    //Serial.println(bfill.position());
    ether.httpServerReply(bfill.position());   
  }
  
  // tm2_ov_cnt changes every 8 ms
  if (old_tm2_ov_cnt == tm2_ov_cnt) return;
  old_tm2_ov_cnt = tm2_ov_cnt;
  if (old_tm2_ov_cnt%5) return;
  
  // the following code runs every 8ms*5=40ms
  // this is done to give more fraction of time to process Ethernet packet loop
  web_mode_button_poll();
  
  // the following code runs every second
  // if 1 second has passed
  if (mytime != time_second_counter) {
    mytime = time_second_counter;
    
    byte i, b;
    unsigned long scheduled_seconds;
    unsigned long scheduled_stop_time;
    byte bitvalue;
    byte sid;
    // loop through each extension board, including the master
    
    for(b=0; b<=(options[OPTION_EXT_BOARDS]); b++) {
      
      bitvalue = valve_bitvalues[b];  // get the bitvalue of the current board
      // loop through each individual station
      for(i=0;i<8;i++) {
        sid = b*8+i;
        
        scheduled_seconds  = get_station_scheduled_seconds(sid);
        scheduled_stop_time= get_station_scheduled_stop_time(sid);
        
        // if the valve is running and the scheduled duration is not infinite
        if (((bitvalue>>i)&1) && scheduled_seconds !=0 ) {
      
          // reliably test if the current time counter has gone past the scheduled stop time
          if (time_second_counter >= scheduled_stop_time) {

            // time_second_counter has gone past the scheduled stop time
            if ((time_second_counter - scheduled_stop_time) <= (ULONG_MAX>>1)) {
              valve_schedule(sid, 0);
              remaining_minutes[sid] = 0;
            } else {
              remaining_minutes[sid] = (ULONG_MAX - time_second_counter + 1 + scheduled_stop_time) / 60;
            }
          }
        
          if (time_second_counter <= scheduled_stop_time) {
          
            // time_second_counter has gone past the scheduled stop time
            if((scheduled_stop_time - time_second_counter) >= (ULONG_MAX>>1)) {
              valve_schedule(sid, 0);
              remaining_minutes[sid] = 0;
            } else {
              remaining_minutes[sid] = (scheduled_stop_time - time_second_counter) / 60;
            }
          }
        }
      }      
    }

    valve_apply();

    // handle LCD display
    lcd_print_time(0);    
    lcd_print_valve(1, ui_anim_chars[mytime%3]);
    
    if (ntp_failure > 5) 
      svc_reboot(); 
    
  }
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
    setSyncInterval(86400);
    setSyncProvider(getNtpTime);
  }

  timer2_int_enable();    // enable timer2
  
  lcd_print_time(0);

}

// poll button press
void web_mode_button_poll() {

  // read button, if something is pressed, wait till release
  byte button = button_read(BUTTON_WAIT_HOLD);

  if (!(button & BUTTON_FLAG_DOWN)) return;  // repond only to button down event

  switch (button & BUTTON_MASK) {
  case BUTTON_1:
    lcd_print_ip(ether.myip, 0);
    delay(2000);

    break;

  case BUTTON_2:
    // switch the board whose status is displayed on the lcd
    lcd_display_board = (lcd_display_board + 1) % (options[OPTION_EXT_BOARDS]+1);
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

// ======================
// Arduino Main Functions
// ======================

void setup() { 

  //Serial.begin(9600);
  
  // sprinkler valve controller setup
  svc_setup();
  // load and set up options
  options_setup();

  // Atmega328 timer2 clock setup
  // normal operation
  TCCR2A = 0x00;
  // 256 pre-scalar --> 31250Hz clock assuming 8MHz CPU freq.
  TCCR2B |= (1<<CS22)|(1<<CS21);
  // if TCNT2 overflows at 250, this will generate a 125Hz overflow cloc

  web_mode_setup();
}

void loop() {

  web_mode_loop();

}

// ============
// UI Functions
// ============


