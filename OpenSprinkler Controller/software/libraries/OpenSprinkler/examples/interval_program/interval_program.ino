
// Example code for OpenSprinkler

/* This is a program-based scheduling algorithm.
 Programs are set similar to a calendar schedule.
 Each program specifies the days, stations,
 start time, end time, interval and duration.
 You can create any number of programs subject to EEPROM size.
 
 Creative Commons Attribution-ShareAlike 3.0 license
 June 2012 @ Rayshobby.net
 */

#include <limits.h>
#include <LiquidCrystal.h>
#include <Wire.h>
#include <OpenSprinkler.h>
#include "program.h"

// This is the path where external Javascripst are stored
// To create custom Javascripts, you need to make a copy of the scripts
// and put them to your own server, or github, or any available file hosting service
#define JAVASCRIPT_PATH  "http://rayshobby.net/scripts/java/svc1.6" 
//"https://github.com/rayshobby/opensprinkler/raw/master/scripts/java/svc1.6"


// ====== Ethernet defines ======
byte mymac[] = { 0x00,0x69,0x69,0x2D,0x30,0x30 }; // mac address
int myhttpport = 80;                              // http port 
byte ntpip[] = {204,9,54,119};                    // Default NTP server ip
int ntpclientportL = 0;                           // Default NTP client port

byte Ethernet::buffer[ETHER_BUFFER_SIZE]; // Ethernet packet buffer
char tmp_buffer[TMP_BUFFER_SIZE+1];       // scratch buffer
BufferFiller bfill;                       // buffer filler

// ====== Object defines ======
OpenSprinkler svc;    // OpenSprinkler object
ProgramData pd;       // ProgramdData object 

// ====== UI defines ======
static char ui_anim_chars[3] = {
  '.', 'o', 'O'};
  
// poll button press
void button_poll() {

  // read button, if something is pressed, wait till release
  byte button = svc.button_read(BUTTON_WAIT_HOLD);

  if (!(button & BUTTON_FLAG_DOWN)) return;  // repond only to button down events

  switch (button & BUTTON_MASK) {
  case BUTTON_1:
    if (button & BUTTON_FLAG_HOLD) {
      // hold button 1 -> start operation
      svc.enable();
    } 
    else {
      // click button 1 -> display ip address and port number
      svc.lcd_print_ip(ether.myip, ether.hisport);
      delay(DISPLAY_MSG_MS);
    }
    break;

  case BUTTON_2:
    if (button & BUTTON_FLAG_HOLD) {
      // hold button 2 -> disable operation
      svc.disable();
    } 
    else {
      // click button 2 -> display status
      svc.lcd_print_status();
    }
    break;

  case BUTTON_3:
    if (button & BUTTON_FLAG_HOLD) {
      // hold button 3 -> reboot
      svc.button_read(BUTTON_WAIT_RELEASE);
      svc.reboot();
    } 
    else {
      // click button 3 -> switch board display (cycle through master and all extension boards)
      svc.status.display_board = (svc.status.display_board + 1) % (svc.options[OPTION_EXT_BOARDS]+1);
    }
    break;
  }
}

// ======================
// Arduino Setup Function
// ======================
void setup() { 

  svc.begin();          // OpenSprinkler init
  pd.init();            // ProgramData init
  svc.options_setup();  // set up options
  if (svc.start_network(mymac, myhttpport)) {  // initialize network
    svc.status.network_failed = 0;
  } else  svc.status.network_failed = true;

  setTime(0, 0, 0, 1, 1, 1970);         // set initial time
  setSyncInterval(3600);  // setup NTP time sync: sync interval 3600 seconds
  setSyncProvider(getNtpTime);  // NTP sync callback function

  svc.enable(); // enable controller operation  
  svc.lcd_print_time(0);  // display time to LCD

  //wdt_enable(WDTO_4S);  // start 4 seconds watchdog timer
}

// =================
// Arduino Main Loop
// =================
void loop()
{
  static unsigned long last_time = 0;
  static unsigned int last_minute = 0x7fff;

  static word pos;
  byte bid, sid, s, pid, bitvalue, sequential;
  unsigned long scheduled_stop_time;

  sequential = svc.options[OPTION_SEQUENTIAL];

  // ====== Process Ethernet packets ======
  pos = ether.packetLoop(ether.packetReceive());
  if (pos) {  // packet received

    bfill = ether.tcpOffset();
    analyze_get_url((char*)Ethernet::buffer+pos);

    ether.httpServerReply(bfill.position());   
  }
  // ======================================

  button_poll();    // process button press

  // if 1 second has passed
  time_t curr_time = now();
  if (last_time != curr_time) {

    last_time = curr_time;
    svc.lcd_print_time(0);       // print time

    // if ntp sync has failed more than 10 times, restart
    // ray: todo
    if (ntp_failure > 10)
      svc.reboot();

    // ====== Check raindelay status ======
    if (svc.status.rain_delayed) {
      if (curr_time >= svc.raindelay_stop_time) {
        // raindelay time is over      
        svc.raindelay_stop();
      }
    }
    
    // ====== Check rain sensor status ======
    svc.status.rain_sensed = digitalRead(PIN_RAINSENSOR) == LOW ? 0 : 1;
    
    
    // ====== Schedule program data ======
    // check program data and schedule stations only if
    // 1) the controller is in program mode (not manual mode)
    // 2) no existing program is running, or parallel running is allowed
    if (svc.status.manual_mode == 0 && (svc.status.program_busy == 0 || sequential==0)) {
      // we are cleared to schedule a new program
      int curr_minute = minute(curr_time);
      // do not check if we are still in the same minute
      if (curr_minute != last_minute) {
        last_minute = curr_minute;
        boolean match_found = false;
                
        // check through every station
        for(bid=0; bid<=svc.options[OPTION_EXT_BOARDS]; bid++) {
          for(s=0;s<8;s++) {
            byte sid = bid*8+s;
            // skip master station because it is not scheduled independently
            if (svc.options[OPTION_MASTER_STATION] == sid+1)  continue;
            // skip a station if it is still running a previous program
            if (pd.remaining_time[sid] != 0)  continue;

            unsigned int duration = pd.check_match(sid, curr_time, &pid);
            if (duration != 0) {
              // a program match is found
              if (sequential && match_found) {
                // if stations run sequentially, and another station is already running
                // set schedule data , but do not run this station yet
                pd.remaining_time[sid] = duration;
                pd.scheduled_duration[sid] = duration;
                pd.scheduled_program_index[sid] = pid+1;
              } 
              else {
                // set schedule data, and run this station
                pd.remaining_time[sid] = duration;
                pd.scheduled_stop_time[sid] = curr_time + (unsigned long) duration;
                pd.scheduled_duration[sid] = duration;
                pd.scheduled_program_index[sid] = pid+1;
                svc.set_station_bit(sid, 1);
                match_found = true;
              }//else
            }//if_duration
          }//for_s
        }//for_bid
        if (match_found) {
          svc.status.program_busy = 1;
          // activate valves
          svc.apply_all_station_bits();
        }
      }//if_check_current_minute
    } //if_cleared_for_checking
    
    // ====== Run program data ======
    // do program bookkeeping if a program is running currently
    if (svc.status.program_busy){
      for(bid=0;bid<=svc.options[OPTION_EXT_BOARDS]; bid++) {
        bitvalue = svc.station_bits[bid];
        for(s=0;s<8;s++) {
          byte sid = bid*8+s;
          // skip master station because it is not scheduled independently
          if (svc.options[OPTION_MASTER_STATION] == sid+1)  continue;
          
          if((bitvalue>>s)&1) {
            // if this station is current running
            scheduled_stop_time = pd.scheduled_stop_time[sid];
            
            // check if its schedule is over
            if (curr_time >= scheduled_stop_time) {
              // station will now be turned off
              svc.set_station_bit(sid, 0);

              // record lastrun log
              pd.lastrun.station = sid+1;
              pd.lastrun.program = pd.scheduled_program_index[sid];
              pd.lastrun.duration = pd.scheduled_duration[sid];
              pd.lastrun.endtime = curr_time;
              
              // reset variables
              pd.remaining_time[sid] = 0;
              pd.scheduled_duration[sid] = 0;
              pd.scheduled_program_index[sid] = 0;

              // if stations run sequentially
              if (svc.status.manual_mode == 0 && sequential) {
                // search for the next station that has a non-zero remaining time
                for(byte nextsid=sid+1;nextsid<(svc.options[OPTION_EXT_BOARDS]+1)*8;nextsid++) {
                  if (svc.options[OPTION_MASTER_STATION] == nextsid+1) continue;  // skip master
                  unsigned int duration = pd.remaining_time[nextsid];
                  if (duration != 0) {
                    // a station is found, turn it on now
                    pd.scheduled_stop_time[nextsid] = curr_time + (unsigned long) duration;
                    svc.set_station_bit(nextsid, 1);
                    break;
                  }//if_duration
                }//for_nextsid
              }//if_sequential
            }//if_schedule_is_over
            else {
              // update remaining minutes
              if (pd.scheduled_stop_time[sid]==ULONG_MAX) {
                pd.remaining_time[sid] = 0;
              } else {
                pd.remaining_time[sid] = (scheduled_stop_time - curr_time);
              }
            }//else
          }//if_station_is_running
        }//end_s
      }//end_bid

      // activate/deactivate valves
      svc.apply_all_station_bits();

      boolean any_station_on = false;
      for(bid=0;bid<=svc.options[OPTION_EXT_BOARDS];bid++) {
        if (svc.station_bits[bid]!=0) {
          any_station_on = true;
          break;
        }
      }
      // if no station is currently running, set program_busy to 0
      if (any_station_on == false) {
        svc.status.program_busy = 0;
        sequential = svc.options[OPTION_SEQUENTIAL];  // update sequential bit
      }
    }//if_some_program_is_running

    // process LCD display
    svc.lcd_print_station(1, ui_anim_chars[curr_time%3]);
  }
}

void manual_station_off(byte sid) {

  byte bid;
  svc.set_station_bit(sid, 0);

  // record lastrun log
  pd.lastrun.station = sid+1;
  pd.lastrun.program = pd.scheduled_program_index[sid];
  pd.lastrun.duration = pd.scheduled_duration[sid];
  pd.lastrun.endtime = now();
  
  // reset variables
  pd.remaining_time[sid] = 0;
  pd.scheduled_duration[sid] = 0;
  pd.scheduled_program_index[sid] = 0;
                
  svc.apply_all_station_bits();                
  
  // check if any station is still running
  boolean any_station_on = false;
  for(bid=0;bid<=svc.options[OPTION_EXT_BOARDS];bid++) {
    if (svc.station_bits[bid]!=0) {
      any_station_on = true;
      break;
    }
  }
  // if no station is currently running, set program_busy to 0
  if (any_station_on == false) {
    svc.status.program_busy = 0;
  }  
}

void manual_station_on(byte sid, int ontimer) {
  pd.remaining_time[sid] = ontimer;
  if (ontimer == 0) {
    pd.scheduled_stop_time[sid]=ULONG_MAX;
  } else { 
    pd.scheduled_stop_time[sid] = now() + (unsigned long) ontimer;
  }
  pd.scheduled_duration[sid] = ontimer;
  pd.scheduled_program_index[sid] = 255;
  svc.set_station_bit(sid, 1);
  svc.status.program_busy = 1;
}

