// FailureDialog.qml (Modern Version)
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import "../theme"
import "../components"

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

                CustomButton {
                    id: cancelButton
                    text: Lang.t("Later")
                    btnColor: "transparent"
                    btnHoverColor: Qt.alpha(Theme.textColor, 0.08)
                    btnTextColor: Qt.alpha(Theme.textColor, 0.7)
                    btnBorderColor: Theme.dividerColor
                    btnBorderWidth: 1
                    font.pixelSize: 13
                    font.weight: Font.Medium

                    onClicked: {
                        console.log("Pengiriman ulang dibatalkan oleh pengguna.");
                        if ((failedAction === "quit" || failedAction === "logout") && typeof logger !== "undefined" && logger) {
                            logger.taskDetailsDialogClosed(failedAction);
                        }
                        root.close();
                    }
                }

                CustomButton {
                    id: retryButton
                    text: Lang.t("Retry")
                    btnColor: Theme.dangerColor
                    btnHoverColor: Qt.lighter(Theme.dangerColor, 1.05)
                    font.pixelSize: 13

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
