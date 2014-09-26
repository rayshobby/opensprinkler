#-------------------------------------------------
#
# Project created by QtCreator 2014-09-11T11:36:52
#
#-------------------------------------------------

QT       += core gui
QT       += network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

greaterThan(QT_MAJOR_VERSION, 4) {
    QT       += widgets serialport
} else {
    include($$QTSERIALPORT_PROJECT_ROOT/src/serialport/qt4support/serialport.prf)
}

TARGET = osFirmwareUpdater
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    handler.cpp

HEADERS  += mainwindow.h \
    defines.h \
    handler.h

FORMS    += mainwindow.ui

RESOURCES +=
