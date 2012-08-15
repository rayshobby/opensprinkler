// Javascript for printing OpenSprinkler schedule page
// Rayshobby.net
// June 2012
var str_days=["Mon","Tue","Wed","Thur","Fri","Sat","Sun"];
function w(s) {document.writeln(s);}
function imgstr(s) {return "<img src=\"http://rayshobby.net/images/icons/svc_"+s+".png\" height=20 align=absmiddle>&nbsp;";}
function del(form,idx) {var p=prompt("Please enter your password:","");if(p!=null){form.elements[0].value=p;form.elements[1].value=idx;form.submit();}}
function mod(form,idx) {form.elements[0].value=idx;form.submit();}
// parse and print days
function pdays(days){
  if((days[0]&0x80)&&(days[1]>1)){
    // this is an interval program 
    days[0]=days[0]&0x7f;
    w("Every "+days[1]+" days, starting in "+days[0]+" days.");
  } else {
    // this is a weekly program 
    for(d=0;d<7;d++) {if(days[0]&(1<<d)) {w(str_days[d]);}}
    if((days[0]&0x80)&&(days[1]==0))  {w("(Even days only)");}
    if((days[0]&0x80)&&(days[1]==1))  {w("(Odd days only)");}
  }
}
// parse and print stations
function pstations(data){
  w("<table border=1><tbody>");
  var bid,s,bits,sn;
  for(bid=0;bid<nboards;bid++){
    w("<tr>");
    bits=data[bid+6];
    for(s=0;s<8;s++){
      sn=bid*8+s+1;
      if(bits&(1<<s)) w("<td style=background-color:#9AFA9A>S"+(sn/10>>0)+(sn%10)+"</td>");
      else w("<td>S"+(sn/10>>0)+(sn%10)+"</td>");
    }
    w("</tr>");
  } 
  w("</tbody></table>\n");
}

w("<h4><a href=/><-Home</a></h4>");
w("<form name=df action=dp method=get><input type=hidden name=pw><input type=hidden name=pid></form>");
w("<form name=mf action=mp method=get><input type=hidden name=pid></form>")
w("<form name=gf action=gp method=get target=\"_blank\"><input type=hidden name=d value=today></form>")
w("<button style=\"height:36\" onclick=mod(mf,-1)>"+imgstr("addall")+"Add a New Program</button>");
w("<button style=\"height:36\" onclick=del(df,-1)>"+imgstr("delall")+"Delete All</button>");
w("<button style=\"height:36\" onclick=\"gf.submit()\">"+imgstr("preview")+"Preview</button><br><hr>");
if(nprogs==0) w("(Empty)");
// print programs
var pid,st,et,iv,du;
for(pid=0;pid<nprogs;pid++) {
  w("<br><b>Program "+(pid+1)+": ");
  // parse and print days
  pdays([pd[pid][0],pd[pid][1]]);
  w("</b>");
  // print time
  st=pd[pid][2];
  et=pd[pid][3];
  iv=pd[pid][4];
  du=pd[pid][5];
  w("<br><b>Time</b>: "+((st/60>>0)/10>>0)+((st/60>>0)%10)+":"+((st%60)/10>>0)+((st%60)%10));
  w(" - "+((et/60>>0)/10>>0)+((et/60>>0)%10)+":"+((et%60)/10>>0)+((et%60)%10));
  w("<br><b>Every</b> "+(iv/60>>0)+" hours "+(iv%60)+" minutes");
  w("<br><b>Run</b>: "+(du/60>>0)+" minutes "+(du%60)+" seconds");
  // parse and print stations
  w("<br><b>Stations applied</b>:<br>");
  pstations(pd[pid]);
  // print buttons
  w("<br><button style=\"height:28\" onclick=del(df,"+pid+")>Delete</button>");
  w("<button style=\"height:28\" onclick=mod(mf,"+pid+")>Modify</button>");
  w("<hr>");
}
