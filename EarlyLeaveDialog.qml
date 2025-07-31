import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material

Dialog {
    id: dialog
    title: "Alasan Keluar Lebih Awal"
    modal: true
    width: 450
    height: 350
    anchors.centerIn: parent
    padding: 0
    closePolicy: Popup.NoAutoClose // Pengguna harus submit atau batal

    // Signal untuk memberitahu main.cpp bahwa dialog ditutup tanpa submit
    signal dialogClosed()

    background: Rectangle {
        color: Material.dialogColor
        radius: 12
    }

    header: Pane {
        padding: 16
        background: Rectangle { color: "transparent" }

        Label {
            text: dialog.title
            font.pixelSize: 20
            font.weight: Font.Bold
            color: Material.primaryTextColor
        }
    }

    contentItem: ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 16

        Label {
            text: "Anda akan keluar sebelum waktu kerja selesai. Mohon berikan alasan Anda."
            wrapMode: Text.Wrap
            Layout.fillWidth: true
            color: Material.secondaryTextColor
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 120

            TextArea {
                id: reasonInput
                placeholderText: "Ketik alasan Anda di sini..."
                wrapMode: Text.Wrap
                font.pixelSize: 14
                color: Material.primaryTextColor
                selectByMouse: true

                background: Rectangle {
                    radius: 8
                    color: Qt.rgba(Material.primaryTextColor.r, Material.primaryTextColor.g, Material.primaryTextColor.b, 0.05)
                    border.color: reasonInput.activeFocus ? Material.accent : Material.dividerColor
                    border.width: reasonInput.activeFocus ? 2 : 1
                }
            }
        }

        // Status label untuk menampilkan informasi
        Label {
            id: statusLabel
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            color: Material.color(Material.Orange)
            font.pixelSize: 12
            visible: false
        }
    }

    footer: DialogButtonBox {
        id: buttonBox
        padding: 16
        background: Rectangle { color: "transparent" }

        Button {
            id: submitButton
            text: buttonBox.enabled ? "Submit dan Keluar" : "Mengirim..."
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            enabled: reasonInput.text.trim().length > 5 && buttonBox.enabled
            Material.background: enabled ? Material.accent : Material.color(Material.Grey)
            Material.foreground: "white"

            onClicked: {
                buttonBox.enabled = false
                statusLabel.text = "Mengirim data ke server..."
                statusLabel.visible = true
                logger.submitEarlyLeaveReason(reasonInput.text.trim())
            }
        }

        Button {
            id: cancelButton
            text: "Batal"
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            Material.background: Material.color(Material.Grey)
            Material.foreground: "white"
            enabled: buttonBox.enabled

            onClicked: {
                dialog.close()
                dialog.dialogClosed() // Emit signal untuk reset flag di main.cpp
            }
        }
    }

    // Fungsi untuk menampilkan dialog
    function show() {
        reasonInput.text = ""
        resetDialog()
        dialog.open()
        reasonInput.forceActiveFocus() // Fokus ke text area
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

        // Fungsi ini akan dipanggil jika submit berhasil
        function onEarlyLeaveReasonSubmitted() {
            // Dialog akan tertutup otomatis ketika aplikasi quit
            statusLabel.text = "Berhasil! Aplikasi akan ditutup..."
            statusLabel.color = Material.color(Material.Green)
        }

        //Jika ingin menambahkan handler untuk submit gagal
        function onEarlyLeaveSubmitFailed(errorMessage) {
            resetDialog()
            statusLabel.text = "Gagal: " + errorMessage
            statusLabel.color = Material.color(Material.Red)
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
}
