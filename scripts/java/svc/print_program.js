// Javascript for printing OpenSprinkler schedule page
// Rayshobby.net
// June 2012
str_days=["Mon","Tue","Wed","Thur","Fri","Sat","Sun"];
function w(s) {document.writeln(s);}
function del(form,idx) {var p=prompt("Please enter your password:","");if(p!=null){form.elements[0].value=p;form.elements[1].value=idx;form.submit();}}
function mod(form,idx) {form.elements[0].value=idx;form.submit();}
// parse and print days
function pdays(days){
	if((days&0x80)!=0){
		// this is an Odd, Even, or N days schedule
		days=days&0x7f;
		if(days==0)	w("Even days");
		else if(days==1) w("Odd days");
		else w("Every "+days+" days");
	}else{
		// this is a weekly schedule
		for(d=0;d<7;d++) {
			if(days&(1<<d)) {w(str_days[d]);}
		}
	}
}
// parse and print stations
function pstations(data){
	w("<table border=1><tbody>");
	var i,s,bits;
	for(i=0;i<nboards;i++){
		w("<tr>");
		bits=data[i+5];
		for(s=0;s<8;s++){
			station=i*8+s+1;
			if(bits&(1<<s)) {
				w("<td style=background-color:#7AFA7A>S"+(station/10>>0)+(station%10)+"</td>");
			} else {
				w("<td>S"+(station/10>>0)+(station%10)+"</td>");
			}
		}
		w("</tr>");
	}	
	w("</tbody></table>\n");
}

w("<h4><a href=/><-Home</a></h4>");
// print html form
w("<form name=df action=dp method=get><input type=hidden name=p><input type=hidden name=i></form>");
w("<form name=mf action=mp method=get><input type=hidden name=i></form>")
w("<button style=\"height:32\" onclick=mod(mf,-1)>Add a new program</button>");
w("<button style=\"height:32\" onclick=del(df,-1)>Delete all</button><hr>");
if(nprograms==0) w("(Empty)");
// print programs
var i,st,et,iv,du;
for(i=0;i<nprograms;i++) {
	w("<br><b>Program "+(i+1)+": ");
	// parse and print days
	pdays(programdata[i][0]);
	w("</b>");
	// print time
	st=programdata[i][1];
	et=programdata[i][2];
	iv=programdata[i][3];
	du=programdata[i][4];
	w("<br><b>Time</b>: "+((st/60>>0)/10>>0)+((st/60>>0)%10)+":"+((st%60)/10>>0)+((st%60)%10));
	w(" - "+((et/60>>0)/10>>0)+((et/60>>0)%10)+":"+((et%60)/10>>0)+((et%60)%10));
	w("<br><b>Every</b> "+(iv/60>>0)+" hours "+(iv%60)+" minutes");
	w("<br><b>Open</b>: "+(du/60>>0)+" minutes "+(du%60)+" seconds");
	// parse and print stations
	w("<br><b>Stations</b>:<br>");
	pstations(programdata[i]);
	// print buttons
	w("<br><button style=\"height:28\" onclick=del(df,"+i+")>Delete</button>");
	w("<button style=\"height:28\" onclick=mod(mf,"+i+")>Modify</button>");
	w("<hr>");
}
