function id(s) {return document.getElementById(s);}
function snf(sid,sbit) {
  if(sbit==1) window.location="/sn"+(sid+1)+"=0"; // turn off station
  else {
    var strmm=id("mm"+sid).value, strss=id("ss"+sid).value;
    var mm=(strmm=="")?0:parseInt(strmm);
    var ss=(strss=="")?0:parseInt(strss);
    if(!(mm>=0&&ss>=0&&ss<60))  {alert("Timer values wrong: "+strmm+":"+strss);return;}
    window.location="/sn"+(sid+1)+"=1"+"&t="+(mm*60+ss);  // turn it off with timer
  }
}
w("<hr>");
w("<span style=\"line-height:36px\"><b>Manual Control:</b> (timer is optional)<br>");
var bid,s,sid,sn,rem,remm,rems,sbit;
for(bid=0;bid<nbrd;bid++){
  for(s=0;s<8;s++){
    sid=bid*8+s;
    sn=sid+1;
    w("Station "+(sn/10>>0)+(sn%10)+": ");
    if(sn==mas) {w(((sbits[bid]>>s)&1?("<b>On</b>").fontcolor("green"):("Off").fontcolor("black"))+" (<b>Master</b>)");}
    else {
      rem=ps[sid][2];remm=rem/60>>0;rems=rem%60;sbit=(sbits[bid]>>s)&1;
      var bg=(sbit?"#FFCCCC":"CCFFCC"),tx=(sbit?"off":"on"),dis=(sbit?"disabled":"");
      w("<button style=\"width:100px;height:32px;background-color:"+bg+";border-radius:8px;\" id=bb"+sid+" onclick=\"snf("+sid+","+sbit+")\">Turn "+tx+"</button>");
      w(sbit?" in ":" with timer ");
      w("<input type=text id=mm"+sid+" size=2 maxlength=3 value="+remm+" "+dis+" />:");
      w("<input type=text id=ss"+sid+" size=2 maxlength=2 value="+rems+" "+dis+" /> (mm:ss)");
    }
    w("<br>");
  }
}
w("</span>");
