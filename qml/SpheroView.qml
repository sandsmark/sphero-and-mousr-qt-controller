import QtQuick 2.0
import QtQuick.Window 2.12
import com.iskrembilen 1.0

import QtGraphicalEffects 1.12

// I don't understand qml anymore...
import "." as Lol

Rectangle {
    id: robotView
    anchors.fill: parent

    property SpheroHandler device

    Connections {
        target: device
        function onConnectedChanged() {
            console.log(" Connected changed! " + device.isConnected)
        }
    }

    Lol.Spinner {
        id: spinner
        anchors.centerIn: parent

        visible: !device.isConnected
    }

    Text {
        id: statusString

        anchors {
            horizontalCenter: spinner.horizontalCenter
            top: spinner.bottom
            topMargin: 20
        }

        font.bold: true
        visible: !device.isConnected
        text: device.statusString
    }

    Text {
        id: name
        anchors {
            horizontalCenter: parent.horizontalCenter
            top: parent.top
            topMargin: 20
        }

        font.bold: true
        text: device.name
    }
    RadialGradient {
        anchors.fill: image
        gradient: Gradient {
            GradientStop { position: 0.3; color: device.color }
            GradientStop { position: 0.5; color: "transparent" }
        }
    }
    Image {
        id: image
        visible: true//device.isConnected
        anchors {
            top: name.bottom
            topMargin: 20
            bottom: signalStrength.top
            bottomMargin: 20
            horizontalCenter: parent.horizontalCenter
        }
        fillMode: Image.PreserveAspectFit

        source: {
            if (device.robotType == SpheroHandler.BB8) {
                return "qrc:images/bb8.png"
            }
        }
    }
//    ColorOverlay {
//        anchors.fill: image
//        source: image
//        color: device.color
//        opacity: 0.3
//    }

    Text {
        id: signalStrength
        anchors {
            horizontalCenter: parent.horizontalCenter
            bottom: parent.bottom
        }
        visible: device.isConnected
        text: "Signal strength: " + Math.ceil(device.signalStrength * 100) + "%"
    }

    Component.onCompleted: forceActiveFocus()

    focus: true
    Keys.onLeftPressed: {
        device.angle -= 10
    }
    Keys.onRightPressed: {
        device.angle += 10
    }
    Keys.onPressed: {
        if (event.isAutoRepeat) {
//            console.log("SKipping auto repeat")
            return
        }

        if (event.key === Qt.Key_Up) {
            device.speed = 128
        }
    }

    Keys.onReleased: {
        if (event.isAutoRepeat) {
//            console.warn("what the fuck qt, auto repeat release events???")
            return
        }
        if (event.key === Qt.Key_Up) {
            device.speed = 0
        }
    }
}
