
import QtQuick 2.14
import QtQuick.Controls 2.3

GridView {

    property int thumbnailSize: 160
    property int thumbnailMargin: 5

    cellWidth: thumbnailSize + thumbnailMargin * 2
    cellHeight: thumbnailSize + thumbnailMargin * 2
    delegate: PhotoDelegate { }
    highlight: highlightId
    keyNavigationEnabled: true

    ScrollBar.vertical: ScrollBar { }

    Component {
        id: highlightId

        Rectangle {
            color: "lightsteelblue";
        }
    }

}
