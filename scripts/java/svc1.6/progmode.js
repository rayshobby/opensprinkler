// print station status
w("<form name=gf action=gp method=get target=\"_blank\"><input type=hidden name=d value=today></form>")
w("<hr><button style=\"height:36\" onclick=\"gf.submit()\">"+imgstr("preview")+"Program Preview</button><br>");
w("<p><b>Station Status:</b></p><span style=\"line-height:22px\">");
var bid,s,sid,sn,rem,remm,rems,off;
off=((en==0||rd!=0||(urs!=0&&rs!=0))?1:0);
for(bid=0;bid<nbrd;bid++){
  for(s=0;s<8;s++){
    sid=bid*8+s;
    sn=sid+1;
    w("Station "+(sn/10>>0)+(sn%10)+": ");
    if(off) w("<strike>");
    if(sn==mas) {w(((sbits[bid]>>s)&1?("<b>On</b>").fontcolor("green"):("Off").fontcolor("black"))+" (<b>Master</b>)");}
    else {
      rem=ps[sid][2];remm=rem/60>>0;rems=rem%60;
      if((sbits[bid]>>s)&1) {
        w(("<b>Running P"+ps[sid][0]).fontcolor("green")+"</b> ("+(remm/10>>0)+(remm%10)+":"+(rems/10>>0)+(rems%10)+" remaining)");
      } else {
        if(rem==0) w("-");
        else w(("Waiting P"+ps[sid][0]+" ("+(remm/10>>0)+(remm%10)+":"+(rems/10>>0)+(rems%10)+" scheduled)").fontcolor("gray"));
      }
    }
    if(off) w("</strike>");
    w("<br>");
  }
}

