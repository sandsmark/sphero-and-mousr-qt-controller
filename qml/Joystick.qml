import QtQuick 2.12
import com.iskrembilen 1.0

Item {
    id: joystick
    width: 400
    height: 400

    // Cheap, "fake" dropshadow
    Image {
        anchors {
            fill: dragArea
            margins: -15
        }
        source: "qrc:/radial-shadow.png"
        smooth: true
    }

    Rectangle {
        id: dragArea
        anchors.centerIn: parent

        width: 200
        height: width
        radius: Math.max(width/2, height/2)
        color: joystick.active ? "black" : "white"

        Item {
            id: dragIndicator
            width: 20
            height: width
            property real defaultX: dragArea.width/2 - width/2
            property real defaultY: dragArea.height/2 - height/2
            x: defaultX
            y: defaultY

            Image {
                anchors {
                    fill: dragIndicatorRect
                    margins: -10
                }
                source: joystick.active ? "qrc:/radial-shadow-invert.png" : "qrc:/radial-shadow.png"
                smooth: true
                opacity: joystick.active ? 1 : 0.2
            }

            Rectangle {
                id: dragIndicatorRect
                anchors.centerIn: parent
                color: "white"
                width: parent.width
                height: parent.height
                radius: width/2
            }
        }
    }

    property real startGlobalX: 0
    property real startGlobalY: 0
    property real startX: 0
    property real startY: 0
    property real speed: 0
    property real heading: 0
    property bool active: false

    MouseArea {
        id: dragMouseArea
        anchors.fill: parent
        enabled: true
        cursorShape: Qt.ArrowCursor
        onPressed:{
            if (!dragArea.contains(mapToItem(dragArea, Qt.point(mouse.x, mouse.y)))) {
                return;
            }
            joystick.active = true;

            const globalPos = mapToGlobal(mouse.x, mouse.y);
            startGlobalX = globalPos.x;
            startGlobalY = globalPos.y;

            const localPos = mapToItem(dragArea, mouse.x, mouse.y);
            startX = localPos.x;
            startY = localPos.y;
            cursorShape = Qt.BlankCursor;
            speed = 0;
        }
        onReleased: {
            if (!joystick.active) {
                return;
            }

            joystick.active = false;
            Cursor.setPosition(startGlobalX, startGlobalY);
            dragIndicator.x = dragIndicator.defaultX;
            dragIndicator.y = dragIndicator.defaultY;

            speed = 0;

            cursorShape = Qt.ArrowCursor;
        }
        onPositionChanged: {
            if (!joystick.active) {
                return;
            }

            var newPos = mapToItem(dragArea, mouse.x, mouse.y);
            const origPos = newPos;
            var changed = false;
            const maxX = dragArea.width + dragIndicator.width * 2;
            const maxY = dragArea.height + dragIndicator.height * 2;
            const minX = -(dragIndicator.width*2);
            const minY = -(dragIndicator.height*2);
            if (newPos.x > maxX) {
                newPos.x = maxX;
                changed = true;
            }
            if (newPos.y > maxY) {
                newPos.y = maxY;
                changed = true;
            }
            if (newPos.x < minX) {
                newPos.x = minX;
                changed = true;
            }
            if (newPos.y < minY) {
                newPos.y = minY;
                changed = true;
            }

            const deltaX = newPos.x - startX;
            const deltaY = newPos.y - startY;
            var distance = Math.sqrt(deltaX * deltaX + deltaY * deltaY);
            const maxDistance = dragArea.width / 2;
            if (distance > maxDistance) {
                distance = maxDistance;
            }

            const angle = Math.atan2(deltaY, deltaX);

            dragIndicator.x = Math.cos(angle) * distance + dragArea.width / 2 - dragIndicator.width/2;
            dragIndicator.y = Math.sin(angle) * distance + dragArea.height / 2 - dragIndicator.height/2;
            dragIndicatorRect.rotation = angle*180/Math.PI;

            heading = angle;
            speed = distance / maxDistance;

            if (changed) {
                const gpos = dragArea.mapToGlobal(newPos.x, newPos.y);
                Cursor.setPosition(gpos.x, gpos.y);
                return;
            }
        }
    }
}
