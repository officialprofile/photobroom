
import QtQuick
import photo_broom.qml 1.0
import "../../Components" as Components


Item {
    required property var tagType
    required property var value

    Variant {
        id: variant
    }

    Component {
        id: textDelegate

        Text {
            verticalAlignment: Text.AlignVCenter
            text: variant.localize(tagType, value);
        }
    }

    Component {
        id: categoryDelegate

        Rectangle {
            radius: 5
            color: variant.toColor(value);
            border.color: Qt.darker(color, 2.0)
            border.width: 1
        }
    }

    Component {
        id: ratingDelegate

        Components.StarRating {
            rating: variant.localize(tagType, value);
        }
    }

    Loader {
        active: value !== undefined
        anchors.fill: parent

        sourceComponent: {
            switch(tagType) {
                case TagEnums.Date: return textDelegate;
                case TagEnums.Time: return textDelegate;
                case TagEnums.Place: return textDelegate;
                case TagEnums.Event: return textDelegate;
                case TagEnums.Rating: return ratingDelegate;
                case TagEnums.Category: return categoryDelegate;
                default: return undefined;
            }
        }
    }
}
