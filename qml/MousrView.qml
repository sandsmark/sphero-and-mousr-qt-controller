import QtQuick 2.0
import QtQuick.Window 2.12
import com.iskrembilen 1.0

import QtQuick.Controls 1.4

// I don't understand qml anymore...
import "." as Lol

Rectangle {
    id: robotView
    property DeviceHandler device
    readonly property int margins: 10
    anchors.fill: parent

    Lol.Spinner {
        id: spinner
        anchors.centerIn: parent

        visible: !device.isConnected
    }

    Text {
        id: statusString

        anchors {
            horizontalCenter: parent.horizontalCenter
            top: spinner.bottom
            topMargin: 20
        }

        font.bold: true
        visible: !device.isConnected
        text: DeviceDiscoverer.device.statusString
    }

    Column {
        id: orientationView
        visible: device.isConnected && !device.isCharging

        x: robotView.margins
        y: robotView.margins
        width: parent.width / 3 - margins * 2

        Image {
            id: topImage
            source: "qrc:images/top.png"
            width: height
            height: robotView.height / 3
            fillMode: Image.PreserveAspectFit

            RotationAnimation on rotation {
                running: topImage.rotation != device.zRotation
                to: device.zRotation
            }
        }

        Image {
            id: frontImage
            source: "qrc:images/front.png"

            width: height
            height: robotView.height / 3
            fillMode: Image.PreserveAspectFit

            RotationAnimation on rotation {
                running: frontImage.rotation != device.xRotation
                to: device.xRotation
            }
        }


        Image {
            id: sideImage
            source: "qrc:images/side.png"
            width: height
            height: robotView.height / 3
            fillMode: Image.PreserveAspectFit

            RotationAnimation on rotation {
                running: sideImage.rotation != device.xRotation
                to: device.xRotation
            }
        }
    }


    Image {
        visible: device.isConnected && device.isCharging

        anchors {
            verticalCenter: parent.verticalCenter
        }

        x: robotView.margins
        width: parent.width / 3 - margins * 2
        height: parent.width / 3 - margins * 2

        fillMode: Image.PreserveAspectFit
        source: "qrc:images/charging.png"
    }


    Column {
        id: statusColumn
        x: parent.width / 3
        y: robotView.margins
        visible: device.isConnected
        width: parent.width / 3 - margins * 2

        Rectangle {
            id: chargeRect
            border.width: 1
            width: statusColumn.width
            height: 50

            Rectangle {
                color: device.isBatteryLow ? "red" :  "yellow"
                anchors.fill: percentageRect
            }

            Rectangle {
                id: percentageRect
                x: 1
                y: 1
                opacity: device.voltage / 100
                color: "green"
                width: parent.width * device.voltage / 100 - 2
                height: parent.height - 2
            }

            // voltage????
            Text {
                anchors.centerIn: parent
                text: device.isFullyCharged ? qsTr("Fully charged") : device.voltage  + "%"
            }
        }

        Text {
            width: statusColumn.width
            visible: device.isCharging
            horizontalAlignment: Text.AlignHCenter
            text: qsTr("Charging")
        }

        Rectangle {
            width: parent.width
            height: errorText.height + robotView.margins

            visible: device.sensorDirty
            color: "red"

            SequentialAnimation on color {
                loops: Animation.Infinite
                ColorAnimation { to: "red"; duration: 1000 }
                ColorAnimation { to: "white"; duration: 100 }
            }

            Text {
                id: errorText
                anchors.centerIn: parent
                text: qsTr("Sensor is dirty\nPlease clean sensor")
                horizontalAlignment: Text.AlignHCenter
            }
        }

        Text {
            width: statusColumn.width
            horizontalAlignment: Text.AlignHCenter
            text: qsTr("Memory usage: ") + device.memory
            opacity: 0.25
        }

        Text {
            width: statusColumn.width
            horizontalAlignment: Text.AlignHCenter
            text: qsTr("Pause time: ") + device.autoPlay.pauseTime
            opacity: 0.25
        }
    }



    Column {
        id: controls
        y: robotView.margins
        spacing: robotView.margins
        width: parent.width / 3 - margins * 2
        visible: device.isConnected

        anchors {
            margins: robotView.margins
            right: parent.right
        }

        Button {
            id: chirpButton
            onClicked: device.chirp()
            text: qsTr("Chirp")
        }

        CheckBox {
            text: qsTr("Auto mode")
            checked: device.isAutoRunning
        }
    }
}
