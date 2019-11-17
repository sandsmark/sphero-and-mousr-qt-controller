TARGET = mousr-qt-controller
TEMPLATE = app

CONFIG += sanitizer sanitize_undefined c++17

QT += quick bluetooth

INCLUDEPATH += .

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += main.cpp \
    MousrHandler.cpp \
    devicediscoverer.cpp

HEADERS += \
    MousrHandler.h \
    devicediscoverer.h

RESOURCES += \
    main.qrc

DISTFILES +=
