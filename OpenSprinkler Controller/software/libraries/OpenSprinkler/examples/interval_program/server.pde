// Example code for OpenSprinkler

/* Server functions
   Creative Commons Attribution-ShareAlike 3.0 license
   Mar 2012 @ Rayshobby.net
*/

#include <OpenSprinkler.h>

// External variables defined in main pde file
extern int ntpclientportL;
extern byte ntpip[];
extern BufferFiller bfill;
extern char tmp_buffer[];
extern OpenSprinkler svc;
extern ProgramData pd;

// ==================
// JavaScript Strings
// ==================
prog_uchar htmlExtJavascriptPath[] PROGMEM = JAVASCRIPT_PATH;

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
    "<meta name=viewport content=\"width=640\">\r\n"
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

// split program data to subsections in order
// to fit each subsection in a single http buffer
#define PROGRAMDATA_SUBSECTION_SIZE 10

// fill program data subsection
void bfill_programdata_sub(byte start_index)
{
  byte pid, bid;
  ProgramStruct prog;    
  for(pid=start_index; pid<start_index+PROGRAMDATA_SUBSECTION_SIZE && pid<pd.nprograms; pid++) {
    pd.read(pid, &prog);
    
    // convert interval remainder (absolute->relative)
    if (prog.days[1] > 1)  pd.drem_to_relative(prog.days);
        
    bfill.emit_p(PSTR("pd[$D]=[$D,$D,$D,$D,$D,$D"), pid, prog.days[0], prog.days[1],
      prog.start_time, prog.end_time, prog.interval, prog.duration);
    for (bid=0; bid<=svc.options[OPTION_EXT_BOARDS]; bid++) {
      bfill.emit_p(PSTR(",$D"),prog.stations[bid]);
    }
    bfill.emit_p(PSTR("];"));
  }
}

boolean print_webpage_programdata_subsection(char *p, byte pos) {
  p=p+pos;
  ether.urlDecode(p);
  
  byte ssid = ((*p)-'0')*10;
  bfill.emit_p(PSTR("$F"), htmlOkHeaderjs);
  bfill_programdata_sub(ssid);
  return true;
}

// fill program data
void bfill_programdata()
{
  byte ssid;
  
  bfill.emit_p(PSTR("<script>var nprogs=$D;var nboards=$D;var pd=[];"),
                    pd.nprograms, svc.options[OPTION_EXT_BOARDS]+1);
  bfill_programdata_sub(0);
  bfill.emit_p(PSTR("</script>\n"));
  if (pd.nprograms > PROGRAMDATA_SUBSECTION_SIZE) {
    // create subsection pages
    for (ssid=1; ssid<=((pd.nprograms-1)/PROGRAMDATA_SUBSECTION_SIZE); ssid++) {
      bfill.emit_p(PSTR("<script src=pds$D.js></script>\n"), ssid);
    }
  }
}

boolean print_webpage_view_program(char *str, byte pos) {

  bfill.emit_p(PSTR("$F$F"), htmlOkHeader, htmlMobileHeader);
  
  bfill_programdata();
  
  bfill.emit_p(PSTR("<script src=\"$F/viewprog.js\"></script>\n"), htmlExtJavascriptPath);

  return true;
}

boolean print_webpage_modify_program(char *p, byte pos) {
  p=p+(pos+1);
  ether.urlDecode(p);
  
  if (!ether.findKeyVal(p, tmp_buffer, TMP_BUFFER_SIZE, "pid")) {
    return false;
  }
  int pid=atoi(tmp_buffer);
  if (!(pid>=-1 && pid< pd.nprograms)) return false;
  bfill.emit_p(PSTR("$F$F"), htmlOkHeader, htmlMobileHeader);
  bfill.emit_p(PSTR("<script>var nboards=$D;var pid=$D;"),
               svc.options[OPTION_EXT_BOARDS]+1, pid);
  // if(pid>-1), this is modifying an existing program
  // if(pid==1), this is adding a new program, no need to provide programdata
  if(pid>-1) {
    ProgramStruct prog;
    pd.read(pid, &prog);
    
    // process interval day remainder (absolute->relative)
    if (prog.days[1] > 1)  pd.drem_to_relative(prog.days);
    
    bfill.emit_p(PSTR("var prog=[$D,$D,$D,$D,$D,$D"),
                 prog.days[0], prog.days[1], prog.start_time, prog.end_time, prog.interval, prog.duration);
    for(byte bid=0;bid<svc.options[OPTION_EXT_BOARDS]+1;bid++) {
      bfill.emit_p(PSTR(",$D"), prog.stations[bid]);
    }
    bfill.emit_p(PSTR("];"));
  }
  bfill.emit_p(PSTR("</script>\n<script src=\"$F/modprog.js\"></script>\n"), htmlExtJavascriptPath);
  return true;
}

/*=============================================
  HTTP GET command format:
  /dp?pw=xxx&pid=xxx
  
  pw: password
  pid:program index (-1 will delete all programs)
  =============================================*/
boolean print_webpage_delete_program(char *p, byte pos) {

  p=p+(pos+1);
  ether.urlDecode(p);
  
  // check password
  if (!ether.findKeyVal(p, tmp_buffer, TMP_BUFFER_SIZE, "pw") || !svc.password_verify(tmp_buffer)) {
    return false;
  }
  
  if (!ether.findKeyVal(p, tmp_buffer, TMP_BUFFER_SIZE, "pid"))
    return false;
    
  int pid=atoi(tmp_buffer);
  if (pid == -1) {
    pd.erase();
  } else if (pid < pd.nprograms) {
    pd.del(pid);
  } else {
    bfill.emit_p(PSTR("$F<script>alert(\"Program index out of range.\")</script>\n"), htmlOkHeader);
    bfill.emit_p(PSTR("<meta http-equiv=\"refresh\" content=\"0; url=/vp\">"));    
    return true;
  }

  bfill.emit_p(PSTR("$F<meta http-equiv=\"refresh\" content=\"0; url=/vp\">"), htmlOkHeader);
  return true;
}

/*=============================================
  HTTP GET command format:
  /gp?d=xx&m=xx&y=xx
  
  d: day (either a number or string 'today')
  m: month
  y: year
  (if any field is missing, will use the current
   date/month/year)
  =============================================*/
boolean print_webpage_plot_program(char *p, byte pos) {
  p=p+(pos+1);
  ether.urlDecode(p);
  
  // yy,mm,dd are simulated date for graphical view
  // devdd is the device day
  int yy,mm,dd,devday,devmin;
  time_t t = now();
  yy = year(t);  mm = month(t);  dd = day(t);
  devday = t/SECS_PER_DAY;  devmin = hour(t)*60+minute();
  
  if (ether.findKeyVal(p, tmp_buffer, TMP_BUFFER_SIZE, "d")) {
    if (strncmp(tmp_buffer, "today", 5)!=0) {
      dd=atoi(tmp_buffer);
      if (!(dd>0 && dd<=31))  return false;
    }
  }
  if (ether.findKeyVal(p, tmp_buffer, TMP_BUFFER_SIZE, "m")) {
    mm=atoi(tmp_buffer);
    if (!(mm>0 && mm<=12))  return false;
  }
  if (ether.findKeyVal(p, tmp_buffer, TMP_BUFFER_SIZE, "y")) {
    yy=atoi(tmp_buffer);
    if (!(yy>=1970))  return false;
  }
  
  bfill.emit_p(PSTR("$F$F<script>var seq=$D,mas=$D,devday=$D,devmin=$D,dd=$D,mm=$D,yy=$D;</script>\n"), htmlOkHeader, htmlMobileHeader,
               svc.options[OPTION_SEQUENTIAL], svc.options[OPTION_MASTER_STATION], devday, devmin, dd, mm, yy);

  bfill_programdata();
  
  bfill.emit_p(PSTR("<script src=\"$F/plotprog.js\"></script>\n"), htmlExtJavascriptPath);
  return true;
}

uint16_t parse_programdata_field(char **p) {
  char* pv;
  int i=0;
  tmp_buffer[i]=0;
  // copy to tmp_buffer until a non-number is encountered
  for(pv=(*p);pv<(*p)+10;pv++) {
    if(!((*pv)>='0'&&(*pv)<='9'))  break;
    else tmp_buffer[i++] = (*pv);
  }
  tmp_buffer[i]=0;
  *p = pv+1;
  return atoi(tmp_buffer);
}

// webpage for handling program changes
boolean print_webpage_change_program(char *p, byte pos) {

  p=p+(pos+1);
  ether.urlDecode(p);
  
  // check password
  if (!ether.findKeyVal(p, tmp_buffer, TMP_BUFFER_SIZE, "pw") || !svc.password_verify(tmp_buffer)) {
    return false;
  }
    
  // parse program index
  if (!ether.findKeyVal(p, tmp_buffer, TMP_BUFFER_SIZE, "pid")) {
    return false;
  }
  int pid=atoi(tmp_buffer);
  if (!(pid>=-1 && pid< pd.nprograms)) return false;
  
  // parse program data
  ProgramStruct prog;
    
  // search for the start of v=[
  char *pv;
  boolean found=false;
  for(pv=p;(*pv)!=0 && pv<p+100;pv++) {
    if(strncmp(pv, "v=[", 3)==0) {
      found=true;
      break;
    }
  }
  if(!found)  return false;
  pv+=3;
  // parse data field
  prog.days[0]= parse_programdata_field(&pv);
  prog.days[1]= parse_programdata_field(&pv);
  prog.start_time = parse_programdata_field(&pv);
  prog.end_time = parse_programdata_field(&pv);
  prog.interval = parse_programdata_field(&pv);
  prog.duration = parse_programdata_field(&pv);
  
  byte bid;
  for(bid=0;bid<svc.options[OPTION_EXT_BOARDS]+1;bid++) {
    prog.stations[bid] = parse_programdata_field(&pv);
  }
  for(;bid<MAX_EXT_BOARDS+1;bid++) {
    prog.stations[bid] = 0;     // clear unused field
  }

  // process interval day remainder (relative-> absolute)
  if (prog.days[1] > 1)  pd.drem_to_absolute(prog.days);
      
  bfill.emit_p(PSTR("$F<script>"), htmlOkHeader);

  if (pid==-1) {
    pd.add(&prog); 
    bfill.emit_p(PSTR("alert(\"New program added.\");"));
  } else {
    pd.modify(pid, &prog);
    bfill.emit_p(PSTR("alert(\"Program $D modified\");"), pid+1);
  }
  bfill.emit_p(PSTR("</script>\n<meta http-equiv=\"refresh\" content=\"0; url=/vp\">"));
  return true;
}

// print home page
boolean print_webpage_home()
{
  byte bid, sid;
  bfill.emit_p(PSTR("$F$F"), htmlOkHeader, htmlMobileHeader);
  bfill.emit_p(PSTR("<script>var ver=$D,devt=$L;\n"),
               SVC_FW_VERSION, now());
  bfill.emit_p(PSTR("var nbrd=$D,seq=$D,tz=$D,sbits=["),
               (int)svc.options[OPTION_EXT_BOARDS]+1,
               (int)svc.options[OPTION_SEQUENTIAL],
               (int)svc.options[OPTION_TIMEZONE]-12
              );
  for(bid=0;bid<=svc.options[OPTION_EXT_BOARDS];bid++)
    bfill.emit_p(PSTR("$D,"), svc.station_bits[bid]);
  bfill.emit_p(PSTR("0];var ps=["));
  for(sid=0;sid<(svc.options[OPTION_EXT_BOARDS]+1)*8;sid++) {
    bfill.emit_p(PSTR("[$D,$D,$D],"), pd.scheduled_program_index[sid], pd.scheduled_duration[sid], pd.remaining_time[sid]);
  } 
  svc.location_get(tmp_buffer);
  bfill.emit_p(PSTR("[0,0,0]];\nvar en=$D,rd=$D,rs=$D,mm=$D,rdst=$L,mas=$D,urs=$D,loc=\"$S\";"),
    svc.status.enabled,
    svc.status.rain_delayed,
    svc.status.rain_sensed,
    svc.status.manual_mode,
    svc.raindelay_stop_time,
    svc.options[OPTION_MASTER_STATION],
    svc.options[OPTION_USE_RAINSENSOR],
    tmp_buffer
  );
  bfill.emit_p(PSTR("var lrun=[$D,$D,$D,$L];\n"),
               pd.lastrun.station, pd.lastrun.program,pd.lastrun.duration,pd.lastrun.endtime);
  // include remote javascript
  bfill.emit_p(PSTR("</script>\n<script src=\"$F/home.js\"></script>\n"), htmlExtJavascriptPath);
  return true;
}

boolean print_webpage_view_options()
{
  bfill.emit_p(PSTR("$F$F"), htmlOkHeader, htmlMobileHeader);
  bfill.emit_p(PSTR("<script>var opts=["));

  byte oid;
  byte noptions = 0;
  // print web editable options
  for (oid=0; oid<NUM_OPTIONS; oid++) {
    if ((svc.option_get_flag(oid)&OPFLAG_WEB_EDIT)==0) continue;
    bfill.emit_p(PSTR("\"$F\",$D,$D,$D,"),
                 svc.options_str[oid],
                 svc.option_get_flag(oid)&OPFLAG_BOOL,
                 (oid==OPTION_TIMEZONE) ? (int)svc.options[oid]-12 : (int)svc.options[oid],
                 oid);
    noptions ++;
  }
  svc.location_get(tmp_buffer);
  bfill.emit_p(PSTR("0];var nopts=$D,loc=\"$S\";"), noptions, tmp_buffer);
  if (svc.options[OPTION_USE_RTC]) {
    bfill.emit_p(PSTR("var yr=$D,mo=$D,dy=$D,hr=$D,min=$D,sec=$D;"),
      year(), month(), day(), hour(), minute(), second());
  }
  bfill.emit_p(PSTR("</script>\n"));
  // include remote javascript
  bfill.emit_p(PSTR("<script src=\"$F/viewoptions.js\"></script>\n"), htmlExtJavascriptPath);
  return true;
}

/*=============================================
  HTTP GET command format:
  /cv?pw=xxx&rst=x&en=x&mm=x&rd=x
  
  pw:  password
  rst: reset (0 or 1)
  en:  enable (0 or 1)
  mm:  manual mode (0 or 1)
  rd:  rain delay hours (0 turns off rain delay)
  =============================================*/
boolean print_webpage_change_values(char *p, byte pos)
{
  p += (pos+1);
  // if no password is attached, or password is incorrect
  if (!ether.findKeyVal(p, tmp_buffer, TMP_BUFFER_SIZE, "pw") || !svc.password_verify(tmp_buffer)) {
    return false;
  }

#define TIME_REBOOT_DELAY  12

  if (ether.findKeyVal(p, tmp_buffer, TMP_BUFFER_SIZE, "rst") && atoi(tmp_buffer) > 0) {
    bfill.emit_p(PSTR("$F<meta http-equiv=\"refresh\" content=\"$D; url=/\">"), htmlOkHeader, TIME_REBOOT_DELAY);
    bfill.emit_p(PSTR("Rebooting, wait for $D seconds..."), TIME_REBOOT_DELAY);
    ether.httpServerReply(bfill.position());   
    svc.reboot();
  } 
  
  if (ether.findKeyVal(p, tmp_buffer, TMP_BUFFER_SIZE, "en")) {
    byte oldvalue = svc.status.enabled;
    if (tmp_buffer[0]=='1') {
      if (oldvalue!=1)  svc.enable(); // do this only if status has changed
    } else if (tmp_buffer[0]=='0') {
      if (oldvalue!=0)  svc.disable(); 
    }
    else {
      return false;
    }
  }   
  
  if (ether.findKeyVal(p, tmp_buffer, TMP_BUFFER_SIZE, "mm")) {
    byte oldvalue = svc.status.manual_mode;
    if (tmp_buffer[0]=='1') {
      if (oldvalue!=1) {
        svc.manual_mode_on(); // do this only if we are not already in manual mode
        pd.reset_runtime();
      }
    }
    else if (tmp_buffer[0]=='0') {
      if (oldvalue!=0) {
        svc.manual_mode_off();
        pd.reset_runtime();
      }
    }
    else return false;
  }
  if (ether.findKeyVal(p, tmp_buffer, TMP_BUFFER_SIZE, "rd")) {
    int rd = atoi(tmp_buffer);
    if (rd>0) {
      svc.raindelay_start(rd);
    } else if (rd==0){
      svc.raindelay_stop();
    } else  return false;
  }  
 
  
  bfill.emit_p(PSTR("$F<meta http-equiv=\"refresh\" content=\"0; url=/\">"), htmlOkHeader);
  return true;
}
  
boolean print_webpage_change_options(char *p, byte pos)
{
  p += (pos+1);

  // if no password is attached, or password is incorrect
  if (!ether.findKeyVal(p, tmp_buffer, TMP_BUFFER_SIZE, "pw") || !svc.password_verify(tmp_buffer)) {
    return false;
  }

  byte old_tz = svc.options[OPTION_TIMEZONE];
  
  // !!! p and bfill share the same buffer, so don't write
  // to bfill before you are done analyzing the buffer !!!
  
  // process option values
  byte err = 0;
  for (byte oid=0; oid<NUM_OPTIONS; oid++) {
    if ((svc.option_get_flag(oid)&OPFLAG_WEB_EDIT)==0) continue;
    if (svc.option_get_flag(oid)&OPFLAG_BOOL)  svc.options[oid] = 0;  // set a bool variable to 0 first
    char tbuf2[5] = {'o', 0, 0, 0, 0};
    itoa(oid, tbuf2+1, 10);
    if (ether.findKeyVal(p, tmp_buffer, TMP_BUFFER_SIZE, tbuf2)) {
      if (svc.option_get_flag(oid)&OPFLAG_BOOL) {
        svc.options[oid] = 1;  // if the bool variable is detected, set to 1
        continue;
      }
      int v = atoi(tmp_buffer);
      if (oid==OPTION_TIMEZONE) { v += 12; }
      if (v>=0 && v<=svc.option_get_max(oid)) {
        svc.options[oid] = v;
      } else {
        err = 1;
      }
    }
  }
  
  if (ether.findKeyVal(p, tmp_buffer, TMP_BUFFER_SIZE, "loc")) {
    ether.urlDecode(tmp_buffer);
    svc.location_set(tmp_buffer);    
  }
  
  // process rtc change
  if (svc.options[OPTION_USE_RTC]) {
    // see if 'change time' checkbox is checked
    if (ether.findKeyVal(p, tmp_buffer, TMP_BUFFER_SIZE, "tchg")) {
      int t[6];
      char tbuf2[3]={'t', 0, 0};
      for (byte tid=0; tid<6; tid++) {
        tbuf2[1] = '0'+tid;
        if (ether.findKeyVal(p, tmp_buffer, TMP_BUFFER_SIZE, tbuf2)) {
          t[tid] = atoi(tmp_buffer);
        }
      }
      if (t[0]<1970 || t[1]<1 || t[1]>12 || t[2]<1 || t[2]>31 || t[3]<0 || t[3]>23 ||
          t[4]<0 || t[4]>59 || t[5]<0 || t[5]>59) {
        err=1;
      } else {
        setTime(t[3], t[4], t[5], t[2], t[1], t[0]);
        RTC.set(now());
      }
    }
  }
  
  if (err) {
    bfill.emit_p(PSTR("$F<script>alert(\"Some values are out of bound.\")</script>\n"), htmlOkHeader);
    bfill.emit_p(PSTR("<meta http-equiv=\"refresh\" content=\"0; url=/vo\">"));
    return true;
  } 

  svc.options_save();
  
  if (ether.findKeyVal(p, tmp_buffer, TMP_BUFFER_SIZE, "npw")) {
    char tbuf2[TMP_BUFFER_SIZE];
    if (ether.findKeyVal(p, tbuf2, TMP_BUFFER_SIZE, "cpw") && strcmp(tmp_buffer, tbuf2) == 0) {
      svc.password_set(tmp_buffer);
      bfill.emit_p(PSTR("$F<script>alert(\"New password set.\")</script>\n"), htmlOkHeader);
    } else {
      bfill.emit_p(PSTR("$F<script>alert(\"Confirmation does not match!\")</script>\n"), htmlOkHeader);
    }
    bfill.emit_p(PSTR("<meta http-equiv=\"refresh\" content=\"0; url=/vo\">"));
    return true;
  }

  bfill.emit_p(PSTR("$F<script>alert(\"Options values saved.\")</script>\n"), htmlOkHeader);
  bfill.emit_p(PSTR("<meta http-equiv=\"refresh\" content=\"0; url=/vo\">"));
  
  if (old_tz != svc.options[OPTION_TIMEZONE]) {
    time_t t = getNtpTime();
    if (t!=0)
      setTime(t);
  }
    
  return true;
}

/*=================================================
  HTTP GET command format:
  /snx     -> get station bit (e.g. /sn1, /sn2 etc.)
  /sn0     -> get all bits
  
  The following will only work if controller is
  switched to manual mode:
  
  /snx=0   -> turn off station 
  /snx=1   -> turn on station
  /snx=1&t=xx -> turn on with timer (in seconds)
  =================================================*/
boolean print_webpage_station_bits(char *p, byte pos) {

  p += pos;
  int sid;
  byte i, sidmin, sidmax;  

  // parse station name
  i=0;
  while((*p)>='0'&&(*p)<='9'&&i<4) {
    tmp_buffer[i++] = (*p++);
  }
  tmp_buffer[i]=0;
  sid = atoi(tmp_buffer);
  if (!(sid>=0&&sid<=(svc.options[OPTION_EXT_BOARDS]+1)*8)) return false;
  
  // parse get/set command
  if ((*p)=='=') {
    if (sid==0) return false;
    sid--;
    // this is a set command
    // can only do it when in manual mode
    if (!svc.status.manual_mode) {
      bfill.emit_p(PSTR("$F<script>alert(\"Station bits can only be set in manual mode.\")</script>\n</script>"), htmlOkHeader);
      return true;
    }
    // parse value
    p++;
    if ((*p)=='0') {
      manual_station_off(sid);
    } else if ((*p)=='1') {
      int ontimer = 0;
      if (ether.findKeyVal(p, tmp_buffer, TMP_BUFFER_SIZE, "t")) {
        ontimer = atoi(tmp_buffer);
        if (!(ontimer>=0))  return false;
      }
      manual_station_on((byte)sid, ontimer);
    } else {
      return false;
    }
    bfill.emit_p(PSTR("$F<meta http-equiv=\"refresh\" content=\"0; url=/\">"), htmlOkHeader);      
    return true;
  } else {
    // this is a get command
    bfill.emit_p(PSTR("$F"), htmlOkHeader);
    if (sid==0) { // print all station bits
      sidmin=0;sidmax=(svc.options[OPTION_EXT_BOARDS]+1)*8;
    } else {  // print one station bit
      sidmin=(sid-1);
      sidmax=sid;
    }
    for (sid=sidmin;sid<sidmax;sid++) {
      if (svc.status.enabled && (!svc.status.rain_delayed) && !(svc.options[OPTION_USE_RAINSENSOR] && svc.status.rain_sensed)) {
        bfill.emit_p(PSTR("$D"), (svc.station_bits[(sid>>3)]>>(sid%8))&1);
      } else bfill.emit_p(PSTR("$D"), 0);
    }
    return true;      
  }
  return false;
}

boolean print_webpage_favicon()
{
  bfill.emit_p(PSTR("$F"), htmlFavicon);
  return true;
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
  } while( ans == 0 && tick < 10 );  
  return ans;
}

// analyze the current url
void analyze_get_url(char *p)
{
  // the tcp packet usually starts with 'GET /' -> 5 chars    
  char *str = p+5;
  boolean success = false;
  if (strncmp(" ", str, 1)==0) {
    success = print_webpage_home();
  } else if (strncmp("favicon.ico", str, 11)==0) {
    success = print_webpage_favicon();
  } else if (strncmp("cv", str, 2)==0) {  // change values
    success = print_webpage_change_values(str, 2);
  } else if (strncmp("vp", str, 2)==0) {  // view program
    success = print_webpage_view_program(str, 2);
  } else if (strncmp("mp", str, 2)==0) {  // modify program
    success = print_webpage_modify_program(str, 2);
  } else if (strncmp("dp", str, 2)==0) {  // delete program
    success = print_webpage_delete_program(str, 2);
  } else if (strncmp("cp", str, 2)==0) {  // change program
    success = print_webpage_change_program(str, 2);
  } else if (strncmp("gp", str, 2)==0) { // graphics view
    success = print_webpage_plot_program(str, 2);
  } else if (strncmp("vo", str, 2)==0) {  // view options
    success = print_webpage_view_options();
  } else if (strncmp("co", str, 2)==0) {  // change options
    success = print_webpage_change_options(str, 2);
  } else if (strncmp("sn", str, 2)==0) { // get/set station bits
    success = print_webpage_station_bits(str, 2);
  } else if (strncmp("pds", str, 3)==0) { // get program data subsection
    success = print_webpage_programdata_subsection(str, 3);
  }
  if (success == false) {
    bfill.emit_p(PSTR("$F"), htmlUnauthorized);
  }
}

