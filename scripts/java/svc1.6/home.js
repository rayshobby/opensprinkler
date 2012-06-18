// Javascript for printing OpenSprinkler homepage
// Rayshobby.net
// June 2012
function w(s) {document.writeln(s);}
// input rain delay value
function setrd(form,idx) {var h=prompt("Enter hours to delay","0");if(h!=null){form.elements[idx].value=h;form.submit()};}
// print menu links
w("<h4><a href=/>[Refresh]</a>&nbsp;&nbsp;<a href=/vo>[Options]</a>&nbsp;&nbsp;<a href=/vp>[Program]</a>&nbsp;&nbsp;<a href=http://rayshobby.net/scripts/python/getweather.py/?location="+loc+">[Weather]</a></h4>");
// print device information
w("<b>Firmware version</b>: "+(ver/10>>0)+"."+(ver%10)+"<br>");
w("<b>Device time</b>: "+(new Date(devt*1000)).toUTCString()+(tz));
// print station status
w("<form name=gf action=gp method=get target=\"_blank\"><input type=hidden name=d value=today></form>")
w("<hr><button style=\"height:32\" onclick=\"gf.submit()\">Program Preview</button><br>");
w("<p><b>Station Status:</b></p><span style=\"line-height:22px\">");
var bid,s,sid,sn,rem,remm,rems;
for(bid=0;bid<nbrd;bid++){
  for(s=0;s<8;s++){
    sid=bid*8+s;
    sn=sid+1;
    w("Station "+(sn/10>>0)+(sn%10)+": ");
    if(sn==mas) {w(((sbits[bid]>>s)&1?("<b>On</b>").fontcolor("green"):("Off").fontcolor("black"))+" (<b>Master</b>)<br>"); continue;}
    rem=ps[sid][2];remm=rem/60>>0;rems=rem%60;
    if((sbits[bid]>>s)&1) {
      w(("<b>Running P"+ps[sid][0]).fontcolor("green")+"</b> ("+(remm/10>>0)+(remm%10)+":"+(rems/10>>0)+(rems%10)+" remaining)");
    } else {
      if(rem==0)  {w("-<br>"); continue;}
      w(("Waiting P"+ps[sid][0]+" ("+(remm/10>>0)+(remm%10)+":"+(rems/10>>0)+(rems%10)+" scheduled)").fontcolor("gray"));
    }
    w("<br>");
  }
}
// print other information
w("</span><br><b>Operation</b>: "+(en?("enabled").fontcolor("green"):("disabled").fontcolor("red")));
w("<br><b>Raindelay</b>: "+(rd?("on").fontcolor("red")+" (till "+(new Date(rdst*1000)).toUTCString()+(tz)+")":("off").fontcolor("black")));
w("<br><b>Rainsense</b>: "+(urs?(rs?("rain").fontcolor("red"):("no rain").fontcolor("green")):"-"));
w("<br><b>Sequential</b>: "+(seq?("yes").fontcolor("green"):("no").fontcolor("red")));
var lrsid=lrun[0],lrpid=lrun[1],lrdur=lrun[2],lret=lrun[3];
if(lrpid!=0 && lrsid!=0) w("<br><b>Last run</b>: "+("Station "+(lrsid/10>>0)+(lrsid%10)+" ran P"+lrpid+" for "+(lrdur/60>>0)+"m"+(lrdur%60)+"s @ "+(new Date(lret*1000)).toUTCString()+(tz)).fontcolor("gray"));
else w("<br><b>Last run</b>: -");
w("<hr>");
// print html form
w("<form name=hf action=cv method=get><p>Password:<input type=password size=10 name=pw></p>");
w("<input type=hidden name=en><input type=hidden name=rd value=0><input type=hidden name=rst value=0>");
w("<input type=button style=\"height:28\" onclick=\"hf.elements[1].value="+(1-en)+";hf.submit();\" value=\""+(en?"Stop operation":"Start operation")+"\">");
w("<input type=button style=\"height:28\" onclick=\"setrd(hf,2)\" value=\"Rain delay\">");
w("<input type=button style=\"height:28\" onclick=\"hf.elements[3].value=1;hf.submit();\" value=\"Reboot\"></form>");
