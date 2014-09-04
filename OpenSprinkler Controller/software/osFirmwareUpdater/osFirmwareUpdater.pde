/* OpenSprinkler GUI-based Firmware Update Tool
 * Rayshobby LLC
 * http://rayshobby.net
 *
 * Initially written by Jonathan Goldin
 * Refined by Ray Wang
 * Published under Creative Commons CC-SA 3.0 license
 */

import g4p_controls.*;
import java.io.InputStreamReader;
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
int WINDOW_HEIGHT = 600;

GDropList dropListDevice;
GDropList dropListFirmwareVersion;

GButton buttonUpload;
GButton buttonREADME;
GButton buttonGetLatestFirmware;

GLabel lblDevice;
GLabel lblFirmware;
GLabel lblHardwareDesc;
GLabel lblBootloading;

GTextArea description;
GTextArea bootloadInstruc;
GTextArea result;

boolean WINDOWS = false;
boolean MACOS = false;
boolean LINUX = false;

ArrayList<String> deviceArray;
ArrayList<String> descriptionArray;
ArrayList<String> bootloaderArray;
ArrayList<String> optionArray;
ArrayList<String[]> firmwareArrays;

int currentDevice;
boolean toUpload = false;
boolean uploadNow = false;

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
 
  file = loadStrings("../os_firmware_info.txt");
  if (file == null) {
    file = loadStrings("./os_firmware_info.txt");
    if (file == null) {
      System.out.println("Cannot open os_firmware_info.txt");
      System.exit(0);
    } 
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
  Font f = new Font("Dialog",Font.PLAIN,14);
  lblDevice = new GLabel(this,FRAME_CONFIG_STARTX, FRAME_CONFIG_STARTY + 50,100,20);
  lblDevice.setFont(f);
  lblDevice.setText("Hardware:");
  lblDevice.setTextBold();
  
  lblHardwareDesc = new GLabel(this,FRAME_CONFIG_STARTX, FRAME_CONFIG_STARTY + 80, 110, 20);
  lblHardwareDesc.setFont(f);
  lblHardwareDesc.setText("Description:");
  lblHardwareDesc.setTextBold();

  lblFirmware = new GLabel(this,FRAME_CONFIG_STARTX, FRAME_CONFIG_STARTY + 190,100,20);
  lblFirmware.setFont(f);
  lblFirmware.setText("Firmware:");
  lblFirmware.setTextBold();
  
  lblBootloading = new GLabel(this,FRAME_CONFIG_STARTX, FRAME_CONFIG_STARTY + 220, 110, 20);
  lblBootloading.setFont(f);
  lblBootloading.setText("Preparation:");
  lblBootloading.setTextBold();
  
  dropListDevice = new GDropList(this,FRAME_CONFIG_STARTX + 100, FRAME_CONFIG_STARTY+ 45,165,150);
  dropListDevice.setFont(f);
  dropListDevice.setItems(deviceArray.toArray(new String[curDevice]),0);
  dropListDevice.setSelected(0);

  currentDevice = dropListDevice.getSelectedIndex();

  dropListFirmwareVersion = new GDropList(this,FRAME_CONFIG_STARTX+100, FRAME_CONFIG_STARTY + 185,165,150);
  dropListFirmwareVersion.setFont(f);
  dropListFirmwareVersion.setItems(firmwareArrays.get(currentDevice),0);
  dropListFirmwareVersion.setSelected(0);
  
  description = new GTextArea(this,FRAME_CONFIG_STARTX, FRAME_CONFIG_STARTY+100,300,80);
  description.setFont(f);
  description.setText(descriptionArray.get(currentDevice));
  description.setTextEditEnabled(false);
  
  bootloadInstruc = new GTextArea(this,FRAME_CONFIG_STARTX, FRAME_CONFIG_STARTY + 240, 300,130);
  bootloadInstruc.setFont(f);
  bootloadInstruc.setText(bootloaderArray.get(currentDevice));
  bootloadInstruc.setTextEditEnabled(false);
  
  buttonUpload = new GButton(this,FRAME_CONFIG_STARTX+50, FRAME_CONFIG_STARTY + 380,150,40,"UPLOAD");
  buttonUpload.setFont(f);
  buttonUpload.setLocalColorScheme(6);
  buttonUpload.setTextBold();
  
  buttonREADME = new GButton(this,FRAME_CONFIG_STARTX+35, FRAME_CONFIG_STARTY + 10,190,30,"README First!");
  buttonREADME.setFont(f);
  buttonREADME.setLocalColorScheme(6);
  buttonREADME.setTextBold();
  
  buttonGetLatestFirmware = new GButton(this,FRAME_CONFIG_STARTX+35, FRAME_CONFIG_STARTY + 560,190,30,"Get Latest Firmware");
  buttonGetLatestFirmware.setFont(f);
  buttonGetLatestFirmware.setLocalColorScheme(6);
  buttonGetLatestFirmware.setTextBold();
  
  result = new GTextArea(this,FRAME_CONFIG_STARTX, FRAME_CONFIG_STARTY + 430,300,120, g4p_controls.GConstants.SCROLLBARS_VERTICAL_ONLY );
  result.setFont(f);
  result.setText("Select hardware and firmware version. Follow Preparation instructions. Then click on UPLOAD.");
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

public void draw(){
  background(250,250,250);    // background color: dark green
  if(uploadNow){
    upload();
  }
  if(toUpload){
    result.setText("uploading, please wait...");
    toUpload = false;
    uploadNow = true;
  }
}

public void handleDropListEvents(GDropList list, GEvent event){
  if(list == dropListDevice){
    currentDevice = dropListDevice.getSelectedIndex();
    dropListFirmwareVersion.setItems(firmwareArrays.get(currentDevice),0);
    description.setText(descriptionArray.get(currentDevice));
    bootloadInstruc.setText(bootloaderArray.get(currentDevice));
    result.setText("Select hardware and firmware version. Follow Preparation instructions. Then click on UPLOAD.");
    dropListFirmwareVersion.setSelected(0);
  }
}
public void handleButtonEvents(GButton button,GEvent event){
  if(button == buttonUpload){
    toUpload = true;
  }
  if(button == buttonREADME){
    readme();
  }
  if(button == buttonGetLatestFirmware){
    get_latest_firmware();
  }
}
public void handleTextEvents(GEditableTextControl textcontrol, GEvent event) {}

void upload(){
  try{
    uploadNow = false;
    String command = "";
    if(WINDOWS){
      command = "../avr/bin/avrdude -C ../avr/bin/avrdude.conf ";
    }
    if(MACOS){
      command = "../avr-macos/bin/avrdude -C ../avr-macos/etc/avrdude.conf ";
    }
    if(LINUX){
      command = "avrdude ";
    }
    String options = optionArray.get(currentDevice);
    String file = "";
    file += "../Firmware/";
    file += dropListDevice.getSelectedText();
    file += "/firmware";
    file += dropListFirmwareVersion.getSelectedText();
    file += ".hex";
    if(!new File(file).isFile()){
      result.setText("Error: file not found at:");
      result.appendText(file);
      return;
    }
    this.setCursor(Cursor.getPredefinedCursor(Cursor.WAIT_CURSOR));
    Process pr = Runtime.getRuntime().exec(command+options+file);
    BufferedReader in = new BufferedReader(new InputStreamReader(pr.getErrorStream()));
    result.setText("");
    String line;
    while((line = in.readLine()) != null){
      result.appendText(line);
    }
  } catch (IOException e){
    result.setText(e.toString());
  } finally {
    this.setCursor(Cursor.getDefaultCursor());
  }
}
void readme(){ 
  String[] readme = loadStrings("../README.txt");
  if(readme == null){
    readme = loadStrings("./README.txt");
  }
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

void get_latest_firmware(){ 
   result.setText("");
   result.appendText("Downloading latest os_firmware_info.txt");    

   String CurrentDataPath = dataPath("");
   // Figuring out Firmware Directory
   println(CurrentDataPath.substring(0,CurrentDataPath.indexOf("osFirmwareUpdater")+18));
//   result.appendText(CurrentDataPath.substring(0,CurrentDataPath.indexOf("osFirmwareUpdater")+18));    
   // Set root working path
   String CurrentRootPath = CurrentDataPath.substring(0,CurrentDataPath.indexOf("osFirmwareUpdater")+18);
   // Backup previous version
   saveStream(CurrentRootPath+"os_firmware_info_backup"+String.valueOf(year())+nf(month(),2)+nf(day(),2)+"_"+nf(hour(),2)+nf(minute(),2)+".txt", "os_firmware_info.txt");
   // Get the latest firmware file
   saveStream(CurrentRootPath+"os_firmware_info.txt", "https://github.com/rayshobby/opensprinkler/raw/master/OpenSprinkler%20Controller/software/osFirmwareUpdater/os_firmware_info.txt");

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

   for(int i = 0; i < curDevice; i++){
      println("Checking for updates for device:" + deviceArray.get(i));
      result.appendText("Checking for updates for device:" + deviceArray.get(i));
      String[] CurrentDeviceFirmwares = firmwareArrays.get(i);
      for(int j = 0; j < CurrentDeviceFirmwares.length; j++) {
//         println(CurrentDeviceFirmwares[j]);    

         String filename = CurrentRootPath + "Firmware/" + deviceArray.get(i)+"/firmware"+CurrentDeviceFirmwares[j]+".hex";

         if (fileExists(filename)){
            println(CurrentDeviceFirmwares[j] + " -> File Exists: " + String.valueOf(filename));
// Commented out so that only updated files listed
//            result.appendText(CurrentDeviceFirmwares[j] + " -> File Exists: " + String.valueOf(filename));
         } else {
            println(CurrentDeviceFirmwares[j] + " -> File Does Not Exist: " + String.valueOf(filename));    
            println("Downloading: " + String.valueOf(filename));    
            result.appendText(CurrentDeviceFirmwares[j] + " -> File Does Not Exist: " + String.valueOf(filename));    
            result.appendText("Downloading: " + String.valueOf(filename));    
            saveStream(CurrentRootPath + "Firmware/"+deviceArray.get(i)+"/firmware"+CurrentDeviceFirmwares[j]+".hex", "https://github.com/rayshobby/opensprinkler/raw/master/OpenSprinkler%20Controller/software/osFirmwareUpdater/Firmware/"+deviceArray.get(i)+"/firmware"+CurrentDeviceFirmwares[j]+".hex");
         }
         
      }  // End Firmware Version Loop
   } // End Device Loop
   println("Process Complete");
   result.appendText("Process Complete");
   println("You must exit and restart osFirmwareUpdater to be able to update using the new firmware versions");
   result.appendText("You must exit and restart osFirmwareUpdater to be able to update using the new firmware versions");

//println(sketchPath(""));
//println(dataPath(""));
}

boolean fileExists(String filename) {

 File file = new File(filename);

 if(!file.exists())
  return false;
   
 return true;
}
