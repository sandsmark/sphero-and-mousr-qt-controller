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

            Lol.Spinner {
                id: spinner
                visible: !DeviceDiscoverer.isError && !robotLoader.isActive
                anchors.centerIn: parent
            }
        }
    }

    Component {
        id: mousrComponent

        Lol.MousrView {
            device: DeviceDiscoverer.device
        }
    }

    Component {
        id: spheroComponent
        Lol.SpheroView {
            device: DeviceDiscoverer.device
        }
    }

    Loader {
        id: robotLoader
        active: DeviceDiscoverer.device
        anchors.fill: parent
        sourceComponent: {
            console.log(DeviceDiscoverer.device)
            if (!DeviceDiscoverer.device) {
                return undefined;
            }
            if (DeviceDiscoverer.device.deviceType === "Mousr") {
                return mousrComponent;
            }
            if (DeviceDiscoverer.device.deviceType === "Sphero") {
                return spheroComponent;
            }
            console.warn("Unhandled device type " + DeviceDiscoverer.device.deviceType)
            return undefined;
        }
//            DeviceDiscoverer.device ? (DeviceDiscoverer.device.deviceType === "Mousr" ?  mousrComponent : spheroComponent) : undefined
    }
}
