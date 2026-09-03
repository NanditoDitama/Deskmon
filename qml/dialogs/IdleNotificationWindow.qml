import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import "../theme"

ApplicationWindow {
    id: root
    width: 340
    height: 180
    title: "Idle Detected"
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
                        text: "Idle Terdeteksi"
                        font.pixelSize: 17
                        font.weight: Font.DemiBold
                        color: Theme.textColor
                        Layout.fillWidth: true
                    }

                    Label {
                        text: "Aktivitas dihentikan sementara"
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
                spacing: 10

                Item { Layout.fillWidth: true }

                Button {
                    id: btnDismiss
                    text: "Dismiss"
                    leftPadding: 18
                    rightPadding: 18
                    topPadding: 10
                    bottomPadding: 10

                    background: Rectangle {
                        radius: Theme.radiusSmall
                        color: btnDismiss.hovered ? Qt.alpha(Theme.textColor, 0.08) : "transparent"
                        border.width: 0
                        Behavior on color { ColorAnimation { duration: 150 } }
                    }

                    contentItem: Text {
                        text: btnDismiss.text
                        font.pixelSize: 13
                        font.weight: Font.Medium
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        color: Qt.alpha(Theme.textColor, 0.7)
                    }

                    onClicked: {
                        root.dismissRequested()
                        root.close()
                    }
                }

                Button {
                    id: btnResume
                    text: "Resume"
                    leftPadding: 24
                    rightPadding: 24
                    topPadding: 10
                    bottomPadding: 10

                    background: Rectangle {
                        radius: Theme.radiusSmall
                        color: btnResume.pressed ? Qt.darker(Theme.primaryColor, 1.1) :
                                                   btnResume.hovered ? Qt.lighter(Theme.primaryColor, 1.05) : Theme.primaryColor
                        border.width: 0
                        Behavior on color { ColorAnimation { duration: 150 } }
                    }

                    contentItem: Text {
                        text: btnResume.text
                        font.pixelSize: 13
                        font.weight: Font.DemiBold
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        color: "white"
                    }

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
