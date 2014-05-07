// Example code for OpenSprinkler Generation 2

/* Server functions
   Creative Commons Attribution-ShareAlike 3.0 license
   Dec 2013 @ Rayshobby.net
*/

#include <OpenSprinklerGen2.h>

// External variables defined in main pde file
extern uint8_t ntpclientportL;
extern BufferFiller bfill;
extern char tmp_buffer[];
extern OpenSprinkler svc;
extern ProgramData pd;
extern byte mymac[];
extern int myport;

prog_uchar htmlOkHeader[] PROGMEM = 
    "HTTP/1.0 200 OK\r\n"
    "Content-Type: text/html\r\n"
    "Pragma: no-cache\r\n"
    "Access-Control-Allow-Origin: *\r\n"
    "\r\n"
;

prog_uchar htmlJSONHeader[] PROGMEM =
    "HTTP/1.0 200 OK\r\n"
    "Content-Type: application/json\r\n"
    "Connnection: close\r\n"
    "Access-Control-Allow-Origin: *\r\n"
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

// printing station names in json
boolean print_webpage_json_stations(char *p)
{
  bfill.emit_p(PSTR("$F"), htmlJSONHeader);
  bfill.emit_p(PSTR("{\"snames\":["));
  byte sid;
  for(sid=0;sid<svc.nstations;sid++) {
    svc.get_station_name(sid, tmp_buffer);
    bfill.emit_p(PSTR("\"$S\""), tmp_buffer);
    if(sid!=svc.nstations-1)
      bfill.emit_p(PSTR(","));
  }
  bfill.emit_p(PSTR("],\"masop\":["));
  for(byte i=0;i<svc.nboards;i++) {
    bfill.emit_p(PSTR("$D"), svc.masop_bits[i]);
    if(i!=svc.nboards-1)
      bfill.emit_p(PSTR(","));
  }
  bfill.emit_p(PSTR("],\"ignore_rain\":["));
  for(byte i=0;i<svc.nboards;i++) {
    bfill.emit_p(PSTR("$D"), svc.ignrain_bits[i]);
    if(i!=svc.nboards-1)
      bfill.emit_p(PSTR(","));
  }  
  
  bfill.emit_p(PSTR("],\"maxlen\":$D}"), STATION_NAME_SIZE);
   
  return true;
}

// webpage for printing station names
boolean print_webpage_view_stations(char *p)
{
  bfill.emit_p(PSTR("$F<script>"), htmlOkHeader);

  // send station data packets
  sendpacket_stationdata();
  
  // send the server variable and javascript packets
  bfill=ether.tcpOffset();
  bfill.emit_p(PSTR("var nboards=$D,maxlen=$D,mas=$D,ipas=$D;"), 
               svc.nboards, STATION_NAME_SIZE, svc.options[OPTION_MASTER_STATION].value,
               svc.options[OPTION_IGNORE_PASSWORD].value);
  // fill master operation bits
  bfill.emit_p(PSTR("var masop=["));
  for(byte i=0;i<svc.nboards;i++) {
    bfill.emit_p(PSTR("$D,"), svc.masop_bits[i]);
  }
  bfill.emit_p(PSTR("0];var ir=["));
  for(byte i=0;i<svc.nboards;i++) {
    bfill.emit_p(PSTR("$D,"), svc.ignrain_bits[i]);
  }  
  bfill.emit_p(PSTR("0];</script>\n<script src=\"$E/viewsn.js\"></script>\n"), ADDR_EEPROM_SCRIPTURL);
  //ether.httpServerReply_with_flags(bfill.position(), TCP_FLAGS_ACK_V|TCP_FLAGS_FIN_V);
  
  return true;
}

void sendpacket_stationdata()
{
  byte sid;
  bfill.emit_p(PSTR("var snames=["));
  for(sid=0;sid<svc.nstations;sid++) {
    svc.get_station_name(sid, tmp_buffer);
    bfill.emit_p(PSTR("\'$S\',"), tmp_buffer);
  }
  bfill.emit_p(PSTR("\'\'];"));
  ether.httpServerReply_with_flags(bfill.position(), TCP_FLAGS_ACK_V);
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
    
  // process ignore rain bits
  tbuf2[0]='i';
  for(bid=0;bid<svc.nboards;bid++) {
    itoa(bid, tbuf2+1, 10);
    if(ether.findKeyVal(p, tmp_buffer, TMP_BUFFER_SIZE, tbuf2)) {
      svc.ignrain_bits[bid] = atoi(tmp_buffer);
    }
  }  
  svc.ignrain_save();
  
  bfill.emit_p(PSTR("$F<script>alert(\"Changes saved.\");$F"), htmlOkHeader, htmlReturnHome);
  return true;
}

// fill buffer with program data
void bfill_programdata()
{
  bfill.emit_p(PSTR("var nprogs=$D,nboards=$D,ipas=$D,mnp=$D,pd=[];"), pd.nprograms, 
                    svc.nboards,svc.options[OPTION_IGNORE_PASSWORD].value, MAX_NUMBER_PROGRAMS);
  //bfill_programdata_sub(0);
  byte pid, bid;
  ProgramStruct prog;
  for(pid=0;pid<pd.nprograms;pid++) {
    pd.read(pid, &prog);
    // convert interval remainder (absolute->relative)
    if (prog.days[1] > 1)  pd.drem_to_relative(prog.days);
    
    bfill.emit_p(PSTR("pd[$D]=[$D,$D,$D,$D,$D,$D,$D"), pid, prog.enabled, 
      prog.days[0], prog.days[1], prog.start_time, prog.end_time, prog.interval, prog.duration);
    for (bid=0; bid<svc.nboards; bid++) {
      bfill.emit_p(PSTR(",$D"),prog.stations[bid]);
    }
    bfill.emit_p(PSTR("];"));
    if (pid%4==3) { // push out a packet every 4 programs
      ether.httpServerReply_with_flags(bfill.position(), TCP_FLAGS_ACK_V);
      bfill=ether.tcpOffset();
    } 
  }
  bfill.emit_p(PSTR("</script>\n"));
}

// print page to set javascript url
boolean print_webpage_view_scripturl(char *p) {
  bfill.emit_p(PSTR("$F"), htmlOkHeader);
  bfill.emit_p(PSTR("<hr /><form name=of action=cu method=get><p><b>Javascript URL:</b> <input type=text size=32 maxlength=127 value=\"$E\" name=jsp></p><p>Set URL where Javascripts are stored. Default is $S<br />If local on uSD card, use . (i.e. dot)</p><p><b>Password:</b><input type=password size=10 name=pw><input type=submit></p><hr /></form>"), ADDR_EEPROM_SCRIPTURL, DEFAULT_JAVASCRIPT_URL);  
  return true;
}

// print program data in json
boolean print_webpage_json_programs(char *p) {
  bfill.emit_p(PSTR("$F"), htmlJSONHeader);
  bfill.emit_p(PSTR("{\"nprogs\":$D,\"nboards\":$D,\"mnp\":$D,\"pd\":["),
               pd.nprograms, svc.nboards, MAX_NUMBER_PROGRAMS);
  byte pid, bid;
  ProgramStruct prog;
  for(pid=0;pid<pd.nprograms;pid++) {
    pd.read(pid, &prog);
    // convert interval remainder (absolute->relative)
    if (prog.days[1] > 1)  pd.drem_to_relative(prog.days);
    
    bfill.emit_p(PSTR("[$D,$D,$D,$D,$D,$D,$D"), prog.enabled, 
      prog.days[0], prog.days[1], prog.start_time, prog.end_time, prog.interval, prog.duration);
    for (bid=0; bid<svc.nboards; bid++) {
      bfill.emit_p(PSTR(",$D"),prog.stations[bid]);
    }
    if(pid!=pd.nprograms-1)
      bfill.emit_p(PSTR("],"));
    else
      bfill.emit_p(PSTR("]"));
    if (pid%4==3) { // push out a packet every 4 programs
      ether.httpServerReply_with_flags(bfill.position(), TCP_FLAGS_ACK_V);
      bfill=ether.tcpOffset();
    } 
  }
  bfill.emit_p(PSTR("]}"));   
  return true; 
}

// webpage for printing run-once program
boolean print_webpage_view_runonce(char *str) {
  bfill.emit_p(PSTR("$F$F<script>"), htmlOkHeader, htmlMobileHeader);
  // send station data packet
  sendpacket_stationdata();  
  bfill=ether.tcpOffset();
  
  // send server variables with javascript
  bfill.emit_p(PSTR("var nboards=$D,mas=$D,ipas=$D,dur=["), 
               svc.nboards, svc.options[OPTION_MASTER_STATION].value, svc.options[OPTION_IGNORE_PASSWORD].value);

  byte sid;
  uint16_t dur;
  unsigned char *addr = (unsigned char*)ADDR_EEPROM_RUNONCE;
  for(sid=0;sid<svc.nstations;sid++, addr+=2) {
    dur=eeprom_read_byte(addr);
    dur=(dur<<8)+eeprom_read_byte(addr+1);
    bfill.emit_p(PSTR("$D,"),dur);
  }
  bfill.emit_p(PSTR("0];</script>\n"));
  bfill.emit_p(PSTR("<script src=\"$E/viewro.js\"></script>\n"), ADDR_EEPROM_SCRIPTURL);
  //ether.httpServerReply_with_flags(bfill.position(), TCP_FLAGS_ACK_V|TCP_FLAGS_FIN_V);
    
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
  unsigned char *addr = (unsigned char*)ADDR_EEPROM_RUNONCE;
  boolean match_found = false;
  for(sid=0;sid<svc.nstations;sid++, addr+=2) {
    dur=parse_listdata(&pv);
    eeprom_write_byte(addr, (dur>>8));
    eeprom_write_byte(addr+1, (dur&0xff));
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

  // send station data packet
  sendpacket_stationdata();  
  bfill=ether.tcpOffset();
  
  // send program data
  bfill_programdata();
  bfill.emit_p(PSTR("<script src=\"$E/viewprog.js\"></script>\n"), ADDR_EEPROM_SCRIPTURL);

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
  
  bfill.emit_p(PSTR("$F$F<script>"), htmlOkHeader, htmlMobileHeader);
  
  // send station data packet
  sendpacket_stationdata();  
  bfill=ether.tcpOffset();
    
  // send service variables and javascript
  bfill.emit_p(PSTR("var nboards=$D,pid=$D,ipas=$D;"), svc.nboards, pid, svc.options[OPTION_IGNORE_PASSWORD].value);
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
  bfill.emit_p(PSTR("</script>\n<script src=\"$E/modprog.js\"></script>\n"), ADDR_EEPROM_SCRIPTURL);
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

  bfill.emit_p(PSTR("$F<script>"), htmlOkHeader);
  // send station data packet
  sendpacket_stationdata();  
  bfill=ether.tcpOffset();
 
  // send server variables and javascript
  bfill.emit_p(PSTR("var seq=$D,mas=$D,wl=$D,sdt=$D,mton=$D,mtoff=$D,devday=$D,devmin=$D,dd=$D,mm=$D,yy=$D;"),
               svc.options[OPTION_SEQUENTIAL].value, svc.options[OPTION_MASTER_STATION].value, svc.options[OPTION_WATER_PERCENTAGE].value,
               svc.options[OPTION_STATION_DELAY_TIME].value, svc.options[OPTION_MASTER_ON_ADJ].value, svc.options[OPTION_MASTER_OFF_ADJ].value,
               devday, devmin, dd, mm, yy);
  bfill.emit_p(PSTR("var masop=["));
  for(byte i=0;i<svc.nboards;i++) {
    bfill.emit_p(PSTR("$D,"), svc.masop_bits[i]);
  }
  bfill.emit_p(PSTR("0];"));
  bfill_programdata();

  bfill.emit_p(PSTR("<script src=\"$E/plotprog.js\"></script>\n"), ADDR_EEPROM_SCRIPTURL);
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

// print controller variables in json
boolean print_webpage_json_controller(char *p)
{
  byte bid, sid;
  unsigned long curr_time = now();  
  bfill.emit_p(PSTR("$F"), htmlJSONHeader);
  //svc.eeprom_string_get(ADDR_EEPROM_LOCATION, tmp_buffer);
  bfill.emit_p(PSTR("{\"devt\":$L,\"nbrd\":$D,\"en\":$D,\"rd\":$D,\"rs\":$D,\"mm\":$D,\"rdst\":$L,\"loc\":\"$E\",\"sbits\":["),
              curr_time,
              svc.nboards,
              svc.status.enabled,
              svc.status.rain_delayed,
              svc.status.rain_sensed,
              svc.status.manual_mode,
              svc.raindelay_stop_time,
              ADDR_EEPROM_LOCATION);
  // print sbits
  for(bid=0;bid<svc.nboards;bid++)
    bfill.emit_p(PSTR("$D,"), svc.station_bits[bid]);  
  bfill.emit_p(PSTR("0],\"ps\":["));
  // print ps
  for(sid=0;sid<svc.nstations;sid++) {
    unsigned long rem = 0;
    if (pd.scheduled_program_index[sid] > 0) {
      rem = (curr_time >= pd.scheduled_start_time[sid]) ? (pd.scheduled_stop_time[sid]-curr_time) : (pd.scheduled_stop_time[sid]-pd.scheduled_start_time[sid]);
      if(pd.scheduled_stop_time[sid]==ULONG_MAX-1)  rem=0;
    }
    bfill.emit_p(PSTR("[$D,$L],"), pd.scheduled_program_index[sid], rem);
  }
  bfill.emit_p(PSTR("[0,0]],\"lrun\":[$D,$D,$D,$L]}"),pd.lastrun.station,
    pd.lastrun.program,pd.lastrun.duration,pd.lastrun.endtime);
  
    
  return true;
}

boolean print_webpage_test(char *p)
{
  bfill.emit_p(PSTR("$F$F"), htmlOkHeader, htmlMobileHeader);
  bfill.emit_p(PSTR("<script src=\"https://jqueryjs.googlecode.com/files/jquery-1.3.2.min.js\"></script>\r\n"));
  bfill.emit_p(PSTR("<script src=\"http://192.168.1.103/mchp.js\"></script>\r\n"));
  bfill.emit_p(PSTR("<script>\r\n"));
  bfill.emit_p(PSTR("$$(init);"));
  bfill.emit_p(PSTR("function init(){func(\"{\\\"devt\\\":0}\");}"));
  bfill.emit_p(PSTR("function func(data){var jd=JSON.parse(data);$$(\"#output\").text(datestr(jd[\"devt\"]*1000));}"));
  bfill.emit_p(PSTR("setTimeout(\"newAJAXCommand('jc', func, true)\",1000);"));
  bfill.emit_p(PSTR("</script>"));
  bfill.emit_p(PSTR("<div id=\"output\">dummy</div>"));
  return true;
}

// print home page
boolean print_webpage_home(char *p)
{
  byte bid, sid;
  unsigned long curr_time = now();

  bfill.emit_p(PSTR("$F$F<script>"), htmlOkHeader, htmlMobileHeader);
  // send station data packet
  sendpacket_stationdata();
  bfill=ether.tcpOffset();

  // send server variables and javascript packets
  bfill.emit_p(PSTR("var ver=$D,devt=$L;\n"),
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
  //svc.eeprom_string_get(ADDR_EEPROM_LOCATION, tmp_buffer);
  bfill.emit_p(PSTR("[0,0]];\nvar en=$D,rd=$D,rs=$D,mm=$D,rdst=$L,mas=$D,urs=$D,wl=$D,ipas=$D,loc=\"$E\";"),
    svc.status.enabled,
    svc.status.rain_delayed,
    svc.status.rain_sensed,
    svc.status.manual_mode,
    svc.raindelay_stop_time,
    svc.options[OPTION_MASTER_STATION].value,
    svc.options[OPTION_USE_RAINSENSOR].value,
    svc.options[OPTION_WATER_PERCENTAGE].value,
    svc.options[OPTION_IGNORE_PASSWORD].value,
    ADDR_EEPROM_LOCATION
  );
  bfill.emit_p(PSTR("\nvar lrun=[$D,$D,$D,$L]</script>\n"),
               pd.lastrun.station, pd.lastrun.program,pd.lastrun.duration,pd.lastrun.endtime);

  bfill.emit_p(PSTR("<script src=\"$E/home.js\"></script>\n"), ADDR_EEPROM_SCRIPTURL);
  //ether.httpServerReply_with_flags(bfill.position(), TCP_FLAGS_ACK_V|TCP_FLAGS_FIN_V);
  return true;
}

// printing options in json
boolean print_webpage_json_options(char *p)
{
  bfill.emit_p(PSTR("$F"), htmlJSONHeader);
  bfill.emit_p(PSTR("{"));
  byte oid;
  for(oid=0;oid<NUM_OPTIONS;oid++) {
    bfill.emit_p(PSTR("\"$F\":$D"),
                 svc.options[oid].json_str,
                 (oid==OPTION_MASTER_OFF_ADJ)?(int)svc.options[oid].value-60:(int)svc.options[oid].value
                );
    if(oid!=NUM_OPTIONS-1)
      bfill.emit_p(PSTR(","));
  }
  bfill.emit_p(PSTR("}"));
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
                 svc.options[oid].json_str,
                 svc.options[oid].max,
                 (oid==OPTION_MASTER_OFF_ADJ)?(int)svc.options[oid].value-60:(int)svc.options[oid].value,
                 oid);
    noptions ++;
  }
  //svc.location_get(tmp_buffer);
  //svc.eeprom_string_get(ADDR_EEPROM_LOCATION, tmp_buffer);
  bfill.emit_p(PSTR("0];var nopts=$D,loc=\"$E\",devt=$L,tz=$D;"), noptions, ADDR_EEPROM_LOCATION,
               now(),(int)svc.options[OPTION_TIMEZONE].value);
  bfill.emit_p(PSTR("</script>\n"));
  // include remote javascript
  bfill.emit_p(PSTR("<script src=\"$E/viewop.js\"></script>\n"), ADDR_EEPROM_SCRIPTURL);
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
#define TIME_REBOOT_DELAY  20

  if (ether.findKeyVal(p, tmp_buffer, TMP_BUFFER_SIZE, "rbt") && atoi(tmp_buffer) > 0) {
    bfill.emit_p(PSTR("$F<meta http-equiv=\"refresh\" content=\"$D; url=/\">"), htmlOkHeader, TIME_REBOOT_DELAY);
    bfill.emit_p(PSTR("Rebooting..."));
    ether.httpServerReply_with_flags(bfill.position(), TCP_FLAGS_ACK_V|TCP_FLAGS_FIN_V);
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
      svc.constatus_save();
      
    } else if (tmp_buffer[0]=='0' &&  svc.status.manual_mode) {
      reset_all_stations();
      svc.status.manual_mode = 0;
      svc.constatus_save();
    }
  }
  if (ether.findKeyVal(p, tmp_buffer, TMP_BUFFER_SIZE, "rd")) {
    int rd = atoi(tmp_buffer);
    if (rd>0) {
      svc.raindelay_stop_time = now() + (unsigned long) rd * 3600;    
      svc.raindelay_start();
    } else if (rd==0){
      svc.raindelay_stop();
    } else  return false;
  }  
 
  bfill.emit_p(PSTR("$F<script>$F"), htmlOkHeader, htmlReturnHome);
  return true;
}

// server function to accept script url changes
boolean print_webpage_change_scripturl(char *p)
{
  p+=3;

  // if no password is attached, or password is incorrect
  if(check_password(p)==false)  return false;
  char temp[128];
  if (ether.findKeyVal(p, temp, 128, "jsp")) {
    ether.urlDecode(temp);
    svc.eeprom_string_set(ADDR_EEPROM_SCRIPTURL, temp);
  }
  bfill.emit_p(PSTR("$F<script>alert(\"Script url saved.\");$F"), htmlOkHeader, htmlReturnHome);  
  return true;
}  
    

// server function to accept option changes
boolean print_webpage_change_options(char *p)
{
  p+=3;

  // if no password is attached, or password is incorrect
  if(check_password(p)==false)  return false;

  // temporarily save some old options values
  byte old_tz =  svc.options[OPTION_TIMEZONE].value;
  byte old_ntp = svc.options[OPTION_USE_NTP].value;
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
  if (ether.findKeyVal(p, tmp_buffer, TMP_BUFFER_SIZE, "ttt")) {
    unsigned long t;
    ether.urlDecode(tmp_buffer);
    t = atol(tmp_buffer);
    setTime(t);  
    if (svc.status.has_rtc) RTC.set(t); // if rtc exists, update rtc
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
  if(svc.options[OPTION_TIMEZONE].value != old_tz ||
     (!old_ntp && svc.options[OPTION_USE_NTP].value)) {
    last_sync_time = 0;
  }
  return true;
}

boolean print_webpage_json_status(char *p)
{
  bfill.emit_p(PSTR("$F"), htmlJSONHeader);
  bfill.emit_p(PSTR("{\"sn\":["));
  byte sid;

  for (sid=0;sid<svc.nstations;sid++) {
    bfill.emit_p(PSTR("$D"), (svc.station_bits[(sid>>3)]>>(sid%8))&1);
    if(sid!=svc.nstations-1) bfill.emit_p(PSTR(","));
  }
  bfill.emit_p(PSTR("],\"nstations\":$D}"), svc.nstations);
   
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
      bfill.emit_p(PSTR("$D"), (svc.station_bits[(sid>>3)]>>(sid%8))&1);
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
  byte ntpip[4] = {
    svc.options[OPTION_NTP_IP1].value, 
    svc.options[OPTION_NTP_IP2].value, 
    svc.options[OPTION_NTP_IP3].value,
    svc.options[OPTION_NTP_IP4].value};
  unsigned long ans;
  byte tick = 0;
  do
  {
    ether.ntpRequest(ntpip, ++ntpclientportL);
    delay(250);
    ans = ntp_wait_response();
    tick ++;
  } 
  while( ans == 0 && tick < 20 );  
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
prog_char _url_jo [] PROGMEM = "jo";
prog_char _url_jn [] PROGMEM = "jn";
prog_char _url_jp [] PROGMEM = "jp";
prog_char _url_jc [] PROGMEM = "jc";
prog_char _url_js [] PROGMEM = "js";
prog_char _url_su [] PROGMEM = "su";
prog_char _url_cu [] PROGMEM = "cu";
prog_char _url_ts [] PROGMEM = "ts";

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
  //{_url_ps,print_webpage_programdata_subsection},
  {_url_vs,print_webpage_view_stations},
  {_url_cs,print_webpage_change_stations},
  {_url_vr,print_webpage_view_runonce},
  {_url_cr,print_webpage_change_runonce},
  {_url_jo,print_webpage_json_options},
  {_url_jn,print_webpage_json_stations},
  {_url_jp,print_webpage_json_programs},
  {_url_jc,print_webpage_json_controller},
  {_url_js,print_webpage_json_status},  
  {_url_su,print_webpage_view_scripturl},
  {_url_cu,print_webpage_change_scripturl},
  {_url_ts,print_webpage_test},
};

// analyze the current url
void analyze_get_url(char *p)
{
  ether.httpServerReplyAck();
  bfill = ether.tcpOffset();

  // the tcp packet usually starts with 'GET /' -> 5 chars    
  char *str = p+5;
  if(str[0]==' ') {
    print_webpage_home(str);  // home page handler
    ether.httpServerReply_with_flags(bfill.position(), TCP_FLAGS_ACK_V|TCP_FLAGS_FIN_V);
  } else {
    // server funtion handlers
    byte i;
    for(i=0;i<sizeof(urls)/sizeof(URLStruct);i++) {
      if(pgm_read_byte(urls[i].url)==str[0]
       &&pgm_read_byte(urls[i].url+1)==str[1]) {
        if ((urls[i].handler)(str) == false) {
          bfill.emit_p(PSTR("$F"), htmlUnauthorized);
        }
        break;
      }
    }
    
    if((i==sizeof(urls)/sizeof(URLStruct)) && svc.status.has_sd) {
      // no server funtion found, file handler
      byte k=0;  
      while (str[k]!=' ' && k<32) {tmp_buffer[k]=str[k];k++;}//search the end, indicated by space
      tmp_buffer[k]=0;
      //Serial.println(tmp_buffer);
      //ether.httpServerReplyAck();
      if (streamfile ((char *)tmp_buffer,TCP_FLAGS_FIN_V)==0) {
        // file not found
      }
    } else {
      ether.httpServerReply_with_flags(bfill.position(), TCP_FLAGS_ACK_V|TCP_FLAGS_FIN_V);
    }
  }
  //delay(50); // add a bit of delay here
}


#ifdef USE_TINYFAT
byte streamfile (char* name , byte lastflag) { //send a file to the buffer 
  unsigned long cur=0;
  if (!file.exists(name)) {return 0;}
  file.openFile(name, FILEMODE_BINARY);
  int  car=512;
  while (car==512) {
    car=file.readBinary();
    for(int i=0;i<car;i++) {
      cur++;
      Ethernet::buffer[cur+53]=file.buffer[i];
    }
    if (cur>=512) {
      ether.httpServerReply_with_flags(cur,TCP_FLAGS_ACK_V, 4);
      cur=0;
    } else {
      if(lastflag==TCP_FLAGS_FIN_V) {
        ether.httpServerReply_with_flags(cur,TCP_FLAGS_ACK_V+TCP_FLAGS_FIN_V, 4);
      }
    }
  }
  file.closeFile();
  return 1;
}
#else
byte streamfile (char* name , byte lastflag) { //send a file to the buffer 
  unsigned long cur=0;
  if(!SD.exists(name))  {return 0;}
  File myfile = SD.open(name);
  while(myfile.available()) {
    int nbytes = myfile.read(Ethernet::buffer+54, 512);
    cur = nbytes;
    if (cur>=512) {
      ether.httpServerReply_with_flags(cur,TCP_FLAGS_ACK_V);
      cur=0;
    } else {
      if(lastflag==TCP_FLAGS_FIN_V) {
        ether.httpServerReply_with_flags(cur,TCP_FLAGS_ACK_V+TCP_FLAGS_FIN_V);
      }
    }
  }
  myfile.close();
  return 1;
}
#endif
