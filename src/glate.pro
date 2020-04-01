#-------------------------------------------------
#
# Project created by QtCreator 2020-03-26T13:53:21
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets network multimedia

include(qhotkey.pri)

TARGET = glate
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# No debug output in release mode
CONFIG(release, debug|release):DEFINES += QT_NO_DEBUG_OUTPUT


# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11

SOURCES += \
        controlbutton.cpp \
        error.cpp \
        history.cpp \
        main.cpp \
        mainwindow.cpp \
        request.cpp \
        settings.cpp \
        share.cpp \
        utils.cpp \
        waitingspinnerwidget.cpp

HEADERS += \
        controlbutton.h \
        error.h \
        history.h \
        mainwindow.h \
        request.h \
        settings.h \
        share.h \
        utils.h \
        waitingspinnerwidget.h

FORMS += \
        error.ui \
        history.ui \
        history_item.ui \
        mainwindow.ui \
        settings.ui \
        share.ui \
        textoptionform.ui

# Default rules for deployment.
isEmpty(PREFIX){
 PREFIX = /usr
}

BINDIR  = $$PREFIX/bin
DATADIR = $$PREFIX/share

target.path = $$BINDIR

icon.files = icons/linguist.png
icon.path = $$DATADIR/icons/hicolor/512x512/apps/

desktop.files = linguist.desktop
desktop.path = $$DATADIR/applications/

INSTALLS += target icon desktop

RESOURCES += \
    icons.qrc \
    qbreeze.qrc \
    resources.qrc

DISTFILES +=

