import QtQuick
import "../theme"

Rectangle {
    id: badge

    property string status: "Pending"
    property int fontSize: Theme.fontSizeSmall - 1

    function getStatusColor(s) {
        var str = s.toLowerCase()
        if (str === "on-progress" || str === "on progress") return Theme.successColor
        if (str === "review" || str === "need review" || str === "on-review") return Theme.infoColor
        if (str === "paused") return Theme.warningColor
        if (str === "need revise") return Theme.dangerColor
        return Theme.neutralColor
    }

    implicitWidth: statusText.implicitWidth + 16
    implicitHeight: statusText.implicitHeight + 8
    radius: Theme.radiusMedium

    color: Qt.rgba(getStatusColor(status).r, getStatusColor(status).g, getStatusColor(status).b, 0.12)
    border.width: 1
    border.color: getStatusColor(status)

    Text {
        id: statusText
        anchors.centerIn: parent
        text: badge.status
        font.pixelSize: badge.fontSize
        font.weight: Font.DemiBold
        color: badge.getStatusColor(badge.status)
    }
}
