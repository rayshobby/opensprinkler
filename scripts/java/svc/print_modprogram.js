// Javascript for printing OpenSprinkler modify schedule page
// Rayshobby.net
// June 2012
function w(s) {document.writeln(s);}
function id(s){return document.getElementById(s);}
// parse time
function parse_time(prefix) {
	var h=parseInt(id(prefix+"h").value,10);
	var m=parseInt(id(prefix+"m").value,10);
	if(!(h>=0&&h<24&&m>=0&&m<60))	{alert("Error: Incorrect time input "+prefix+".");return -1;}
	return h*60+m;
}
// fill time
function fill_time(prefix,idx) {
	var t=programdata[idx];
	id(prefix+"h").value=""+((t/60>>0)/10>>0)+((t/60>>0)%10);
	id(prefix+"m").value=""+((t%60)/10>>0)+((t%60)%10);
}
// handle form submit
function fsubmit(f) {
	var errmsg = "",days=0,i,s,sid;
	// process days
	if(id("days_week").checked) {
		for(i=0;i<7;i++) {if(id("d"+i).checked) {days |= (1<<i);	}}
	}
	else if(id("days_odd").checked) {days=0x81;}
	else if(id("days_even").checked) {days=0x80;}
	else if(id("days_n").checked) {
		days=parseInt(id("dn").value,10);
		if(!(days>1&&days<128)) {alert("Error: Incorrect number of days: "+days+".");return;}
		days|=0x80;
	}
	if(days==0) {alert("Error: You have not selected any day.");return;}
	// process stations
	var stations=[0],station_selected=0;
	for(i=0;i<nboards;i++) {
		stations[i]=0;
		for(s=0;s<8;s++) {
			sid=i*8+s;
			if(id("s"+sid).checked) {
				stations[i] |= 1<<s;station_selected=1;
			}
		}
	}
	if(station_selected==0) {alert("Error: You have not selected any station.");return;}
	// process time
	var start_time,end_time,interval,duration;
	if((start_time=parse_time("ts")) < 0)	return;
	if((end_time=parse_time("te")) < 0)	return;
	if(!(start_time<end_time))	{alert("Error: Start time must be prior to end time.");return;}
	if((interval=parse_time("ti")) < 0)	return;
	var dm=parseInt(id("tdm").value,10);
	var ds=parseInt(id("tds").value,10);
	duration=dm*60+ds;
	if(!(dm>=0&&ds>=0&&ds<60&&duration>0))	{alert("Error: Incorrect duration.");return;}
	p=prompt("Please enter your password:","");
	if(p!=null){
		f.elements[0].value=p;
		f.elements[1].value=program_index;
		f.elements[2].value="["+days+","+start_time+","+end_time+","+interval+","+duration;
		for(i=0;i<nboards;i++) {f.elements[2].value+=","+stations[i];}
		f.elements[2].value+="]";
		f.submit();
	}
}
// handle form cancel
function fcancel() {window.location.href="/vp";}
// print html form
w((program_index>-1)?"<h4>Modify Program "+(program_index+1)+":</h4>":"<h4>Add a New Program</h4>");
w("<form name=mf action=cp method=get><input type=hidden name=p><input type=hidden name=i><input type=hidden name=v>");
w("<b>Days:</b><br>");
w("<input type=radio name=days id=days_week>Weekday:<input type=checkbox id=d0>Mon<input type=checkbox id=d1>Tue<input type=checkbox id=d2>Wed<input type=checkbox id=d3>Thu<input type=checkbox id=d4>Fri<input type=checkbox id=d5>Sat<input type=checkbox id=d6>Sun<br>")
w("<input type=radio name=days id=days_odd>Odd days<br><input type=radio name=days id=days_even>Even days<br><input type=radio name=days id=days_n>Every <input type=text size=2 id=dn> days<hr>");
w("<b>Stations:</b> (click to select)<br>");
var i,s;
var sid;
for(i=0;i<nboards;i++) {
	for(s=0;s<8;s++) {
		sid=i*8+s;
		w("<input type=checkbox id=s"+sid+">S"+((sid+1)/10>>0)+((sid+1)%10));
	}
	w("<br>");
}
w("<hr><b>Time</b>: <input type=text size=2 id=tsh> : <input type=text size=2 id=tsm> -> <input type=text size=2 id=teh> : <input type=text size=2 id=tem> (hh:mm)<br><b>Every</b>: <input type=text size=2 id=tih> hours <input type=text size=2 id=tim> minutes <br><b>Duration</b>: <input type=text size=2 id=tdm> minutes <input type=text size=2 id=tds> seconds<hr></form>");
w("<button style=\"height:32\" onclick=\"fsubmit(mf)\">Submit</button><button style=\"height:32\" onclick=\"fcancel()\">Cancel</button>");
// default values
id("days_week").checked=true;
id("dn").value="3";
id("tsh").value="06";id("tsm").value="00";id("teh").value="18";id("tem").value="00";
id("tih").value="04";id("tim").value="30";id("tdm").value="05";id("tds").value="15";
// fill in existing program values
if(program_index>-1) {
	// process days
	var _days=programdata[0];
	if(_days&0x80) {
		_days=_days&0x7f;
		if(_days==0x00) {id("days_even").checked=true;}
		else if(_days==0x01) {id("days_odd").checked=true;}
		else {id("days_n").checked=true;id("dn").value=_days;}
	} else {
		id("days_week").checked=true;
		for(i=0;i<7;i++) {if(_days&(1<<i))	id("d"+i).checked=true;}
	}
	// process time
	fill_time("ts",1);
	fill_time("te",2);
	fill_time("ti",3);
	var t=programdata[4];
	id("tdm").value=""+((t/60>>0)/10>>0)+((t/60>>0)%10);
	id("tds").value=""+((t%60)/10>>0)+((t%60)%10);
	// process stations
	var bits;
	for(i=0;i<nboards;i++) {
		bits=programdata[i+5];
		for(s=0;s<8;s++) {sid=i*8+s;id("s"+sid).checked=(bits&(1<<s)?true:false);}
	}
}
