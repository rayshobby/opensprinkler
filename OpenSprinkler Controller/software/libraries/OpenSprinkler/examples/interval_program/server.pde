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

// fill program data
void bfill_programdata()
{
  byte pid, bid;
  ProgramStruct prog;  
  
  bfill.emit_p(PSTR("var nprogs=$D;var nboards=$D;var pd=["),
                    pd.nprograms, svc.options[OPTION_EXT_BOARDS]+1);
  for (pid=0; pid<pd.nprograms; pid++) {
    pd.read(pid, &prog);
    
    // convert interval remainder (absolute->relative)
    if (prog.days[1] > 1)  pd.drem_to_relative(prog.days);
        
    bfill.emit_p(PSTR("[$D,$D,$D,$D,$D,$D"), prog.days[0], prog.days[1], prog.start_time, prog.end_time, prog.interval, prog.duration);
    for (bid=0; bid<=svc.options[OPTION_EXT_BOARDS]; bid++) {
      bfill.emit_p(PSTR(",$D"),prog.stations[bid]);
    }
    if (pid==(pd.nprograms-1))
      bfill.emit_p(PSTR("]"));
    else
      bfill.emit_p(PSTR("],"));
  }
  bfill.emit_p(PSTR("];"));
}

boolean print_webpage_view_program(char *str, byte pos) {

  bfill.emit_p(PSTR("$F$F<script>"), htmlOkHeader, htmlMobileHeader);
  
  bfill_programdata();
  
  bfill.emit_p(PSTR("</script>\n<script src=\"$F/viewprog.js\"></script>\n"), htmlExtJavascriptPath);

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
    pd.clear();
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
  
  bfill.emit_p(PSTR("$F$F<script>var seq=$D,mas=$D,devday=$D,devmin=$D,dd=$D,mm=$D,yy=$D;"), htmlOkHeader, htmlMobileHeader,
               svc.options[OPTION_SEQUENTIAL], svc.options[OPTION_MASTER_STATION], devday, devmin, dd, mm, yy);

  bfill_programdata();
  
  bfill.emit_p(PSTR("</script>\n<script src=\"$F/plotprog.js\"></script>\n"), htmlExtJavascriptPath);
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
  bfill.emit_p(PSTR("[0,0,0]];\nvar en=$D,rd=$D,rs=$D,rdst=$L,mas=$D,urs=$D,loc=\"$S\";"),
    svc.status.enabled,
    svc.status.rain_delayed,
    svc.status.rain_sensed,
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
  bfill.emit_p(PSTR("0];var nopts=$D,loc=\"$S\";</script>\n"), noptions, tmp_buffer);
  // include remote javascript
  bfill.emit_p(PSTR("<script src=\"$F/viewoptions.js\"></script>\n"), htmlExtJavascriptPath);
  return true;
}

boolean print_webpage_change_values(char *p, byte pos)
{
  p += (pos+1);
  // if no password is attached, or password is incorrect
  if (!ether.findKeyVal(p, tmp_buffer, TMP_BUFFER_SIZE, "pw") || !svc.password_verify(tmp_buffer)) {
    return false;
  }

  if (ether.findKeyVal(p, tmp_buffer, TMP_BUFFER_SIZE, "en")) {
    if (tmp_buffer[0]=='1') svc.enable();
    else if (tmp_buffer[0]=='0') svc.disable();
  }   
  
  if (ether.findKeyVal(p, tmp_buffer, TMP_BUFFER_SIZE, "rd")) {
    int rd = atoi(tmp_buffer);
    if (rd>0) {
      svc.raindelay_start(rd);
    } else {
      svc.raindelay_stop();
    }
  }  
  
#define TIME_REBOOT_DELAY  15

  if (ether.findKeyVal(p, tmp_buffer, TMP_BUFFER_SIZE, "rst") && atoi(tmp_buffer) > 0) {
    bfill.emit_p(PSTR("$F<meta http-equiv=\"refresh\" content=\"$D; url=/\">"), htmlOkHeader, TIME_REBOOT_DELAY);
    bfill.emit_p(PSTR("Rebooting, wait for $D seconds..."), TIME_REBOOT_DELAY);
    ether.httpServerReply(bfill.position());   
    svc.reboot();
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
  
  if (old_tz != svc.options[OPTION_TIMEZONE])
    getNtpTime();
    
  return true;
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
  }
  
  if (success == false) {
    bfill.emit_p(PSTR("$F"), htmlUnauthorized);
  }
}

