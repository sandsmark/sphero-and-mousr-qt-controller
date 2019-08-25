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
        visible: !device.isCharging
        width: parent.width / 3 - margins * 2

        Image {
            source: "qrc:images/top.png"
            rotation: device.zRotation
            width: height
            height: robotView.height / 3
            fillMode: Image.PreserveAspectFit
        }

        Image {
            source: "qrc:images/front.png"
            rotation: device.xRotation
            width: height
            height: robotView.height / 3
            fillMode: Image.PreserveAspectFit
        }


        Image {
            source: "qrc:images/side.png"
            rotation: device.yRotation
            width: height
            height: robotView.height / 3
            fillMode: Image.PreserveAspectFit
        }
    }


    Image {
        visible: device.isCharging

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
                opacity: device.voltage / 100
                color: "green"
                width: parent.width * device.voltage / 100
                height: parent.height
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

        Text {
            width: statusColumn.width
            horizontalAlignment: Text.AlignHCenter
            text: qsTr("Memory usage: ") + device.memory
            opacity: 0.25
        }
    }



    Column {
        id: controls
        y: robotView.margins
        spacing: robotView.margins
        width: parent.width / 3 - margins * 2

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
