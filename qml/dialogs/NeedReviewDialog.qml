import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import "../theme"
import "../components"

ApplicationWindow {
    id: needReviewDialog
    width: 440
    height: 350
    title: Lang.t("Review Request Reason")
    modality: Qt.ApplicationModal
    flags: Qt.Dialog | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
    color: "transparent"

    x: (Screen.width - width) / 2
    y: (Screen.height - height) / 2

    visible: false

    property int taskId: -1
    property var loggerRef: (typeof logger !== "undefined") ? logger : null

    Rectangle {
        id: card
        anchors.fill: parent
        radius: Theme.radiusLarge
        color: Theme.cardColor
        border.color: Theme.borderColor
        border.width: 1
        opacity: needReviewDialog.visible ? 1 : 0
        scale: needReviewDialog.visible ? 1 : 0.96

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
                            text: Lang.t("Review Request")
                            font.pixelSize: 16
                            font.weight: Font.DemiBold
                            font.family: "Segoe UI"
                            color: Theme.textColor
                            Layout.fillWidth: true
                        }

                        Label {
                            text: Lang.t("The task will be moved to 'Need Review' status. The manager will receive a notification to conduct a review.")
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
                        onClicked: needReviewDialog.close()
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

                RowLayout {
                    Layout.fillWidth: true

                    Label {
                        text: Lang.t("Review Request Reason") + " *"
                        font.pixelSize: 12
                        font.weight: Font.Medium
                        font.family: "Segoe UI"
                        color: Theme.textColor
                        Layout.fillWidth: true
                    }

                    Label {
                        text: reasonInput.text.length + "/" + reasonInput.maxLength
                        font.pixelSize: 11
                        font.family: "Segoe UI"
                        color: reasonInput.text.length > reasonInput.maxLength * 0.9 ? Theme.dangerColor : Theme.mutedTextColor
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: Theme.radiusSmall
                    color: Theme.surfaceColor
                    border.color: reasonInput.activeFocus ? Theme.primaryColor : Theme.borderColor
                    border.width: 1
                    Behavior on border.color { ColorAnimation { duration: 150 } }

                    ScrollView {
                        anchors.fill: parent
                        anchors.margins: 2
                        clip: true

                        ScrollBar.vertical: CustomScrollBar {}
                        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                        TextArea {
                            id: reasonInput
                            width: parent.width
                            placeholderText: Lang.t("Example: Need verification from project manager, technical issues, etc.")
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

                            property int maxLength: 500
                            onTextChanged: {
                                if (text.length > maxLength) {
                                    text = text.substring(0, maxLength)
                                }
                            }
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
                    text: Lang.t("Cancel")
                    Layout.preferredHeight: Theme.controlHeight
                    btnColor: "transparent"
                    btnHoverColor: Qt.alpha(Theme.textColor, 0.08)
                    btnTextColor: Theme.textColor
                    btnBorderColor: Theme.borderColor
                    btnBorderWidth: 1
                    font.pixelSize: 13
                    font.weight: Font.Medium

                    onClicked: needReviewDialog.close()
                }

                CustomButton {
                    id: submitButton
                    property bool isLoading: false
                    text: enabled ? Lang.t("Submit") : Lang.t("Sending...")
                    enabled: reasonInput.text.trim().length > 0
                    Layout.preferredHeight: Theme.controlHeight
                    btnColor: Theme.primaryColor
                    btnHoverColor: Theme.primaryHoverColor
                    btnTextColor: Theme.isDarkMode ? "#0F172A" : "#FFFFFF"
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                    opacity: enabled ? 1.0 : 0.45

                    onClicked: {
                        var activeLogger = needReviewDialog.loggerRef ? needReviewDialog.loggerRef : ((typeof logger !== "undefined") ? logger : null);
                        if (!activeLogger) {
                            console.error("Logger not set. Please provide logger instance when opening dialog.");
                            return;
                        }

                        activeLogger.submitNeedReview(needReviewDialog.taskId, reasonInput.text.trim());
                        needReviewDialog.close();
                    }
                }
            }
        }
    }

    function openWithTaskId(id, loggerInstance) {
        taskId = id;
        if (loggerInstance) {
            loggerRef = loggerInstance;
        }
        reasonInput.text = "";
        submitButton.isLoading = false;
        show();
        reasonInput.forceActiveFocus();
    }

    Shortcut {
        sequences: ["Escape"]
        onActivated: cancelButton.clicked()
    }

    Shortcut {
        sequences: ["Return", "Enter"]
        enabled: submitButton.enabled
        onActivated: submitButton.clicked()
    }

    Component.onCompleted: {
        if (!loggerRef && typeof logger !== "undefined") {
            needReviewDialog.loggerRef = logger;
        }
        reasonInput.forceActiveFocus();
    }
}
