import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material

Dialog {
    id: earlyLeaveReasonDialog
    modal: true
    width: 400
    height: 300
    anchors.centerIn: parent
    padding: 0
    closePolicy: Popup.NoAutoClose

    // Theme aware colors
    readonly property bool isDarkMode: Material.theme === Material.Dark

    // Signal untuk memberitahu main.cpp bahwa dialog ditutup tanpa submit
    signal dialogClosed()

    background: Rectangle {
        color: cardColor
        radius: 16
        border.color: Qt.rgba(primaryColor.r, primaryColor.g, primaryColor.b, 0.15)
        border.width: 1
    }

    // === KONTEN =======================================================
    contentItem: ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 18

        // HEADER: ikon + judul
        RowLayout {
            Layout.fillWidth: true
            spacing: 14

            // ikon modern
            Rectangle {
                width: 40
                height: 40
                radius: 20
                color: Qt.rgba(primaryColor.r, primaryColor.g, primaryColor.b, 0.12)
                Layout.alignment: Qt.AlignVCenter

                // animasi pulse halus
                SequentialAnimation on scale {
                    running: dialog.visible
                    loops: Animation.Infinite
                    NumberAnimation { from: 1.0; to: 1.08; duration: 1200; easing.type: Easing.InOutQuad }
                    NumberAnimation { from: 1.08; to: 1.0; duration: 1200; easing.type: Easing.InOutQuad }
                }

                Image {
                    anchors.centerIn: parent
                    width: 24
                    height: 24
                    source: "qrc:/assets/icon.ico"
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Label {
                    text: "Keluar Lebih Awal"
                    font.pixelSize: 17
                    font.weight: Font.DemiBold
                    color: textColor
                    Layout.fillWidth: true
                }

                Label {
                    text: "Mohon berikan alasan Anda"
                    font.pixelSize: 13
                    color: Qt.rgba(textColor.r, textColor.g, textColor.b, 0.6)
                    Layout.fillWidth: true
                }
            }
        }

        // TEXT AREA dengan styling modern
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 8

            Label {
                text: "Alasan"
                font.pixelSize: 13
                font.weight: Font.Medium
                color: textColor
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumHeight: 100
                radius: 10
                color: Qt.rgba(textColor.r, textColor.g, textColor.b, 0.03)
                border.color: reasonInput.activeFocus ? primaryColor : dividerColor
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
                        placeholderText: text.length > 0 ? "" : "Silahkan isi alasan di sini"
                        placeholderTextColor: lightTextColor
                        wrapMode: Text.Wrap
                        font.pixelSize: 13
                        color: textColor
                        selectByMouse: true
                        leftPadding: 10
                        rightPadding: 10
                        topPadding: 10
                        bottomPadding: 10
                        background: Item {}
                    }
                }
            }
        }


        // Status label
        Label {
            id: statusLabel
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            font.pixelSize: 12
            visible: false
            horizontalAlignment: Text.AlignHCenter
        }

        // TOMBOL AKSI MODERN
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Item { Layout.fillWidth: true }

            Button {
                id: cancelButton
                text: "Batal"
                enabled: buttonBox.enabled

                leftPadding: 18
                rightPadding: 18
                topPadding: 10
                bottomPadding: 10

                background: Rectangle {
                    radius: 8
                    color: cancelButton.hovered ? Qt.rgba(textColor.r, textColor.g, textColor.b, 0.08) : "transparent"
                    border.width: 0

                    Behavior on color { ColorAnimation { duration: 150 } }
                }

                contentItem: Text {
                    text: cancelButton.text
                    font.pixelSize: 13
                    font.weight: Font.Medium
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: Qt.rgba(textColor.r, textColor.g, textColor.b, 0.7)
                }

                onClicked: {
                    earlyLeaveReasonDialog.close()
                    earlyLeaveReasonDialog.dialogClosed()
                }
            }

            Button {
                id: submitButton
                text: buttonBox.enabled ? "Submit" : "Mengirim..."
                enabled: reasonInput.text.trim().length > 1 && buttonBox.enabled

                leftPadding: 24
                rightPadding: 24
                topPadding: 10
                bottomPadding: 10

                background: Rectangle {
                    radius: 8
                    color: submitButton.enabled ?
                               (submitButton.pressed ? Qt.darker(primaryColor, 1.1) :
                                                       submitButton.hovered ? Qt.lighter(primaryColor, 1.05) : primaryColor) :
                               Qt.rgba(textColor.r, textColor.g, textColor.b, 0.2)
                    border.width: 0

                    Behavior on color { ColorAnimation { duration: 150 } }
                }

                contentItem: Text {
                    text: submitButton.text
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: submitButton.enabled ? "white" : Qt.rgba(textColor.r, textColor.g, textColor.b, 0.4)
                }

                onClicked: {
                    buttonBox.enabled = false
                    statusLabel.text = "Mengirim data..."
                    statusLabel.color = primaryColor
                    statusLabel.visible = true
                    logger.submitEarlyLeaveReason(reasonInput.text.trim())
                }
            }
        }
    }

    // Hidden item untuk enable/disable buttons
    Item {
        id: buttonBox
        property bool enabled: true
    }

    // Fungsi untuk menampilkan dialog
    function show() {
        reasonInput.text = ""
        resetDialog()
        earlyLeaveReasonDialog.open()
        reasonInput.forceActiveFocus()
    }

    // Fungsi untuk reset dialog ke kondisi awal
    function resetDialog() {
        buttonBox.enabled = true
        statusLabel.visible = false
        statusLabel.text = ""
    }

    // Handle ketika dialog ditutup
    onClosed: {
        resetDialog()
    }

    // Connect ke logger untuk handle berbagai kondisi
    Connections {
        target: logger

        function onEarlyLeaveReasonSubmitted() {
            statusLabel.text = "Berhasil! Aplikasi akan ditutup..."
            statusLabel.color = Material.color(Material.Green)
            statusLabel.visible = true
            // optional: dialog.close()
        }

        function onEarlyLeaveSubmitFailed(errorMessage) {
            resetDialog()
            statusLabel.text = "Gagal: " + errorMessage
            statusLabel.color = primaryColor
            statusLabel.visible = true
        }
    }


    // Keyboard shortcuts
    Keys.onPressed: function(event) {
        if (event.key === Qt.Key_Enter || event.key === Qt.Key_Return) {
            if (event.modifiers & Qt.ControlModifier) {
                if (submitButton.enabled) {
                    submitButton.clicked()
                }
            }
        } else if (event.key === Qt.Key_Escape) {
            if (cancelButton.enabled) {
                cancelButton.clicked()
            }
        }
    }

    // Animasi entrance
    enter: Transition {
        ParallelAnimation {
            NumberAnimation {
                property: "opacity"
                from: 0
                to: 1
                duration: 250
                easing.type: Easing.OutCubic
            }
            NumberAnimation {
                property: "scale"
                from: 0.96
                to: 1
                duration: 250
                easing.type: Easing.OutBack
                easing.overshoot: 1.2
            }
        }
    }

    exit: Transition {
        NumberAnimation {
            property: "opacity"
            from: 1
            to: 0
            duration: 150
            easing.type: Easing.InCubic
        }
    }
}
