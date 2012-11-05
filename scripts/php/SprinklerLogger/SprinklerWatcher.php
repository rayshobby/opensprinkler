<?php 
// Written by David B. Gustavson, dbg@SCIzzL.com , starting October 2012.
date_default_timezone_set('America/Los_Angeles');
$datetime=Date("Y-m-d H:i:s",time());
//printf("Date=".$datetime."\n");
$newSprinklerValveSettings=file_get_contents('http://192.168.1.18/sn0');
//printf("New Sprinklers=".$newSprinklerValveSettings."\n");
$oldSprinklerValveSettings=file_get_contents("/Library/WebServer/Documents/SprinklerPrevious.txt");
//printf("Previous Sprinklers=".$oldSprinklerValveSettings."\n");
if ($newSprinklerValveSettings!=$oldSprinklerValveSettings)
{
//printf("Saw a change\n");
file_put_contents ("/Library/WebServer/Documents/SprinklerChanges.txt", $newSprinklerValveSettings."--".$datetime."\n",FILE_APPEND);
//printf("Wrote the change\n".$newSprinklerValveSettings."--".$datetime."\n");
file_put_contents ("/Library/WebServer/Documents/SprinklerPrevious.txt", $newSprinklerValveSettings);
//printf("Updated Previous\n");
};

// Use this with the following plist file, saved in /Library/LaunchDaemons/com.scizzl.sprinklerlogger.plist
// and the following terminal commands as needed:
// sudo launchctl load -w com.scizzl.sprinklerlogger.plist
// sudo launchctl list com.scizzl.sprinklerlogger.plist
// sudo launchctl unload com.scizzl.sprinklerlogger.plist

// StartInterval shorter than 11 always seems to result in 11 seconds anyway, so no point.

// The files SprinklerChanges.txt and SprinklerPrevious.txt seem to need to be owned by
// root/wheel for this to work. (This runs even if nobody is logged in, as root.)
// 
// <?xml version="1.0" encoding="UTF-8" (end this line with questionmark greater-than instead of this comment)
// <!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" \
//   "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
// <plist version="1.0">
// <dict>
//   <key>Label</key>
//   <string>com.scizzl.sprinklerlogger.plist</string>
//   <key>ProgramArguments</key>
//     <array>
//     <string>php</string>
//     <string>/Library/WebServer/Documents/SprinklerWatcher.php</string>
//     </array>
//   <key>StartInterval</key>
//     <integer>20</integer>
//   <key>StandardOutPath</key>
//     <string>/var/log/sprinklerjob.log</string>
//   <key>StandardErrorPath</key>
//     <string>/var/log/sprinklerjob.log</string>
//   <key>Debug</key>
//     <true/>
// </dict>
// </plist>

?>