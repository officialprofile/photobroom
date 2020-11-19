
import QtQuick 2.15
import QtQuick.Controls 2.15
import "../Components" as Components

Item {
    id: main

    state: "Detecting Faces"

    function setDetectionState(state) {
        if (state === 0) {
            // nothing to do
        } else if (state === 1) {
            main.state = "Faces Detected";
            notificationTimer.running = true;
        } else if (state === 2) {
            main.state = "No Face Detected";
            notificationTimer.running = true;
        } else if (state === 10) {
            main.state = "Notification Hidden";
        }
    }

    Timer {
        id: notificationTimer
        interval: 4000
        onTriggered: setDetectionState(10);
    }

    Components.ZoomableImage {
        id: imageArea
        objectName: "flickablePhoto"

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: notificationArea.top

        boundsBehavior: Flickable.StopAtBounds

        Item {
            id: shadow
            anchors.fill: parent

            property real leftEdge: 100
            property real rightEdge: 200
            property real topEdge: 50
            property real bottomEdge: 100

            Rectangle {
                id: leftRect
                x: 0
                y: 0

                width: shadow.leftEdge
                height: shadow.height

                color: "black"
                opacity: 0.5
            }

            Rectangle {
                id: rightRect
                x: shadow.rightEdge
                y: 0

                width: shadow.width - shadow.rightEdge
                height: shadow.height

                color: "black"
                opacity: 0.5
            }

            Rectangle {
                id: topRect
                x: shadow.leftEdge
                y: 0

                width: shadow.rightEdge - shadow.leftEdge
                height: shadow.topEdge

                color: "black"
                opacity: 0.5
            }

            Rectangle {
                id: bottomRect
                x: shadow.leftEdge
                y: shadow.bottomEdge

                width: shadow.rightEdge - shadow.leftEdge
                height: shadow.height - shadow.bottomEdge

                color: "black"
                opacity: 0.5
            }
        }
    }

    Rectangle {
        id: notificationArea

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        height: row.height
        color: "dodgerblue"

        radius: 5

        Row {
            id: row

            height: busyIndicator.height
            leftPadding: 5

            Text {
                id: status
                text: qsTr("Detecting and analyzing faces")
                anchors.verticalCenter: parent.verticalCenter
                font.pixelSize: 12
            }

            BusyIndicator {
                id: busyIndicator
                running: false
            }
        }
    }
    states: [
        State {
            name: "Detecting Faces"

            PropertyChanges {
                target: busyIndicator
                running: true
            }
        },
        State {
            name: "No Face Detected"

            PropertyChanges {
                target: status
                text: qsTr("Could not detect any face.")
            }
        },
        State {
            name: "Faces Detected"

            PropertyChanges {
                target: status
                text: qsTr("Faces detected a recognised.")
            }
        },
        State {
            name: "Notification Hidden"

            PropertyChanges {
                target: notificationArea
                height: 0
            }
        }
    ]

    transitions: [
        Transition {
            id: facesDetected

            from: "*"
            to: "Notification Hidden"

            PropertyAnimation { target: notificationArea; properties: "height"; }
        }
    ]
}

/*##^##
Designer {
    D{i:0;autoSize:true;height:480;width:640}
}
##^##*/
