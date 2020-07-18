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
        color: "black"

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
            color: DeviceDiscoverer.isError ? "gray" : "black"

            BorderImage {
                source: "qrc:/images/corners.svg"
                anchors.fill: parent
                anchors.margins: -10
                border { left: 30; top: 30; right: 30; bottom: 30 }
                SequentialAnimation on opacity {
                    running: DeviceDiscoverer.isScanning
                    loops: Animation.Infinite
                    NumberAnimation { to: 0.4; duration: 500 }
                    NumberAnimation { to: 1; duration: 500 }
                }
            }

            ListView {
                id: devicesList
                anchors {
                    top: parent.verticalCenter
                    left: parent.left
                    right: parent.right
                    bottom: parent.bottom
                    margins: 10
                }

                clip: true

                visible: !DeviceDiscoverer.isError && !robotLoader.isActive
                model: DeviceDiscoverer.availableDevices

                delegate: Item {
                    width: 200
                    height: 60
                    property string signalText: ""
                    Connections {
                        target: DeviceDiscoverer
                        function onSignalStrengthChanged(deviceName, strength) {
                            if (deviceName !== modelData) {
                                return;
                            }
                            signalText = " (" + Math.floor(strength * 100) + "%)";
                        }
                    }

                    BorderImage {
                        source: "qrc:/images/end.svg"
                        anchors.fill: parent
                        opacity: matchMouse.containsMouse ? 1 : 0.5
                        Behavior on opacity { NumberAnimation { duration: 200 } }
                        border { left: 25; top: 25; right: 25; bottom: 25 }
                    }

                    MouseArea {
                        id: matchMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            DeviceDiscoverer.connectDevice(modelData)
                        }
                    }

                    Text {
                        id: title
                        text: modelData + signalText
                        anchors.centerIn: parent
                        color: "white"
                    }
                }
            }

            Text {
                id: statusText
                anchors {
                    bottom: parent.verticalCenter
                    bottomMargin: 10
                }

                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                font.bold: true
                visible: text.length > 0
                text: {
                    if (DeviceDiscoverer.device && DeviceDiscoverer.device.statusString.length > 0) {
                        return DeviceDiscoverer.device.statusString
                    }
                    return DeviceDiscoverer.statusString
                }
                color: DeviceDiscoverer.isError ? "#a00" : "white"
                wrapMode: Text.Wrap
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
        active: DeviceDiscoverer.device && DeviceDiscoverer.device.isConnected
        anchors.fill: parent
        sourceComponent: {
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
