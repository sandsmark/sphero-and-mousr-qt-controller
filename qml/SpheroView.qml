import QtQuick 2.0
import QtQuick.Window 2.12
import com.iskrembilen 1.0

// I don't understand qml anymore...
import "." as Lol

Rectangle {
    id: robotView
    anchors.fill: parent

    property SpheroHandler device

    Connections {
        target: device
        onConnectedChanged: {
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
    Image {
        visible: device.isConnected
        anchors {
            top: name.bottom
            topMargin: 20
            bottom: signalStrength.top
            bottomMargin: 20
            horizontalCenter: parent.horizontalCenter
        }
        fillMode: Image.PreserveAspectFit

        source: {
            if (device.robotType == SpheroHandler.Bb8) {
                return "qrc:images/bb8.png"
            }
        }
    }

    Text {
        id: signalStrength
        anchors {
            horizontalCenter: parent.horizontalCenter
            bottom: parent.bottom
        }
        visible: device.isConnected
        text: "Signal strength: " + Math.ceil(device.signalStrength * 100) + "%"
    }
}
