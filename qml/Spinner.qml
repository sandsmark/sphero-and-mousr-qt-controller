import QtQuick 2.0
import QtQuick.Window 2.12
import com.iskrembilen 1.0

// I don't understand qml anymore...
import "." as Lol

Item {
    width: spinner.width
    height: spinner.height

    Item {
        id: spinner
        width: 100
        height: width
        anchors.centerIn: parent

        RotationAnimation on rotation {
            running: spinner.visible
            loops: Animation.Infinite
            from: 0
            to: 360
            duration: 1000
        }


        Rectangle {
            color: circle.color
            width: parent.width / 2
            height: width
            border.width: 2
            border.color: circle.border.color
        }

        Rectangle {
            id: circle
            width: parent.width
            height: width
            radius: parent.width / 2
            border.width: 2
            border.color: "white"

            SequentialAnimation on color {
                running: spinner.visible
                loops: Animation.Infinite
                ColorAnimation { to: "white"; duration: 500 }
                ColorAnimation { to: "black"; duration: 500 }
            }
            SequentialAnimation on border.color {
                running: spinner.visible
                loops: Animation.Infinite
                ColorAnimation { to: "black"; duration: 500 }
                ColorAnimation { to: "white"; duration: 500 }
            }
        }

        Rectangle {
            color: circle.color
            x: 2
            y: 2
            width: parent.width / 2 - 4
            height: width
        }
    }
}
