import QtQuick 2.0
import QtQuick.Window 2.12
import QtGraphicalEffects 1.13

import com.iskrembilen 1.0

// I don't understand qml anymore...
import "." as Lol

Window {
    id: window
    flags: Qt.FramelessWindowHint | Qt.Dialog
    visible: true

    width: 1024
    height: 768

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

    Image {
        anchors.fill: parent
        source: "qrc:images/bg.png"
        fillMode: Image.Tile
    }

    Repeater {
        model: 9
        Item {
            anchors.fill: parent
            Item {
                id: layer1
                property int layerIndex: index + 1
                anchors.fill: parent
                visible: false

                Repeater {
                    model: 10
                    Rectangle {
                        id: bubble
                        color: "lightblue"
                        property real fromX: Math.random() * window.width - width
                        property real fromY: Math.random() * window.height - height
                        property real toX: Math.random() * window.width - width
                        property real toY: Math.random() * window.height - height

                        property bool animStarted: false

                        x: fromX
                        y: fromY
                        width: Math.random() * 10 + layer1.layerIndex * 10
                        height: width
                        radius: width / 2

                        Component.onCompleted: {
                            animStartTimer.start()
                        }

                        Timer {
                            id: animStartTimer
                            interval: Math.random() * 5000
                            repeat: false
                            onTriggered: { animStarted = true; }
                        }

                        SequentialAnimation on x {
                            id: xAnim
                            running: animStarted && deviceDiscovery.visible
                            loops: Animation.Infinite
                            PropertyAnimation { easing.type: Easing.InOutSine; to: bubble.toX; duration: Math.random() * 1000 + 10000 }
                            PropertyAnimation { easing.type: Easing.InOutSine; to: bubble.fromX; duration: Math.random() * 1000 + 10000 }
                        }
                        SequentialAnimation on y {
                            id: yAnim
                            running: animStarted && deviceDiscovery.visible
                            loops: Animation.Infinite
                            PropertyAnimation { easing.type: Easing.InOutSine; to: bubble.toY; duration: Math.random() * 1000 + 10000 }
                            PropertyAnimation { easing.type: Easing.InOutSine; to: bubble.fromY; duration: Math.random() * 1000 + 10000 }
                        }
                    }
                }
            }

            FastBlur {
                anchors.fill: layer1
                source: layer1
                radius: 10 * index
                opacity: 1 - layer1.layerIndex * 0.05
            }
        }
    }

    Item {
        id: deviceDiscovery
        anchors.fill: parent

        visible: !robotLoader.sourceComponent
        opacity: 0.75

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
                    top: parent.top
                    left: parent.left
                    right: parent.right
                    bottom: parent.bottom
                    margins: 10
                }

                clip: true

                visible: !DeviceDiscoverer.device
                model: DeviceDiscoverer.availableDevices

                delegate: Lol.Button {
                    color: DeviceDiscoverer.displayColor(modelData)

                    function updateText(strength) {
                        text = DeviceDiscoverer.displayName(modelData) + " (" + Math.floor(strength * 100) + "%)";

                        active = strength > 0
                    }

                    Component.onCompleted: {
                        updateText(DeviceDiscoverer.signalStrength(modelData))
                    }

                    Connections {
                        target: DeviceDiscoverer
                        function onSignalStrengthChanged(deviceName, strength) {
                            if (deviceName === modelData) {
                                updateText(strength)
                            }
                        }
                    }

                    onClicked: {
                        DeviceDiscoverer.connectDevice(modelData)
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
        active: DeviceDiscoverer.device !== null && DeviceDiscoverer.device !== undefined && DeviceDiscoverer.device.isConnected

        anchors.fill: parent
        sourceComponent: {
            if (!DeviceDiscoverer.device || !DeviceDiscoverer.device.isConnected) {
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
    }

}
