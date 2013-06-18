// Example code for OpenSprinkler

/* Server functions
   Creative Commons Attribution-ShareAlike 3.0 license
   Feb 2013 @ Rayshobby.net
*/

#include <OpenSprinkler.h>

// External variables defined in main pde file
extern uint8_t ntpclientportL;
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

prog_uchar htmlUnauthorized[] PROGMEM = 
    "HTTP/1.0 401 Unauthorized\r\n"
    "Content-Type: text/html\r\n"
    "\r\n"
    "<h1>401 Unauthorized</h1>"
;

prog_uchar htmlReturnHome[] PROGMEM = 
  "window.location=\"/\";</script>\n"
;


// check and verify password
boolean check_password(char *p)
{
  if (svc.options[OPTION_IGNORE_PASSWORD].value)  return true;
  if (!ether.findKeyVal(p, tmp_buffer, TMP_BUFFER_SIZE, "pw") || !svc.password_verify(tmp_buffer)) {
    return false;
  }
  return true;
}
  
// fill buffer with station names
void bfill_station_names()
{
  byte sid;
  bfill.emit_p(PSTR("snames=["));
  for(sid=0;sid<svc.nstations;sid++) {
    svc.get_station_name(sid, tmp_buffer);
    bfill.emit_p(PSTR("\'$S\',"), tmp_buffer);
  }
  bfill.emit_p(PSTR("\'\'];\n"));
}

// webpage for printing station names
boolean print_webpage_view_stations(char *p)
{
  bfill.emit_p(PSTR("$F<script>var nboards=$D,maxlen=$D,mas=$D,ipas=$D,"), htmlOkHeader,
               svc.nboards, STATION_NAME_SIZE, svc.options[OPTION_MASTER_STATION].value,
               svc.options[OPTION_IGNORE_PASSWORD].value);
  bfill_station_names();
  // fill master operation bits
  bfill.emit_p(PSTR("var masop=["));
  for(byte i=0;i<svc.nboards;i++) {
    bfill.emit_p(PSTR("$D,"), svc.masop_bits[i]);
  }
  bfill.emit_p(PSTR("0];</script>\n<script src=\"$F/viewstations.js\"></script>\n"), htmlExtJavascriptPath);
  return true;
}

// This is part of javascript for printing station names
boolean print_webpage_station_names(char *p) {

  bfill.emit_p(PSTR("$Fvar "), htmlOkHeader);
  bfill_station_names();
  return true;
}

// server function for accepting station name changes
boolean print_webpage_change_stations(char *p)
{
  p+=3;
  
  // check password
  if(check_password(p)==false)  return false;
  
  byte sid,bid;
  char tbuf2[4] = {'s', 0, 0, 0};
  // process station names
  for(sid=0;sid<svc.nstations;sid++) {
    itoa(sid, tbuf2+1, 10);
    if(ether.findKeyVal(p, tmp_buffer, TMP_BUFFER_SIZE, tbuf2)) {
      ether.urlDecode(tmp_buffer);
      svc.set_station_name(sid, tmp_buffer);
    }
  }

  // process station master operation bits
  tbuf2[0]='m';
  for(bid=0;bid<svc.nboards;bid++) {
    itoa(bid, tbuf2+1, 10);
    if(ether.findKeyVal(p, tmp_buffer, TMP_BUFFER_SIZE, tbuf2)) {
      svc.masop_bits[bid] = atoi(tmp_buffer);
    }
  }
  svc.masop_save();
  
  bfill.emit_p(PSTR("$F<script>alert(\"Changes saved.\");$F"), htmlOkHeader, htmlReturnHome);
  return true;
}

// split program data to subsections in order
// to fit each subsection in a single http buffer
#define PROGRAMDATA_SUBSECTION_SIZE  8

// fill program data subsection
void bfill_programdata_sub(byte start_index)
{
  byte pid, bid;
  ProgramStruct prog;    
  for(pid=start_index; pid<start_index+PROGRAMDATA_SUBSECTION_SIZE && pid<pd.nprograms; pid++) {
    pd.read(pid, &prog);
    
    // convert interval remainder (absolute->relative)
    if (prog.days[1] > 1)  pd.drem_to_relative(prog.days);
        
    bfill.emit_p(PSTR("pd[$D]=[$D,$D,$D,$D,$D,$D,$D"), pid, prog.enabled, 
      prog.days[0], prog.days[1], prog.start_time, prog.end_time, prog.interval, prog.duration);
    for (bid=0; bid<svc.nboards; bid++) {
      bfill.emit_p(PSTR(",$D"),prog.stations[bid]);
    }
    bfill.emit_p(PSTR("];"));
  }
}

// Javascript for printing program data subsection page
boolean print_webpage_programdata_subsection(char *p) {
  p+=2;
  ether.urlDecode(p);
  
  byte ssid = ((*p)-'0')*PROGRAMDATA_SUBSECTION_SIZE;
  bfill.emit_p(PSTR("$F"), htmlOkHeader);
  bfill_programdata_sub(ssid);
  return true;
}

// fill buffer with program data
void bfill_programdata()
{
  byte ssid;
  
  bfill.emit_p(PSTR("var nprogs=$D,nboards=$D,ipas=$D,mnp=$D,pd=[];"),
                    pd.nprograms, svc.nboards,svc.options[OPTION_IGNORE_PASSWORD].value, MAX_NUMBER_PROGRAMS);
  bfill_programdata_sub(0);
  bfill.emit_p(PSTR("</script>\n"));
  if (pd.nprograms > PROGRAMDATA_SUBSECTION_SIZE) {
    // create subsection pages
    for (ssid=1; ssid<=((pd.nprograms-1)/PROGRAMDATA_SUBSECTION_SIZE); ssid++) {
      bfill.emit_p(PSTR("<script src=ps$D.js></script>\n"), ssid);
    }
  }
}

// webpage for printing run-once program
boolean print_webpage_view_runonce(char *str) {
  bfill.emit_p(PSTR("$F$F<script>var nboards=$D,mas=$D,ipas=$D;</script>\n"), htmlOkHeader, htmlMobileHeader,
               svc.nboards, svc.options[OPTION_MASTER_STATION].value, svc.options[OPTION_IGNORE_PASSWORD].value);
  bfill.emit_p(PSTR("<script src=\"pn.js\"></script>\n"));
  bfill.emit_p(PSTR("<script src=\"$F/viewro.js\"></script>\n"), htmlExtJavascriptPath);
  
  return true;
}

// server function to accept run-once program
boolean print_webpage_change_runonce(char *p) {
  p+=3;
  ether.urlDecode(p);
  
  // check password
  if(check_password(p)==false)  return false;
  
  // search for the start of v=[
  char *pv;
  boolean found=false;
  for(pv=p;(*pv)!=0 && pv<p+100;pv++) {
    if(strncmp(pv, "t=[", 3)==0) {
      found=true;
      break;
    }
  }
  if(!found)  return false;
  pv+=3;
  
  // reset all stations and prepare to run one-time program
  reset_all_stations();
      
  byte sid;
  uint16_t dur;
  boolean match_found = false;
  for(sid=0;sid<svc.nstations;sid++) {
    dur=parse_listdata(&pv);
    if (dur>0) {
      pd.scheduled_stop_time[sid] = dur;
      pd.scheduled_program_index[sid] = 254;      
      match_found = true;
    }
  }
  if(match_found) {
    schedule_all_stations(now(), svc.options[OPTION_SEQUENTIAL].value);
  }
  bfill.emit_p(PSTR("$F<script>$F"), htmlOkHeader, htmlReturnHome);
  return true;
}


// webpage for printing program summary page
boolean print_webpage_view_program(char *str) {

  bfill.emit_p(PSTR("$F$F<script>"), htmlOkHeader, htmlMobileHeader);
  
  bfill_programdata();
  
  // print station names
  bfill.emit_p(PSTR("<script src=\"pn.js\"></script>\n"));
  
  bfill.emit_p(PSTR("<script src=\"$F/viewprog.js\"></script>\n"), htmlExtJavascriptPath);

  return true;
}

// webpage for printing program modification page 
boolean print_webpage_modify_program(char *p) {
  p+=3;
  ether.urlDecode(p);
  
  if (!ether.findKeyVal(p, tmp_buffer, TMP_BUFFER_SIZE, "pid")) {
    return false;
  }
  int pid=atoi(tmp_buffer);
  if (!(pid>=-1 && pid< pd.nprograms)) return false;
  bfill.emit_p(PSTR("$F$F"), htmlOkHeader, htmlMobileHeader);
  bfill.emit_p(PSTR("<script>var nboards=$D,pid=$D,ipas=$D;"), svc.nboards, pid, svc.options[OPTION_IGNORE_PASSWORD].value);
  if(pid>-1) {
    ProgramStruct prog;
    pd.read(pid, &prog);
    
    // process interval day remainder (absolute->relative)
    if (prog.days[1] > 1)  pd.drem_to_relative(prog.days);
    
    bfill.emit_p(PSTR("var prog=[$D,$D,$D,$D,$D,$D,$D"), prog.enabled,
                 prog.days[0], prog.days[1], prog.start_time, prog.end_time, prog.interval, prog.duration);
    for(byte bid=0;bid<svc.nboards;bid++) {
      bfill.emit_p(PSTR(",$D"), prog.stations[bid]);
    }
    bfill.emit_p(PSTR("];"));
  }
  // print station names
  bfill.emit_p(PSTR("</script>\n<script src=\"pn.js\"></script>\n"));  
  bfill.emit_p(PSTR("<script src=\"$F/modprog.js\"></script>\n"), htmlExtJavascriptPath);
  return true;
}

/*=============================================
  Delete Program
  
  HTTP GET command format:
  /dp?pw=xxx&pid=xxx
  
  pw: password
  pid:program index (-1 will delete all programs)
  =============================================*/
boolean print_webpage_delete_program(char *p) {

  p+=3;
  ether.urlDecode(p);
  
  // check password
  if(check_password(p)==false)  return false;
  
  if (!ether.findKeyVal(p, tmp_buffer, TMP_BUFFER_SIZE, "pid"))
    return false;
    
  int pid=atoi(tmp_buffer);
  if (pid == -1) {
    pd.erase();
  } else if (pid < pd.nprograms) {
    pd.del(pid);
  } else {
    return false;
  }

  bfill.emit_p(PSTR("$F<script>window.location=\"/vp\";</script>\n"), htmlOkHeader);
  return true;
}

/*=============================================
  Plot Program Data
  
  HTTP GET command format:
  /gp?d=xx&m=xx&y=xx
  
  d: day (either a number or string 'today')
  m: month
  y: year
  (if any field is missing, will use the current
   date/month/year)
  =============================================*/
boolean print_webpage_plot_program(char *p) {
  p+=3;
  ether.urlDecode(p);
  
  // yy,mm,dd are simulated date for graphical view
  // devdd is the device day
  int yy,mm,dd,devday,devmin;
  time_t t = now();
  yy = year(t);  mm = month(t);  dd = day(t);
  devday = t/SECS_PER_DAY;  devmin = hour(t)*60+minute(t);
  
  if (ether.findKeyVal(p, tmp_buffer, TMP_BUFFER_SIZE, "d")) {
    dd=atoi(tmp_buffer);
    if (dd==0)  dd=day(t);
  }
  if (ether.findKeyVal(p, tmp_buffer, TMP_BUFFER_SIZE, "m")) {
    mm=atoi(tmp_buffer);
  }
  if (ether.findKeyVal(p, tmp_buffer, TMP_BUFFER_SIZE, "y")) {
    yy=atoi(tmp_buffer);
  }
  
  bfill.emit_p(PSTR("$F<script>var seq=$D,mas=$D,wl=$D,sdt=$D,mton=$D,mtoff=$D,devday=$D,devmin=$D,dd=$D,mm=$D,yy=$D;"),
               htmlOkHeader, svc.options[OPTION_SEQUENTIAL].value, svc.options[OPTION_MASTER_STATION].value, svc.options[OPTION_WATER_LEVEL].value,
               svc.options[OPTION_STATION_DELAY_TIME].value, svc.options[OPTION_MASTER_ON_ADJ].value, svc.options[OPTION_MASTER_OFF_ADJ].value,
               devday, devmin, dd, mm, yy);
  bfill.emit_p(PSTR("var masop=["));
  for(byte i=0;i<svc.nboards;i++) {
    bfill.emit_p(PSTR("$D,"), svc.masop_bits[i]);
  }
  bfill.emit_p(PSTR("0];"));
  bfill_programdata();
  bfill.emit_p(PSTR("<script src=\"pn.js\"></script>\n"));    
  bfill.emit_p(PSTR("<script src=\"$F/plotprog.js\"></script>\n"), htmlExtJavascriptPath);
  return true;
}

// parse one number from a comma separate list
uint16_t parse_listdata(char **p) {
  char* pv;
  int i=0;
  tmp_buffer[i]=0;
  // copy to tmp_buffer until a non-number is encountered
  for(pv=(*p);pv<(*p)+10;pv++) {
    if ((*pv)=='-' || (*pv)=='+' || ((*pv)>='0'&&(*pv)<='9'))
      tmp_buffer[i++] = (*pv);
    else
      break;
  }
  tmp_buffer[i]=0;
  *p = pv+1;
  return atoi(tmp_buffer);
}

// server function to accept program changes
boolean print_webpage_change_program(char *p) {

  p+=3;
  ether.urlDecode(p);
  
  // check password
  if(check_password(p)==false)  return false;
    
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
  prog.enabled = parse_listdata(&pv);
  prog.days[0]= parse_listdata(&pv);
  prog.days[1]= parse_listdata(&pv);
  prog.start_time = parse_listdata(&pv);
  prog.end_time = parse_listdata(&pv);
  prog.interval = parse_listdata(&pv);
  prog.duration = parse_listdata(&pv);

  byte bid;
  for(bid=0;bid<svc.nboards;bid++) {
    prog.stations[bid] = parse_listdata(&pv);
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
  bfill.emit_p(PSTR("window.location=\"/vp\";</script>\n"));
  return true;
}

// print home page
boolean print_webpage_home(char *p)
{
  byte bid, sid;
  unsigned long curr_time = now();
  bfill.emit_p(PSTR("$F$F"), htmlOkHeader, htmlMobileHeader);
  bfill.emit_p(PSTR("<script>var ver=$D,devt=$L;\n"),
               SVC_FW_VERSION, curr_time);
  bfill.emit_p(PSTR("var nbrd=$D,tz=$D,sbits=["), (int)svc.nboards, (int)svc.options[OPTION_TIMEZONE].value);
  for(bid=0;bid<svc.nboards;bid++)
    bfill.emit_p(PSTR("$D,"), svc.station_bits[bid]);
  bfill.emit_p(PSTR("0];var ps=["));
  for(sid=0;sid<svc.nstations;sid++) {
    unsigned long rem = 0;
    if (pd.scheduled_program_index[sid] > 0) {
      rem = (curr_time >= pd.scheduled_start_time[sid]) ? (pd.scheduled_stop_time[sid]-curr_time) : (pd.scheduled_stop_time[sid]-pd.scheduled_start_time[sid]);
      if(pd.scheduled_stop_time[sid]==ULONG_MAX-1)  rem=0;
    }
    bfill.emit_p(PSTR("[$D,$L],"), pd.scheduled_program_index[sid], rem);
  } 
  //svc.location_get(tmp_buffer);
  svc.eeprom_string_get(ADDR_EEPROM_LOCATION, tmp_buffer);
  bfill.emit_p(PSTR("[0,0]];\nvar en=$D,rd=$D,rs=$D,mm=$D,rdst=$L,mas=$D,urs=$D,wl=$D,ipas=$D,loc=\"$S\";"),
    svc.status.enabled,
    svc.status.rain_delayed,
    svc.status.rain_sensed,
    svc.status.manual_mode,
    svc.raindelay_stop_time,
    svc.options[OPTION_MASTER_STATION].value,
    svc.options[OPTION_USE_RAINSENSOR].value,
    svc.options[OPTION_WATER_LEVEL].value,
    svc.options[OPTION_IGNORE_PASSWORD].value,
    tmp_buffer
  );
  bfill.emit_p(PSTR("\nvar lrun=[$D,$D,$D,$L]</script>\n"),
               pd.lastrun.station, pd.lastrun.program,pd.lastrun.duration,pd.lastrun.endtime);
  // print station names
  bfill.emit_p(PSTR("<script src=\"pn.js\"></script>\n"));
  // include remote javascript
  bfill.emit_p(PSTR("<script src=\"$F/home.js\"></script>\n"), htmlExtJavascriptPath);
  return true;
}

// webpage for printing options page
boolean print_webpage_view_options(char *p)
{
  bfill.emit_p(PSTR("$F$F"), htmlOkHeader, htmlMobileHeader);
  bfill.emit_p(PSTR("<script>var opts=["));

  byte oid;
  byte noptions = 0;
  // print web editable options
  for (oid=0; oid<NUM_OPTIONS; oid++) {
    if ((svc.options[oid].flag&OPFLAG_WEB_EDIT)==0) continue;  
    bfill.emit_p(PSTR("\"$F\",$D,$D,$D,"),
                 svc.options[oid].str,
                 (svc.options[oid].max==1)?1:0,
                 (oid==OPTION_MASTER_OFF_ADJ)?(int)svc.options[oid].value-60:(int)svc.options[oid].value,
                 oid);
    noptions ++;
  }
  //svc.location_get(tmp_buffer);
  svc.eeprom_string_get(ADDR_EEPROM_LOCATION, tmp_buffer);
  bfill.emit_p(PSTR("0];var nopts=$D,loc=\"$S\";"), noptions, tmp_buffer);
  bfill.emit_p(PSTR("</script>\n"));
  // include remote javascript
  bfill.emit_p(PSTR("<script src=\"$F/viewoptions.js\"></script>\n"), htmlExtJavascriptPath);
  return true;
}

/*=============================================
  Change Controller Values
  
  HTTP GET command format:
  /cv?pw=xxx&rsn=x&rbt=x&en=x&mm=x&rd=x
  
  pw:  password
  rsn: reset all stations (0 or 1)
  rbt: reboot controller (0 or 1)
  en:  enable (0 or 1)
  mm:  manual mode (0 or 1)
  rd:  rain delay hours (0 turns off rain delay)
  =============================================*/
boolean print_webpage_change_values(char *p)
{
  p+=3;
  // if no password is attached, or password is incorrect
  if(check_password(p)==false)  return false;

  if (ether.findKeyVal(p, tmp_buffer, TMP_BUFFER_SIZE, "rsn")) {
    reset_all_stations();
  }
#define TIME_REBOOT_DELAY  10

  if (ether.findKeyVal(p, tmp_buffer, TMP_BUFFER_SIZE, "rbt") && atoi(tmp_buffer) > 0) {
    bfill.emit_p(PSTR("$F<meta http-equiv=\"refresh\" content=\"$D; url=/\">"), htmlOkHeader, TIME_REBOOT_DELAY);
    bfill.emit_p(PSTR("Rebooting..."));
    ether.httpServerReply(bfill.position());   
    svc.reboot();
  } 
  
  if (ether.findKeyVal(p, tmp_buffer, TMP_BUFFER_SIZE, "en")) {
    if (tmp_buffer[0]=='1' && !svc.status.enabled)  svc.enable();
    else if (tmp_buffer[0]=='0' &&  svc.status.enabled)  svc.disable();
  }   
  
  if (ether.findKeyVal(p, tmp_buffer, TMP_BUFFER_SIZE, "mm")) {
    if (tmp_buffer[0]=='1' && !svc.status.manual_mode) {
      reset_all_stations();
      svc.status.manual_mode = 1;
      
    } else if (tmp_buffer[0]=='0' &&  svc.status.manual_mode) {
      reset_all_stations();
      svc.status.manual_mode = 0;
    }
  }
  if (ether.findKeyVal(p, tmp_buffer, TMP_BUFFER_SIZE, "rd")) {
    int rd = atoi(tmp_buffer);
    if (rd>0) {
      svc.raindelay_start(rd);
    } else if (rd==0){
      svc.raindelay_stop();
    } else  return false;
  }  
 
  bfill.emit_p(PSTR("$F<script>$F"), htmlOkHeader, htmlReturnHome);
  return true;
}

// server function to accept option changes
boolean print_webpage_change_options(char *p)
{
  p+=3;

  // if no password is attached, or password is incorrect
  if(check_password(p)==false)  return false;

  // !!! p and bfill share the same buffer, so don't write
  // to bfill before you are done analyzing the buffer !!!
  
  // process option values
  byte err = 0;
  for (byte oid=0; oid<NUM_OPTIONS; oid++) {
    if ((svc.options[oid].flag&OPFLAG_WEB_EDIT)==0) continue;
    if (svc.options[oid].max==1)  svc.options[oid].value = 0;  // set a bool variable to 0 first
    char tbuf2[5] = {'o', 0, 0, 0, 0};
    itoa(oid, tbuf2+1, 10);
    if (ether.findKeyVal(p, tmp_buffer, TMP_BUFFER_SIZE, tbuf2)) {
      if (svc.options[oid].max==1) {
        svc.options[oid].value = 1;  // if the bool variable is detected, set to 1
        continue;
      }
      int v = atoi(tmp_buffer);
      if (oid==OPTION_MASTER_OFF_ADJ) {v+=60;} // master off time
      if (v>=0 && v<=svc.options[oid].max) {
        svc.options[oid].value = v;
      } else {
        err = 1;
      }
    }
  }
  
  if (ether.findKeyVal(p, tmp_buffer, TMP_BUFFER_SIZE, "loc")) {
    ether.urlDecode(tmp_buffer);
    //svc.location_set(tmp_buffer);    
    svc.eeprom_string_set(ADDR_EEPROM_LOCATION, tmp_buffer);
  }
  
  if (err) {
    bfill.emit_p(PSTR("$F<script>alert(\"Values out of bound!\");window.location=\"/vo\";</script>\n"), htmlOkHeader);
    return true;
  } 

  svc.options_save();
  
  if (ether.findKeyVal(p, tmp_buffer, TMP_BUFFER_SIZE, "npw")) {
    char tbuf2[TMP_BUFFER_SIZE];
    if (ether.findKeyVal(p, tbuf2, TMP_BUFFER_SIZE, "cpw") && strncmp(tmp_buffer, tbuf2, 16) == 0) {
      //svc.password_set(tmp_buffer);
      svc.eeprom_string_set(ADDR_EEPROM_PASSWORD, tmp_buffer);
      bfill.emit_p(PSTR("$F<script>alert(\"New password set.\");$F"), htmlOkHeader, htmlReturnHome);
      return true;
    } else {
      bfill.emit_p(PSTR("$F<script>alert(\"New passwords must match!\");window.location=\"/vo\";</script>\n"), htmlOkHeader);
      return true;
    }
  }

  bfill.emit_p(PSTR("$F<script>alert(\"Options values saved.\");$F"), htmlOkHeader, htmlReturnHome);  
  return true;
}

/*=================================================
  Get/Set Station Bits:
  
  HTTP GET command format:
  /snx     -> get station bit (e.g. /sn1, /sn2 etc.)
  /sn0     -> get all bits
  
  The following will only work if controller is
  switched to manual mode:
  
  /snx=0   -> turn off station 
  /snx=1   -> turn on station
  /snx=1&t=xx -> turn on with timer (in seconds)
  =================================================*/
boolean print_webpage_station_bits(char *p) {

  p+=2;
  int sid;
  byte i, sidmin, sidmax;  

  // parse station name
  i=0;
  while((*p)>='0'&&(*p)<='9'&&i<4) {
    tmp_buffer[i++] = (*p++);
  }
  tmp_buffer[i]=0;
  sid = atoi(tmp_buffer);
  if (!(sid>=0&&sid<=svc.nstations)) return false;
  
  // parse get/set command
  if ((*p)=='=') {
    if (sid==0) return false;
    sid--;
    // this is a set command
    // can only do it when in manual mode
    if (!svc.status.manual_mode) {
      //bfill.emit_p(PSTR("$F<script>alert(\"Station bits can only be set in manual mode.\")</script>\n"), htmlOkHeader);
      return false;
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
    bfill.emit_p(PSTR("$F<meta http-equiv=\"refresh\" content=\"1; url=/\">"), htmlOkHeader);      
    return true;
  } else {
    // this is a get command
    bfill.emit_p(PSTR("$F"), htmlOkHeader);
    if (sid==0) { // print all station bits
      sidmin=0;sidmax=(svc.nstations);
    } else {  // print one station bit
      sidmin=(sid-1);
      sidmax=sid;
    }
    for (sid=sidmin;sid<sidmax;sid++) {
      if (svc.status.enabled && (!svc.status.rain_delayed) && !(svc.options[OPTION_USE_RAINSENSOR].value && svc.status.rain_sensed)) {
        bfill.emit_p(PSTR("$D"), (svc.station_bits[(sid>>3)]>>(sid%8))&1);
      } else bfill.emit_p(PSTR("$D"), 0);
    }
    return true;      
  }

  return false;
}

/*boolean print_webpage_favicon()
{
  bfill.emit_p(PSTR("$F"), htmlFavicon);
  return true;
}*/

// =============
// NTP Functions
// =============

unsigned long ntp_wait_response()
{
  uint32_t time;
  unsigned long start = millis();
  do {
    ether.packetLoop(ether.packetReceive());
    if (ether.ntpProcessAnswer(&time, ntpclientportL))
    {
      if ((time & 0x80000000UL) ==0){
        time+=2085978496;
      }else{
        time-=2208988800UL;
      }
      return time + (int32_t)3600/4*(int32_t)(svc.options[OPTION_TIMEZONE].value-48);
    }
  } while(millis() - start < 1000); // wait at most 1 seconds for ntp result
  return 0;
}
unsigned long getNtpTime()
{
  unsigned long ans;
  byte tick = 0;
  do
  {
    ether.ntpRequest(ntpip, ++ntpclientportL);
    delay(250);
    ans = ntp_wait_response();
    tick ++;
  } 
  while( ans == 0 && tick < 5 );  
  return ans;
}

struct URLStruct{
  PGM_P PROGMEM url;
  boolean (*handler)(char*);
};

// Server function urls
// !!!Important!!!: to save space, each url must be two characters long
prog_char _url_cv [] PROGMEM = "cv";
prog_char _url_vp [] PROGMEM = "vp";
prog_char _url_mp [] PROGMEM = "mp";
prog_char _url_dp [] PROGMEM = "dp";
prog_char _url_cp [] PROGMEM = "cp";
prog_char _url_gp [] PROGMEM = "gp";
prog_char _url_vo [] PROGMEM = "vo";
prog_char _url_co [] PROGMEM = "co";
prog_char _url_sn [] PROGMEM = "sn";
prog_char _url_ps [] PROGMEM = "ps";
prog_char _url_vs [] PROGMEM = "vs";
prog_char _url_cs [] PROGMEM = "cs";
prog_char _url_vr [] PROGMEM = "vr";
prog_char _url_cr [] PROGMEM = "cr";
prog_char _url_pn [] PROGMEM = "pn";

// Server function handlers
URLStruct urls[] = {
  {_url_cv,print_webpage_change_values},
  {_url_vp,print_webpage_view_program},
  {_url_mp,print_webpage_modify_program},
  {_url_dp,print_webpage_delete_program},
  {_url_cp,print_webpage_change_program},
  {_url_gp,print_webpage_plot_program},
  {_url_vo,print_webpage_view_options},
  {_url_co,print_webpage_change_options},
  {_url_sn,print_webpage_station_bits},
  {_url_ps,print_webpage_programdata_subsection},
  {_url_vs,print_webpage_view_stations},
  {_url_cs,print_webpage_change_stations},
	{_url_vr,print_webpage_view_runonce},
	{_url_cr,print_webpage_change_runonce},
  {_url_pn,print_webpage_station_names}
};

// analyze the current url
void analyze_get_url(char *p)
{
  // the tcp packet usually starts with 'GET /' -> 5 chars    
  char *str = p+5;
 
  if(str[0]==' ') {
    print_webpage_home(str);  // home page handler
  } else {
    for(byte i=0;i<15;i++) {
      if(pgm_read_byte(urls[i].url)==str[0]
       &&pgm_read_byte(urls[i].url+1)==str[1]) {
        if ((urls[i].handler)(str) == false) {
          bfill.emit_p(PSTR("$F"), htmlUnauthorized);
        }
        break;
      }
    }
  }
  delay(50); // add a bit of delay here
}

