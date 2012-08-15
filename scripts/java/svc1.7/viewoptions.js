// Javascript for printing OpenSprinkler option page 
// Rayshobby.net
// June 2012
function w(s) {document.writeln(s);}
// print menu links
w("<a href=/><-Home</a>");
// print html form
w("<form name=of action=co method=get>");
var oid,name,isbool,value,index;
for(oid=0;oid<nopts;oid++){
  name=opts[oid*4+0];
  isbool=opts[oid*4+1];
  value=opts[oid*4+2];
  index=opts[oid*4+3];
  if(isbool)  w("<h4>"+name+" <input type=checkbox "+(value>0?"checked":"")+" name=o"+index+"></h4>");
  else  w("<h4>"+name+" <input type=text size=3 maxlength=3 value="+value+" name=o"+index+"></h4>");
}

w("<font size=3><b>Location:</b><input type=text maxlength=30 value=\""+loc+"\" name=loc></font><br><font size=2>(City name or zip code. Use, or + in place of space,<br>such as \'New,York\' or \'New+York\').</font>");
if (typeof yr != 'undefined') {
w("<h4>Change time? <input type=checkbox name=tchg><br>");
w("<input type=text size=4 maxlength=4 value="+yr+" name=t0>-<input type=text size=2 maxlength=2 value="+mo+" name=t1>-<input type=text size=2 maxlength=2 value="+dy+" name=t2>(y-m-d) <input type=text size=2 maxlength=2 value="+(hr/10>>0)+(hr%10)+" name=t3>:<input type=text size=2 maxlength=2 value="+(min/10>>0)+(min%10)+" name=t4>:<input type=text size=2 maxlength=2 value="+(sec/10>>0)+(sec%10)+" name=t5>(h:m:s)</h4>");
}
w("<h4>Password:<input type=password size=10 name=pw></h4>");
w("<button style=\"height:36\" onclick=\"of.submit()\">Submit Changes</button>");
w("<h4>Change password</b>:<input type=password size=10 name=npw>&nbsp;&nbsp;Confirm:&nbsp;<input type=password size=10 name=cpw></h4>");
w("</form>");
