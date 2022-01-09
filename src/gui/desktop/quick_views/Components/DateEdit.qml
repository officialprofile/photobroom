
import QtQuick
import QtQuick.Controls


Row {
    id: root

    required property date value
    signal accepted()

    TextInput {
        id: input

        height: parent.height
        verticalAlignment: TextInput.AlignVCenter

        inputMask: "99.99.9999"
        color: valid? "black": "red"

        property bool valid: true

        onTextChanged: {
            const userDate = Date.fromLocaleDateString(Qt.locale(), input.text, "d.M.yyyy");

            input.valid = isNaN(userDate.getTime()) == false
            if (input.valid)
                root.value = userDate
        }

        onEditingFinished: root.accepted()
    }

    Component.onCompleted: {
        input.text = root.value.toLocaleDateString(Qt.locale(), "dd.MM.yyyy");
        input.forceActiveFocus();
    }
}
