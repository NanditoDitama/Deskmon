import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import "../theme"

ApplicationWindow {
    id: needReviewDialog
    width: 510
    height: 490
    title: "Alasan Permintaan Review"
    modality: Qt.ApplicationModal
    flags: Qt.Dialog | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
    color: "transparent"

    // Pusatkan window
    x: (Screen.width - width) / 2
    y: (Screen.height - height) / 2

    visible: false

    property int taskId: -1
    property var logger: null

    // Modern background without shadow
    Rectangle {
        id: card
        anchors.fill: parent
        radius: 16
        color: Theme.cardColor
        border.color: Qt.alpha(Theme.primaryColor, 0.15)
        border.width: 1
        opacity: needReviewDialog.visible ? 1 : 0
        scale: needReviewDialog.visible ? 1 : 0.96

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

                    // Animasi pulse halus
                    SequentialAnimation on scale {
                        running: needReviewDialog.visible
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
                        text: "Permintaan Review"
                        font.pixelSize: 17
                        font.weight: Font.DemiBold
                        color: Theme.textColor
                        Layout.fillWidth: true
                    }

                    Label {
                        text: "Berikan alasan yang jelas untuk review"
                        font.pixelSize: 13
                        color: Qt.alpha(Theme.textColor, 0.6)
                        Layout.fillWidth: true
                    }
                }
            }

            // Info box
            Rectangle {
                Layout.fillWidth: true
                height: 64
                radius: 10
                color: Qt.alpha(Theme.primaryColor, 0.08)
                border.color: Qt.alpha(Theme.primaryColor, 0.2)
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 10

                    Label {
                        text: "💡"
                        font.pixelSize: 16
                        Layout.alignment: Qt.AlignTop
                    }

                    Label {
                        text: "Tugas akan dipindahkan ke status 'Need Review'. Manajer akan menerima notifikasi untuk melakukan review."
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
                    text: "Alasan Permintaan Review *"
                    font.pixelSize: 13
                    font.weight: Font.Medium
                    color: Theme.textColor
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumHeight: 120
                    radius: 10
                    color: Qt.alpha(Theme.textColor, 0.03)
                    border.color: reasonInput.activeFocus ? Theme.primaryColor : Theme.dividerColor
                    border.width: reasonInput.activeFocus ? 2 : 1
                    Behavior on border.color { ColorAnimation { duration: 200 } }
                    Behavior on border.width { NumberAnimation { duration: 200 } }

                    ScrollView {
                        anchors.fill: parent
                        anchors.margins: 2
                        clip: true

                        ScrollBar.vertical.policy: ScrollBar.AsNeeded
                        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                        TextArea {
                            id: reasonInput
                            width: parent.width
                            placeholderText: "Contoh: Butuh verifikasi dari manajer proyek, ada kendala teknis, dll."
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

                            property int maxLength: 500
                            onTextChanged: {
                                if (text.length > maxLength) {
                                    text = text.substring(0, maxLength)
                                }
                            }
                        }

                        // Style scrollbar setelah component complete
                        Component.onCompleted: {
                            if (ScrollBar.vertical) {
                                ScrollBar.vertical.width = 6
                                ScrollBar.vertical.opacity = 0.5
                            }
                        }
                    }

                    // Character counter
                    Label {
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        anchors.margins: 8
                        text: reasonInput.text.length + "/" + reasonInput.maxLength
                        font.pixelSize: 10
                        color: reasonInput.text.length > reasonInput.maxLength * 0.9 ? Theme.dangerColor : Theme.lightTextColor
                        opacity: 0.6
                        z: 1  // Pastikan label di atas scrollbar
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
                    text: "Batal"

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
                        needReviewDialog.close()
                    }
                }

                Button {
                    id: submitButton
                    text: enabled ? "Submit" : "Mengirim..."
                    enabled: reasonInput.text.trim().length > 0

                    leftPadding: 24
                    rightPadding: submitButton.isLoading ? 40 : 24
                    topPadding: 10
                    bottomPadding: 10

                    property bool isLoading: false

                    background: Rectangle {
                        radius: 8
                        color: submitButton.enabled ?
                                   (submitButton.pressed ? Qt.darker(Theme.primaryColor, 1.1) :
                                    submitButton.hovered ? Qt.lighter(Theme.primaryColor, 1.05) : Theme.primaryColor) :
                                   Qt.alpha(Theme.primaryColor, 0.4)
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
                            visible: submitButton.isLoading

                            Rectangle {
                                width: 6
                                height: 6
                                radius: 3
                                color: "white"
                                anchors.top: parent.top
                                anchors.horizontalCenter: parent.horizontalCenter
                            }

                            RotationAnimation on rotation {
                                running: submitButton.isLoading
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
                        if (!logger) {
                            console.error("Logger not set. Please provide logger instance when opening dialog.");
                            return;
                        }

                        isLoading = true;
                        enabled = false;

                        var payload = {
                            "status": "need-review",
                            "alasan": reasonInput.text.trim()
                        };

                        var apiUrl = "https://deskmon.pranala-dt.co.id/api/update-status-task/" +
                                needReviewDialog.taskId + "/" + logger.currentUserId;

                        var request = new XMLHttpRequest();
                        request.open("POST", apiUrl);
                        request.setRequestHeader("Content-Type", "application/json");
                        request.setRequestHeader("Authorization", "Bearer " + logger.authToken);

                        request.onreadystatechange = function() {
                            if (request.readyState === XMLHttpRequest.DONE) {
                                submitButton.isLoading = false;
                                if (request.status === 200) {
                                    console.log("Task status updated to 'need review' with reason.");
                                    if (logger && logger.fetchAndStoreTasks) {
                                        logger.fetchAndStoreTasks();
                                    }
                                    needReviewDialog.close();
                                    if (logger && logger.notify) {
                                        logger.notify("success", "Berhasil mengajukan Need Review");
                                        logger.refreshTasks()
                                    }
                                } else {
                                    console.error("Failed to update task status:", request.status, request.responseText);
                                    submitButton.enabled = true;
                                    if (logger && logger.notify) {
                                        logger.notify("warning", "Gagal mengajukan Need Review");
                                    }
                                }
                            }
                        };

                        request.send(JSON.stringify(payload));
                    }
                }
            }
        }
    }

    // Fungsi untuk membuka dialog
    function openWithTaskId(id, loggerInstance) {
        taskId = id;
        if (loggerInstance) {
            logger = loggerInstance;
        }
        reasonInput.text = "";
        submitButton.isLoading = false
        submitButton.enabled = true
        show();
    }

    // Keyboard shortcuts
    Shortcut {
        sequences: ["Escape"]
        onActivated: cancelButton.clicked()
    }

    Shortcut {
        sequences: ["Return", "Enter"]
        enabled: submitButton.enabled
        onActivated: {
            if (Qt.platform.os === "windows" || Qt.platform.os === "linux" || Qt.platform.os === "osx") {
                // Ctrl on Windows/Linux, Cmd on macOS
                submitButton.clicked()
            }
        }
    }

    // Fokus awal
    Component.onCompleted: {
        // Coba cari logger dari context property jika belum diset
        if (!logger && typeof logger !== "undefined") {
            needReviewDialog.logger = logger;
        }
        reasonInput.forceActiveFocus();
    }
}
