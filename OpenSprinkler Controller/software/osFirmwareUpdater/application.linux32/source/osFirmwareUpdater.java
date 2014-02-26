import processing.core.*; 
import processing.data.*; 
import processing.event.*; 
import processing.opengl.*; 

import g4p_controls.*; 
import java.io.InputStreamReader; 
import java.awt.Cursor; 
import java.awt.BorderLayout; 
import javax.swing.JDialog; 
import javax.swing.JFrame; 
import javax.swing.JLabel; 
import javax.swing.JProgressBar; 
import javax.swing.JOptionPane; 

import java.util.HashMap; 
import java.util.ArrayList; 
import java.io.File; 
import java.io.BufferedReader; 
import java.io.PrintWriter; 
import java.io.InputStream; 
import java.io.OutputStream; 
import java.io.IOException; 

public class osFirmwareUpdater extends PApplet {

/* OpenSprinkler GUI-based Firmware Update Tool
 * Rayshobby LLC
 * http://rayshobby.net
 *
 * Initially written by Jonathan Goldin
 * Refined by Ray Wang
 * Published under Creative Commons CC-SA 3.0 license
 */
 













int FRAME_CONFIG_STARTX = 0;
int FRAME_CONFIG_STARTY = 0;
int WINDOW_WIDTH = 250;
int WINDOW_HEIGHT = 550;

GDropList dropListDevice;
GDropList dropListFirmwareVersion;

GButton buttonUpload;
GButton buttonREADME;

GLabel lblDevice;
GLabel lblFirmware;
GLabel lblHardwareDesc;
GLabel lblBootloading;

GTextArea description;
GTextArea bootloadInstruc;
GTextArea result;

ArrayList<String> deviceArray;
ArrayList<String> descriptionArray;
ArrayList<String> bootloaderArray;
ArrayList<String> commandArray;
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
  commandArray = new ArrayList<String>();
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
    commandArray.add(file[line]);
    line+=2;
    curDevice++;
  }
  
  lblDevice = new GLabel(this,FRAME_CONFIG_STARTX, FRAME_CONFIG_STARTY + 50,80,20);
  lblDevice.setText("Hardware:");
  lblDevice.setTextBold();
  
  lblHardwareDesc = new GLabel(this,FRAME_CONFIG_STARTX, FRAME_CONFIG_STARTY + 80, 90, 20);
  lblHardwareDesc.setText("Description:");
  lblHardwareDesc.setTextBold();

  lblFirmware = new GLabel(this,FRAME_CONFIG_STARTX, FRAME_CONFIG_STARTY + 190,80,20);
  lblFirmware.setText("Firmware:");
  lblFirmware.setTextBold();
  
  lblBootloading = new GLabel(this,FRAME_CONFIG_STARTX, FRAME_CONFIG_STARTY + 220, 90, 20);
  lblBootloading.setText("Preparation:");
  lblBootloading.setTextBold();
  
  dropListDevice = new GDropList(this,FRAME_CONFIG_STARTX + 80, FRAME_CONFIG_STARTY+ 50,150,100);
  dropListDevice.setItems(deviceArray.toArray(new String[curDevice]),0);
  dropListDevice.setSelected(0);

  currentDevice = dropListDevice.getSelectedIndex();

  dropListFirmwareVersion = new GDropList(this,FRAME_CONFIG_STARTX+80, FRAME_CONFIG_STARTY + 190,150,100);
  dropListFirmwareVersion.setItems(firmwareArrays.get(currentDevice),0);
  dropListFirmwareVersion.setSelected(0);
  
  description = new GTextArea(this,FRAME_CONFIG_STARTX, FRAME_CONFIG_STARTY+100,250,75);
  description.setText(descriptionArray.get(currentDevice));
  description.setTextEditEnabled(false);
  
  bootloadInstruc = new GTextArea(this,FRAME_CONFIG_STARTX, FRAME_CONFIG_STARTY + 240, 250,125);
  bootloadInstruc.setText(bootloaderArray.get(currentDevice));
  bootloadInstruc.setTextEditEnabled(false);
  
  buttonUpload = new GButton(this,FRAME_CONFIG_STARTX+50, FRAME_CONFIG_STARTY + 380,150,40,"UPLOAD");
  buttonUpload.setLocalColorScheme(6);
  buttonUpload.setTextBold();
  
  buttonREADME = new GButton(this,FRAME_CONFIG_STARTX+35, FRAME_CONFIG_STARTY + 10,190,30,"README First!");
  buttonREADME.setLocalColorScheme(6);
  buttonREADME.setTextBold();
  
  result = new GTextArea(this,FRAME_CONFIG_STARTX, FRAME_CONFIG_STARTY + 430,250,120, g4p_controls.GConstants.SCROLLBARS_VERTICAL_ONLY );
  result.setText("Select hardware and firmware version. Follor Preparation instructions. Then click on UPLOAD.");
  result.setTextEditEnabled(false);
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
    result.setText("Select hardware and firmware version. Follor Preparation instructions. Then click on UPLOAD.");
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
}
public void handleTextEvents(GEditableTextControl textcontrol, GEvent event) {}

public void upload(){
  try{
    uploadNow = false;
    String command = commandArray.get(currentDevice);
    String file = "";//System.getProperty("user.dir");
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
    Process pr = Runtime.getRuntime().exec(command+file);
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
public void readme(){ 
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
  static public void main(String[] passedArgs) {
    String[] appletArgs = new String[] { "osFirmwareUpdater" };
    if (passedArgs != null) {
      PApplet.main(concat(appletArgs, passedArgs));
    } else {
      PApplet.main(appletArgs);
    }
  }
}
