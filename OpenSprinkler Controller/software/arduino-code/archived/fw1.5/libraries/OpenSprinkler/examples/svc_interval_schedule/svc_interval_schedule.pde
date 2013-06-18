// Example code for OpenSprinkler

/* This is an interval-based scheduling program which allows you to
   set how often to water during a day, and the desired water time for each station.
   Creative Commons Attribution-ShareAlike 3.0 license
   Mar 2012 @ Rayshobby.net
*/

#include <limits.h>
#include <OpenSprinkler.h>
#include <avr/wdt.h>

// ====== UI defines ======
static char ui_anim_chars[3] = {
  '.', 'o', 'O'};

// ====== Web defines ======
static byte mymac[] = { 
  0x00,0x69,0x69,0x2D,0x30,0x30 };  // ethernet mac address

byte Ethernet::buffer[ETHER_BUFFER_SIZE];    // Ethernet packet buffer
OpenSprinkler svc;

unsigned long raindelay_stop_time;
byte raindelay_stop_clocktime[4];
byte serial_schedule = 0;
byte station_busy = 0;
unsigned int last_checked_minute = 0x7fff;

// scratch buffer
char tmp_buffer[TMP_BUFFER_SIZE+1];

// schedule data buffer
unsigned int sc_data_buffer[16];

// buffer filler
BufferFiller bfill;

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

// poll button press
void button_poll() {

  // read button, if something is pressed, wait till release
  byte button = svc.button_read(BUTTON_WAIT_HOLD);

  if (!(button & BUTTON_FLAG_DOWN)) return;  // repond only to button down event

  switch (button & BUTTON_MASK) {
  case BUTTON_1:
    if (button & BUTTON_FLAG_HOLD) {
	    // long hold button 1, start operation
      start_operation();
    } 
    else {
    	// click button 1, display ip address
      svc.lcd_print_ip(ether.myip, 0);
      delay(1000);
    }
    break;

  case BUTTON_2:
    if (button & BUTTON_FLAG_HOLD) {
	    // long hold button 2, stop operation
      stop_operation();
    } else {
    	// click button 2, set rain delay
      int rd = svc.ui_set_raindelay();
      if (rd>0) {
        start_raindelay(rd);        
      } else if (rd==0) {
        stop_raindelay();
      }
    }
    break;

  case BUTTON_3:
    if (button & BUTTON_FLAG_HOLD) {
	    // long hold button 3, set time
      if (svc.options[OPTION_NTP_SYNC]) {
        svc.lcd_print_line_clear_pgm(PSTR("NTP enabled."), 0);
        delay(1000);
      } 
      else {
        svc.ui_set_time();
      }
    } 
    else {
    	// click button 3, change display board
      //svc.ui_toggle_time_display();
      // switch the board whose status is displayed on the lcd
      svc.lcd_display_board = (svc.lcd_display_board + 1) % (svc.options[OPTION_EXT_BOARDS]+1);
    }
    break;
  }
}

void init_failed()
{
  svc.lcd_print_lines_clear_pgm(PSTR("Network init"), PSTR(" failed!"));
  delay(DISPLAY_MSG_MS);
  if (svc.options[OPTION_REQUIRE_NETWORK]) 
    svc.reboot();
}

void start_raindelay(byte rd) {
  raindelay_stop_time = time_second_counter + (unsigned long) rd * 3600;
  time_t t = now() + (unsigned long) rd * 3600;
  raindelay_stop_clocktime[0] = hour(t);
  raindelay_stop_clocktime[1] = minute(t);
  raindelay_stop_clocktime[2] = month(t);
  raindelay_stop_clocktime[3] = day(t);
  svc.raindelayed = 1;
  svc.station_apply();
}

void stop_raindelay() {
  svc.raindelayed = 0;
}

void start_operation() {
  svc.enabled = 1;
  if (svc.options[OPTION_MULTISTATION] == 0) {
  	// serialize station schedule
  	serial_schedule = 1;  	
  } else if (svc.multistation_check() == true) {
    svc.station_apply();
    svc.lcd_print_station(1, 'o');  
  } 
}

void stop_operation() {
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

  if (svc.options[OPTION_REQUIRE_NETWORK]) {
    svc.lcd_print_lines_clear_pgm(PSTR("Connecting to"), PSTR(" the network..."));
  
    // Ethernet init
    if (!ether.begin(sizeof Ethernet::buffer, mymac)) {
      init_failed();
    }
  
    if (svc.options[OPTION_DHCP]) {
      // attempt DHCP
      if (!ether.dhcpSetup()) {
        init_failed();
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
        init_failed();
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

  start_operation();  // start controller operation

  svc.lcd_print_time(0);
  
  // we are in operation, start watchdog timer now
  wdt_enable(WDTO_4S);
}

// main loop
void loop()
{
  static unsigned long mytime = 0;
  static int16_t old_tm2_ov_cnt = -1;

  static word pos;
  byte bidx, i;
  byte err = 0;
  byte bitvalue;
  unsigned long scheduled_stop_time;
  
  if (svc.options[OPTION_REQUIRE_NETWORK]) {
    pos = ether.packetLoop(ether.packetReceive());
    if (pos) {  // package received
  
      bfill = ether.tcpOffset();
      analyze_get_url((char*)Ethernet::buffer+pos);
      
      //Serial.println(bfill.position());
      
      ether.httpServerReply(bfill.position());   
    }
  }

  // tm2_ov_cnt changes every 8ms
  if (old_tm2_ov_cnt == tm2_ov_cnt) return;
  old_tm2_ov_cnt = tm2_ov_cnt;  
  if (old_tm2_ov_cnt%5) return;
  // the following code runs every 8ms*5=40ms

  button_poll();    // check button press

  // if 1 second has passed
  if (mytime != time_second_counter) {
    mytime = time_second_counter;

    svc.lcd_print_time(0);       // print time
    
    // if ntp sync has failed more than 10 times, restart
    if (ntp_failure > 10)
      svc.reboot();
      
    // check raindelay status
    if (svc.raindelayed) {
       // divide by 256 before comparing,
       // so we don't miss detection within a 1 minute window
      if ((time_second_counter>>6) == (raindelay_stop_time>>6)) 
        stop_raindelay();
    }
    
    // handle schedule data
    // first check if we are busy executing a previous schedule

    if (station_busy == 0) {
      // if no station is busy (we are cleared for new schedule)
      unsigned current_minute = (unsigned int)hour() * 60 + (unsigned int)minute();
      boolean match_found = false;
      // do not check again if we are still in the same minute
      if (current_minute != last_checked_minute) {
        last_checked_minute = current_minute;
	// look through every board and station
	for(bidx=0; bidx<=svc.options[OPTION_EXT_BOARDS]; bidx++) {
	  schedule_read_buffer(bidx, (byte*)sc_data_buffer);
	  for(i=0;i<8;i++) {
	    byte si = bidx*8+i;
	    // do not process the master station
	    //if (svc.options[OPTION_MASTER_STATION] == si+1)  continue;
	    if (check_schedule_match(sc_data_buffer[2*i], current_minute) != 0) {
	    // if the current time matches the schedule, start the station	  	  		
	  
            unsigned int duration = sc_data_buffer[2*i+1];
            if (duration == 0) continue;
            if (serial_schedule && match_found) {
              // if stations are serialized, and one station is already open
              // only set duration
              svc.remaining_time[si] = duration;
            } else {
              // set duration and stop time, and schedule the station to open
              svc.remaining_time[si] = duration;
              svc.set_station_scheduled_stop_time(si, time_second_counter + (unsigned long)duration);
              svc.station_schedule(si, 1);
              match_found = true;
            }
          }
        }
      }
      if (match_found) {
        err = 0;
        station_busy = 1;
        if (svc.multistation_check() == false) {
          err = 1;
        }
        if (!err) {
          svc.station_apply();
        }
      }
    }
  } else {
    // at least one station is open currently (we are not yet done with the previous schedule)
    for(bidx=0; bidx<=svc.options[OPTION_EXT_BOARDS]; bidx++) {
      bitvalue = svc.station_bitvalues[bidx];  // get the bitvalue of the current board
      for(i=0;i<8;i++) {
        byte si = bidx*8+i;
        // do not process the master station
        //if (svc.options[OPTION_MASTER_STATION] == si+1)  continue;
        if((bitvalue>>i)&1) {
          // if this station is current open
          scheduled_stop_time= svc.get_station_scheduled_stop_time(si);
          
          // check if the current time counter has gone past the scheduled stop time
          if (time_second_counter >= scheduled_stop_time) {
            // turn station off
            svc.station_schedule(si, 0);
            svc.remaining_time[si] = 0;
            // the current station has turned off
            
            // if stations are serialized
            if (serial_schedule) {
              // search for the next station that has a non-zero remaining time
              for(byte _si=si+1;_si<(svc.options[OPTION_EXT_BOARDS]+1)*8;_si++) {
                //if (svc.options[OPTION_MASTER_STATION] == _si+1) continue;
                
                unsigned int duration = svc.remaining_time[_si];
                if (duration != 0) {
                  svc.set_station_scheduled_stop_time(_si, time_second_counter + (unsigned long)duration);
                  svc.station_schedule(_si, 1);
                  break;
                }
              }
            }
          } else {
            // calculate remaining minutes
            svc.remaining_time[si] = (scheduled_stop_time - time_second_counter);
          }//end_if
        } // end_if
      } // end_for
    } // end_for
    
    //svc.station_apply();
    
    err = 0;
    if (svc.multistation_check() == false) {
      err = 1;
    }
    if (!err) {
      svc.station_apply();
    }
    
    boolean any_station_on = false;
    
    for(bidx=0;bidx<=svc.options[OPTION_EXT_BOARDS]; bidx++) {
      bitvalue = svc.station_bitvalues[bidx];
      
      if (bitvalue != 0) {
        any_station_on = true;
        break;
      }
    }
    // if no station is currently on, set station_busy to 0
    
    if (any_station_on == false) {
      station_busy = 0;
    }
  }

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

