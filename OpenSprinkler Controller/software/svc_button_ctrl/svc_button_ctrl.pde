// Example code for OpenSprinkler controller

/* This example shows a simple button and http get based control interface
   Creative Commons Attribution-ShareAlike 3.0 license
   Mar 2012 @ Rayshobby.net
*/

#include <limits.h>
#include <OpenSprinkler.h>

// ====== UI defines ======
static char ui_anim_chars[3] = {
  '.', 'o', 'O'};
  
// ====== Web defines ======
static byte mymac[] = { 
  0x00,0x69,0x69,0x2D,0x30,0x30 };  // ethernet mac address

byte Ethernet::buffer[ETHER_BUFFER_SIZE];    // Ehternet packet buffer

OpenSprinkler svc;

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
    //Serial.println(bfill.position());  // print packet size to make sure we are not exceeding the limit
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
    for(b=0; b<=(svc.options[OPTION_EXT_BOARDS]); b++) {
      
      bitvalue = svc.station_bitvalues[b];  // get the bitvalue of the current board
      // loop through each individual station
      for(i=0;i<8;i++) {
        sid = b*8+i;
        
        // get schedule and stop time
        scheduled_seconds  = svc.get_station_scheduled_seconds(sid);
        scheduled_stop_time= svc.get_station_scheduled_stop_time(sid);
        
        // if the station is currently running and the scheduled duration is not infinite
        if (((bitvalue>>i)&1) && scheduled_seconds !=0 ) {
      
          // reliably test if the current time counter has gone past the scheduled stop time
          if (time_second_counter >= scheduled_stop_time) {

            // time_second_counter has gone past the scheduled stop time
            if ((time_second_counter - scheduled_stop_time) <= (ULONG_MAX>>1)) {
              // turn station off
              svc.station_schedule(sid, 0);
              svc.remaining_time[sid] = 0;
            } else {
              // calculate remaining minutes
              svc.remaining_time[sid] = (ULONG_MAX - time_second_counter + 1 + scheduled_stop_time) / 60;
            }
          }
        
          if (time_second_counter <= scheduled_stop_time) {
          
            // time_second_counter has gone past the scheduled stop time
            if((scheduled_stop_time - time_second_counter) >= (ULONG_MAX>>1)) {
              // turn station off
              svc.station_schedule(sid, 0);
              svc.remaining_time[sid] = 0;
            } else {
              // calculate remaining minutes
              svc.remaining_time[sid] = (scheduled_stop_time - time_second_counter) / 60;
            }
          }
        }
      }      
    }

    // execute schedule
    svc.station_apply();

    // handle LCD display
    svc.lcd_print_time(0);    // print current time
    svc.lcd_print_station(1, ui_anim_chars[mytime%3]);  // print station values
    
    // if ntp sync has failed more than 5 times, restart
    if (ntp_failure > 5) 
      svc.reboot(); 
    
  }
  } 
}

void web_mode_init_failed()
{
  svc.lcd_print_lines_clear_pgm(PSTR("Network init"), PSTR(" failed!"));
  delay(DISPLAY_MSG_MS);
  svc.reboot();
}

void web_mode_setup()
{
  svc.lcd_print_lines_clear_pgm(PSTR("Connecting to"), PSTR(" the network..."));

  // Ethernet init
  if (!ether.begin(sizeof Ethernet::buffer, mymac)) {
    web_mode_init_failed();
  }

  if (svc.options[OPTION_DHCP]) {
    // attempt DHCP
    if (!ether.dhcpSetup()) {
      web_mode_init_failed();
    }
  }
  else {
    // static ip
    byte staticip[] = {
      svc.options[OPTION_STATIC_IP1],
      svc.options[OPTION_STATIC_IP2],
      svc.options[OPTION_STATIC_IP3],
      svc.options[OPTION_STATIC_IP4]    };

    byte gateway[] = {
      svc.options[OPTION_GATEWAY_IP1],
      svc.options[OPTION_GATEWAY_IP2],
      svc.options[OPTION_GATEWAY_IP3],
      svc.options[OPTION_GATEWAY_IP4]    };

    if (!ether.staticSetup(staticip, gateway)) {
      web_mode_init_failed();
    }
  }

  setTime(12, 0, 0, 1, 1, 2011);  // set initial time
  
  // setup NTP sync if its option is enabled
  if (svc.options[OPTION_NTP_SYNC] == 1) {
    setSyncInterval(86400);
    setSyncProvider(getNtpTime);
  }

  timer2_int_enable();    // enable timer2
  
  svc.lcd_print_time(0);

}

// poll button press
void web_mode_button_poll() {

  // read button, if something is pressed, wait till release
  byte button = svc.button_read(BUTTON_WAIT_HOLD);

  if (!(button & BUTTON_FLAG_DOWN)) return;  // repond only to button down event

  switch (button & BUTTON_MASK) {
  case BUTTON_1:
    svc.lcd_print_ip(ether.myip, 0);
    delay(2000);

    break;

  case BUTTON_2:
    
    break;
    
  case BUTTON_3:
    if (button & BUTTON_FLAG_HOLD) {
      if (svc.options[OPTION_NTP_SYNC]) {
        svc.lcd_print_line_clear_pgm(PSTR("NTP enabled."), 0);
        delay(1000);
      } 
      else {
        svc.ui_set_time();
      }
    } 
    else {
      //svc.ui_toggle_time_display();
      // switch the board whose status is displayed on the lcd
      svc.lcd_display_board = (svc.lcd_display_board + 1) % (svc.options[OPTION_EXT_BOARDS]+1);
    }
    break;
  }
}

// ======================
// Arduino Main Functions
// ======================
void setup() { 

  //Serial.begin(9600);
  
  // setup
  svc.begin();
  // load and set up options
  svc.options_setup();

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


