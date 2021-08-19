
import QtQuick 2.0
import photo_broom.qml 1.0


Item {
    width: repeater.width
    height: repeater.height

    Repeater {
        id: repeater

        width: childrenRect.width
        height: childrenRect.height

        model: ObservablesRegistry.executors

        Column {

            Text {
                text: "Executor"
                font.bold: true
            }

            Text {
                text: qsTr("Tasks in queue") + ": " + modelData.awaitingTasks
            }

            Text {
                text: qsTr("Tasks executed")+ ": " + modelData.tasksExecuted
            }
        }
    }
}
