// Example code for Sprinkler Valve Controller (SVC)
// Server functions
// Licensed under GPL V2
// Dec 2011 @Rayshobby

#include <Wire.h>
#include <EEPROM.h>
#include <Time.h>
#include <LiquidCrystal.h>
#include <EtherCard.h>
#include "defines.h"

// buffer filler
BufferFiller bfill;
// scratch buffer
char tmp_buffer[TMP_BUFFER_SIZE+1];

// Default NTP server ip
byte ntpip[] = {204,9,54,119};
// Default NTP client
int ntpclientportL = 0;

// =======================
// HTML/JAVASCRIPT STRINGS
// =======================

prog_uchar htmlOkHeader[] PROGMEM = 
    "HTTP/1.0 200 OK\r\n"
    "Content-Type: text/html\r\n"
    "Pragma: no-cache\r\n"
    "\r\n"
;

prog_uchar htmlOkHeaderjs[] PROGMEM = 
    "HTTP/1.0 200 OK\r\n"
    "Content-Type: application/x-javascript\r\n"
    "Pragma: no-cache\r\n"
    "\r\n"
;

prog_uchar htmlMobileHeader[] PROGMEM =
    "<meta name=viewport content=\"width=480\">\n"
;

prog_uchar htmlRefreshHeader[] PROGMEM =
   "<meta http-equiv=\"refresh\" content=\"60\">\n"
;

prog_uchar htmlNotFound[] PROGMEM = 
    "HTTP/1.0 404 Not Found\r\n"
    "Content-Type: text/html\r\n\r\n"
    "<h1>404 Not Found</h1>"
;

prog_uchar htmlFavicon[] PROGMEM = 
    "HTTP/1.0 301 Moved Permanently\r\nLocation: "
    "http://rayshobby.net/rayshobby.ico"
    "\r\n\r\nContent-Type: text/html\r\n\r\n"
    "<h1>301 Moved Permanently</h1>\n"
;

prog_uchar htmlUnauthorized[] PROGMEM = 
    "HTTP/1.0 401 Unauthorized\r\n"
    "Content-Type: text/html\r\n"
    "\r\n"
    "<h1>401 Unauthorized</h1>"
;


// Javascript p1
// populate Html form input values
void print_webpage_p1()
{
  bfill.emit_p(PSTR("$F"), htmlOkHeaderjs);
  bfill.emit_p(PSTR(
    "function w(s){document.writeln(s)}\n"
    "function sf(i){h=document.getElementById(\"h\"+i);m=document.getElementById(\"m\"+i);"
    "f.elements[0].value=i;f.elements[1].value=(v>>i)&1?0:1;f.elements[2].value=h.value;f.elements[3].value=m.value;f.submit()}\n"
    "function it(st,rt){\n"
    "for(i=0;i<8;i++){\n"
    "b=document.getElementById(\"b\"+i);h=document.getElementById(\"h\"+i);m=document.getElementById(\"m\"+i);b.style.height=32;\n"
    "if((v>>i)&1){b.style.backgroundColor=\"#C02020\";b.value=\"Turn it Off\";h.value=rt[i]/3600>>0;m.value=(st[i]?1:0)+(rt[i]%3600)/60>>0;"
    "h.disabled=true;m.disabled=true;}\n"
    "else{b.style.backgroundColor=\"#20C020\";b.value=\"Turn it On\";h.value=st[i]/3600>>0;m.value=(st[i]%3600)/60>>0;}\n"
    "}}\n"
  ) );
}

// print home page
void print_webpage_home()
{
  bfill.emit_p(PSTR("$F$F$F"
    "<script src=p1.js></script>\n"
    "<h3>Device time: $D$D:$D$D $F $D$D-$D$D</h3>"),
    htmlOkHeader, htmlMobileHeader,htmlRefreshHeader,
    hour()/10, hour()%10, minute()/10, minute()%10, days_str[weekday_today()],
    month()/10, month()%10, day()/10, day()%10
  );

  bfill.emit_p(PSTR(
    "<script>\n"
    "v=$D;\n"
    "w(\"<form name=f action=set method=get><input type=hidden name=b><input type=hidden name=v><input type=hidden name=h><input type=hidden name=m></form>\");\n"
    "for(i=0;i<8;i++){w(\"<h3>Station \"+(i+1)+\": <input type=button id=b\"+i+\" onClick=sf(\"+i+\")> \"+((v>>i)&1?\"in\":\"duration\")+\" "
    "<input type=text id=h\"+i+\" size=2 maxlength=2 />:"
    "<input type=text id=m\"+i+\" size=2 maxlength=2 /></h3>\")}\n"
    ), valve_bitvalue
  );
  bfill.emit_p(PSTR("it(new Array("));
  byte i;
  for(i=0; i<8; i++) {
    bfill.emit_p(PSTR("$D,"), scheduled_seconds[i]);
  }
  bfill.emit_p(PSTR("0), new Array("));
  for(i=0; i<8; i++) {
    bfill.emit_p(PSTR("$D,"), remaining_seconds[i]);
    
  }  
  bfill.emit_p(PSTR("0));\n</script>\n"));
}

// set values according to form inputs
void print_webpage_set_valve(char *p)
{
  int8_t i = -1;
  int8_t v = -1;
  int8_t h = -1;
  int8_t m = -1;
  if (ether.findKeyVal(p, tmp_buffer, TMP_BUFFER_SIZE, "b"))
    i = atoi(tmp_buffer);
    
  if (ether.findKeyVal(p, tmp_buffer, TMP_BUFFER_SIZE, "v"))
    v = atoi(tmp_buffer);
    
  if (ether.findKeyVal(p, tmp_buffer, TMP_BUFFER_SIZE, "h"))
    h = atoi(tmp_buffer);
  
  if (ether.findKeyVal(p, tmp_buffer, TMP_BUFFER_SIZE, "m"))
    m = atoi(tmp_buffer);
    
  if (i<0||i>7||v<0||v>1||h<0||h>120||m<0||m>60) {
    bfill.emit_p(PSTR("$F"), htmlUnauthorized);
    return;
  }

  // if this sets valve to open
  if (v==1) {
    // calculate timing
    scheduled_seconds[i] = (unsigned long)h*3600+(unsigned long)m*60;
    remaining_seconds[i] = scheduled_seconds[i];
    scheduled_stop_time[i] = time_second_counter + scheduled_seconds[i];
  }
  valve_schedule(i, v);
  valve_apply();

  bfill.emit_p(PSTR("$F<meta http-equiv=\"refresh\" content=\"5; url=/\">"), htmlOkHeader);

}

void print_webpage_favicon()
{
  bfill.emit_p(PSTR("$F"), htmlFavicon);
}

void print_webpage_unauthorized()
{
  bfill.emit_p(PSTR("$F"), htmlUnauthorized); 
}

// =============
// NTP Functions
// =============

unsigned long ntp_wait_response()
{
  uint32_t time;
  for (uint32_t i=0; i<100000; i++) {
    ether.packetLoop(ether.packetReceive());
    if (ether.ntpProcessAnswer(&time, ntpclientportL))
    {
      if ((time & 0x80000000UL) ==0){
        time+=2085978496;
      }else{
        time-=2208988800UL;
      }   
      return time + (int32_t)3600*(int32_t)(options[OPTION_TIMEZONE]-12);
    }
    
  }  
  return 0;
}

unsigned long getNtpTime()
{
  unsigned long ans;
  byte tick = 0;
  do {
    ether.ntpRequest(ntpip, ntpclientportL);
    ans = ntp_wait_response();
    delay(250);
    tick ++;
  } while( ans == 0 && tick < 15 );  
  return ans;
}

// analyze the current url
int analyze_get_url(char *p)
{
  if (strncmp("GET /", p, 5) == 0) {
    p = p+5;
    if (strncmp(" ", p, 1)==0) {      
      print_webpage_home();
    } else if (strncmp("favicon.ico", p, 11)==0) {
      print_webpage_favicon();
    } else if (strncmp("set", p, 3)==0) {
      print_webpage_set_valve(p+3);
    } else if (strncmp("p1.js", p, 5)==0) {
      print_webpage_p1();
    } else {
      bfill.emit_p(PSTR("$F"), htmlUnauthorized);
    }
    return 0;
  } else {
    bfill.emit_p(PSTR("$F"), htmlUnauthorized);
    return 0;
  }
}

