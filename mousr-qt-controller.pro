TARGET = mousr-qt-controller
TEMPLATE = app

CONFIG += sanitizer sanitize_undefined c++17

QT += quick bluetooth

INCLUDEPATH += .

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += main.cpp \
    DeviceHandler.cpp \
    devicediscoverer.cpp

HEADERS += \
    DeviceHandler.h \
    devicediscoverer.h

RESOURCES += \
    qml/main.qrc
