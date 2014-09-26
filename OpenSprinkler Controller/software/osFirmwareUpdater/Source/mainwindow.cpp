#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    myHandler = new Handler(this);
    if(myHandler->findWorkingDir()){
        dispDefaultOutputText();
        populateMenus();
    } else {
        ui->outputBox->setText("Error: missing " FWCONFIG_FILENAME);
        this->setEnabled(false);
    }

}

MainWindow::~MainWindow()
{
    delete myHandler;
    delete ui;
}

void MainWindow::on_btnDetect_clicked()
{
    if(!myHandler->detectDevice()){
        ui->cmbDevice->setCurrentIndex(0);
        ui->outputBox->setText("Failed to auto-detect device.\n");
        ui->outputBox->append("For OpenSprinkler v2.1, make sure the device is in bootloading mode (click Help below).\n");
        ui->outputBox->append("If you know your device version, you can also select manually from the Device dropdown list.");
    } else {
        QString dname= myHandler->deviceList[myHandler->curr_device].c_str();
        ui->outputBox->setText("Found " + dname + "!\n");

        if (dname.endsWith("2.1")) {
            ui->outputBox->append("Please re-enter bootloader and then click on 'Upload'.");
        } else {
            ui->outputBox->append("Next, click on 'Upload'.");
        }
        ui->cmbDevice->setCurrentIndex(myHandler->curr_device);
        ui->cmbDevice->currentIndexChanged(myHandler->curr_device);
    }

}

void MainWindow::on_btnDownload_clicked()
{
    ui->outputBox->setText("Downloading firmware...");
    QApplication::processEvents();
    this->setEnabled(false);
    QApplication::processEvents();
    if(!myHandler->downloadFirmwares()){
        ui->outputBox->append("Error. Check log.txt for details.");
    } else {
        ui->outputBox->append("Success!");
    }
    this->setEnabled(true);
}

void MainWindow::on_btnUpload_clicked()
{
    ui->outputBox->clear();
    if (!myHandler->curr_device) {
        ui->outputBox->setText("Please select a device first.");
        return;
    }
    ui->outputBox->setText("Uploading firmware...");
    QApplication::setOverrideCursor(Qt::WaitCursor);
    this->setEnabled(false);
    int ret = myHandler->uploadFirmware(ui->cmbFirmware->currentIndex());
    if(ret){
        ui->outputBox->append("Failed.\n");
        ui->outputBox->append("Check log.txt for details");
    } else {
        ui->outputBox->append("Success!");
    }
    this->setEnabled(true);
    QApplication::restoreOverrideCursor();
}

void MainWindow::on_btnHelp_clicked()
{
    ui->outputBox->setText("Instructions:\n");

    ui->outputBox->setText("For OpenSprinkler 2.1, enter bootloader first -> "
                           "press and hold button B2 while plugging in the USB cable, then release the "
                           "button within 1-2 seconds.\n");
    ui->outputBox->append("For all other versions: just plug in the USB cable.");
}

void MainWindow::populateMenus()
{
    if(!myHandler->loadConfigFile()){
        ui->outputBox->setText("Failed to load " FWCONFIG_FILENAME);
        return;
    }
    populateDevices();
    populateFirmwares(ui->cmbDevice->currentIndex());
}

void MainWindow::populateDevices()
{
    ui->cmbDevice->clear();

    QStringList list;
    for(unsigned int i = 0; i < myHandler->deviceList.size(); i++){
        list.append(myHandler->deviceList[i].c_str());
    }
    ui->cmbDevice->addItems(list);

}

void MainWindow::populateFirmwares(int index)
{
    ui->cmbFirmware->clear();
    QStringList list;
    for(int i = 0; i < myHandler->firmwareCount[index]; i++){
        list.append(myHandler->firmwareList[index][i].c_str());
    }
    ui->cmbFirmware->addItems(list);
}

void MainWindow::dispDefaultOutputText()
{
    ui->outputBox->setText("Click 'Download Firmware' to obtain the latest firmware.\n");
    ui->outputBox->append("Click 'Detect Device' to auto-detect your device version.\n");
}

void MainWindow::on_cmbDevice_currentIndexChanged(int index)
{
    myHandler->curr_device = index;
    populateFirmwares(index);
}
