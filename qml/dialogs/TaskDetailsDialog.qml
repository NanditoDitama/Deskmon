// TaskDetailsDialog.qml (Modern Version)
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import "../theme"

ApplicationWindow {
    id: root
    width: 510
    height: 490
    title: Lang.t("Task Details")
    modality: Qt.ApplicationModal
    flags: Qt.Dialog | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
    color: "transparent"

    // Pusatkan window
    x: (Screen.width - width) / 2
    y: (Screen.height - height) / 2

    visible: false

    property int currentTaskId: -1
    property string action: ""
    property int nextTaskId: -1
    property bool isLoading: false


    // Modern background
    Rectangle {
        id: card
        anchors.fill: parent
        radius: Theme.radiusLarge
        color: Theme.cardColor
        border.color: Qt.alpha(Theme.primaryColor, 0.15)
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

                // Icon modern
                Rectangle {
                    width: 40
                    height: 40
                    radius: 20
                    color: Qt.alpha(Theme.primaryColor, 0.12)
                    Layout.alignment: Qt.AlignTop

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
                        text: Lang.t("Task Details")
                        font.pixelSize: 17
                        font.weight: Font.DemiBold
                        color: Theme.textColor
                        Layout.fillWidth: true
                    }

                    Label {
                        text: Lang.t("Add task details (Optional)")
                        font.pixelSize: 13
                        color: Qt.alpha(Theme.textColor, 0.6)
                        Layout.fillWidth: true
                    }
                }
            }

            // Info box
            Rectangle {
                Layout.fillWidth: true
                height: 52
                radius: Theme.radiusMedium
                color: Qt.alpha(Theme.primaryColor, 0.08)
                border.color: Qt.alpha(Theme.primaryColor, 0.2)
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 10

                    Label {
                        text: "ℹ️"
                        font.pixelSize: 16
                        Layout.alignment: Qt.AlignTop
                    }

                    Label {
                        text: Lang.t("You can add details of what you worked on in the previous task here (Optional)")
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                        color: Theme.textColor
                        font.pixelSize: 12
                        lineHeight: 1.2
                    }
                }
            }

            // Text input section
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 8

                Label {
                    text: Lang.t("Task Details")
                    font.pixelSize: 13
                    font.weight: Font.Medium
                    color: Theme.textColor
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumHeight: 120
                    radius: Theme.radiusMedium
                    color: Qt.alpha(Theme.textColor, 0.03)
                    border.color: detailsInput.activeFocus ? Theme.primaryColor : Theme.dividerColor
                    border.width: detailsInput.activeFocus ? 2 : 1
                    Behavior on border.color { ColorAnimation { duration: 200 } }
                    Behavior on border.width { NumberAnimation { duration: 200 } }

                    ScrollView {
                        anchors.fill: parent
                        anchors.margins: 2
                        clip: true

                        ScrollBar.vertical.policy: ScrollBar.AsNeeded
                        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                        TextArea {
                            id: detailsInput
                            width: parent.width
                            placeholderText: Lang.t("Example: Finished fixing bugs on a feature...")
                            placeholderTextColor: Theme.lightTextColor
                            wrapMode: Text.Wrap
                            font.pixelSize: 13
                            color: Theme.textColor
                            selectByMouse: true
                            leftPadding: 10
                            rightPadding: 10
                            topPadding: 10
                            bottomPadding: 10
                            background: Item {}
                        }
                        Component.onCompleted: {
                            if (ScrollBar.vertical) {
                                ScrollBar.vertical.width = 6
                                ScrollBar.vertical.opacity = 0.5
                            }
                        }
                    }

                }
            }


            // TOMBOL AKSI
            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Item { Layout.fillWidth: true }

                Button {
                    id: cancelButton
                    text: Lang.t("Skip")
                    enabled: true // Tombol Lewati selalu bisa diklik
                    leftPadding: 18
                    rightPadding: 18
                    topPadding: 10
                    bottomPadding: 10

                    background: Rectangle {
                        radius: Theme.radiusSmall
                        color: cancelButton.hovered ? Theme.dividerColor : "transparent"
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
                        console.log("Dialog detail ditutup. Aksi: " + root.action);
                        if (logger) { // Ini sekarang akan merujuk ke global logger
                            logger.taskDetailsDialogClosed(root.action);
                        }
                        root.close();
                    }
                }

                Button {
                    id: submitButton
                    text: root.isLoading ? Lang.t("Sending...") : Lang.t("Submit")
                    enabled: detailsInput.text.trim().length > 0 && !root.isLoading

                    leftPadding: 24
                    rightPadding: root.isLoading ? 40 : 24
                    topPadding: 10
                    bottomPadding: 10

                    background: Rectangle {
                        radius: Theme.radiusSmall
                        color: Theme.primaryColor
                        border.width: 0

                        Behavior on color { ColorAnimation { duration: 150 } }

                        // Loading spinner
                        Rectangle {
                            width: 14
                            height: 14
                            radius: 7
                            color: "transparent"
                            border.color: "white"
                            border.width: 2
                            anchors.right: parent.right
                            anchors.rightMargin: 12
                            anchors.verticalCenter: parent.verticalCenter
                            visible: root.isLoading

                            Rectangle {
                                width: 6
                                height: 6
                                radius: 3
                                color: "white"
                                anchors.top: parent.top
                                anchors.horizontalCenter: parent.horizontalCenter
                            }



                            RotationAnimation on rotation {
                                running: root.isLoading
                                from: 0
                                to: 360
                                duration: 800
                                loops: Animation.Infinite
                            }
                        }
                    }

                    contentItem: Text {
                        text: submitButton.text
                        font.pixelSize: 13
                        font.weight: Font.DemiBold
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        color: "white"
                    }

                    onClicked: {
                        root.isLoading = true;
                        if (logger) { // Ini sekarang akan merujuk ke global logger
                            logger.submitTaskDetails(currentTaskId, detailsInput.text.trim(), root.action, root.nextTaskId);
                        }
                    }
                }

                Connections {
                    target: logger

                    function onTaskDetailsSubmissionSuccess() {
                        // Tutup dialog (atau reset form)
                        root.visible = false
                        // Tidak perlu panggil notifikasi di sini karena C++ sudah emit showNotification("success", ...)
                    }

                    function onTaskDetailsSubmissionFailed(message) {
                        // Tampilkan pesan pada dialog (opsional)
                        errorText.text = message
                        // Notifikasi global juga sudah dipicu dari C++: showNotification("error", message)
                    }
                }
            }
        }
    }
    // Fungsi untuk menampilkan dialog
    function show(taskId, actionType, nextId) {
        root.currentTaskId = taskId;
        root.action = actionType;
        root.nextTaskId = nextId;
        detailsInput.text = "";
        root.isLoading = false;
        root.visible = true;
        detailsInput.forceActiveFocus();
    }

    // Keyboard shortcuts
    Shortcut {
        sequences: ["Escape"]
        enabled: !root.isLoading
        onActivated: cancelButton.clicked()
    }

    Shortcut {
        sequences: ["Return", "Enter"]
        enabled: submitButton.enabled
        onActivated: {
            if (Qt.platform.os === "windows" || Qt.platform.os === "linux" || Qt.platform.os === "osx") {
                submitButton.clicked()
            }
        }
    }
}


