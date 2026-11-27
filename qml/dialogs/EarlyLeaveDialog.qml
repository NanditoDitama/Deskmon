import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material
import "../theme"
import "../components"

Dialog {
    id: earlyLeaveReasonDialog
    modal: true
    width: 400
    height: 300
    x: parent ? (parent.width - width) / 2 : 0
    y: parent ? (parent.height - height) / 2 : 0
    padding: 0
    closePolicy: Popup.NoAutoClose

    // Signal untuk memberitahu main.cpp bahwa dialog ditutup tanpa submit
    signal dialogClosed()

    background: Rectangle {
        color: Theme.cardColor
        radius: 16
        border.color: Qt.alpha(Theme.primaryColor, 0.15)
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
                color: Qt.alpha(Theme.primaryColor, 0.12)
                Layout.alignment: Qt.AlignVCenter

                // animasi pulse halus
                SequentialAnimation on scale {
                    running: earlyLeaveReasonDialog.visible
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
                    text: Lang.t("Early Clock Out")
                    font.pixelSize: 17
                    font.weight: Font.DemiBold
                    color: Theme.textColor
                    Layout.fillWidth: true
                }

                Label {
                    text: Lang.t("Please provide your reason")
                    font.pixelSize: 13
                    color: Qt.alpha(Theme.textColor, 0.6)
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
                text: Lang.t("Reason")
                font.pixelSize: 13
                font.weight: Font.Medium
                color: Theme.textColor
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumHeight: 100
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

                    ScrollBar.vertical: CustomScrollBar {}
                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                    TextArea {
                        id: reasonInput
                        width: parent.width
                        placeholderText: text.length > 0 ? "" : Lang.t("Please enter your reason here")
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

            CustomButton {
                id: cancelButton
                text: Lang.t("Cancel")
                enabled: buttonBox.buttonsEnabled
                btnColor: "transparent"
                btnHoverColor: Qt.alpha(Theme.textColor, 0.08)
                btnTextColor: Qt.alpha(Theme.textColor, 0.7)
                font.pixelSize: 13
                font.weight: Font.Medium

                onClicked: {
                    earlyLeaveReasonDialog.close()
                    earlyLeaveReasonDialog.dialogClosed()
                }
            }

            CustomButton {
                id: submitButton
                text: buttonBox.buttonsEnabled ? Lang.t("Submit") : Lang.t("Sending...")
                enabled: reasonInput.text.trim().length > 1 && buttonBox.buttonsEnabled
                btnColor: submitButton.enabled ? Theme.primaryColor : Qt.alpha(Theme.textColor, 0.2)
                btnHoverColor: Qt.lighter(Theme.primaryColor, 1.05)
                btnTextColor: submitButton.enabled ? "white" : Qt.alpha(Theme.textColor, 0.4)
                font.pixelSize: 13

                onClicked: {
                    buttonBox.buttonsEnabled = false
                    statusLabel.text = Lang.t("Sending data...")
                    statusLabel.color = Theme.primaryColor
                    statusLabel.visible = true
                    logger.submitEarlyLeaveReason(reasonInput.text.trim())
                }
            }
        }
    }

    // Hidden item untuk enable/disable buttons
    Item {
        id: buttonBox
        property bool buttonsEnabled: true
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
        buttonBox.buttonsEnabled = true
        statusLabel.visible = false
        statusLabel.text = ""
    }

    // Handle ketika dialog ditutup
    onClosed: {
        resetDialog()
    }

    // Connect ke logger untuk handle status submit
    Connections {
        target: logger

        function onEarlyLeaveReasonSubmitted() {
            statusLabel.text = Lang.t("Success! The application will close...")
            statusLabel.color = Theme.successColor
            statusLabel.visible = true
        }

        function onEarlyLeaveSubmitFailed(errorMessage) {
            buttonBox.buttonsEnabled = true
            statusLabel.text = errorMessage ? Lang.t(errorMessage) : Lang.t("Failed to send data to server.")
            statusLabel.color = Theme.dangerColor
            statusLabel.visible = true
        }
    }


    // Keyboard shortcuts
    Shortcut {
        sequences: [ "Ctrl+Return", "Ctrl+Enter" ]
        onActivated: if (submitButton.enabled) submitButton.clicked()
    }
    Shortcut {
        sequences: [ StandardKey.Close, "Escape" ]
        onActivated: if (cancelButton.enabled) cancelButton.clicked()
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
