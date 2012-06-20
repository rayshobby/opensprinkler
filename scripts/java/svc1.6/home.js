// Javascript for printing OpenSprinkler homepage
// Rayshobby.net
// June 2012
function w(s) {document.writeln(s);}
// input rain delay value
function setrd(form,idx) {var h=prompt("Enter hours to delay","0");if(h!=null){form.elements[idx].value=h;form.submit()};}
function imgstr(s) {return "<img src=\"http://rayshobby.net/images/icons/svc_"+s+".png\" height=20 align=absmiddle>&nbsp;";}
function datestr(t) {return (new Date(t)).toUTCString()+((tz>=0)?"+":"")+tz;}
// print menu links
w("<h4><a href=/>[Refresh]</a>&nbsp;&nbsp;<a href=/vo>[Options]</a>&nbsp;&nbsp;<a href=/vp>[Program]</a>&nbsp;&nbsp;<a href=http://rayshobby.net/scripts/python/getweather.py/?location="+loc+">[Weather]</a></h4>");
// print device information
w("<b>Firmware version</b>: "+(ver/10>>0)+"."+(ver%10)+"<br>");
w("<b>Device time</b>: "+datestr(devt*1000));
w("<script type=\"text/javascript\" src=\"http://rayshobby.net/scripts/java/svc1.6/"+((mm)?"manualmode.js":"progmode.js")+"\"></script>");
// print other information
w("</span><br><b>Operation</b>: "+(en?("enabled").fontcolor("green"):("DISABLED").fontcolor("red")));
w("<br><b>Raindelay</b>: "+(rd?("ON").fontcolor("red")+" (till "+datestr(rdst*1000)+")":("off").fontcolor("black")));
w("<br><b>Rainsense</b>: "+(urs?(rs?("Rain Detected").fontcolor("red"):("no rain").fontcolor("green")):"-"));
w("<br><b>Sequential</b>: "+(seq?("yes").fontcolor("green"):("no").fontcolor("blue")));
var lrsid=lrun[0],lrpid=lrun[1],lrdur=lrun[2],lret=lrun[3];
if(lrpid!=0 && lrsid!=0) w("<br><b>Last run</b>: "+("Station "+(lrsid/10>>0)+(lrsid%10)+" ran "+(lrpid==255?"manual mode":("P"+lrpid))+" for "+(lrdur/60>>0)+"m"+(lrdur%60)+"s @ "+datestr(lret*1000)).fontcolor("gray"));
else w("<br><b>Last run</b>: -");
w("<hr>");
// print html form
w("<form name=hf action=cv method=get><p>Password:<input type=password size=10 name=pw></p>");
w("<input type=hidden name=en><input type=hidden name=rd value=0><input type=hidden name=rst value=0><input type=hidden name=mm value=0></form>");
w("<button style=\"height:36\" onclick=\"hf.elements[1].value="+(1-en)+";hf.submit();\">"+imgstr(en?"stop":"start")+(en?"Stop Operation":"Start Operation")+"</button>");
w("<button style=\"height:36\" onclick=\"hf.elements[4].value="+(1-mm)+";hf.submit();\">"+imgstr(mm?"auto":"manual")+(mm?"Manual Off":"Switch to Manual")+"</button>");
w("<button style=\"height:36\" onclick=\"setrd(hf,2)\">"+imgstr("rain")+"Rain Delay</button>");
w("<button style=\"height:36\" onclick=\"hf.elements[3].value=1;hf.submit();\">"+imgstr("reboot")+"Reboot</button>");
