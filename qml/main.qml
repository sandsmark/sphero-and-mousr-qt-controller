import QtQuick 2.0
import QtQuick.Window 2.12
import com.iskrembilen 1.0

// I don't understand qml anymore...
import "." as Lol

Window {
    id: window
    flags: Qt.FramelessWindowHint | Qt.Dialog
    visible: true

    width: 750
    height: 500

    MouseArea {
        anchors.fill: parent

        property point startPos: Qt.point(1, 1)

        onPressed: {
            startPos  = Qt.point(mouse.x,mouse.y)
        }

        onPositionChanged: {
            var delta = Qt.point(mouse.x-startPos.x, mouse.y-startPos.y)
            window.x += delta.x;
            window.y += delta.y;
        }
    }

    Rectangle {
        id: deviceDiscovery
        anchors.fill: parent
        color: "white"

        visible: !robotLoader.active

        Image {
            anchors.fill: parent
            source: "qrc:images/bg.png"
            fillMode: Image.Tile
        }

        BorderImage {
            anchors.fill: statusBox
            anchors.margins: -8
            border { left: 10; top: 10; right: 10; bottom: 10 }
            source: "qrc:images/shadow.png"
        }

        Rectangle {
            id: statusBox
            anchors {
                fill: parent
                margins: 50
            }
            border.width: 2
            color: DeviceDiscoverer.isError ? "lightGray" : "white"

            Text {
                anchors {
                    fill: parent
                    margins: 20
                }
                font.bold: true
                visible: DeviceDiscoverer.statusString.length > 0
                text: DeviceDiscoverer.statusString
                color: DeviceDiscoverer.isError ? "#a00" : "black"
                wrapMode: Text.Wrap
            }

            Item {
                id: spinner
                width: 100
                height: width
                anchors.centerIn: parent
                visible: !DeviceDiscoverer.isError

                RotationAnimation on rotation {
                    running: spinner.visible
                    loops: Animation.Infinite
                    from: 0
                    to: 360
                    duration: 500
                }

                SequentialAnimation on opacity {
                    running: spinner.visible
                    loops: Animation.Infinite
                    PropertyAnimation { to: 1; duration: 500 }
                    PropertyAnimation { to: 0.1; duration: 500 }
                }

                Rectangle {
                    width: parent.width
                    height: width
                    radius: parent.width / 2
                    border.width: 2
                }

                Rectangle {
                    width: parent.width / 2
                    height: width
                }
            }

//            Text {
//                anchors {
//                    fill: parent
//                    margins: 20
//                }
//                font.bold: true
//                visible: DeviceDiscoverer.device
//                text: visible ? DeviceDiscoverer.device.statusString : ""

//                wrapMode: Text.Wrap
//            }
        }
    }

    Loader {
        id: robotLoader
        active: DeviceDiscoverer.device// && DeviceDiscoverer.device.isConnected
        anchors.fill: parent
        sourceComponent: Lol.MousrView {
            device: DeviceDiscoverer.device
        }
    }
}
