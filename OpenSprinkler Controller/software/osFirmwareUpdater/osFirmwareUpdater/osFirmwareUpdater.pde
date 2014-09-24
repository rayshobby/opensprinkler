/* OpenSprinkler GUI-based Firmware Update Tool
 * Rayshobby LLC
 * http://rayshobby.net
 *
 * Initially written by Jonathan Goldin
 * Refined by Ray Wang
 * Published under Creative Commons CC-SA 3.0 license
 */
import java.net.*;
import g4p_controls.*;
import java.io.InputStreamReader;
import processing.serial.*;
//import java.awt.Cursor;
import java.awt.*;
//import java.awt.BorderLayout;

import javax.swing.JDialog;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JProgressBar;
import javax.swing.JOptionPane;

int FRAME_CONFIG_STARTX = 0;
int FRAME_CONFIG_STARTY = 0;
int WINDOW_WIDTH = 300;
int WINDOW_HEIGHT = 435;

final static boolean RELEASE = true;

GDropList dropListDevice;
GDropList dropListFirmwareVersion;

GButton buttonUpload;
GButton buttonREADME;
GButton buttonGetLatestFirmware;
GButton buttonDetectDeviceHardware;
GButton buttonUpdate;

GLabel lblDevice;
GLabel lblFirmware;
GLabel lblHardwareDesc;
GLabel lblBootloading;
GLabel lblURL;

GTextArea description;
//GTextArea bootloadInstruc;
GTextArea result;

boolean WINDOWS = false;
boolean MACOS = false;
boolean LINUX = false;

ArrayList<String> deviceArray;
ArrayList<String> descriptionArray;
ArrayList<String> bootloaderArray;
ArrayList<String> optionArray;
ArrayList<String[]> firmwareArrays;

Serial device = null;
int currentDevice;
boolean toUpload = false;
boolean uploadNow = false;
boolean toReadme = false;
Font f;

public void setup(){
  size(WINDOW_WIDTH,WINDOW_HEIGHT);
  G4P.setGlobalColorScheme(5);
  
  deviceArray = new ArrayList<String>();
  descriptionArray = new ArrayList<String>();
  bootloaderArray = new ArrayList<String>();
  optionArray = new ArrayList<String>();
  firmwareArrays = new ArrayList<String[]>();
  
  String[] file = null;
  int line = 0;
  int curDevice = 0;  
//  String CurrentDataPath = dataPath("");
//  String CurrentRootPath = CurrentDataPath.substring(0,CurrentDataPath.indexOf("osFirmwareUpdater")+18);
  file = loadStrings("../os_firmware_info.txt");
  if (file == null) {
    System.out.println("Cannot open os_firmware_info.txt");
    System.exit(0); 
  }
    
  while(line < file.length){
    deviceArray.add(file[line]);
    line+=2;
    descriptionArray.add(file[line]);
    line+=2;
    bootloaderArray.add(file[line]);
    line+=2;
    int firmwareCount = Integer.parseInt(file[line]);
    firmwareArrays.add(new String[firmwareCount]);
    line+=2;
    for(int i = 0; i < firmwareCount; i++){
      firmwareArrays.get(curDevice)[i] = file[line];
      line++;
    }
    line++;
    optionArray.add(file[line]);
    line+=2;
    curDevice++;
  }
  f = new Font("Dialog",Font.PLAIN,14);
  lblDevice = new GLabel(this,FRAME_CONFIG_STARTX, FRAME_CONFIG_STARTY + 65,100,20);
  lblDevice.setFont(f);
  lblDevice.setText("Hardware:");
  lblDevice.setTextBold();
  
  lblHardwareDesc = new GLabel(this,FRAME_CONFIG_STARTX, FRAME_CONFIG_STARTY + 100, 110, 20);
  lblHardwareDesc.setFont(f);
  lblHardwareDesc.setText("Description:");
  lblHardwareDesc.setTextBold();

  lblFirmware = new GLabel(this,FRAME_CONFIG_STARTX, FRAME_CONFIG_STARTY + 190,100,20);
  lblFirmware.setFont(f);
  lblFirmware.setText("Firmware:");
  lblFirmware.setTextBold();
  
  lblBootloading = new GLabel(this,FRAME_CONFIG_STARTX, FRAME_CONFIG_STARTY + 220, 300, 20);
  lblBootloading.setFont(f);
  lblBootloading.setText("For release notes, please visit:");
//  lblBootloading.setTextBold();
  
  lblURL = new GLabel(this,FRAME_CONFIG_STARTX,FRAME_CONFIG_STARTY+ 235,300,20);
  lblURL.setFont(f);
  lblURL.setText("www.opensprinkler.com");
//  lblURL.setTextBold();
  
  dropListDevice = new GDropList(this,FRAME_CONFIG_STARTX + 100, FRAME_CONFIG_STARTY+ 60,180,150);
  dropListDevice.setFont(f);
  dropListDevice.setItems(deviceArray.toArray(new String[curDevice]),0);
  dropListDevice.setSelected(0);

  currentDevice = dropListDevice.getSelectedIndex();

  dropListFirmwareVersion = new GDropList(this,FRAME_CONFIG_STARTX+100, FRAME_CONFIG_STARTY + 185,180,150);
  dropListFirmwareVersion.setFont(f);
  dropListFirmwareVersion.setItems(firmwareArrays.get(currentDevice),0);
  dropListFirmwareVersion.setSelected(0);
  
  description = new GTextArea(this,FRAME_CONFIG_STARTX, FRAME_CONFIG_STARTY+120,300,60);
  description.setFont(f);
  description.setText(descriptionArray.get(currentDevice));
  description.setTextEditEnabled(false);
/*  
  bootloadInstruc = new GTextArea(this,FRAME_CONFIG_STARTX, FRAME_CONFIG_STARTY + 240, 300,130);
  bootloadInstruc.setFont(f);
  bootloadInstruc.setText(bootloaderArray.get(currentDevice));
  bootloadInstruc.setTextEditEnabled(false);
*/  
  buttonUpload = new GButton(this,FRAME_CONFIG_STARTX+75, FRAME_CONFIG_STARTY + 265,150,40,"UPLOAD");
  buttonUpload.setFont(f);
  buttonUpload.setLocalColorScheme(1);
  buttonUpload.setTextBold();

  buttonUpdate = new GButton(this,FRAME_CONFIG_STARTX + 50,FRAME_CONFIG_STARTY + 10,200,40,"Update and Detect");
  buttonUpdate.setFont(f);
  buttonUpdate.setLocalColorScheme(1);
  buttonUpdate.setTextBold();

  result = new GTextArea(this,FRAME_CONFIG_STARTX, FRAME_CONFIG_STARTY + 315,300,120, g4p_controls.GConstants.SCROLLBARS_VERTICAL_ONLY );
  result.setFont(f);
  result.setText("After selecting Hardware and Firmware, please click on UPLOAD.");
  result.setTextEditEnabled(false);
  
  String OS = System.getProperty("os.name");
  OS = OS.toLowerCase();
  if(OS.contains("mac")){
    MACOS = true;
  } else if(OS.contains("win")){
    WINDOWS = true;
  } else if(OS.contains("nux")){
    LINUX = true;
  }  
}
boolean firstLoop = true;
public void draw(){
  background(250,250,250);
  if(uploadNow){
    upload();
    uploadNow = false;
  }
  if(toUpload){
    result.setText("uploading, please wait...");
    toUpload = false;
    uploadNow = true;
  }
  if(toReadme){
    readme();
    toReadme = false;
  }
}
public void updateDevice(){
  currentDevice = dropListDevice.getSelectedIndex();
  dropListFirmwareVersion.setItems(firmwareArrays.get(currentDevice),0);
  description.setText(descriptionArray.get(currentDevice));
//  bootloadInstruc.setText(bootloaderArray.get(currentDevice));
  dropListFirmwareVersion.setSelected(0);
}  

public void handleDropListEvents(GDropList list, GEvent event){
  if(list == dropListDevice){
    updateDevice();
  }
}
public void handleButtonEvents(GButton button,GEvent event){
  if(button == buttonUpload){
    toUpload = true;
  }
/*  
  if(button == buttonREADME){
    readme();
  }
  if(button == buttonGetLatestFirmware){
    get_latest_firmware();
  }
  if(button == buttonDetectDeviceHardware){
    detectDevice();
  }
*/
  if(button == buttonUpdate){
    result.setText("");
    get_latest_firmware();
    detectDevice();
    if(currentDevice != 0){
      toReadme = true;
    }
  }
}
public void handleTextEvents(GEditableTextControl textcontrol, GEvent event) {}

void upload(){
//  String CurrentDataPath = dataPath("");
//  String CurrentRootPath = CurrentDataPath.substring(0,CurrentDataPath.indexOf("osFirmwareUpdater")+18);
  if(dropListDevice.getSelectedText().equals("No Device Selected")){
    result.setText("No device selected to upload to.");
    return;
  }
  try{
    uploadNow = false;
    String command = "";
    if(WINDOWS){
      command = "../avr/bin/avrdude -C ../avr/bin/avrdude.conf ";
    }
    if(MACOS){
      command = sketchPath("") + "../avr-macos/bin/avrdude -C "+sketchPath("")+"../avr-macos/etc/avrdude.conf ";
    }
    if(LINUX){
      command = "avrdude ";
    }
    String options = optionArray.get(currentDevice);
    if(deviceArray.get(currentDevice).equals("OpenSprinkler_v2.2")){
      options += " -P " + get_serial_port();
    }
    String flash = " -q -F -U flash:w:";
//    String file = CurrentRootPath;
    String file = "";
    if(MACOS){
      file = sketchPath("");
    }
    file += "../Firmware/";
    file += dropListDevice.getSelectedText();
    file += "/firmware";
    file += dropListFirmwareVersion.getSelectedText();
    file += ".hex";
    if(!fileExists(file)){
      result.setText("Error: file not found at:");
      result.appendText(file);
      return;
    }
    this.setCursor(Cursor.getPredefinedCursor(Cursor.WAIT_CURSOR));
    Process pr = Runtime.getRuntime().exec(command+options+flash+file);
    println(command+options+flash+file);
    BufferedReader in = new BufferedReader(new InputStreamReader(pr.getErrorStream()));
    result.setText("");
    PrintWriter output = createWriter("log.txt");
    String line;
    boolean hasOutput = false;
    while((line = in.readLine()) != null){
      if(line.contains("writing") || line.contains("written") || line.contains("verifying") || line.contains("verified")){
        result.appendText(line.substring(8,line.length()));
        hasOutput = true;
      }
      output.println(line);
    }
    if(!hasOutput){
      result.appendText("Failed.  Check log.txt for full details");
    }
    output.flush();
    output.close();
  } catch (IOException e){
    result.setText(e.toString());
  } finally {
    this.setCursor(Cursor.getDefaultCursor());
  }
}

void detectDevice(){
  boolean usbtiny = false;
  boolean usbasp = false;
  boolean arduino = false;
  boolean v1_x = false;
  try{    
    String command = "";
    if(WINDOWS){
      command = "../avr/bin/avrdude -C ../avr/bin/avrdude.conf ";
    }
    if(MACOS){
      command = sketchPath("") + "../avr-macos/bin/avrdude -C "+sketchPath("")+"../avr-macos/etc/avrdude.conf ";
    }
    if(LINUX){
      command = "avrdude ";
    }
    
    
    String[] commands = new String[optionArray.size()-1];
    for(int i = 1; i < optionArray.size(); i++){
      if(optionArray.get(i).contains("-c arduino")){
        commands[i-1] = command + optionArray.get(i) + " -P " + get_serial_port();
      } else {
        commands[i-1] = command + optionArray.get(i);
      }
      if(WINDOWS){
        commands[i-1] = commands[i-1].replace("/","\\");
      }
    }
    Process pr;
    String line;
    BufferedReader in;
    
    for(int i = 0; i < commands.length; i++){
      pr = Runtime.getRuntime().exec(commands[i]);
      in = new BufferedReader(new InputStreamReader(pr.getErrorStream()));    
      while((line = in.readLine()) != null){
        if(line.toLowerCase().contains("device signature = ")){
          if(line.contains("0x1e95")){
            v1_x = true;
            break;
          } else if(commands[i].contains("usbtiny")){
            usbtiny = true;
            break;
          } else if(commands[i].contains("usbasp")){
            usbasp = true;
            break;
          } else if(commands[i].contains("arduino")){
            arduino = true;
            break;
          }
        }
      }
      if(v1_x || usbtiny || usbasp || arduino){
       break;
      } 
    }
    if(v1_x){
      result.appendText("\nOpenSprinkler v1.x detected.");
      result.appendText("\nThis Program does not support legacy hardware.");
    }
    if(usbtiny || usbasp || arduino){
      result.appendText("\nDevice detected.");
    }
    if(usbtiny){
      dropListDevice.setSelected(deviceArray.indexOf("OpenSprinkler_v2.0"));
      updateDevice();
    }
    if(usbasp){
      dropListDevice.setSelected(deviceArray.indexOf("OpenSprinkler_v2.1"));
      updateDevice();
      result.appendText("\nReturn device to bootloader mode before uploading");
    }
    if(arduino){
      dropListDevice.setSelected(deviceArray.indexOf("OpenSprinkler_v2.2"));
      updateDevice();
    }
    if(!usbtiny && !usbasp && !arduino && !v1_x){
       dropListDevice.setSelected(deviceArray.indexOf("No Device Selected"));
       updateDevice();
       result.appendText("No device detected.");
    }
  } catch (IOException e){
    result.setText(e.toString());
  } 
}
void readme(){ 
//  String CurrentDataPath = dataPath("");
//  String CurrentRootPath = CurrentDataPath.substring(0,CurrentDataPath.indexOf("osFirmwareUpdater")+18);
  String[] readme = loadStrings("../README.txt");
  if(readme == null){
    System.out.println("no readme file found!!!");
    return;
  }
  String fullString = "";
  for(int i = 0; i < readme.length; i++){
    fullString += readme[i] + "\n";
  }
  JOptionPane.showMessageDialog(frame, fullString);
}
String get_serial_port(){
  String port = "";
  String[] list = Serial.list();
  if(WINDOWS){
    port = list[0];
  } else {
    for(int i = 0; i < list.length; i++){
      if(MACOS){
        if(list[i].contains("tty") && list[i].contains("ch341")){
          port = list[i];
          break;
        }
      } else if(LINUX){
        if(list[i].contains("USB")){
          port = list[i];
          break;
        }
      }
    }
  }
  return port;
}

void get_latest_firmware(){
  boolean failed = false;

//   result.setText("");
   result.appendText("\nChecking for new firmware...");    

//   String CurrentDataPath = dataPath("");
   // Figuring out Firmware Directory
//   println(CurrentDataPath.substring(0,CurrentDataPath.indexOf("osFirmwareUpdater")+18));
//   result.appendText(CurrentDataPath.substring(0,CurrentDataPath.indexOf("osFirmwareUpdater")+18));    
   // Set root working path
//   String CurrentRootPath = CurrentDataPath.substring(0,CurrentDataPath.indexOf("osFirmwareUpdater")+18);
   // Backup previous version
   saveStream("../os_firmware_info_backup.txt", "../os_firmware_info.txt");
   // Get the latest firmware file
   
/////////////////////// TO FIX
//Comment back in when file is updated
   if (RELEASE) {
     failed = !saveStream("../os_firmware_info.txt", "https://github.com/rayshobby/opensprinkler/raw/master/OpenSprinkler%20Controller/software/osFirmwareUpdater/os_firmware_info.txt");
   }
////////////////////////
   if(!failed){
     failed = !saveStream("../README.txt","https://raw.githubusercontent.com/rayshobby/opensprinkler/master/OpenSprinkler%20Controller/software/osFirmwareUpdater/README.txt");
   }

  String[] file = null;
  int line = 0;
  int curDevice = 0;
 
  file = loadStrings("../os_firmware_info.txt");
  if (file == null) {
    file = loadStrings("./os_firmware_info.txt");
    if (file == null) {
      System.out.println("Cannot open os_firmware_info.txt");
      System.exit(0);
    } 
  }
  deviceArray = new ArrayList<String>();
  descriptionArray = new ArrayList<String>();
  bootloaderArray = new ArrayList<String>();
  firmwareArrays = new ArrayList<String[]>();
  optionArray = new ArrayList<String>();
  while(line < file.length){
    deviceArray.add(file[line]);
    line+=2;
    descriptionArray.add(file[line]);
    line+=2;
    bootloaderArray.add(file[line]);
    line+=2;
    int firmwareCount = Integer.parseInt(file[line]);
    firmwareArrays.add(new String[firmwareCount]);
    line+=2;
    for(int i = 0; i < firmwareCount; i++){
      firmwareArrays.get(curDevice)[i] = file[line];
      line++;
    }
    line++;
    optionArray.add(file[line]);
    line+=2;
    curDevice++;
  }
  //i =1 to skip 'no device selected'
  int newFirmwareCount = 0;
   for(int i = 1; i < deviceArray.size(); i++){
      if(failed){break;}
      println("Checking for updates for device:" + deviceArray.get(i));
      String[] CurrentDeviceFirmwares = firmwareArrays.get(i);
      for(int j = 0; j < CurrentDeviceFirmwares.length; j++) {
         if(failed){break;}
//         println(CurrentDeviceFirmwares[j]);    
         if(CurrentDeviceFirmwares[j] == null){
           break;
         }
         String filename = "../Firmware/" + deviceArray.get(i)+"/firmware"+CurrentDeviceFirmwares[j]+".hex";

         if (fileExists(filename)){
            println(CurrentDeviceFirmwares[j] + " -> File Exists: " + String.valueOf(filename));
// Commented out so that only updated files listed
//            result.appendText(CurrentDeviceFirmwares[j] + " -> File Exists: " + String.valueOf(filename));
         } else {
            println(CurrentDeviceFirmwares[j] + " -> File Does Not Exist: " + String.valueOf(filename));    
            println("Downloading: " + String.valueOf(filename));
//            result.appendText(CurrentDeviceFirmwares[j] + " -> File Does Not Exist: " + String.valueOf(filename));    
//            result.appendText("Downloading: " + String.valueOf(filename));
            failed = !saveStream(filename, "https://github.com/rayshobby/opensprinkler/raw/master/OpenSprinkler%20Controller/software/osFirmwareUpdater/Firmware/"+deviceArray.get(i)+"/firmware"+CurrentDeviceFirmwares[j]+".hex");
            newFirmwareCount++;
         }
         
      }  // End Firmware Version Loop
   } // End Device Loop
   println("Process Complete");
   if(failed){
     result.appendText("Firmware file missing. Not up to date");
   } else {
     if(newFirmwareCount != 0){
       if(newFirmwareCount > 1){
         result.appendText("\n" + newFirmwareCount + " new firmware files downloaded");
       } else {
         result.appendText("\n" + newFirmwareCount + " new firmware file downloaded");
       }
     }
     result.appendText("\nFirmware up to date.");
   }
   
  dropListDevice.setItems(deviceArray.toArray(new String[curDevice]),0);
  updateDevice();
   
//   println("You must exit and restart osFirmwareUpdater to be able to update using the new firmware versions");
//   result.appendText("You must exit and restart osFirmwareUpdater to be able to update using the new firmware versions");

//println(sketchPath(""));
//println(dataPath(""));
}

boolean fileExists(String filename) {

 //File file = new File(filename);
 File file = sketchFile(filename);

 if(!file.exists())
  return false;
   
 return true;
}
