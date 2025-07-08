import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Dialog {
    id: addDialog
    title: "Tambah Aplikasi Baru"
    width: 500
    height: 400
    modal: true

    property int currentUserId: -1
    property int selectedType: 1

    ColumnLayout {
        anchors.fill: parent
        spacing: 15

        ComboBox {
            id: cmbApps
            Layout.fillWidth: true
            model: logger.getAvailableApps()

            popup.contentItem: ListView {
                implicitHeight: 200
                model: cmbApps.popup.visible ? cmbApps.delegateModel : null
                ScrollBar.vertical: ScrollBar {}
            }
        }

        TextField {
            id: txtWebsite
            Layout.fillWidth: true
            placeholderText: "Nama Website (jika browser)"
            visible: ["Chrome","Firefox","Edge","Safari","Opera"].includes(cmbApps.currentText)
        }

        TextField {
            id: txtForUser
            Layout.fillWidth: true
            placeholderText: "Untuk User (0 = semua, contoh: 1,2,3)"
            text: "0"
        }

        GroupBox {
            title: "Kategori Produktivitas"
            Layout.fillWidth: true

            Column {
                spacing: 10

                RadioButton {
                    text: "Produktif"
                    checked: true
                    onCheckedChanged: if(checked) addDialog.selectedType = 1
                }

                RadioButton {
                    text: "Non-Produktif"
                    onCheckedChanged: if(checked) addDialog.selectedType = 2
                }

                RadioButton {
                    text: "Netral"
                    onCheckedChanged: if(checked) addDialog.selectedType = 3
                }
            }
        }

        Button {
            text: "Simpan"
            Layout.alignment: Qt.AlignRight
            onClicked: {
                if(logger.checkAppExists(cmbApps.currentText, txtWebsite.text, currentUserId)) {
                    existDialog.open()
                } else {
                    saveApp()
                }
            }
        }
    }

    Dialog {
        id: existDialog
        title: "Konfirmasi"
        width: 300
        Label {
            text: "Aplikasi ini sudah ada!\nApakah ingin memperbarui?"
            wrapMode: Text.Wrap
        }
        standardButtons: Dialog.Yes | Dialog.No
        onAccepted: saveApp()
    }

    function saveApp() {
        logger.addProductivityApp(
            cmbApps.currentText,
            txtWebsite.text,
            selectedType,
            txtForUser.text,
            currentUserId
        )
        addDialog.close()
        mainDialog.listView.model = logger.getProductivityAppsByUser(currentUserId)
    }
}
