// Javascript for printing OpenSprinkler option page 
// Rayshobby.net
// June 2012
function w(s) {document.writeln(s);}

// print menu links
w("<a href=/><-Home</a>");

// print html form
w("<form name=of action=co method=get>");
var i,name,isbool,value,index;
for(i=0;i<noptions;i++){
	name=options[i*4+0];
	isbool=options[i*4+1];
	value=options[i*4+2];
	index=options[i*4+3];
	if(isbool)  w("<h4>"+name+" <input type=checkbox "+(value>0?"checked":"")+" name=o"+index+"></h4>");
	else  w("<h4>"+name+" <input type=text size=4 value="+value+" name=o"+index+"></h4>");
}

w("<font size=3><b>Location:</b><input type=text value=\""+loc+"\" name=loc></font><br><font size=2>(City name or zip code. Use , or + in place of space).</font>");
w("<h4>Password:<input type=password size=10 name=p></h4>");
w("<input type=submit style=\"height:28\" value=\"Submit Changes\">");
w("<h4>Change password</b>:<input type=password size=10 name=np>&nbsp;&nbsp;Confirm:&nbsp;<input type=password size=10 name=cp></h4>");
w("</form>");
