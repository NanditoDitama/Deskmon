import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls.Material 2.15

Dialog {
    id: mainDialog
    title: "Aplikasi Produktivitas"
    width: 800
    height: 600
    modal: true
    padding: 20

    property int currentUserId: logger.currentUserId

    Rectangle {
        anchors.fill: parent
        color: "#f5f5f5"
        radius: 10

        ColumnLayout {
            anchors.fill: parent
            spacing: 15

            // Daftar Aplikasi
            ListView {
                id: listView
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: logger.getProductivityAppsByUser(currentUserId)

                header: RowLayout {
                    width: listView.width
                    spacing: 10
                    Label { text: "Aplikasi"; Layout.preferredWidth: 200; font.bold: true }
                    Label { text: "Website"; Layout.preferredWidth: 200; font.bold: true }
                    Label { text: "Status"; Layout.preferredWidth: 150; font.bold: true }
                    Label { text: "Tipe"; Layout.preferredWidth: 100; font.bold: true }
                }

                delegate: Rectangle {
                    width: listView.width
                    height: 40
                    color: index % 2 === 0 ? "#ffffff" : "#f8f8f8"

                    RowLayout {
                        anchors.fill: parent
                        spacing: 10

                        Label {
                            text: modelData.aplikasi
                            Layout.preferredWidth: 200
                            elide: Text.ElideRight
                        }

                        Label {
                            text: modelData.window_title || "-"
                            Layout.preferredWidth: 200
                            elide: Text.ElideRight
                        }

                        Label {
                            text: modelData.jenis === 0 ? "Pending" : "Aktif"
                            Layout.preferredWidth: 150
                            color: modelData.jenis === 0 ? "orange" : "green"
                        }

                        Label {
                            text: {
                                switch(modelData.productivity) {
                                    case 1: return "Produktif";
                                    case 2: return "Non-Produktif";
                                    case 3: return "Netral";
                                    default: return "-";
                                }
                            }
                            Layout.preferredWidth: 100
                            color: {
                                switch(modelData.productivity) {
                                    case 1: return "green";
                                    case 2: return "red";
                                    default: return "gray";
                                }
                            }
                        }
                    }
                }

                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AlwaysOn
                }
            }

            // Tombol Footer
            RowLayout {
                Layout.alignment: Qt.AlignRight
                spacing: 10

                Button {
                    text: "Tutup"
                    onClicked: mainDialog.close()
                    Material.background: "#757575"
                    Material.foreground: "white"
                }

                Button {
                    text: "Tambah Baru"
                    onClicked: addDialog.open()
                    Material.background: "#2196F3"
                    Material.foreground: "white"
                }
            }
        }
    }

    // Dialog Tambah Aplikasi
    AddAppDialog {
        id: addDialog
        currentUserId: mainDialog.currentUserId
    }
}
