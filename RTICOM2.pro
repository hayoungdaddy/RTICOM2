#-------------------------------------------------
#
# Project created by QtCreator 2019-01-21T04:37:15
#
#-------------------------------------------------

QT       += core gui sql quick location

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = RTICOM2
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11

SOURCES += \
        main.cpp \
        mainwindow.cpp \
    writelog.cpp \
    recvmessage.cpp \
    setstation.cpp \
    configuration.cpp \
    stateofhealth.cpp \
    mycrc.c

HEADERS += \
        mainwindow.h \
    common.h \
    writelog.h \
    recvmessage.h \
    setstation.h \
    configuration.h \
    stateofhealth.h \
    mycrc.h

FORMS += \
        mainwindow.ui \
    setstation.ui \
    configuration.ui \
    stateofhealth.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    RTICOM2.qrc

LIBS += -lactivemq-cpp
INCLUDEPATH += /usr/local/include/activemq-cpp-3.9.5
INCLUDEPATH += /usr/local/apr/include/apr-1/
