// FailureDialog.qml (Modern Version)
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import "../theme"

ApplicationWindow {
    id: root
    width: 340
    height: 180
    title: Lang.t("An Error Occurred")
    modality: Qt.ApplicationModal
    flags: Qt.Dialog | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
    color: "transparent"

    // Pusatkan window
    x: (Screen.width - width) / 2
    y: (Screen.height - height) / 2

    visible: false

    // Properti untuk menyimpan data yang akan dikirim ulang
    property int failedTaskId: -1
    property string failedDetails: ""
    property string failedAction: ""
    property int failedNextTaskId: -1
    property var logger: null

    // Modern background
    Rectangle {
        id: card
        anchors.fill: parent
        radius: 16
        color: Theme.cardColor
        border.color: Qt.alpha(Theme.dangerColor, 0.15)
        border.width: 1
        opacity: root.visible ? 1 : 0
        scale: root.visible ? 1 : 0.96

        Behavior on opacity { NumberAnimation { duration: 250; easing.type: Easing.OutCubic } }
        Behavior on scale { NumberAnimation { duration: 250; easing.type: Easing.OutBack; easing.overshoot: 1.2 } }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 24
            spacing: 20

            // HEADER: ikon + judul
            RowLayout {
                Layout.fillWidth: true
                spacing: 14

                // Icon error
                Rectangle {
                    width: 40
                    height: 40
                    radius: 20
                    color: Qt.alpha(Theme.dangerColor, 0.12)
                    Layout.alignment: Qt.AlignVCenter

                    // Pulse animation
                    SequentialAnimation on scale {
                        running: root.visible
                        loops: Animation.Infinite
                        NumberAnimation { from: 1.0; to: 1.08; duration: 1200; easing.type: Easing.InOutQuad }
                        NumberAnimation { from: 1.08; to: 1.0; duration: 1200; easing.type: Easing.InOutQuad }
                    }

                    Image {
                        anchors.centerIn: parent
                        width: 24
                        height: 24
                        source: "qrc:/icon.ico"
                        fillMode: Image.PreserveAspectFit
                        smooth: true
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    Label {
                        text: Lang.t("An Error Occurred")
                        font.pixelSize: 17
                        font.weight: Font.DemiBold
                        color: Theme.textColor
                        Layout.fillWidth: true
                    }

                    Label {
                        id: errorMessageLabel
                        text: Lang.t("Failed to send data to server.")
                        font.pixelSize: 13
                        color: Qt.alpha(Theme.textColor, 0.6)
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                        maximumLineCount: 2
                        elide: Text.ElideRight
                    }
                }
            }

            // SPACER
            Item { Layout.fillHeight: true }

            // TOMBOL AKSI
            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Item { Layout.fillWidth: true }

                Button {
                    id: cancelButton
                    text: Lang.t("Later")

                    leftPadding: 18
                    rightPadding: 18
                    topPadding: 10
                    bottomPadding: 10

                    background: Rectangle {
                        radius: 8
                        color: cancelButton.hovered ? Qt.alpha(Theme.textColor, 0.08) : "transparent"
                        border.width: 1
                        border.color: Theme.dividerColor

                        Behavior on color { ColorAnimation { duration: 150 } }
                    }

                    contentItem: Text {
                        text: cancelButton.text
                        font.pixelSize: 13
                        font.weight: Font.Medium
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        color: Qt.alpha(Theme.textColor, 0.7)
                    }

                    onClicked: {
                        console.log("Pengiriman ulang dibatalkan oleh pengguna.");
                        if (failedAction === "quit" && logger) {
                            logger.taskDetailsDialogClosed("quit");
                        }
                        root.close();
                    }
                }

                Button {
                    id: retryButton
                    text: Lang.t("Retry")

                    leftPadding: 24
                    rightPadding: 24
                    topPadding: 10
                    bottomPadding: 10

                    background: Rectangle {
                        radius: 8
                        color: retryButton.pressed ? Qt.darker(Theme.dangerColor, 1.1) :
                               retryButton.hovered ? Qt.lighter(Theme.dangerColor, 1.05) : Theme.dangerColor
                        border.width: 0

                        Behavior on color { ColorAnimation { duration: 150 } }
                    }

                    contentItem: Text {
                        text: retryButton.text
                        font.pixelSize: 13
                        font.weight: Font.DemiBold
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        color: "white"
                    }

                    onClicked: {
                        console.log("Mencoba lagi untuk mengirim detail task...");
                        if (logger) {
                            logger.submitTaskDetails(failedTaskId, failedDetails, failedAction, failedNextTaskId);
                        }
                        root.close();
                    }
                }
            }
        }
    }

    // Fungsi untuk menampilkan dialog
    function show(message, taskId, details, action, nextTaskId) {
        errorMessageLabel.text = message;
        failedTaskId = taskId;
        failedDetails = details;
        failedAction = action;
        failedNextTaskId = nextTaskId;
        root.visible = true;
    }

    // Keyboard shortcuts
    Shortcut {
        sequences: ["Escape"]
        onActivated: cancelButton.clicked()
    }

    Shortcut {
        sequences: ["Return", "Enter"]
        onActivated: retryButton.clicked()
    }

    // Fokus awal
    Component.onCompleted: retryButton.forceActiveFocus()
}
