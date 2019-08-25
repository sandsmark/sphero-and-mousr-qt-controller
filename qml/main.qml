import QtQuick 2.0
import QtQuick.Window 2.12
import com.iskrembilen 1.0

// I don't understand qml anymore...
import "." as Lol

Window {
    flags: "Dialog"
    visible: true

    width: 750
    height: 500

    Rectangle {
        id: deviceDiscovery
        anchors.fill: parent
        color: "gray"

        visible: !robotLoader.active

        onVisibleChanged: {
            console.log("Visible changed: " + visible)
            if (visible) {
                DeviceDiscoverer.startScanning()
            } else {
                DeviceDiscoverer.stopScanning()
            }
        }
        Component.onCompleted: {
            console.log("Created");
            DeviceDiscoverer.startScanning()
        }

        Rectangle {
            id: statusBox
            anchors {
                fill: parent
                margins: 50
            }
            radius: 10
            border.width: 2

            Text {
                anchors {
                    fill: parent
                    margins: 20
                }
                font.bold: true
                visible: DeviceDiscoverer.statusString.length > 0
                text: DeviceDiscoverer.statusString
                wrapMode: Text.Wrap
            }

            Item {
                width: 100
                height: width
                anchors.centerIn: parent

                RotationAnimation on rotation {
                          loops: Animation.Infinite
                          from: 0
                          to: 360
                          duration: 500
                      }

                SequentialAnimation on opacity {
                        loops: Animation.Infinite
                        PropertyAnimation { to: 1; duration: 500 }
                        PropertyAnimation { to: 0.1; duration: 500 }
                    }

                Rectangle {
                    width: parent.width
                    height: width
                    radius: parent.width / 2
                    border.width: 2
                }

                Rectangle {
                    width: parent.width / 2
                    height: width
                }
            }

            Text {
                anchors {
                    fill: parent
                    margins: 20
                }
                font.bold: true
                visible: DeviceDiscoverer.device
                text: visible ? DeviceDiscoverer.device.statusString : ""

                wrapMode: Text.Wrap
            }
        }
    }

    Loader {
        id: robotLoader
        active: DeviceDiscoverer.device && DeviceDiscoverer.device.isConnected
        anchors.fill: parent
        sourceComponent: Lol.RobotView {
            device: DeviceDiscoverer.device
        }
    }
}
