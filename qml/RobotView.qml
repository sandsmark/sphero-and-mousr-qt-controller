import QtQuick 2.0
import QtQuick.Window 2.12
import com.iskrembilen 1.0

import QtQuick.Controls 1.4

Rectangle {
    id: robotView
    property DeviceHandler device
    readonly property int margins: 10
    anchors.fill: parent
//    visible: device.isConnected // TODO separate component so we don't need to check all the time

    Column {
        id: orientationView
        x: robotView.margins
        y: robotView.margins

        Image {
            source: "qrc:images/top.png"
            rotation: device ? device.zRotation : 0
            width: height
            height: robotView.height / 3
            fillMode: Image.PreserveAspectFit
        }

        Image {
            source: "qrc:images/front.png"
            rotation: device ? device.xRotation : 0
            width: height
            height: robotView.height / 3
            fillMode: Image.PreserveAspectFit
        }


        Image {
            source: "qrc:images/side.png"
            rotation: device ? device.yRotation : 0
            width: height
            height: robotView.height / 3
            fillMode: Image.PreserveAspectFit
        }
    }

    Column {
        id: statusColumn
        y: robotView.margins

        anchors {
            left: orientationView.right
            right: controls.left
            margins: robotView.margins
        }

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
                opacity: device.voltage / 100
                color: "green"
                width: parent.width * device.voltage / 100
                height: parent.height
            }

            // voltage????
            Text {
                anchors.centerIn: parent
                text: device.voltage  + "%"
            }
        }

        Text {
            width: statusColumn.width
            visible: device.isCharging
            horizontalAlignment: Text.AlignHCenter
            text: qsTr("Charging")
        }

        Item {
            visible: device.isCharging
            width: parent.width
            height: robotView.height / 3

            Image {
                anchors.centerIn: parent
                width: parent.height
                height: parent.height
                fillMode: Image.PreserveAspectFit
                source: "qrc:images/charging.png"
            }
        }

    }



    Column {
        id: controls
        y: robotView.margins
        spacing: robotView.margins

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
