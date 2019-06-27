TARGET = mousr-qt-controller
TEMPLATE = app

CONFIG += sanitizer sanitize_undefined

QT += quick bluetooth

INCLUDEPATH += .

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += main.cpp \
    devicediscoverer.cpp

HEADERS += \
    devicediscoverer.h

RESOURCES += \
    qml/main.qrc
