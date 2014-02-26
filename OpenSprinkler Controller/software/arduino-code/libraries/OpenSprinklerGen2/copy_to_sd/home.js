// JS for printing OpenSprinkler homepage
// Firmware v2.1
// All content published under:
// Creative Commons Attribution ShareAlike 3.0 License
// Sep 2013, Rayshobby.net
// Redirect to mobile app if mobile device detected. This is a very rudimentery scan and needs to be expanded upon later
//if (navigator.userAgent.match(/Android|iPhone|iPad|iPod/i)) window.location.href="index.htm"
function w(s) {document.writeln(s);}
function link(s) {window.location=s;}
function linkn(s){window.open(s, '_blank');}
// input rain delay value
function setrd(form,idx) {var h=prompt("Enter hours to delay","0");if(h!=null){form.elements[idx].value=h;form.submit()};}
function ib(s,t,v) {w("<button style=\"height:44px\" onclick="+s+">"+v+"</button>");}
function datestr(t) {var _t=tz-48; return (new Date(t)).toUTCString()+((_t>=0)?"+":"-")+(Math.abs(_t)/4>>0)+":"+((Math.abs(_t)%4)*15/10>>0)+((Math.abs(_t)%4)*15%10);}
function rsn() {var p="";if(!ipas) p=prompt("Please enter your password:","");if(p!=null) window.location="/cv?pw="+p+"&rsn=1";}
function id(s) {return document.getElementById(s);}
function snf(sid,sbit) {
  if(sbit==1) window.location="/sn"+(sid+1)+"=0"; // turn off station
  else {
    var strmm=id("mm"+sid).value, strss=id("ss"+sid).value;
    var mm=(strmm=="")?0:parseInt(strmm);
    var ss=(strss=="")?0:parseInt(strss);
    if(!(mm>=0&&ss>=0&&ss<60))  {alert("Timer values wrong: "+strmm+":"+strss);return;}
    window.location="/sn"+(sid+1)+"=1"+"&t="+(mm*60+ss);  // turn it off with timer
  }
}
// print menu links
ib("link(\"/\")","reset","Refresh");
ib("link(\"/vo\")","options","Options");
ib("link(\"/vs\")","edit","Stations");
ib("link(\"/vp\")","cal","Programs");
ib("linkn(\"http://igoogle.wunderground.com/cgi-bin/findweather/getForecast?query="+loc+"\")","weather","Weather");
w("<p></p>");
// print device information
if(ver>=100) w("<b>Firmware version</b>: "+(ver/100>>0)+"."+((ver/10>>0)%10)+"."+(ver%10)+"<br>");
else w("<b>Firmware version</b>: "+(ver/10>>0)+"."+(ver%10)+"<br>");
w("<b>Device time</b>: "+datestr(devt*1000)+"<hr>");
if(!mm) {
ib("linkn(\"/gp?d=0\")","preview","Program Preview");
ib("rsn()","del","Stop All Stations");
ib("link(\"/vr\")","start","Run-Once Program");
w("<p><b>Station Status</b>:</p>");
w("<table border=1>");
var bid,s,sid,sn,rem,remm,rems,off,pname;
off=((en==0||rd!=0||(urs!=0&&rs!=0))?1:0);
for(bid=0;bid<nbrd;bid++){
  for(s=0;s<8;s++){
    w("<tr><td bgcolor=\"#E4E4E4\">");
    sid=bid*8+s;
    sn=sid+1;
    w(snames[sid]+':&nbsp;&nbsp;');
    w("</td><td>");
    if(off) w("<strike>");
    if(sn==mas) {w(((sbits[bid]>>s)&1?("<b>On</b>").fontcolor("green"):("Off").fontcolor("black"))+" (<b>Master</b>)");}
    else {
      rem=ps[sid][1];remm=rem/60>>0;rems=rem%60;
      pname="P"+ps[sid][0];
      if(ps[sid][0]==255||ps[sid][0]==99) pname="Manual Program";
      if(ps[sid][0]==254||ps[sid][0]==98) pname="Run-once Program";
      if((sbits[bid]>>s)&1) {
        w(("<b>Running "+pname).fontcolor("green")+"</b> ("+(remm/10>>0)+(remm%10)+":"+(rems/10>>0)+(rems%10)+" remaining)");
      } else {
        if(ps[sid][0]==0) w("<font color=lightgray>(closed)</font>");
        else w(("Waiting "+pname+" ("+(remm/10>>0)+(remm%10)+":"+(rems/10>>0)+(rems%10)+" scheduled)").fontcolor("gray"));
      }
    }
    if(off) w("</strike>");
    w("</td></tr>");
  }
}
w("</table>");
} else {
w("<b>Manual Control:</b> (timer is optional)<p></p>");
w("<table border=1>");
var bid,s,sid,sn,rem,remm,rems,sbit;
for(bid=0;bid<nbrd;bid++){
  for(s=0;s<8;s++){
    w("<tr><td bgcolor=\"#E4E4E4\">");
    sid=bid*8+s;
    sn=sid+1;
    //w("Station "+(sn/10>>0)+(sn%10)+": ");
    w(snames[sid]+":&nbsp;&nbsp;</td><td>");
    if(sn==mas) {w(((sbits[bid]>>s)&1?("<b>On</b>").fontcolor("green"):("Off").fontcolor("black"))+" (<b>Master</b>)");}
    else {
      rem=ps[sid][1];
      if(rem>65536) rem=0;
      remm=rem/60>>0;rems=rem%60;sbit=(sbits[bid]>>s)&1;
      var bg=(sbit?"#FFCCCC":"#CCFFCC"),tx=(sbit?"off":"on"),dis=(sbit?"disabled":"");
      w("<button style=\"width:100px;height:32px;background-color:"+bg+";border-radius:8px;\" id=bb"+sid+" onclick=\"snf("+sid+","+sbit+")\">Turn "+tx+"</button>");
      w(sbit?" in ":" with timer ");
      w("<input type=text id=mm"+sid+" size=2 maxlength=3 value="+remm+" "+dis+" />:");
      w("<input type=text id=ss"+sid+" size=2 maxlength=2 value="+rems+" "+dis+" /> (mm:ss)");
    }
    w("</td>");
  }
}
w("</table>");
}
// print status and other information
w("<br><b>Operation</b>: "+(en?("on").fontcolor("green"):("OFF").fontcolor("red")));
w("<br><b>Raindelay</b>: "+(rd?("ON").fontcolor("red")+" (till "+datestr(rdst*1000)+")":("off").fontcolor("black")));
w("<br><b>Rainsense</b>: "+(urs?(rs?("Rain Detected").fontcolor("red"):("no rain").fontcolor("green")):"<font color=gray>n/a</font>"));
w("<br><b>% Water Time</b>: <font color="+((wl==100)?"green":"red")+">"+wl+"\%</font>");
var lrsid=lrun[0],lrpid=lrun[1],lrdur=lrun[2],lret=lrun[3];
var pname="P"+lrpid;
if(lrpid==255||lrpid==99) pname="Manual Mode";
if(lrpid==254||lrpid==98) pname="Run-once Program";
dstr=(new Date(lret*1000)).toUTCString().replace("GMT","").replace("UTC","");
if(lrpid!=0) w("<br><b>Log</b>: "+(snames[lrsid]+" ran "+pname+" for "+(lrdur/60>>0)+"m"+(lrdur%60)+"s @ "+dstr).fontcolor("gray"));
else w("<br><b>Log</b>: <font color=gray>n/a</font>");
w("<hr>");
// print html form
w("<form name=hf action=cv method=get><p>Password:<input type=password "+(ipas?"disabled":"")+" size=10 id=pwd name=pw></p>");
w("<input type=hidden name=en><input type=hidden name=rd value=0><input type=hidden name=rbt value=0><input type=hidden name=mm value=0></form>");
ib("\"hf.elements[1].value="+(1-en)+";hf.submit();\"",en?"stop":"start",en?"Stop Operation":"Start Operation");
ib("\"hf.elements[4].value="+(1-mm)+";hf.submit();\"",mm?"auto":"manual",mm?"Manual Off":"Manual On");
ib("\"setrd(hf,2)\"","rain","Rain Delay");
ib("\"hf.elements[3].value=1;hf.submit();\"","reboot","Reboot");
w("<p></p><hr><br>");
