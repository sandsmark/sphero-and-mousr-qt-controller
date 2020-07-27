TARGET = mousr-qt-controller
TEMPLATE = app

CONFIG += sanitizer sanitize_undefined # sanitize_address
CONFIG += c++2a

QT += quick bluetooth

INCLUDEPATH += src

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    src/main.cpp \
    src/devicediscoverer.cpp \
    src/mousr/AutoplayConfig.cpp \
    src/mousr/MousrHandler.cpp \
    src/sphero/SpheroHandler.cpp \


HEADERS += \
    src/BasicTypes.h \
    src/devicediscoverer.h \
    src/mousr/MousrHandler.h \
    src/mousr/AutoplayConfig.h \
    src/sphero/v1/CommandPackets.h \
    src/sphero/v1/ResponsePackets.h \
    src/sphero/v2/Constants.h \
    src/sphero/v2/Packets.h \
    src/sphero/SpheroHandler.h \
    src/sphero/Uuids.h \
    src/utils.h

RESOURCES += \
    main.qrc

DISTFILES += \
    qml/main.qml \
    qml/SpheroView.qml \
    qml/MousrView.qml \
    qml/Spinner.qml \
