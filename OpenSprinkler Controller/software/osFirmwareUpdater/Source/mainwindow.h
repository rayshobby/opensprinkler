#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "handler.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_btnDetect_clicked();

    void on_btnDownload_clicked();

    void on_btnUpload_clicked();

    void on_btnHelp_clicked();

    void on_cmbDevice_currentIndexChanged(int index);

private:
    Ui::MainWindow *ui;

    Handler* myHandler;

    void populateMenus();

    void populateDevices();

    void populateFirmwares(int index);

    void dispDefaultOutputText();

};

#endif // MAINWINDOW_H
