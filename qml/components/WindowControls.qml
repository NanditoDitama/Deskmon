import QtQuick
import QtQuick.Window
import "../theme"

Row {
    id: controlsRoot
    spacing: 4
    visible: Qt.platform.os !== "osx"

    readonly property var targetWindow: Window.window
    readonly property bool isMaximized: targetWindow !== null && targetWindow.visibility === Window.Maximized

    // Minimize
    Rectangle {
        id: minBtn
        width: 34
        height: Theme.controlHeight
        radius: Theme.radiusSmall
        color: minArea.pressed
               ? (Theme.isDarkMode ? Qt.rgba(1, 1, 1, 0.18) : Qt.rgba(0, 0, 0, 0.12))
               : (minArea.containsMouse
                  ? (Theme.isDarkMode ? Qt.rgba(1, 1, 1, 0.10) : Qt.rgba(0, 0, 0, 0.06))
                  : "transparent")

        Behavior on color { ColorAnimation { duration: 120 } }

        Rectangle {
            anchors.centerIn: parent
            width: 11
            height: 1.5
            radius: 0.75
            color: Theme.textColor
        }

        MouseArea {
            id: minArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                if (controlsRoot.targetWindow)
                    controlsRoot.targetWindow.showMinimized()
            }
        }

        CustomToolTip {
            text: Lang.t("Minimize")
            shown: minArea.containsMouse
        }
    }

    // Maximize / Restore
    Rectangle {
        id: maxBtn
        width: 34
        height: Theme.controlHeight
        radius: Theme.radiusSmall
        color: maxArea.pressed
               ? (Theme.isDarkMode ? Qt.rgba(1, 1, 1, 0.18) : Qt.rgba(0, 0, 0, 0.12))
               : (maxArea.containsMouse
                  ? (Theme.isDarkMode ? Qt.rgba(1, 1, 1, 0.10) : Qt.rgba(0, 0, 0, 0.06))
                  : "transparent")

        Behavior on color { ColorAnimation { duration: 120 } }

        // Single square when windowed
        Rectangle {
            visible: !controlsRoot.isMaximized
            anchors.centerIn: parent
            width: 11
            height: 11
            radius: 1.5
            color: "transparent"
            border.color: Theme.textColor
            border.width: 1.4
        }

        // Overlapping transparent squares when maximized
        Item {
            visible: controlsRoot.isMaximized
            anchors.centerIn: parent
            width: 12
            height: 12

            // Front square
            Rectangle {
                x: 0
                y: 3
                width: 9
                height: 9
                radius: 1
                color: "transparent"
                border.color: Theme.textColor
                border.width: 1.4
            }

            // Back L-corner (top + right edges only, no opaque fill)
            Rectangle {
                x: 3
                y: 0
                width: 9
                height: 1.4
                radius: 0.7
                color: Theme.textColor
            }
            Rectangle {
                x: 10.6
                y: 0
                width: 1.4
                height: 9
                radius: 0.7
                color: Theme.textColor
            }
        }

        MouseArea {
            id: maxArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                if (!controlsRoot.targetWindow)
                    return
                if (controlsRoot.isMaximized)
                    controlsRoot.targetWindow.showNormal()
                else
                    controlsRoot.targetWindow.showMaximized()
            }
        }

        CustomToolTip {
            text: controlsRoot.isMaximized ? Lang.t("Restore") : Lang.t("Maximize")
            shown: maxArea.containsMouse
        }
    }

    // Close (hides to tray via Main.qml onClosing)
    Rectangle {
        id: closeBtn
        width: 34
        height: Theme.controlHeight
        radius: Theme.radiusSmall
        color: closeArea.pressed
               ? "#C42B1C"
               : (closeArea.containsMouse ? "#E81123" : "transparent")

        Behavior on color { ColorAnimation { duration: 120 } }

        Image {
            anchors.centerIn: parent
            width: 16
            height: 16
            source: "image://icon/close.svg?" + (closeArea.containsMouse || closeArea.pressed ? "#FFFFFF" : Theme.textColor)
            sourceSize.width: 48
            sourceSize.height: 48
            fillMode: Image.PreserveAspectFit
            smooth: true
            mipmap: true
        }

        MouseArea {
            id: closeArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                if (controlsRoot.targetWindow)
                    controlsRoot.targetWindow.close()
            }
        }

        CustomToolTip {
            text: Lang.t("Close")
            shown: closeArea.containsMouse
        }
    }
}
