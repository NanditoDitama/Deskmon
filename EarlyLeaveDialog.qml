import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material

Dialog {
    id: dialog
    title: "Alasan Keluar Lebih Awal"
    modal: true
    width: 450
    height: 300
    anchors.centerIn: parent
    padding: 0
    closePolicy: Popup.NoAutoClose // Pengguna harus submit

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

        TextArea {
            id: reasonInput
            Layout.fillWidth: true
            Layout.fillHeight: true
            placeholderText: "Ketik alasan Anda di sini..."
            wrapMode: Text.Wrap
            font.pixelSize: 14
            color: Material.primaryTextColor
            background: Rectangle {
                radius: 8
                color: Qt.rgba(Material.primaryTextColor.r, Material.primaryTextColor.g, Material.primaryTextColor.b, 0.05)
                border.color: Material.dividerColor
                border.width: 1
            }
        }
    }

    footer: DialogButtonBox {
        id: buttonBox
        padding: 16
        background: Rectangle { color: "transparent" }

        Button {
            text: "Submit dan Keluar"
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            enabled: reasonInput.text.trim().length > 5 // Aktifkan jika alasan diisi
            Material.background: Material.accent
            onClicked: {
                buttonBox.enabled = false // Nonaktifkan tombol untuk mencegah klik ganda
                logger.submitEarlyLeaveReason(reasonInput.text.trim())
                // Dialog akan ditutup secara otomatis oleh main.cpp saat aplikasi keluar
            }
        }
    }

    function show() { // <-- Ganti nama dari 'open' menjadi 'show'
            reasonInput.text = ""
            buttonBox.enabled = true
            dialog.open() // <-- Biarkan ini tetap 'dialog.open()'
    }
}
