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

    Column {
        id: orientationView
        visible: !device.isCharging

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
                running: sideImage.rotation != device.yRotation
                to: device.yRotation
            }
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
            text: qsTr("Pause time: ") + device.autoplayPauseTime
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

        GroupBox {
            title: qsTr("Auto mode")
            width: parent.width - margins

            Column {
                spacing: margins

                CheckBox {
                    text: qsTr("Enabled")
                    checked: device.isAutoRunning
                    onCheckedStateChanged: {
                        console.log("Disable or enable, what u want", checkedState)
                    }
                }

                ComboBox {
                    model: device.autoplayGameModeNames()
                    currentIndex: device.autoplayGameMode
                    onActivated: {
                        device.autoplayGameMode = index

                        // Restore the binding
                        currentIndex = Qt.binding(function() { return device.autoplayGameMode })
                    }
                }

                ComboBox {
                    model: device.autoplayDrivingModeNames()
                    currentIndex: device.autoplayDrivingMode
                    onActivated: {
                        device.autoplayDrivingMode = index

                        // Restore the binding
                        currentIndex = Qt.binding(function() { return device.autoplayDrivingMode })
                    }
                }
            }
        }

    }
}
