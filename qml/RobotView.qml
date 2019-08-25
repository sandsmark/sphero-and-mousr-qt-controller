import QtQuick 2.0
import QtQuick.Window 2.12
import com.iskrembilen 1.0

Item {
    id: robotView
    property DeviceHandler device
    visible: device.isConnected // TODO separate component so we don't need to check all the time

    Image {
        source: "qrc:images/top.png"
        rotation: device.xRotation

    }
}
