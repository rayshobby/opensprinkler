// Rayshobby.net
// Javascript for printing OpenSprinkler schedule page
// June 2012

// colors to draw different programs
var prog_color=["rgba(0,0,200,0.5)","rgba(0,200,0,0.5)","rgba(200,0,0,0.5)","rgba(0,200,200,0.5)"];
var margin=80,stwidth=30,stheight=120;
var winwidth=stwidth*nboards*8+margin, winheight=24*stheight+margin;
var ctx; // graphics context
function w(s) {document.writeln(s);}
function check_match(sid,simminutes,simdate,simday) {
  // simdate is Java date object, simday is the #days since 1970 01-01
  var i,bid=(sid>>3),s=(sid%8),prog,wd,dn,drem;
  for(i=0;i<nprogs;i++) {
    prog=pd[i];
    if((prog[6+bid]&(1<<s))==0) continue; // station bit checking
    if ((prog[0]&0x80)&&(prog[1]>1)) {  // inverval checking
      dn=prog[1];drem=prog[0]&0x7f;
      if((simday%dn)!=((devday+drem)%dn)) continue; // remainder checking
    } else {
      wd=(simdate.getDay()+6)%7; // getDay assumes sunday is 0, converts to Monday 0
      if((prog[0]&(1<<wd))==0)  continue; // weekday checking
      dt=simdate.getDate(); // day of the month
      if((prog[0]&0x80)&&(prog[1]==0))  {if((dt%2)!=0)  continue;} // even day checking
      if((prog[0]&0x80)&&(prog[1]==1))  { // odd day checking
        if(dt==31)  continue;
        else if (dt==29 && simdate.getMonth()==1) continue;
        else if ((dt%2)!=1) continue;
      }
    }
    if(simminutes<prog[2] || simminutes>prog[3])  continue; // start and end time checking
    if(prog[4]==0)  continue;
    if(((simminutes-prog[2])/prog[4]>>0)*prog[4] == (simminutes-prog[2])) { // interval checking
      return [prog[5], i];
    }
  }
  return [0,-1];  // no match found
}
function getx(sid)  {return margin+sid*stwidth-stwidth/2;}  // x coordinate given a station
function gety(t)    {return 60+t*stheight/60;}  // y coordinate given a time
function plot_bar(sid,simminutes,off_seconds,dur,pid) { // plot program bar
  ctx.fillStyle=prog_color[pid%4];  // select color
  off_seconds=(simminutes*60+off_seconds);  // starting time in seconds
  ctx.fillRect(getx(sid),gety(off_seconds/60),stwidth,(dur/60)*stheight/60);  // plot bar
  ctx.fillStyle="black";
  ctx.fillText("P"+(pid+1), getx(sid)+stwidth/2, gety(off_seconds/60)+6); // plot program name
}
function plot_master(sid,simminutes,runtime) {  // plot master station
  ctx.fillStyle="rgba(64,64,64,0.5)";
  ctx.fillRect(getx(sid),gety(simminutes),stwidth,(runtime)*stheight/60);
}
function plot_currtime() {
  ctx.strokeStyle="rgba(200,0,0,0.5)";
  ctx.beginPath();
  ctx.moveTo(margin-stwidth/2,gety(devmin));
  ctx.lineTo(winwidth,gety(devmin));
  ctx.stroke();
}
function plot_sched(simminutes,dur_array,pid_array,et_array) { // plot schedule stored in array data
  var sid,total_seconds=0;
  for(sid=0;sid<nboards*8;sid++) {
    if((dur_array[sid]!=0)&&(pid_array[sid]!=-1)) {
      if(seq==1) {  // sequential 
        plot_bar(sid,simminutes,total_seconds,dur_array[sid],pid_array[sid]);
        total_seconds+= dur_array[sid]; // accumulate duration
      } else {  // concurrent
        // this program has just started running
        if(et_array[sid] == simminutes+(dur_array[sid]/60)) {
          plot_bar(sid,simminutes,0,dur_array[sid],pid_array[sid]);
        }
        total_seconds=60;
      }
    }
  }
  return total_seconds;
}
function draw() {
  var canvas=document.getElementById("canvas");
  ctx=canvas.getContext("2d");  // get draw context
  var sid,sn,t;
  var simdate = new Date(yy,mm-1,dd,0,0,0); // Java Date object, assumes month starts from 0
  var simday = (simdate.getTime()/1000/3600/24)>>0;
  // draw table and grid
  ctx.fillStyle="black";
  ctx.textAlign="center";
  ctx.textBaseline="middle";
  ctx.font="14px Monospace";
  ctx.fillText("Program Preview of "+(simday==devday?"Today":simdate.toDateString()),winwidth/2+20,8);
  ctx.font="11px Arial";
  ctx.fillText("(Warning: plot does not show dynamic changes)",winwidth/2+20,24);
  // vertical grid, stations
  ctx.beginPath();
  for(sid=0;sid<nboards*8;sid++) {
    sn=sid+1;
    ctx.fillText("S"+(sn/10>>0)+(sn%10), margin+sid*stwidth, margin/2);
    ctx.moveTo(getx(sid), 30);
    ctx.lineTo(getx(sid), winheight);
  }
  ctx.moveTo(getx(sid), 30);
  ctx.lineTo(getx(sid), winheight);
  // horizontal grid, time
  for(t=0;t<24;t++) {
    ctx.fillText(""+(t/10>>0)+(t%10)+":00", margin/2, 60+t*stheight);
    ctx.moveTo(margin-stwidth/2-10,gety(t*60));
    ctx.lineTo(margin-stwidth/2,gety(t*60));
  }
  ctx.stroke();
  if(simday==devday)  plot_currtime();
  // plot program data by a full simulation
  var simminutes=0,busy=0,match_found=0,bid,s,sid,match=[0,0];
  var dur_array=new Array(nboards*8),pid_array=new Array(nboards*8);
  var et_array=new Array(nboards*8);
  for(sid=0;sid<nboards*8;sid++)  {
    dur_array[sid]=0;pid_array[sid]=-1;et_array[sid]=-1;
  }
  do { // check through every station
    busy=0;
    match_found=0;
    for(bid=0;bid<nboards;bid++) {
      for(s=0;s<8;s++) {
        sid=bid*8+s;
        if (mas==sid+1) continue; // skip master station
        if (simminutes<et_array[sid]) {busy=1;continue;} // skip stations currently running
        match=check_match(sid,simminutes,simdate,simday);
        if(match[0]!=0) {
          dur_array[sid]=match[0];pid_array[sid]=match[1];
          et_array[sid]=simminutes+match[0]/60;
          match_found=1;
        }//if
      }//for_s
    }//for_i
    var elapsed_time=0;
    if (match_found) {
      elapsed_time=(plot_sched(simminutes,dur_array,pid_array,et_array)/60>>0);
      if(mas>0) plot_master(mas-1,simminutes,elapsed_time);
      simminutes+=elapsed_time;
      for(sid=0;sid<nboards*8;sid++)  {dur_array[sid]=0;pid_array[sid]=-1;} // clear program data
    } else {
      elapsed_time=1;
      if(mas>0&&busy) plot_master(mas-1,simminutes,elapsed_time);
      simminutes++; // increment simulation time
    }
  } while(simminutes<24*60); // simulation ends
  if(simday==devday)  window.scrollTo(0,gety((devmin/60>>0)*60)); // scroll to the hour line cloest to the current time
}
w("<body onload=\"draw()\">");
w("<canvas id=\"canvas\" width=\""+winwidth+"\" height=\""+winheight+"\"></canvas>");
w("</body>");

