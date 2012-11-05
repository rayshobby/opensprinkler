<html>
<head>
<meta http-equiv="content-type" content="text/html; charset=us-ascii" />
<meta http-equiv="refresh" content="20;url=Sprinklers.php">
<style type="text/css">
body{font-family:"Courier"}</style>
<title>Sprinkler Logs</title>
</head>
<body>
<?php  
//Written by David B. Gustavson, 20121021
$SprinklerValveHistory=file_get_contents("SprinklerChanges.txt");
date_default_timezone_set('America/Los_Angeles');

// Append the current state onto the changes file
$newSprinklerValveSettings=file_get_contents('http://192.168.1.18/sn0');
$datetime=Date("Y-m-d H:i:s",time());
$SprinklerValveHistory.=$newSprinklerValveSettings."--".$datetime."\n";
$timeViewWindow='7 days'; // How far back to look '24 hours' or '14 days'
$timeEarliest=strtotime(Date("Y-m-d H:i:s",strtotime("-".$timeViewWindow,time())));
$Lines=explode("\n",$SprinklerValveHistory);

$ValveName=array(
"V01=Myrtle rhs Unused",
"V02=Pear, Myrtle",
"V03=Myrtle left Unused",
"V04=Cmt Top Left Unused",
"V05=Windmill Drip",
"V06=Cline North",
"V07=Garden Drip",
"V08=Unused",
"V09=Fr by Hydrant",
"V10=Fr North",
"V11=Fr Mailbox",
"V12=Fr Walks",
"V13=Cline South Unused",
"V14=Unused",
"V15=Unused",
"V16=Unused"
);
 for ($i=0;$i<count($Lines);$i++){
	$ELines[$i]=explode("--",$Lines[$i]);
	if (count($ELines[$i])>1){// Skip lines that aren't formatted as records, e.g. comments
		$timeThis=strtotime($ELines[$i][1]);
		if ($timeThis>$timeEarliest){// Ignore lines that aren't recent
			$SprinklerPattern[]=str_split($ELines[$i][0]);
			$SprinklerTime[]=$ELines[$i][1];
			$SprinklerTimeConverted[]=strtotime($ELines[$i][1]);
		};
	};
 };

for ($i=0;$i<count($SprinklerPattern);$i++){
  $ResultLine=" ";
   for ($j=0;$j<16;$j++){
    if (($i>0)&&($SprinklerPattern[$i-1][$j]=="1")&&($SprinklerPattern[$i][$j]=="0")||
      ($i==count($SprinklerPattern)-1)&&($SprinklerPattern[$i][$j]=="1"))
    {$TimeNow=$SprinklerTimeConverted[$i]; $TimeBegin=$TimeNow;
      for ($k=1;$k<$i;$k++)
      {
       if ($SprinklerPattern[$i-$k][$j]=="1"){$TimeBegin=$SprinklerTimeConverted[$i-$k];}else{break;};
      };
     $TimeElapsed=$TimeNow-$TimeBegin;

$ResultLine.=" ".$ValveName[$j].((($i==count($SprinklerPattern)-1)&&($SprinklerPattern[$i][$j]=="1"))?" has been on for ":" was on for ").
     $TimeElapsed." seconds.  ";
     
     $ValveHistory[$j][]= array($SprinklerTime[$i], $TimeElapsed, ((($i==count($SprinklerPattern)-1)&&($SprinklerPattern[$i][$j]=="1"))?" Running Now":""));
    };
   };
//  echo $SprinklerPattern[$i][0].$SprinklerPattern[$i][1].$SprinklerPattern[$i][2].$SprinklerPattern[$i][3]. $SprinklerPattern[$i][4].$SprinklerPattern[$i][5].$SprinklerPattern[$i][6].$SprinklerPattern[$i][7]. $SprinklerPattern[$i][8].$SprinklerPattern[$i][9].$SprinklerPattern[$i][10].$SprinklerPattern[$i][11]. $SprinklerPattern[$i][12].$SprinklerPattern[$i][13].$SprinklerPattern[$i][14].$SprinklerPattern[$i][15]. " @ ".$SprinklerTime[$i]."==".$SprinklerTimeConverted[$i].$ResultLine."<br/>";
};  
echo "<table><caption><font size=5><b>Valve Operations, Date/Time <br/>(last $timeViewWindow)</b></font></caption>";
// echo "<tr><th>Valve Operations</th><th>Date/Time (last $timeViewWindow)</th></tr>";
for ($j=0;$j<16;$j++)
{
 $ct=count($ValveHistory[$j]);
 if ($ct>0) {
 echo "\n<tr><td><b>".$ValveName[$j].",</b></td><td><b>".$ct." run period".($ct>1?"s":"").", ending at</b></td></tr>";
 for ($k=0;$k<count($ValveHistory[$j]);$k++){
//  $thatDate=getdate(strtotime($ValveHistory[$j][$k][0]));
  $theTime=date_format(date_create($ValveHistory[$j][$k][0]), 'D, M j, Y, g:i A');
echo "\n<tr><td>".number_format(($ValveHistory[$j][$k][1]/60), 1, '.', ',')." min</td><td>".$theTime.$ValveHistory[$j][$k][2]."</td></tr>";
 };
 };
};
echo "</table>";

// Now get the information about near-term FUTURE SCHEDULED WATERING:
$schedule=file_get_contents("http://192.168.1.18/gp?d=0");
$schedulex=preg_replace("#.*(mas=\d+),(.*);</script>.*#s", "var $1, $2;\n", $schedule);

$status=file_get_contents("http://192.168.1.18/");
$statusx=preg_replace("#.*(ver=\d+),(devt=\d+);.*(tz=\d+),.*(en=\d+),(rd=\d+),(rs=\d+),mm=(\d+),(rdst=\d+),.*,(urs=\d+),(wl=\d+),.*,(loc=.+?\");.*#s", "var $1, $2, $3, $4, $5, $6, ManualMode=$7, $8, $9, $10;\n", $status);

$schedsummary="";
$tdays=7; //  This sets the number of days to look ahead
for ($t=0;$t<$tdays;$t++){ 
	$tsch=Date("Y-m-d, l",strtotime("$t days",time()));
	$year=substr($tsch,0,4);
	$month=substr($tsch,5,2);
	$monthm1=$month-1;
	$day=substr($tsch,8,2);
	$dayname=substr($tsch,12);
	$schedsumm[]=$schedulex.$statusx."\nvar simdate = new Date($year,$monthm1,$day,0,0,0); // Java Date object, assumes month starts from 0\nvar simday = (simdate.getTime()/1000/3600/24)>>0;\n";
	$scheddate[]=$tsch;
	$schedsummary.=$tsch."\n".$schedulex.$statusx."\n";
};
// For debugging
//file_put_contents ("/Library/WebServer/Documents/SprinklerSchedule.txt", $schedule."\n\nSchedule Extracted\n".$schedulex."\n\n Status=\n".$status."\n\n Statusx=\n".$statusx."\n\nResult:".$schedulex."\n".$statusx."\nvar simdate = new Date($year,$monthm1,$day,0,0,0); // Java Date object, assumes month starts from 0\nvar simday = (simdate.getTime()/1000/3600/24)>>0;\n");
?>
<script>
// Javascript for printing OpenSprinkler schedule page
// Firmware v1.8
// All content is published under:
// Creative Commons Attribution ShareAlike 3.0 License
// Sep 2012, Rayshobby.net
// Oct 2012, modified for use by David Gustavson in logging polling display
// colors to draw different programs
var prog_color=["rgba(0,0,200,0.5)","rgba(0,200,0,0.5)","rgba(200,0,0,0.5)","rgba(0,200,200,0.5)"];
var rain_color="<font color='black'>";
var xstart=80,ystart=80,stwidth=40,stheight=180;
var winwidth=stwidth*nboards*8+xstart, winheight=26*stheight+ystart;
var sid,sn,t;
var simdate = new Date(yy,mm-1,dd,0,0,0); // Java Date object, assumes month starts from 0
var simday = (simdate.getTime()/1000/3600/24)>>0;
function w(s) {document.writeln(s);}
function check_match(prog,simminutes,simdate,simday) {
  // simdate is Java date object, simday is the #days since 1970 01-01
  var wd,dn,drem;
  if(prog[0]==0)  return 0;
  if ((prog[1]&0x80)&&(prog[2]>1)) {  // inverval checking
    dn=prog[2];drem=prog[1]&0x7f;
    if((simday%dn)!=((devday+drem)%dn)) return 0; // remainder checking
  } else {
    wd=(simdate.getDay()+6)%7; // getDay assumes sunday is 0, converts to Monday 0
    if((prog[1]&(1<<wd))==0)  return 0; // weekday checking
    dt=simdate.getDate(); // day of the month
    if((prog[1]&0x80)&&(prog[2]==0))  {if((dt%2)!=0)  return 0;} // even day checking
    if((prog[1]&0x80)&&(prog[2]==1))  { // odd day checking
      if(dt==31)  return 0;
      else if (dt==29 && simdate.getMonth()==1) return 0;
      else if ((dt%2)!=1) return 0;
    }
  }
  if(simminutes<prog[3] || simminutes>prog[4])  return 0; // start and end time checking
  if(prog[5]==0)  return 0;
  if(((simminutes-prog[3])/prog[5]>>0)*prog[5] == (simminutes-prog[3])) { // interval checking
    return 1;
  }
  return 0;  // no match found
}
function getrunstr(start,end){ // run time string
  var h,m,s,str;
  h=start/3600>>0;m=(start/60>>0)%60;s=start%60;
  str=""+(h/10>>0)+(h%10)+":"+(m/10>>0)+(m%10)+":"+(s/10>>0)+(s%10);
  h=end/3600>>0;m=(end/60>>0)%60;s=end%60;
  str+="->"+(h/10>>0)+(h%10)+":"+(m/10>>0)+(m%10)+":"+(s/10>>0)+(s%10);
  return str;
} 
function plot_bar(sid,start,pid,end) { // plot program bar
    if ((rd!==0)&&(simdate.getTime()/1000+start+(tz-48)*900<=rdst)){rain_color="<font color='red'>";rain_skip="Skip";}else{rain_color="<font color='black'>";rain_skip="";};
  w("<tr><td>"+rain_color+snames[sid]+"</td><td>"+rain_color+getrunstr(start,end)+"</td><td align='center'>"+rain_color+"P"+pid+"</td><td align='center'>"+rain_color+((end-start)/60>>0)+"  "+rain_skip+"</td></tr>");
}
function run_sched(simseconds,st_array,pid_array,et_array) { // run and plot schedule stored in array data
  var sid,endtime=simseconds;
  for(sid=0;sid<nboards*8;sid++) {
    if(pid_array[sid]) {
      plot_bar(sid,st_array[sid],pid_array[sid],et_array[sid]);
      if((mas>0)&&(mas!=sid+1)&&(masop[sid>>3]&(1<<(sid%8))))
        plot_master(st_array[sid]+mton, et_array[sid]+mtoff-60);
      endtime=et_array[sid];
    }
  }
  return endtime;
}
function draw_program() {
  // plot program data by a full simulation
  var simminutes=0,busy=0,match_found=0,bid,s,sid,pid,match=[0,0];
  var st_array=new Array(nboards*8),pid_array=new Array(nboards*8);
  var et_array=new Array(nboards*8);
  for(sid=0;sid<nboards*8;sid++)  {
    st_array[sid]=0;pid_array[sid]=0;et_array[sid]=0;
  }
  do { // check through every program
    busy=0;
    match_found=0;
    for(pid=0;pid<nprogs;pid++) {
      var prog=pd[pid];
      if(check_match(prog,simminutes,simdate,simday)) {
        for(sid=0;sid<nboards*8;sid++) {
          bid=sid>>3;s=sid%8;
          if(mas==(sid+1)) continue; // skip master station
          if(prog[7+bid]&(1<<s)) {
            et_array[sid]=prog[6]*wl/100>>0;pid_array[sid]=pid+1;
            match_found=1;
          }//if
        }//for_sid
      }//if_match
    }//for_pid
    if(match_found) {
      var acctime=simminutes*60;
      for(sid=0;sid<nboards*8;sid++) {
        if(et_array[sid]) {
          st_array[sid]=acctime;acctime+=et_array[sid];
          et_array[sid]=acctime;acctime+=sdt;
          busy=1;
        }//if
      }//for
    } 
    if (busy) {
      var endminutes=run_sched(simminutes*60,st_array,pid_array,et_array)/60>>0;
      if(simminutes!=endminutes) simminutes=endminutes;
      else simminutes++;
      for(sid=0;sid<nboards*8;sid++)  {st_array[sid]=0;pid_array[sid]=0;et_array[sid]=0;} // clear program data
    } else {
      simminutes++; // increment simulation time
    }
  } while(simminutes<24*60); // simulation ends
//  if(simday==devday)  window.scrollTo(0,gety((devmin/60>>0)*60)); // scroll to the hour line cloest to the current time
}
<?php 
$varsnames="'";
for ($k=0;$k<count($ValveName);$k++){$varsnames.=$ValveName[$k]."','";};
echo "var snames=[$varsnames'];\n\n\n";
echo $schedsumm[0];
echo 'function datestr(t) {var _t=tz-48; return (new Date(t)).toUTCString()+((_t>=0)?"+":"-")+(Math.abs(_t)/4>>0)+":"+((Math.abs(_t)%4)*15/10>>0)+((Math.abs(_t)%4)*15%10);}'."\n";
echo "w(\"<p><font size=5><b>Scheduled within the next $tdays days:</b></font>\");\n";
// print status and other information
echo 'if(ver>=100) w("<br><b>Firmware version</b>: "+(ver/100>>0)+"."+((ver/10>>0)%10)+"."+(ver%10)+"<br>");';
echo 'else w("<br><b>Firmware version</b>: "+(ver/10>>0)+"."+(ver%10)+"<br>");';
echo 'w("<b>Device time</b>: "+datestr(devt*1000));';
echo 'w("<br><b>Operation</b>: "+(en?("on").fontcolor("green"):("OFF").fontcolor("red")));';
echo 'w("<br><b>Raindelay</b>: "+(rd?("ON").fontcolor("red")+" (blocks watering until "+datestr(rdst*1000)+")":("off").fontcolor("black")));';
echo 'w("<br><b>Rainsense</b>: "+(urs?(rs?("Rain Detected").fontcolor("red"):("no rain").fontcolor("green")):"<font color=gray>n/a</font>"));';
echo 'w("<br><b>Water level</b>: <font color="+((wl==100)?"green":"red")+">"+wl+"\%</font><p>");';

for ($t=0;$t<count($schedsumm);$t++) {
echo $schedsumm[$t];
echo "w(\"<table><caption><b>Scheduled for $scheddate[$t]:</b></caption><tr><th width=150>Valve</th><th width=150>Time</th><th>Program</th><th>Minutes</th></tr>\"\n);";
echo "draw_program();";
echo "w(\"</table>--------------------------------------------------<p>\");\n\n";
};

?>
</script>
</body>
</html>
