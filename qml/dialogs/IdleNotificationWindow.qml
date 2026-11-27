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
    title: Lang.t("Idle Detected")
    modality: Qt.ApplicationModal
    flags: Qt.Dialog | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
    color: "transparent"

    // Pusatkan window
    x: (Screen.width  - width)  / 2
    y: (Screen.height - height) / 2

    signal resumeRequested()
    signal dismissRequested()

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

                Rectangle {
                    width: 40
                    height: 40
                    radius: 20
                    color: Qt.alpha(Theme.primaryColor, 0.12)
                    Layout.alignment: Qt.AlignVCenter

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
                        text: Lang.t("Idle Detected")
                        font.pixelSize: 17
                        font.weight: Font.DemiBold
                        color: Theme.textColor
                        Layout.fillWidth: true
                    }

                    Label {
                        text: Lang.t("Activity temporarily paused")
                        font.pixelSize: Theme.fontSizeSmall
                        color: Qt.alpha(Theme.textColor, 0.6)
                        Layout.fillWidth: true
                    }
                }
            }

            Item { Layout.fillHeight: true }

            // TOMBOL AKSI
            RowLayout {
                Layout.fillWidth: true

                CustomButton {
                    id: btnResume
                    text: Lang.t("Resume")
                    Layout.fillWidth: true
                    Layout.preferredHeight: 36
                    btnColor: Theme.primaryColor
                    btnHoverColor: Theme.primaryHoverColor
                    btnTextColor: Theme.isDarkMode ? "#0F172A" : "#FFFFFF"
                    font.pixelSize: 13
                    font.weight: Font.DemiBold

                    onClicked: {
                        root.resumeRequested()
                        root.close()
                    }
                }
            }
        }
    }

    Shortcut {
        sequences: [ StandardKey.Close, "Escape" ]
        onActivated: {
            root.dismissRequested()
            root.close()
        }
    }

    Shortcut {
        sequences: [ StandardKey.Accept, "Return", "Enter" ]
        onActivated: btnResume.clicked()
    }

    Component.onCompleted: btnResume.forceActiveFocus()
}
