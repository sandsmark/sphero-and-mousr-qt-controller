TARGET = mousr-qt-controller
TEMPLATE = app
QT += quick

INCLUDEPATH += .

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += main.cpp \
    devicediscoverer.cpp

HEADERS += \
    devicediscoverer.h
