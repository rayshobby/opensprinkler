// Example code for OpenSprinkler

/* Server functions
   Creative Commons Attribution-ShareAlike 3.0 license
   Mar 2012 @ Rayshobby.net
*/

// ++++++ A large part of the web interface design is based on tuxgraphics eth-pooltimer-1.0 ++++++

#include <OpenSprinkler.h>
#include "schedule.h"

// buffer filler
BufferFiller bfill;

// scratch buffer
char tmp_buffer[TMP_BUFFER_SIZE+1];

// Default NTP server ip
byte ntpip[] = {204,9,54,119};

// Default NTP client
int ntpclientportL = 0;

extern unsigned long time_second_counter;
extern OpenSprinkler svc;

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

/*prog_uchar htmlMobileHeader[] PROGMEM =
    "<meta name=viewport content=\"width=device-width\">\n"
;*/

// Javascript for generating and submitting schedule changes
prog_uchar htmlScheduleTable_js0[] PROGMEM =
    "function w(s){document.writeln(s)}\n"
    "function fc(){pw=prompt(\"Password:\",\"\");if(pw!=null){gt(fm.elements[0]);fm.elements[1].value=pw;fm.submit()};}\n"
    "function gt(ft){ft.value=\"b\"+brd+\"d\"+day+\"x\";\n"
    "for(i=si;i<ei;i++){b=0;for(j=0;j<8;j++){if(document.getElementById(\"t\"+i+\"-\"+j).style.backgroundColor) b|=(1<<j);}\n"
    "if(b) ft.value+=(i.toString())+\"i\"+(b.toString())+\"v\";} ft.value+=\"q\";}\n"
    "function wr(i,j){w(\"<td id=t\"+i+\"-\"+j+\" onmousedown=cb(this) onmouseover=mb(this)>&nbsp;</td>\")}\n"    
    "function cb(e){c=1;col=e.style.backgroundColor?\"\":\"7AFA7A\";e.style.backgroundColor=col;}\n"
    "function mb(e){if(c) e.style.backgroundColor=col}\n"
    "function er(){for(i=si;i<ei;i++){for(j=0;j<8;j++){document.getElementById(\"t\"+i+\"-\"+j).style.backgroundColor=\"\";}}}\n"     
;

// Javascript for displaying clickable schedule table
prog_uchar htmlScheduleTable_js1[] PROGMEM =
    "col=\"\",c=0;\n"
    "w(\"<form name=fm action=cs method=get><input type=hidden name=t><input type=hidden name=p></form>\");\n"
    "w(\"<p><button type=button onclick=er()>Clear all</button></p>\");\n"       
    "w(\"<table border=1 onmouseup=\\\"c=0\\\" onmousedown=\\\"if(typeof event.preventDefault!=\'undefined\'){event.preventDefault();}\\\">\");\n"
    "for(i=si;i<ei;i++){if(!((i-si)%12)){w(\"<tr><td><button type=button onclick=fc()>Submit</button></td>\");for(j=0;j<8;j++){z=brd*8+j+1;w(\"<td>S\"+(z/10>>0)+(z%10)+\"</td>\");}}\n"
    "w(\"</tr>\");h=i/sph>>0;m=(i%sph)*(60/sph);t=\"\"+(h/10>>0)+(h%10)+\":\"+(m/10>>0)+(m%10);\n"
    "w(\"<tr><td\"+((i==ic)?\" style=\\\"background-color:yellow;\\\">\":\">\")+t+\"</td>\");\n"
    "for(j=0;j<8;j++){wr(i,j);}\n"
    "w(\"</tr>\");}\n"
    "w(\"</table>\");\n"
;

// Javascript for loading existing schedule and display to table    
prog_uchar htmlScheduleTable_js2[] PROGMEM =
    "function it(a){\n"
    "for(i=0;i<a.length-1;i+=2){\n"
    "for(j=0;j<8;j++){\n"
    "e=document.getElementById(\"t\"+a[i]+\"-\"+j);\n"
    "if(e!=null&&(a[i+1]&(1<<j))) e.style.backgroundColor=\"#7AFA7A\";\n"
    "}}}\n"
;

// Javascript for printing values in the homepage
prog_uchar htmlScheduleTable_js3[] PROGMEM =
    "function w(s){document.writeln(s)}\n"
    "function srd(f){h=prompt(\"Enter hours to delay\",\"0\");if(h!=null){f.elements[2].value=h;f.submit()};}\n"
    "w(\"<hr /><p><b>Station Status:</b></p>\")\n"
    "for(d=0;d<nd;d++){for(i=0;i<8;i++){sid=d*8+i+1;\n"
    "w(\"S\"+(sid/10>>0)+(sid%10)+\": \"+((vv[d]>>i)&1?\"O\":\"*\")+\"<br />\")}}\n"
    "w(\"<hr /><b>Multi-Stn</b>: \"+(mc?(\"cleared\").fontcolor(\"green\"):(\"error\").fontcolor(\"red\")));\n"
    "w(\"<br /><b>Operation</b>: \"+(go?(\"enabled\").fontcolor(\"green\"):(\"disabled\").fontcolor(\"red\")))\n;"
    "w(\"<br /><b>Raindelay</b>: \"+(rd?(\"on\").fontcolor(\"red\")+\" (till \"+(rdh/10>>0)+(rdh%10)+\":\"+(rdm/10>>0)+(rdm%10)+\" \"+(rdo/10>>0)+(rdo%10)+\"-\"+(rdd/10>>0)+(rdd%10)+\")\":(\"off\").fontcolor(\"black\")))\n;"
;
  
byte *htmlWebpage_js[] = {htmlScheduleTable_js0,
                          htmlScheduleTable_js1,
                          htmlScheduleTable_js2,
                          htmlScheduleTable_js3};

// print javascript webpage
void print_webpage_js(byte idx)
{
  bfill.emit_p(PSTR("$Fsph=$D;\n$F"), htmlOkHeaderjs, SC_SLOTS_PER_HOUR, htmlWebpage_js[idx]);
}

void print_webpage_select_schedule()
{
  bfill.emit_p(PSTR("$F<a href=/><-home</a><br /><h4><b>Select Stations to Schedule:</b></h4><ul>"), htmlOkHeader);

  for (byte i=0;i<=svc.options[OPTION_EXT_BOARDS];i++) {
    bfill.emit_p(PSTR("<li><a href=/sc?day=$D&brd=$D>S$D$D - S$D$D</a>"),
      svc.weekday_today(),
      i,
      (i*8+1)/10,
      (i*8+1)%10,
      ((i+1)*8)/10,
      ((i+1)*8)%10);
    if (i==0)
      bfill.emit_p(PSTR(" (master)</li>"));
    else
      bfill.emit_p(PSTR(" (ext.$D)</li>"), i);
      
  }
  bfill.emit_p(PSTR("</ul>"));
}

// webpage for setting schedules
void print_webpage_set_schedule(char *str, byte pos) {

  if (str[pos] != '?')  return;
  byte day = 0, brd = 0;
  if (ether.findKeyVal(str+pos+1, tmp_buffer, TMP_BUFFER_SIZE, "day")>0) {
    ether.urlDecode(tmp_buffer);
    //if(!isDigit(tmp_buffer[0])) return;
    tmp_buffer[1]=0;
    day = atoi(tmp_buffer);
  }
  if (ether.findKeyVal(str+pos+1, tmp_buffer, TMP_BUFFER_SIZE, "brd")>0) {
    ether.urlDecode(tmp_buffer);
    tmp_buffer[1]=0;
    brd = atoi(tmp_buffer);
  }
  // check correct parameters
  if (day<0 || day>6 || brd<0 || brd>svc.options[OPTION_EXT_BOARDS]) {
    bfill.emit_p(PSTR("$F"), htmlUnauthorized);
    return;
  }
  bfill.emit_p(PSTR("$F"), htmlOkHeader);
  
  int i, j, k;
  // print schedule menu bar
  bfill.emit_p(PSTR("<a href=/sel><-Schedule</a>&nbsp;&nbsp;"));
  for(i=0; i<7; i++) {
    if (i==day) {
      bfill.emit_p(PSTR("<b>$F</b>&nbsp;&nbsp;"), svc.days_str[i]);
    } else {
      bfill.emit_p(PSTR("<a href=/sc?day=$D&brd=$D>[$F]</a>&nbsp;&nbsp;"),
        i,
        brd,
        svc.days_str[i]
      );
    }
  }
  
  // print schedule
  bfill.emit_p(PSTR("<script>day=$D,brd=$D,ic=$D,ns=$D,si=$D,ei=$D;</script>\n<script src=p0.js></script>\n<script src=p1.js></script>\n<script src=p2.js></script>\n"),
                    day, brd, day==(svc.weekday_today()) ? schedule_time_to_slot(hour(), minute()) : -1,
                    SC_NUM_SLOTS_PER_DAY,
                    (int)svc.options[OPTION_DAY_START]*SC_SLOTS_PER_HOUR,
                    (int)svc.options[OPTION_DAY_END]*SC_SLOTS_PER_HOUR);
  bfill.emit_p(PSTR("<script>\n"));
  bfill.emit_p(PSTR("it(new Array("));
  
  
  // load current schedule from eeprom
  byte buf[EEPROM_BLOCK_SIZE];
  int cnt=0;
  for(i=0;i<SC_NUM_SLOTS_PER_DAY;i+=EEPROM_BLOCK_SIZE) {
    schedule_read_slots(brd, day, i, buf);
    for(j=0;j<EEPROM_BLOCK_SIZE;j++) {
      if(buf[j]) bfill.emit_p(PSTR("$D,$D,"), (uint16_t)i+(uint16_t)j, (uint16_t)buf[j]);
    }
  }
  bfill.emit_p(PSTR("0));\n"));
  bfill.emit_p(PSTR("</script>"));
}

// webpage for handling schedule changes
void print_webpage_change_schedule(char *p, byte pos) {
  if (strncmp(p+pos, "?t=", 3) != 0) return;
  p=p+(pos+3);
  ether.urlDecode(p);  
  // if no password is attached, or password is incorrect
  if (!ether.findKeyVal(p, tmp_buffer, TMP_BUFFER_SIZE, "p") || !svc.password_verify(tmp_buffer) || strncmp(p, "b", 1) !=0) {
    bfill.emit_p(PSTR("$F"), htmlUnauthorized);
    return;
  }  
  
  p++;
  p[1]=0;
  byte brd=atoi(p);
  p=p+2;  
  p[1]=0;
  byte day=atoi(p);
  
  if(day<0 || day>6 || brd<0 || brd>svc.options[OPTION_EXT_BOARDS])
  {
    bfill.emit_p(PSTR("$F"), htmlUnauthorized);
    return;
  }  
  
  p=p+2;
  byte nchar=0;
  int pair[2];
  byte write_alert = 0;
  
  // !!! must read from the buffer first, do not do bfill.emit_p yet !!!
  if (svc.ext_eeprom_write_lock()) {
    schedule_clear_day(brd, day);
    for(;nchar<16;p++) {   // if an element is too long, something is wrong
      if ((*p)=='q' || (*p)==0)  break;
      if ((*p)=='i') {
        tmp_buffer[nchar]=0;
        pair[0]=atoi(tmp_buffer);
        nchar=0;
      } else if ((*p)=='v') {
        tmp_buffer[nchar]=0;
        pair[1]=atoi(tmp_buffer);
        nchar=0;
        schedule_write_slot(brd, day, pair[0], pair[1]);
      } else
        tmp_buffer[nchar++]=(*p);
    }
    svc.ext_eeprom_write_unlock();
  } else {
    write_alert = 1;
  }
  bfill.emit_p(PSTR("$F"), htmlOkHeader);  
  if (write_alert)
    bfill.emit_p(PSTR("<script>alert(\"Someone else is writing. Wait a few seconds and try again.\")</script>\n"));
    
#define TIME_REDIRECT_DELAY 2

  bfill.emit_p(PSTR("<meta http-equiv=\"refresh\" content=\"$D; url=/sc?day=$D&brd=$D\">"), TIME_REDIRECT_DELAY, day, brd);
  bfill.emit_p(PSTR("Redirect in $D seconds..."), TIME_REDIRECT_DELAY);
 
}

// print home page
// displays all necessary status
void print_webpage_home()
{
  bfill.emit_p(PSTR("$F"
    "<h4>"
    "<a href=/>[Refresh]</a>&nbsp;"
    "<a href=/so>[Config]</a>&nbsp;"
    "<a href=/sel>[Schedule]</a></h4>"), htmlOkHeader);
    
  bfill.emit_p(PSTR(
    "<b>FW version</b>: $D.$D<br/><b>Device time</b>: $D$D:$D$D $F $D$D-$D$D<br />"
    ), SVC_FW_VERSION/10, SVC_FW_VERSION%10, 
    hour()/10, hour()%10, minute()/10, minute()%10,
    svc.days_str[svc.weekday_today()], month()/10, month()%10, day()/10, day()%10
  );
  bfill.emit_p(PSTR("<script>nd=$D;vv=new Array("), (int)svc.options[OPTION_EXT_BOARDS]+1);
  for(byte bidx=0;bidx<=svc.options[OPTION_EXT_BOARDS];bidx++)
    bfill.emit_p(PSTR("$D,"), svc.get_board_schedule(bidx));
  bfill.emit_p(PSTR("0);mc=$D;go=$D;rd=$D;rdh=$D;rdm=$D;rdo=$D;rdd=$D;</script>\n<script src=p3.js></script>\n"),
    svc.multistation_check(),
    svc.enabled,
    svc.raindelayed,
    raindelay_stop_clocktime[0],
    raindelay_stop_clocktime[1],
    raindelay_stop_clocktime[2],
    raindelay_stop_clocktime[3]
  );

  bfill.emit_p(PSTR(
    "<form name=hf action=co method=get><p>Password:<input type=password size=10 name=p></p>"
    "<input type=hidden name=ve><input type=hidden name=rd value=0><input type=hidden name=rst value=0>"
    "<input type=button onclick=\"hf.elements[1].value=$D;hf.submit();\" value=\"$F\">"
    "<input type=button onclick=\"srd(hf)\" value=\"Rain delay\">"
    "<input type=button onclick=\"hf.elements[3].value=1;hf.submit();\" value=\"Reboot device\"></form>"    
    ), 
    (1-svc.enabled), svc.enabled ? PSTR("Stop operation") : PSTR("Start oepration")
  );
}

void print_webpage_set_options()
{
  bfill.emit_p(PSTR("$F"), htmlOkHeader);
  
  // print "back to home" link
  bfill.emit_p(PSTR("<a href=/><-home</a>"));  
  
  bfill.emit_p(PSTR("<form name=f action=co method=get>"));
  byte i;
  // print web editable options
  for (i=0; i<NUM_OPTIONS; i++) {
    if ((svc.option_get_flag(i)&OPFLAG_WEB_EDIT)==0) continue;
    bfill.emit_p(PSTR("<h4>$F<input type=text size=4 value=$D name=o$D></h4>"),
      svc.options_str[i],
      (i==OPTION_TIMEZONE) ? (int)svc.options[i]-12 : svc.options[i],
      i);
  }
  bfill.emit_p(PSTR(
    "<h4>Password:<input type=password size=10 name=p></h4><input type=hidden name=cs value=0>"
    "<input type=hidden name=rst value=0><input type=submit value=\"Submit changes\">"
    "<input type=button onclick=\"f.cs.value=1;f.submit();\" value=\"Clear schedule\">"
    "<h4>Change password</b>:<input type=password size=10 name=np>&nbsp;&nbsp;Confirm:&nbsp;<input type=password size=10 name=cp></h4>"
    "</form>"));
}

void print_webpage_change_options(char *p, byte pos)
{
  p += (pos+1);
  // if no password is attached, or password is incorrect
  if (!ether.findKeyVal(p, tmp_buffer, TMP_BUFFER_SIZE, "p") || !svc.password_verify(tmp_buffer)) {
    bfill.emit_p(PSTR("$F"), htmlUnauthorized);
    return;
  }
  
  // !!! p and bfill share the same buffer, so don't write
  // to bfill before you are done analyzing the buffer !!!
  
  if (ether.findKeyVal(p, tmp_buffer, TMP_BUFFER_SIZE, "ve")) {
    if (tmp_buffer[0]=='1') web_mode_start_operation();
    else if (tmp_buffer[0]=='0') web_mode_stop_operation();
  }
  
  if (ether.findKeyVal(p, tmp_buffer, TMP_BUFFER_SIZE, "rd")) {
    int v = atoi(tmp_buffer);
    if (v>=0 && v<SC_RAINDELAY_MAX) {
      web_mode_start_raindelay(v);
    } else {
      web_mode_stop_raindelay();
    }
  }
  
  if (ether.findKeyVal(p, tmp_buffer, TMP_BUFFER_SIZE, "cs") && atoi(tmp_buffer) > 0) {
    for(byte i=0;i<=svc.options[OPTION_EXT_BOARDS];i++) {
      schedule_clear_all(i);
    }
  }
  
  // process option values
  byte err = 0;
  for (byte i=0; i<NUM_OPTIONS; i++) {
    if ((svc.option_get_flag(i)&OPFLAG_WEB_EDIT)==0) continue;
    char tbuf2[5] = {'o', 0, 0, 0, 0};
    itoa(i, tbuf2+1, 10);
    if (ether.findKeyVal(p, tmp_buffer, TMP_BUFFER_SIZE, tbuf2)) {
      int v = atoi(tmp_buffer);
      if (i==OPTION_TIMEZONE) { v += 12; }
      if (v>=0 && v<=svc.option_get_max(i)) {
        svc.options[i] = v;
      } else {
        err = 1;
      }
    }
  }
  
  svc.options_save();
  
  if (err) {
    bfill.emit_p(PSTR("$F<script>alert(\"Some values are out of bound.\")</script>\n"), htmlOkHeader);
    bfill.emit_p(PSTR("<meta http-equiv=\"refresh\" content=\"0; url=/so\">"));
    return;
  } 
  
  if (ether.findKeyVal(p, tmp_buffer, TMP_BUFFER_SIZE, "np")) {
    char tbuf2[TMP_BUFFER_SIZE];
    if (ether.findKeyVal(p, tbuf2, TMP_BUFFER_SIZE, "cp") && strcmp(tmp_buffer, tbuf2) == 0) {
      svc.password_set(tmp_buffer);
      bfill.emit_p(PSTR("$F<script>alert(\"New password set.\")</script>\n"), htmlOkHeader);
    } else {
      bfill.emit_p(PSTR("$F<script>alert(\"Confirmation does not match!\")</script>\n"), htmlOkHeader);
    }
    bfill.emit_p(PSTR("<meta http-equiv=\"refresh\" content=\"0; url=/so\">"));
    return;
  }
 
  
#define TIME_REBOOT_DELAY  15

  tmp_buffer[0]=0;
  if (ether.findKeyVal(p, tmp_buffer, TMP_BUFFER_SIZE, "rst") && atoi(tmp_buffer) > 0) {
    bfill.emit_p(PSTR("$F<meta http-equiv=\"refresh\" content=\"$D; url=/\">"), htmlOkHeader, TIME_REBOOT_DELAY);
    bfill.emit_p(PSTR("Rebooting, wait for $D seconds..."), TIME_REBOOT_DELAY);
    ether.httpServerReply(bfill.position());   
    svc.reboot();
  }
  
  bfill.emit_p(PSTR("$F<meta http-equiv=\"refresh\" content=\"2; url=/\">"), htmlOkHeader);
  return;  
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
  for (uint32_t i=0; i<10000; i++) {
    ether.packetLoop(ether.packetReceive());
    if (ether.ntpProcessAnswer(&time, ntpclientportL))
    {
      if ((time & 0x80000000UL) ==0){
        time+=2085978496;
      }else{
        time-=2208988800UL;
      }   
      return time + (int32_t)3600*(int32_t)(svc.options[OPTION_TIMEZONE]-12);
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
  } while( ans == 0 && tick < 5 );  
  return ans;
}

// analyze the current url
void analyze_get_url(char *p)
{
  // the tcp packet usually starts with 'GET /' -> 5 chars    
  char *str = p+5;
  if (strncmp(" ", str, 1)==0) {
    print_webpage_home();
  } else if (strncmp("favicon.ico", str, 11)==0) {
    print_webpage_favicon();
  } else if(strncmp("sel", str, 3)==0) {
    print_webpage_select_schedule();
  } else if (strncmp("sc", str, 2)==0) {
    print_webpage_set_schedule(str, 2);
  } else if (strncmp("cs", str, 2)==0) {
    print_webpage_change_schedule(str, 2);
  } else if (strncmp("so", str, 2)==0) {
    print_webpage_set_options();
  } else if (strncmp("co", str, 2)==0) {
    print_webpage_change_options(str, 2);
  } else if (strncmp("p0.js", str, 5)==0) {
    print_webpage_js(0);
  } else if (strncmp("p1.js", str, 5)==0) {
    print_webpage_js(1);
  } else if (strncmp("p2.js", str, 5)==0) {
    print_webpage_js(2);
  } else if (strncmp("p3.js", str, 5)==0) {
    print_webpage_js(3);
  } else {
    bfill.emit_p(PSTR("$F"), htmlUnauthorized);
  }
  /*else if (strncmp("GET /ct", str, 7)==0) {
    proc_webpage_changetime(str, 7);
  }
  else if (strncmp("GET /so", str, 7)==0) {
    print_webpage_setoption();
  } else if (strncmp("GET /co", str, 7)==0) { 
  } else if (strncmp("GET /sm", str, 7)==0) {
    print_webpage_setmode();
  } else if (strncmp("GET /cm", str, 7)==0) {
  }
  */
}

