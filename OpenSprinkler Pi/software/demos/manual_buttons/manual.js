function id(s) {return document.getElementByid(s);}
function w(s) {document.write(s);}
function action_cv(i,v) {window.location=('./cv?sid='+i+'&v='+v);}
w('<h3><b>OpenSprinkler Pi Manual Mode</b></h3>');
var i,s;
for(i=0;i<nstations;i++) {
   w('<p>Station '+((i+1)/10>>0)+((i+1)%10)+': <button style=\"width:100;height:28;background-color:');
   s=values[i];
   if(s) {
       w('lightgreen\" onclick=action_cv('+i+',0)>Turn Off');
   } else {
       w('lightgray\" onclick=action_cv('+i+',1)>Turn On');
   }
   w('</button></p>');
}
