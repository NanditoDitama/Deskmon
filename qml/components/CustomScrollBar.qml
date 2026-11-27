import QtQuick
import QtQuick.Controls
import "../theme"

ScrollBar {
    id: control
    orientation: Qt.Vertical
    hoverEnabled: true
    policy: ScrollBar.AsNeeded
    implicitWidth: 10
    padding: 1
    anchors.right: parent ? parent.right : undefined
    anchors.top: parent ? parent.top : undefined
    anchors.bottom: parent ? parent.bottom : undefined

    property bool containerHovered: true
    readonly property bool isScrollable: size > 0 && size < 1.0
    readonly property bool isRevealed: isScrollable && (containerHovered || hovered || pressed || active)

    background: Rectangle {
        color: "transparent"
    }

    contentItem: Rectangle {
        visible: control.isScrollable
        implicitWidth: (control.hovered || control.pressed) ? 8 : 4
        radius: width / 2
        color: control.pressed
               ? Theme.primaryColor
               : (control.hovered ? Theme.lightTextColor : Theme.mutedTextColor)
        opacity: !control.isRevealed ? 0.0 : (control.pressed ? 0.95 : (control.hovered ? 0.85 : 0.45))

        Behavior on implicitWidth {
            NumberAnimation { duration: 150; easing.type: Easing.OutCubic }
        }
        Behavior on opacity {
            NumberAnimation { duration: 150 }
        }
    }
}
