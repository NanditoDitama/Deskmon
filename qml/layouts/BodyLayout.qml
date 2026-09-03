import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"
import "../dashboard"
import "../dialogs"

GridLayout {
    id: root
    Layout.fillWidth: true
    Layout.fillHeight: true
    Layout.margins: 10
    columns: 1
    columnSpacing: 16
    rowSpacing: 16

    signal needReviewRequested(int taskId)

    // Top Card: Productivity View
    Frame {
        id: combinedCard
        Layout.fillWidth: true
        Layout.preferredHeight: 320
        padding: 16

        background: Rectangle {
            color: Theme.cardColor
            radius: Theme.radiusLarge
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
        columnSpacing: 14
        rowSpacing: 14

        Frame {
            Layout.fillWidth: true
            Layout.fillHeight: true
            padding: 16
            background: Rectangle {
                color: Theme.cardColor
                radius: Theme.radiusSmall
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
            padding: 16
            background: Rectangle {
                color: Theme.cardColor
                radius: Theme.radiusSmall
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
            text: "Mark as Need Review"
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
