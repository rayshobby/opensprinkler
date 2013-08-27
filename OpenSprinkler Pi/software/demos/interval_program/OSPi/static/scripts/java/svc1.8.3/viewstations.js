// Javascript for changing OpenSprinkler station names and master operation bits
// Firmware v1.8
// All content is published under:
// Creative Commons Attribution ShareAlike 3.0 License
// Sep 2012, Rayshobby.net

function w(s) {document.writeln(s);}
function imgstr(s) {return "<img src=\""+baseurl+"/static/images/icons/svc_"+s+".png\" height=20 align=absmiddle>&nbsp;";}
function rst() {
  var sid,sn;
  for(sid=0;sid<sd['nbrd']*8;sid++) {
    sn=sid+1;
    document.getElementById("n"+sid).value="S"+(sn/10>>0)+(sn%10);
  }
}
function fsubmit(f) {
  if(sd['mas']>0) {
    var s, bid, sid, v;
    for(bid=0;bid<sd['nbrd'];bid++) {
      v=0;
      for(s=0;s<8;s++){
        sid=bid*8+(7-s);
        v=v<<1;
        if(sid+1==sd['mas']) {v=v+1;continue;}
        if(document.getElementById("mc"+sid).checked) {
          v=v+1;
        }
      }
      f.elements["m"+bid].value=v;
    }
  }
  f.submit();
}
function fcancel() {window.location="/";}
w("<div align=\"center\" style=\"background-color:#EEEEEE;border:2px solid gray;padding:5px 10px;width:240px;border-radius:10px;box-shadow:3px 3px 2px #888888;\">");
w("<font size=3><b>Set Stations:</b></font><br>");
w("<font size=2>(Maximum name length is "+sd['station_name_length']+" letters).</font></div><p></p>");
var sid,sn,bid,s;
w("<span style=\"line-height:32px\"><form name=sf action=cs method=get>");
for(sid=0;sid<sd['nbrd']*8;sid++) {
  sn=sid+1;
  bid=sid>>3;
  s=sid%8;
  w("Station "+(sn/10>>0)+(sn%10)+":");
  w("<input type=text size="+sd['station_name_length']+" maxlength="+sd['station_name_length']+" value=\""+snames[sid]+"\" name=s"+sid+" id=n"+sid+">&nbsp;");
  if (sid+1==sd['mas']) w("(<b>Master</b>)");
  else if (sd['mas']>0) w("<input type=checkbox "+(masop[bid]&(1<<s)?"checked":"")+" id=mc"+sid+">Activate master?");
  w("<br>");
}
w("<hr><font size=3><b>Password:</b><input type=password size=10 "+(sd['ipas']?"disabled":"")+" name=pw></font><p></p>");
for(bid=0;bid<sd['nbrd'];bid++) {
  w("<input type=hidden name=m"+bid+">");
}
w("</form></span>");
w("<button style=\"height:36\" onclick=\"fsubmit(sf)\">"+imgstr("submit")+"<b>Submit Changes</b></button>");
w("<button style=\"height:36\" onclick=\"rst()\">"+imgstr("reset")+"Reset Names</button>");
w("<button style=\"height:36\" onclick=\"fcancel()\">"+imgstr("delall")+"Cancel</button>");
