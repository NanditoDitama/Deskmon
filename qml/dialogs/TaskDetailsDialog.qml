// TaskDetailsDialog.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import "../theme"
import "../components"

ApplicationWindow {
    id: root
    width: 440
    height: 340
    title: Lang.t("Task Details")
    modality: Qt.ApplicationModal
    flags: Qt.Dialog | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
    color: "transparent"

    x: (Screen.width - width) / 2
    y: (Screen.height - height) / 2

    visible: false

    property int currentTaskId: -1
    property string action: ""
    property int nextTaskId: -1
    property bool isLoading: false

    Rectangle {
        id: card
        anchors.fill: parent
        radius: Theme.radiusLarge
        color: Theme.cardColor
        border.color: Theme.borderColor
        border.width: 1
        opacity: root.visible ? 1 : 0
        scale: root.visible ? 1 : 0.96

        Behavior on opacity { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }
        Behavior on scale { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // Clean Header
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: headerCol.implicitHeight + 28

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 20
                    anchors.rightMargin: 16
                    anchors.topMargin: 16
                    anchors.bottomMargin: 12
                    spacing: 12

                    ColumnLayout {
                        id: headerCol
                        Layout.fillWidth: true
                        spacing: 3

                        Label {
                            text: Lang.t("Task Details")
                            font.pixelSize: 16
                            font.weight: Font.DemiBold
                            font.family: "Segoe UI"
                            color: Theme.textColor
                            Layout.fillWidth: true
                        }

                        Label {
                            text: Lang.t("You can add details of what you worked on in the previous task here (Optional)")
                            font.pixelSize: 12
                            font.family: "Segoe UI"
                            color: Theme.mutedTextColor
                            wrapMode: Text.Wrap
                            Layout.fillWidth: true
                            lineHeight: 1.3
                        }
                    }

                    Button {
                        id: closeBtn
                        Layout.alignment: Qt.AlignTop
                        Layout.preferredWidth: 28
                        Layout.preferredHeight: 28
                        topInset: 0
                        bottomInset: 0
                        leftInset: 0
                        rightInset: 0
                        padding: 0
                        flat: true
                        enabled: !root.isLoading
                        onClicked: cancelButton.clicked()
                        background: Rectangle {
                            radius: Theme.radiusSmall
                            color: closeBtn.hovered ? Qt.alpha(Theme.textColor, 0.08) : "transparent"
                        }
                        contentItem: Text {
                            text: "×"
                            font.pixelSize: 18
                            color: Theme.lightTextColor
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }

                Rectangle {
                    anchors.bottom: parent.bottom
                    width: parent.width
                    height: 1
                    color: Theme.dividerColor
                }
            }

            // Body
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.margins: 20
                Layout.bottomMargin: 14
                spacing: 8

                Label {
                    text: Lang.t("Add task details (Optional)")
                    font.pixelSize: 12
                    font.weight: Font.Medium
                    font.family: "Segoe UI"
                    color: Theme.textColor
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: Theme.radiusSmall
                    color: Theme.surfaceColor
                    border.color: detailsInput.activeFocus ? Theme.primaryColor : Theme.borderColor
                    border.width: 1
                    Behavior on border.color { ColorAnimation { duration: 150 } }

                    ScrollView {
                        anchors.fill: parent
                        anchors.margins: 2
                        clip: true

                        ScrollBar.vertical: CustomScrollBar {}
                        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                        TextArea {
                            id: detailsInput
                            width: parent.width
                            placeholderText: Lang.t("Example: Finished fixing bugs on a feature...")
                            placeholderTextColor: Theme.mutedTextColor
                            wrapMode: Text.Wrap
                            font.pixelSize: 13
                            font.family: "Segoe UI"
                            color: Theme.textColor
                            selectByMouse: true
                            leftPadding: 10
                            rightPadding: 10
                            topPadding: 8
                            bottomPadding: 8
                            background: Item {}
                        }
                    }
                }
            }

            // Footer Buttons
            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 20
                Layout.rightMargin: 20
                Layout.bottomMargin: 18
                spacing: 10

                Item { Layout.fillWidth: true }

                CustomButton {
                    id: cancelButton
                    text: Lang.t("Skip")
                    enabled: !root.isLoading
                    Layout.preferredHeight: Theme.controlHeight
                    btnColor: "transparent"
                    btnHoverColor: Qt.alpha(Theme.textColor, 0.08)
                    btnTextColor: Theme.textColor
                    btnBorderColor: Theme.borderColor
                    btnBorderWidth: 1
                    font.pixelSize: 13
                    font.weight: Font.Medium

                    onClicked: {
                        console.log("Dialog detail ditutup. Aksi: " + root.action);
                        if (logger) {
                            logger.taskDetailsDialogClosed(root.action);
                        }
                        root.close();
                    }
                }

                CustomButton {
                    id: submitButton
                    text: root.isLoading ? Lang.t("Sending...") : Lang.t("Submit")
                    enabled: detailsInput.text.trim().length > 0 && !root.isLoading
                    Layout.preferredHeight: Theme.controlHeight
                    btnColor: Theme.primaryColor
                    btnHoverColor: Theme.primaryHoverColor
                    btnTextColor: Theme.isDarkMode ? "#0F172A" : "#FFFFFF"
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                    opacity: enabled ? 1.0 : 0.45

                    onClicked: {
                        root.isLoading = true;
                        if (logger) {
                            logger.submitTaskDetails(currentTaskId, detailsInput.text.trim(), root.action, root.nextTaskId);
                        }
                    }
                }

                Connections {
                    target: logger

                    function onTaskDetailsSubmissionSuccess() {
                        root.visible = false
                    }

                    function onTaskDetailsSubmissionFailed(message) {
                        root.isLoading = false
                    }
                }
            }
        }
    }

    function show(taskId, actionType, nextId) {
        root.currentTaskId = taskId;
        root.action = actionType;
        root.nextTaskId = nextId;
        detailsInput.text = "";
        root.isLoading = false;
        root.visible = true;
        detailsInput.forceActiveFocus();
    }

    Shortcut {
        sequences: ["Escape"]
        enabled: !root.isLoading
        onActivated: cancelButton.clicked()
    }

    Shortcut {
        sequences: ["Return", "Enter"]
        enabled: submitButton.enabled
        onActivated: submitButton.clicked()
    }
}
