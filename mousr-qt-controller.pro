TARGET = mousr-qt-controller
TEMPLATE = app

CONFIG += sanitizer sanitize_undefined c++2a

QT += quick bluetooth

INCLUDEPATH += src

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    src/main.cpp \
    src/devicediscoverer.cpp \
    src/mousr/MousrHandler.cpp \
    src/sphero/SpheroHandler.cpp \


HEADERS += \
    src/devicediscoverer.h \
    src/mousr/MousrHandler.h \
    src/mousr/AutoplayConfig.h \
    src/sphero/SpheroHandler.h \
    src/utils.h

RESOURCES += \
    main.qrc

DISTFILES +=
