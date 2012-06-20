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
  else  w("<h4>"+name+" <input type=text size=4 value="+value+" name=o"+index+"></h4>");
}

w("<font size=3><b>Location:</b><input type=text value=\""+loc+"\" name=loc></font><br><font size=2>(City name or zip code. Use, or + in place of space,<br>such as \'New,York\' or \'New+York\').</font>");
w("<h4>Password:<input type=password size=10 name=pw></h4>");
w("<button style=\"height:36\" onclick=\"of.submit()\">Submit Changes</button>");
w("<h4>Change password</b>:<input type=password size=10 name=npw>&nbsp;&nbsp;Confirm:&nbsp;<input type=password size=10 name=cpw></h4>");
w("</form>");
