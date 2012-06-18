// Example code for OpenSprinkler

/* This is a web-based schedule program which allows you to
   set detailed schedules for each station using a table-style interface.
   Creative Commons Attribution-ShareAlike 3.0 license
   Mar 2012 @ Rayshobby.net
*/

#include <OpenSprinkler.h>

#include <avr/wdt.h>
#include "schedule.h"

// ====== UI defines ======
static char ui_anim_chars[3] = {
  '.', 'o', 'O'};

// ====== Web defines ======
static byte mymac[] = { 
  0x00,0x69,0x69,0x2D,0x30,0x30 };  // ethernet mac address

byte Ethernet::buffer[ETHER_BUFFER_SIZE];    // Ehternet packet buffer

OpenSprinkler svc;

unsigned long raindelay_stop_time;
byte raindelay_stop_clocktime[4];


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

// read schedule of the current time from external eeprom
// bidx: board index
byte web_mode_get_schedule_now(byte bidx) {

  uint16_t tick=0;    // time out in 5 seconds, prevents dead loop
  while(svc.ext_eeprom_busy==1 && tick<1000) {
    delay(1); 
    tick++;
  }
  if (tick==1000) return 0;
  time_t t = now();
  return schedule_read(bidx, svc.weekday_today(), hour(t), minute(t));
}

// web mode main loop
void web_mode_loop()
{
  static unsigned long mytime = 0;
  static int16_t old_tm2_ov_cnt = -1;

  static word pos;
  byte bidx;
  
  if (svc.options[OPTION_REQUIRE_NETWORK]) {
	  pos = ether.packetLoop(ether.packetReceive());
  	if (pos) {  // package received

  	  bfill = ether.tcpOffset();
  	  analyze_get_url((char*)Ethernet::buffer+pos);
    
  	  // .println(bfill.position());
    
  	  ether.httpServerReply(bfill.position());   
  	}
  }

  // tm2_ov_cnt changes every 8ms
  if (old_tm2_ov_cnt == tm2_ov_cnt) return;
  old_tm2_ov_cnt = tm2_ov_cnt;  
  if (old_tm2_ov_cnt%5) return;
  // the following code runs every 8ms*5=40ms

  web_mode_button_poll();    // check button press

  // if 1 second has passed
  if (mytime != time_second_counter) {
    mytime = time_second_counter;

    svc.lcd_print_time(0);       // print time
    
    // if ntp sync has failed more than 5 times, restart
    if (ntp_failure > 5)
      svc.reboot();
      
    // check raindelay status
    if (svc.raindelayed) {
       // divide by 256 before comparing,
       // so we don't miss detection within a 1 minute window
      if ((time_second_counter>>6) == (raindelay_stop_time>>6)) 
        web_mode_stop_raindelay();
    }
    
    // get schedule slot
    for(bidx=0; bidx<=svc.options[OPTION_EXT_BOARDS]; bidx++) {
    	svc.board_schedule(bidx, web_mode_get_schedule_now(bidx));
    }
    byte err = 0;
    if (svc.multistation_check() == false) {
      err = 1;
    }

    if (!err) svc.station_apply();

    // display animation
    if (svc.enabled && !svc.raindelayed) {
      if (err)
        svc.lcd_print_station(1, mytime%2 ? '!' : ' ');
      else
        svc.lcd_print_station(1, ui_anim_chars[mytime%3]);
    } 
    else {
      svc.lcd_print_station(1, 'x');
    }
  }
}

// poll button press
void web_mode_button_poll() {

  // read button, if something is pressed, wait till release
  byte button = svc.button_read(BUTTON_WAIT_HOLD);

  if (!(button & BUTTON_FLAG_DOWN)) return;  // repond only to button down event

  switch (button & BUTTON_MASK) {
  case BUTTON_1:
    if (button & BUTTON_FLAG_HOLD) {
    	// long hold button 1, start operation
      web_mode_start_operation();
    } 
    else {
    	// click button 1, display ip
      svc.lcd_print_ip(ether.myip, 0);
      delay(1000);
    }
    break;

  case BUTTON_2:
    if (button & BUTTON_FLAG_HOLD) {
	    // long hold button 2, stop operation
      web_mode_stop_operation();
    } else {
    	// click button 2, set rain delay
      int rd = svc.ui_set_raindelay();
      if (rd>0) {
        web_mode_start_raindelay(rd);        
      } else if (rd==0) {
        web_mode_stop_raindelay();
      }
    }
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
      
      // click button 3: switch the board whose status is displayed on the lcd
      svc.lcd_display_board = (svc.lcd_display_board + 1) % (svc.options[OPTION_EXT_BOARDS]+1);
    }
    break;
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
  if (svc.options[OPTION_REQUIRE_NETWORK]) {
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
	}

  setTime(12, 0, 0, 1, 1, 2011);  // set initial time

  // setup NTP sync if its option is enabled
  if (svc.options[OPTION_REQUIRE_NETWORK] == 1 && svc.options[OPTION_NTP_SYNC] == 1) {
    setSyncInterval(86400);
    setSyncProvider(getNtpTime);
  }

  timer2_int_enable();    // enabled timer2

  web_mode_start_operation();  // start controller operation

  svc.lcd_print_time(0);

  
  // we are in operation, start watchdog timer now
  wdt_enable(WDTO_4S);
  
}

void web_mode_start_raindelay(byte rd) {
  raindelay_stop_time = time_second_counter + (unsigned long) rd * 3600;
  time_t t = now() + (unsigned long) rd * 3600;
  raindelay_stop_clocktime[0] = hour(t);
  raindelay_stop_clocktime[1] = minute(t);
  raindelay_stop_clocktime[2] = month(t);
  raindelay_stop_clocktime[3] = day(t);
  svc.raindelayed = 1;
  svc.station_apply();
}

void web_mode_stop_raindelay() {
  svc.raindelayed = 0;
}

void web_mode_start_operation() {
  svc.enabled = 1;
  if (svc.multistation_check() == true) {
    svc.station_apply();
    svc.lcd_print_station(1, 'o');  
  } 
}

void web_mode_stop_operation() {
  svc.enabled = 0;
  svc.station_apply();
  svc.lcd_print_station(1, 'x');  
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

