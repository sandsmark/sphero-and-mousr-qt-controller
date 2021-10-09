cmake_minimum_required(VERSION 3.5)

project(mousr-qt-controller LANGUAGES CXX)

find_package(Qt5 COMPONENTS Bluetooth Quick REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

add_executable(mousr-qt-controller
    src/main.cpp
    src/devicediscoverer.cpp
    src/devicediscoverer.h
    src/BasicTypes.h
    src/utils.h
    src/Cursor.cpp
    src/Cursor.h

    src/mousr/AutoplayConfig.cpp
    src/mousr/MousrHandler.cpp
    src/mousr/MousrHandler.h
    src/mousr/AutoplayConfig.h

    src/sphero/SpheroHandler.cpp
    src/sphero/SpheroHandler.h
    src/sphero/Uuids.h
    src/sphero/v1/CommandPackets.h
    src/sphero/v1/ResponsePackets.h
    src/sphero/v2/Constants.h
    src/sphero/v2/Packets.h

    main.qrc

    qml/main.qml
    qml/SpheroView.qml
    qml/MousrView.qml
    qml/Spinner.qml
)

target_link_libraries(mousr-qt-controller PRIVATE Qt5::Quick Qt5::Bluetooth)
target_include_directories(mousr-qt-controller PRIVATE src)
install(TARGETS mousr-qt-controller DESTINATION bin)
