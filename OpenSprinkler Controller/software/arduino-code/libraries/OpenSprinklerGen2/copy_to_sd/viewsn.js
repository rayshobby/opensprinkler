// JS for changing OpenSprinkler station names and master operation bits
// Firmware 2.0.5
// All content published under:
// Creative Commons Attribution ShareAlike 3.0 License
// Sep 2013, Rayshobby.net
function w(s) {document.writeln(s);}
function ib(s,t,v) {w("<button style=\"height:44px\" onclick="+s+">"+v+"</button>");}
function rst() {
  var sid,sn;
  for(sid=0;sid<nboards*8;sid++) {
    sn=sid+1;
    document.getElementById("n"+sid).value="S"+(sn/10>>0)+(sn%10);
  }
}
function fsubmit(f) {
  var s, bid, sid, v, r;
  for(bid=0;bid<nboards;bid++) {
    v=0;
    r=0;
    for(s=0;s<8;s++){
      sid=bid*8+(7-s);
      v=v<<1;
      r=r<<1;
      if(sid+1==mas) {v=v+1;}
      else if(mas>0 && document.getElementById("mc"+sid).checked) {
        v=v+1;
      }
      if(document.getElementById("ir"+sid).checked) {
        r=r+1;
      }
    }
    if(mas>0) f.elements["m"+bid].value=v;
    f.elements["i"+bid].value=r;
  }
  f.submit();
}
function fcancel() {window.location="/";}
w("<div align=\"center\" style=\"background-color:#EEEEEE;border:2px solid gray;padding:5px 10px;width:240px;border-radius:10px;box-shadow:3px 3px 2px #888888;\">");
w("<font size=3><b>Set Stations:</b></font><br>");
w("<font size=2>(Maximum name length is "+maxlen+" letters).</font></div><p></p>");
var sid,sn,bid,s,x;
w("<span style=\"line-height:32px\"><form name=sf action=cs method=get>");
for(sid=0;sid<nboards*8;sid++) {
  sn=sid+1;
  bid=sid>>3;
  s=sid%8;
  w("Station "+(sn/10>>0)+(sn%10)+":");
  w("<input type=text size="+maxlen+" maxlength="+maxlen+" value=\""+snames[sid]+"\" name=s"+sid+" id=n"+sid+">&nbsp;");
  if (sid+1==mas) { 
    w("(<b>Master</b>)");
    for(x=0;x<8;x++) w("&nbsp;");
  } else {
    if (mas>0) w("<input type=checkbox "+(masop[bid]&(1<<s)?"checked":"")+" id=mc"+sid+">Activate master?");
  }
  w("&nbsp;<input type=checkbox "+(ir[bid]&(1<<s)?"checked":"")+" id=ir"+sid+">Ignore rain?");
  w("<br>");
}
w("<hr><font size=3><b>Password:</b><input type=password size=10 "+(ipas?"disabled":"")+" name=pw></font><p></p>");
for(bid=0;bid<nboards;bid++) {
  w("<input type=hidden name=m"+bid+">");
  w("<input type=hidden name=i"+bid+">");
}
w("</form></span>");
ib("\"fsubmit(sf)\"","submit","<b>Submit Changes</b>");
ib("\"rst()\"","reset","Reset Names");
ib("\"fcancel()\"","delall","Cancel");
