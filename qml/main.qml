import QtQuick 2.0
import QtQuick.Window 2.12
import com.iskrembilen 1.0

Window {
    flags: "Dialog"
    visible: true

    width: 750
    height: 500

    Rectangle {
        id: deviceDiscovery
        anchors.fill: parent
        color: "gray"

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
//                onVisibleChanged: console.log(DeviceDiscoverer.device)
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
}
