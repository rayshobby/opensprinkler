// Javascript for printing OpenSprinkler homepage
// Rayshobby.net
// June 2012
function w(s) {document.writeln(s);}
// input rain delay value
function srd(form,idx) {var h=prompt("Enter hours to delay","0");if(h!=null){form.elements[idx].value=h;form.submit()};}
// print menu links
w("<h4><a href=/>[Refresh]</a>&nbsp;&nbsp;<a href=/vo>[Options]</a>&nbsp;&nbsp;<a href=/vp>[Program]</a>&nbsp;&nbsp;<a href=http://rayshobby.net/scripts/python/getweather.py/?location="+loc+">[Weather]</a></h4>");
// print device information
w("<b>Firmware version</b>: "+(ver/10>>0)+"."+(ver%10)+"<br>");
w("<b>Device time</b>: "+(thour/10>>0)+(thour%10)+":"+(tmin/10>>0)+(tmin%10)+" "+tweekday+" "+(tmonth/10>>0)+(tmonth%10)+"-"+(tday/10>>0)+(tday%10));
// print station status
w("<hr><p><b>Station Status:</b></p><span style=\"line-height:22px\">");
var i,s,sid,id,rem,remm,rems;
for(i=0;i<nboard;i++){
  for(s=0;s<8;s++){
    sid=i*8+s;
		id=sid+1;
		w("S"+(id/10>>0)+(id%10)+": ");
		if(id==ms)	{w(((status_bits[i]>>s)&1?("<b>On</b>").fontcolor("green"):("Off").fontcolor("black"))+" (<b>Master</b>)<br>"); continue;}
		rem=progstat[sid][2];remm=rem/60>>0;rems=rem%60;
		if((status_bits[i]>>s)&1) {
			w(("<b>Running P"+progstat[sid][0]).fontcolor("green")+"</b> ("+(remm/10>>0)+(remm%10)+":"+(rems/10>>0)+(rems%10)+" remaining)");
		} else {
			if(rem==0)	{w("-<br>"); continue;}
			w(("Waiting  P"+progstat[sid][0]).fontcolor("gray")+" ("+(remm/10>>0)+(remm%10)+":"+(rems/10>>0)+(rems%10)+" scheduled)");
		}
		w("<br>");
  }
}
// print other information
w("</span><br><b>Operation</b>: "+(enable?("enabled").fontcolor("green"):("disabled").fontcolor("red")));
w("<br><b>Raindelay</b>: "+(rd?("on").fontcolor("red")+" (till "+(rdh/10>>0)+(rdh%10)+":"+(rdm/10>>0)+(rdm%10)+" "+(rdo/10>>0)+(rdo%10)+"-"+(rdd/10>>0)+(rdd%10)+")":("off").fontcolor("black")));
w("<br><b>Rainsense</b>: "+(urs?(rainsense?("rain").fontcolor("red"):("no rain").fontcolor("green")):"-"));
w("<br><b>Serialize</b>: "+(serialize?("yes").fontcolor("green"):("no").fontcolor("red")));
w("<hr>");
// print html form
w("<form name=hf action=cv method=get><p>Password:<input type=password size=10 name=p></p>");
w("<input type=hidden name=ve><input type=hidden name=rd value=0><input type=hidden name=rst value=0>");
w("<input type=button style=\"height:28\" onclick=\"hf.elements[1].value="+(1-enable)+";hf.submit();\" value=\""+(enable?"Emerg. Stop":"Start")+"\">");
w("<input type=button style=\"height:28\" onclick=\"srd(hf,2)\" value=\"Rain delay\">");
w("<input type=button style=\"height:28\" onclick=\"hf.elements[3].value=1;hf.submit();\" value=\"Reboot\"></form>");
