// JS for printing OpenSprinkler option page 
// Firmware v2.1
// All content published under:
// Creative Commons Attribution ShareAlike 3.0 License
// Sep 2013, Rayshobby.net
var str_tooltips=["Example: GMT-4:00, GMT+5:30.", "Use NTP sync", "HTTP port (effective after reboot).", "HTTP port, from 0 to 65535 (effective after reboot).", "Automatic reconnection if network fails", "Number of extension boards", "Sequential running or concurrent running", "Station delay time (in seconds), from 0 to 240.", "Select master station", "Master on delay (in seconds), from 0 to 60.", "Master off delay (in seconds), from -60 to 60.", "Use rain sensor", "Rain sensor type", "% Watering time, from 0 to 250.", "Ignore web password", "Sets the last byte of MAC, from 0 to  255 (effective after reboot)."];

var str_titles=["Time zone:", "NTP sync?", "HTTP port:", "HTTP port:", "Auto reconnect?", "# of exp. boards:", "Sequential mode?", "Station delay:", "Master station:", "Master on adj.:", "Master off adj.:", "Use rain sensor:", "Normally open?", "% Water time:", "Ignore password?", "Device ID:"]
function w(s) {document.writeln(s);}
function id(s) {return document.getElementById(s);}
function submit_form(f) {
  // process time zone value
  var th=parseInt(f.elements["th"].value,10);
  var tq=parseInt(f.elements["tq"].value,10);
  tq=(tq/15>>0)/4.0;th=th+(th>=0?tq:-tq);
  // huge hack, needs to find a more elegant way
  f.elements["o1"].value=((th+12)*4)>>0;
  f.elements["o12"].value=(f.elements["htp"].value)&0xff;
  f.elements["o13"].value=(f.elements["htp"].value>>8)&0xff;
  f.elements["o18"].value=f.elements["mas"].value;
  f.elements["ttt"].value=Date.UTC(f.elements["tyy"].value,f.elements["tmm"].value-1,f.elements["tdd"].value,
                  f.elements["thh"].value,f.elements["tmi"].value,0,0)/1000;
  f.submit();
}
function fcancel() {window.location="/";}
function fshow() {
  var oid,tip;
  for(oid=0;oid<nopts;oid++){
    tip=id("tip"+oid);
    if(tip!=null) tip.hidden=false;
  }
  tip=id("tiploc");
  if(tip!=null) tip.hidden=false;
}
function ntpcheck() {
  var cb=id("ntpcb");
  if(cb.checked) {
    id("iyy").disabled=true;id("imm").disabled=true;id("idd").disabled=true;
    id("ihh").disabled=true;id("imi").disabled=true;id("itt").disabled=true;
  } else {
    id("iyy").disabled=false;id("imm").disabled=false;id("idd").disabled=false;
    id("ihh").disabled=false;id("imi").disabled=false;id("itt").disabled=false;
  }
}

w("<div align=\"center\" style=\"background-color:#EEEEEE;border:2px solid gray;padding:5px 10px;width:240px;border-radius:10px;box-shadow:3px 3px 2px #888888;\">");
w("<b>Set Options</b>:<br><font size=2>(Hover on each option to see tooltip)</font></div>");
w("<p></p>");
w("<button style=\"height:24\" onclick=\"fshow();return false;\">Show Tooltips</button>");
// print html form
w("<form name=of action=co method=get>");
var oid,name,isbool,value,index,pasoid=0;
for(oid=0;oid<nopts;oid++){
  name=opts[oid*4+0];
  isbool=(opts[oid*4+1]==1)?1:0;
  value=opts[oid*4+2];
  index=opts[oid*4+3];
  if(name=="ipas") pasoid=oid;
  if(isbool) {
    if(name=="ntp") {
      w("<p title=\""+str_tooltips[oid]+"\"><b>"+str_titles[oid]+"</b> <input type=checkbox "+(value>0?"checked":"")+" name=o"+index+" id=ntpcb onclick=\"ntpcheck()\">");
    } else {
      w("<p title=\""+str_tooltips[oid]+"\"><b>"+str_titles[oid]+"</b> <input type=checkbox "+(value>0?"checked":"")+" name=o"+index+">");
    }
  }
  else {
    if (name=="tz") {
      w("<input type=hidden value=0 name=o"+index+">");
      tz=value-48;
      w("<p title=\""+str_tooltips[oid]+"\"><b>"+str_titles[oid]+"</b> GMT<input type=text size=3 maxlength=3 value="+(tz>=0?"+":"-")+(Math.abs(tz)/4>>0)+" name=th>");
      w(":<input type=text size=3 maxlength=3 value="+((Math.abs(tz)%4)*15/10>>0)+((Math.abs(tz)%4)*15%10)+" name=tq>");
    } else if (name=="mas") {
      w("<input type=hidden value=0 name=o"+index+">");
      w("<p title=\""+str_tooltips[oid]+"\"><b>"+str_titles[oid]+"</b> <select name=mas><option "+(value==0?" selected ":" ")+"value=0>None</option>");
      for(i=1;i<=8;i++) w("<option "+(value==i?" selected ":" ")+"value="+i+">Station 0"+i+"</option>");
      w("</select>");
    } else if (name=="hp0") {
      w("<input type=hidden value=0 name=o"+index+"><input type=hidden value=0 name=o"+(index+1)+">");
      var port=value+(opts[(oid+1)*4+2]<<8);
      w("<p title=\""+str_tooltips[oid]+"\"><b>"+str_titles[oid]+"</b> <input type=text size=5 maxlength=5 value="+port+" name=htp>");
      oid++;
    } else {
      w("<p title=\""+str_tooltips[oid]+"\"><b>"+str_titles[oid]+"</b> <input type=text size=3 maxlength=3 value="+value+" name=o"+index+">");
    }
  }
  //w("</p>");
  w(" <span style=\"background-color:#FFF2B8;\" id=tip"+oid+" hidden=\"hidden\"><font size=2>"+str_tooltips[oid]+"</font></span></p>");
  if (name=="ntp") {
    var d=new Date(devt*1000);
    w("<p title=\"Set time\"><b>Time</b>: <input type=text size=4 maxlength=4 value="+d.getUTCFullYear()+" name=tyy id=iyy>-<input type=text size=2 maxlength=2 value="+(d.getUTCMonth()+1)+" name=tmm id=imm>-<input type=text size=2 maxlength=2 value="+d.getUTCDate()+" name=tdd id=idd> (y-m-d) <input type=text size=2 maxlength=2 value="+(d.getUTCHours())+" name=thh id=ihh>:<input type=text size=2 maxlength=2 value="+(d.getUTCMinutes())+" name=tmi id=imi> (h:m)<input type=hidden value=0 name=ttt id=itt></p>");
    ntpcheck();
  }
}
w("<p title=\"City name or zip code. Use comma or + in place of space.\"><b>Location:</b> <input type=text size=16 maxlength=31 value=\""+loc+"\" name=loc> <span style=\"background-color:#FFF2B8;\" id=tiploc hidden=\"hidden\"><font size=2>City name or zip code. Use comma or + in place of space.</font></span></p>");
w("<h4>Password:<input type=password size=10 "+(opts[pasoid*4+2]?"disabled":"")+" name=pw></h4>");
w("<button style=\"height:36\" onclick=\"submit_form(of)\"><b>Submit Changes</b></button>");
w("<button style=\"height:36\" onclick=\"fcancel();return false;\">Cancel</button>");
w("<h4>Change password</b>:<input type=password size=10 name=npw>&nbsp;&nbsp;Confirm:&nbsp;<input type=password size=10 name=cpw></h4>");
w("</form>");
