import QtQuick
import "../theme"

Rectangle {
    id: cardRoot

    property color cardColor: Theme.cardColor
    property real cardRadius: Theme.radiusMedium
    property color cardBorderColor: Theme.dividerColor
    property int cardBorderWidth: 1

    color: cardColor
    radius: cardRadius
    border.color: cardBorderColor
    border.width: cardBorderWidth
}
