import QtQuick 2.0
import QtQuick.Window 2.12
import com.iskrembilen 1.0

// I don't understand qml anymore...
import "." as Lol

Item {
    id: spinner
    width: 100
    height: width
    
    RotationAnimation on rotation {
        running: spinner.visible
        loops: Animation.Infinite
        from: 0
        to: 360
        duration: 500
    }
    
    SequentialAnimation on opacity {
        running: spinner.visible
        loops: Animation.Infinite
        PropertyAnimation { to: 1; duration: 500 }
        PropertyAnimation { to: 0.5; duration: 500 }
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
