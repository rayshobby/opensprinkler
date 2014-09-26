#include "handler.h"

QT_USE_NAMESPACE

using namespace std;

const string gitHub = GITHUB;

Handler::Handler(QObject* parent):
    QObject(parent)
{
    curr_device = NO_DEVICE;
}

Handler::~Handler()
{
    clearLists();
}

// Find the correct working directory
bool Handler::findWorkingDir()
{
    QDir tmp(QCoreApplication::applicationDirPath());
    // go up the chain until we find the config file
    while(!fileExists(FWCONFIG_FILENAME)){
        qDebug() << tmp.path();
        if(tmp.path() == "/"){
            break;
        }
        tmp.cdUp();
        QDir::setCurrent(tmp.path());
    }
    return fileExists(FWCONFIG_FILENAME);
}

// Load firmware configuration file
// and create UI lists
bool Handler::loadConfigFile()
{
    clearLists();
    QFile configFile(FWCONFIG_FILENAME);
    configFile.open(QIODevice::ReadOnly);
    if(!configFile.isOpen()){
        return false;
    }
    int count = 0;
    QTextStream configStream(&configFile);
    while(!configStream.atEnd()){
        deviceList.push_back(configStream.readLine().toStdString());
        configStream.readLine();
        commandList.push_back(configStream.readLine().toStdString());
        configStream.readLine();
        count = configStream.readLine().toInt();
        firmwareCount.push_back(count);
        string* newStringArray = new string[count];
        configStream.readLine();
        for(int i = 0; i < count; i++){
            newStringArray[i] = configStream.readLine().toStdString();
        }
        firmwareList.push_back(newStringArray);
        configStream.readLine();
    }
    return true;
}

// Download all firmwares
bool Handler::downloadFirmwares(){
#ifdef RELEASE
    qDebug() << "here";
    //rename(FWCONFIG_FILENAME, FWCONFIG_FILENAME "_backup.txt");
    download(FWCONFIG_FILENAME);
    /*if(!fileExists(FWCONFIG_FILENAME)){
        rename("info_backup.txt","info.txt");
        return false;
    }*/
    loadConfigFile();
#endif
    bool success = true;
    for(unsigned int i = 1; i < firmwareList.size(); i++){
        for(int j = 0; j < firmwareCount[i]; j++){
            string fileName = deviceList[i] + "/firmware" + firmwareList[i][j] + ".hex";
            if(downloadFile("Firmware/" + fileName)==false ||
               !fileExists(("Firmware/" + fileName))){
                success = false;
            }
        }
    }
    return success;
}

// Detect device
bool Handler::detectDevice(){

    curr_device = NO_DEVICE;
    string initCommand = getCommand();
    if(initCommand.empty()){
        return false;
    }
    string detectCommand = "";

    for(unsigned int i = 1; i < commandList.size(); i++){
        detectCommand = initCommand + commandList[i];
        QProcess p;
        p.start(detectCommand.c_str());
        p.waitForFinished(-1);
        QString output(p.readAllStandardError());
        p.close();

        if(output.indexOf("Device signature = ") != -1){
            if(output.indexOf("0x1e95") != -1){
                return false;
            }
            curr_device = i;
            return true;
        }
    }
    return false;
}

// Upload firmware
int Handler::uploadFirmware(int firmwareIndex){
    string command = getCommand();
    if(command.empty()) {
        writeLog("Command is empty.");
        return 1;
    }
    string options = commandList[curr_device];
    if(curr_device == NO_DEVICE) {
        writeLog("No device is selected.");
        return 2;
    }
    string flash = " -q -F -U flash:w:";
    string file = "./Firmware/" + deviceList[curr_device] + "/firmware" + firmwareList[curr_device][firmwareIndex] + ".hex";
    QProcess p;
    p.start((command + options + flash + file).c_str());
    bool finished = false;
    do {
        finished = p.waitForFinished(100);
        QCoreApplication::processEvents();  // give event loop time to process text display
    } while(!finished);
    QString output(p.readAllStandardError());
    p.close();

    writeLog(output.toStdString());
    if(output.indexOf("verified") != -1){
        return 0;
    }
    writeLog("Error uploading firmware.");
    return 3;
}

// Write to log
void Handler::writeLog(std::string text) {
    ofstream logFile;
    logFile.open(LOG_FILENAME, ios_base::app);
    logFile << "\n====== ";
    logFile << QDateTime::currentDateTime().toString().toStdString();
    logFile << " ======\n";
    logFile << text;
    logFile.close();
}

// Clear UI lists
void Handler::clearLists()
{
    deviceList.clear();
    commandList.clear();
    firmwareCount.clear();
    for(unsigned int i = 0; i < firmwareList.size(); i++){
        delete[] firmwareList[i];
    }
    firmwareList.clear();
}

// Construct avrdude command
string Handler::getCommand()
{
    string comport = "";
    // detect comport of CH340G
    foreach (const QSerialPortInfo &serialPortInfo, QSerialPortInfo::availablePorts()) {
        if(serialPortInfo.hasVendorIdentifier() &&
           serialPortInfo.vendorIdentifier() == 0x1a86 &&
           serialPortInfo.hasProductIdentifier() &&
           serialPortInfo.productIdentifier() == 0x7523) {
           comport = serialPortInfo.portName().toStdString();
        }
    }

    string command;
#ifdef _WIN32
    command = "./Windows/avr/bin/avrdude -C ./Windows/avr/bin/avrdude.conf ";
    if (comport!="") {
        command +="-P ";
        command += comport;
        command += " ";
    }
#elif __APPLE__
    command = "./MacOSX/avr-macos/bin/avrdude -C ./MacOSX/avr-macos/etc/avrdude.conf ";

    if(comport!=""){
        command += "-P \"/dev/tty.";
        command += comport;
        command += "\" ";
    }
#elif __linux
    command = "avrdude ";

    if (comport!="") {
        command += "-P /dev/";
        command += comport;
        command += " ";
    }
#endif
    return command;
}

// Check if file exists in the folder
bool Handler::fileExists(string name)
{
       QFile file(name.c_str());
       return file.exists();
}

// Download a single file
bool Handler::downloadFile(string file)
{
    QNetworkAccessManager m_WebCtrl;
    QUrl url((gitHub + file).c_str());
    QEventLoop loop;
    QNetworkRequest request(url);
    QNetworkReply* reply = m_WebCtrl.get(request);
    connect(reply,SIGNAL(finished()),&loop, SLOT(quit()));
    loop.exec();
    QString fileName = url.toDisplayString().remove(gitHub.c_str());

    QByteArray m_DownloadedData;
    m_DownloadedData = reply->readAll();
    reply->deleteLater();
    if (m_DownloadedData.startsWith("Not Found")) {
        writeLog((fileName + " not found from source.\n").toStdString());
        return false;
    }
    QFile qfile("./" + fileName);
    qfile.open(QIODevice::WriteOnly);
    qfile.write(m_DownloadedData);
    qfile.close();
    return true;
}


