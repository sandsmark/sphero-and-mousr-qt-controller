import QtQuick 2.0
import QtQuick.Window 2.12
import com.iskrembilen 1.0

import QtQuick.Controls 1.4

// I don't understand qml anymore...
import "." as Lol

Rectangle {
    id: robotView
    property MousrHandler device
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
                direction: RotationAnimation.Shortest
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
                direction: RotationAnimation.Shortest
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
                direction: RotationAnimation.Shortest
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

            visible: device.sensorDirty || device.stuck
            color: "red"

            SequentialAnimation on color {
                loops: Animation.Infinite
                ColorAnimation { to: "red"; duration: 1000 }
                ColorAnimation { to: "white"; duration: 100 }
            }

            Text {
                id: errorText
                anchors.centerIn: parent
                text: {
                    if (device.stuck) {
                        return qsTr("Device is stuck!")
                    } else if (device.sensorDirty) {
                        return qsTr("Sensor is dirty\nPlease clean sensor")
                    } else if (device.isFlipped) {
                        return qsTr("Device is flipped!\nPress Space or flip manually.")
                    } else {
                        return qsTr("Unknown error")
                    }
                }
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
        enabled: !device.isCharging

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
            text: qsTr("Driver assist")
            checked: device.driverAssistEnabled
            onCheckedStateChanged: {
                device.driverAssistEnabled = checkedState === Qt.Checked
            }
        }

        GroupBox {
            title: qsTr("Auto mode")
            width: parent.width - margins

            Column {
                spacing: margins
                width: parent.width

                CheckBox {
                    text: qsTr("Enabled")
                    checked: device.isAutoRunning
                    width: parent.width
                    onCheckedStateChanged: {
                        device.isAutoRunning = checkedState === Qt.Checked
                    }
                }

                ComboBox {
                    model: device.autoplayGameModeNames()
                    currentIndex: device.autoplayGameMode
                    width: parent.width
                    onActivated: {
                        device.autoplayGameMode = index

                        // Restore the binding
                        currentIndex = Qt.binding(function() { return device.autoplayGameMode })
                    }
                }

                ComboBox {
                    model: device.autoplayDrivingModeNames()
                    currentIndex: device.autoplayDrivingMode
                    width: parent.width
                    onActivated: {
                        device.autoplayDrivingMode = index

                        // Restore the binding
                        currentIndex = Qt.binding(function() { return device.autoplayDrivingMode })
                    }
                }
            }
        }

    }

    Component.onCompleted: forceActiveFocus()

    Keys.enabled: !device.isAutoRunning
    focus: true

    Keys.onLeftPressed: {
        device.rotate(MousrHandler.Left);
    }
    Keys.onRightPressed: {
        device.rotate(MousrHandler.Right);
    }
    Keys.onPressed: {
        if (event.key === Qt.Key_Up) {
            if (event.modifiers == Qt.ShiftModifier) {
                device.speed = 1.;
            } else {
                device.speed = 0.5;
            }
        } else if (event.key === Qt.Key_Down) {
            device.speed = -0.5;
        } else {
            return;
        }
        device.controlsPressed = true;
    }

    Keys.onReleased: {
        if (event.isAutoRepeat) {
//            console.warn("what the fuck qt, auto repeat release events???")
            return
        }
        device.controlsPressed = false;

        if (event.key === Qt.Key_Up || event.key === Qt.Key_Down) {
            device.speed = 0;
            device.stop()
        } else if (event.key === Qt.Key_Return) {
            device.flickTail();
        } else if (event.key === Qt.Key_Space) {
            device.flip();
        }
    }
}
