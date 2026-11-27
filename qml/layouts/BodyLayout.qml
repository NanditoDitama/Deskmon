import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import "../theme"
import "../dashboard"
import "../dialogs"

GridLayout {
    id: root
    readonly property bool isCompact: Window.window ? Window.window.width < 1060 : false
    Layout.fillWidth: true
    Layout.fillHeight: true
    Layout.margins: isCompact ? Theme.spacingMedium : Theme.spacingLarge
    columns: 1
    columnSpacing: isCompact ? Theme.spacingMedium : Theme.spacingLarge
    rowSpacing: isCompact ? Theme.spacingMedium : Theme.spacingLarge

    signal needReviewRequested(int taskId)

    // Top Card: Productivity View
    Frame {
        id: combinedCard
        Layout.fillWidth: true
        Layout.preferredHeight: root.isCompact ? 295 : 320
        padding: root.isCompact ? Theme.spacingMedium : Theme.spacingLarge

        background: Rectangle {
            color: Theme.cardColor
            radius: Theme.radiusMedium
            layer.enabled: true
            border.color: Theme.dividerColor
            border.width: 1
        }

        ProductivityView {
            anchors.fill: parent
        }
    }

    // Bottom Grid: Current Task + Activity Monitor
    GridLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        columns: 2
        columnSpacing: root.isCompact ? Theme.spacingMedium : Theme.spacingLarge
        rowSpacing: root.isCompact ? Theme.spacingMedium : Theme.spacingLarge

        Frame {
            Layout.fillWidth: true
            Layout.fillHeight: true
            padding: root.isCompact ? Theme.spacingMedium : Theme.spacingLarge
            background: Rectangle {
                color: Theme.cardColor
                radius: Theme.radiusMedium
                border.color: Theme.dividerColor
                border.width: 1
            }

            CurrentTaskView {
                id: currentTaskView
                anchors.fill: parent
            }
        }

        Frame {
            Layout.fillWidth: true
            Layout.fillHeight: true
            padding: root.isCompact ? Theme.spacingMedium : Theme.spacingLarge
            background: Rectangle {
                color: Theme.cardColor
                radius: Theme.radiusMedium
                border.color: Theme.dividerColor
                border.width: 1
            }

            ActivityMonitorView {
                anchors.fill: parent
            }
        }
    }

    // Context Menu for Tasks
    Menu {
        id: stableTaskMenu
        property int taskId: -1
        property int userId: -1
        property string authToken: ""

        MenuItem {
            text: Lang.t("Mark as Need Review")
            font.pixelSize: 13

            background: Rectangle {
                color: parent.hovered ? Qt.rgba(255/255, 152/255, 0/255, 0.1) : "transparent"
                radius: Theme.radiusSmall
            }

            onTriggered: {
                root.needReviewRequested(stableTaskMenu.taskId)
            }
        }
    }

    TaskDetailPopup {
        id: taskDetailPopup
    }

    ConfirmSwitchDialog {
        id: confirmSwitchDialog
        onConfirmed: function(targetTaskId) {
            if (typeof logger !== "undefined") {
                logger.setActiveTask(targetTaskId)
            }
            currentTaskView.isTimeUpPopupOpen = false
            currentTaskView.isTimeUpWarningOpen = false
        }
    }
}
