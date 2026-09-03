import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material
import QtQuick.Dialogs
import QtQuick.Effects
import QtQuick.Window
import "../theme"

ApplicationWindow {
    id: warningWindowComponent
    width: 340
    height: 180
    title: qsTr("Peringatan")
    visible: false
    modality: Qt.ApplicationModal
    flags: Qt.Dialog | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
    color: "transparent"

    property string newText: ""
    property string titleText: "Peringatan Waktu Task"

    // Animasi
    Behavior on opacity {
        NumberAnimation { duration: 150 }
    }

    // === KARTU MODERN ====================================================
    Rectangle {
        id: card
        anchors.fill: parent
        radius: 16
        color: Theme.cardColor
        border.color: Qt.alpha(Theme.primaryColor, 0.15)
        border.width: 1
        opacity: warningWindowComponent.visible ? 1 : 0
        scale: warningWindowComponent.visible ? 1 : 0.96

        Behavior on opacity { NumberAnimation { duration: 250; easing.type: Easing.OutCubic } }
        Behavior on scale { NumberAnimation { duration: 250; easing.type: Easing.OutBack; easing.overshoot: 1.2 } }

        // === KONTEN =======================================================
        Column {
            anchors.fill: parent
            anchors.margins: 24
            spacing: 20

            // HEADER: ikon + judul
            Row {
                width: parent.width
                spacing: 14

                // ikon modern dengan gradient
                Rectangle {
                    width: 40
                    height: 40
                    radius: 20
                    color: Qt.alpha(Theme.primaryColor, 0.12)
                    anchors.verticalCenter: parent.verticalCenter

                    // animasi pulse halus
                    SequentialAnimation on scale {
                        running: warningWindowComponent.visible
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

                Column {
                    width: parent.width - 54
                    spacing: 2
                    anchors.verticalCenter: parent.verticalCenter

                    Label {
                        text: warningWindowComponent.titleText
                        font.pixelSize: 17
                        font.weight: Font.DemiBold
                        color: Theme.textColor
                        width: parent.width
                    }

                    Label {
                        text: newText
                        font.pixelSize: 13
                        color: Qt.alpha(Theme.textColor, 0.6)
                        width: parent.width
                        wrapMode: Text.Wrap
                        maximumLineCount: 2
                        elide: Text.ElideRight
                    }
                }
            }

            // SPACER
            Item {
                width: parent.width
                height: 1
                Layout.fillHeight: true
            }

            // TOMBOL AKSI MODERN
            Row {
                width: parent.width
                spacing: 10
                layoutDirection: Qt.RightToLeft

                Button {
                    id: closeButton
                    text: "Tutup"

                    leftPadding: 24
                    rightPadding: 24
                    topPadding: 10
                    bottomPadding: 10

                    background: Rectangle {
                        radius: 8
                        color: closeButton.pressed ? Qt.darker(Theme.primaryColor, 1.1) :
                               closeButton.hovered ? Qt.lighter(Theme.primaryColor, 1.05) : Theme.primaryColor
                        border.width: 0

                        Behavior on color { ColorAnimation { duration: 150 } }
                    }

                    contentItem: Text {
                        text: closeButton.text
                        font.pixelSize: 13
                        font.weight: Font.DemiBold
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        color: "white"
                    }

                    onClicked: {
                        warningWindowComponent.close()
                    }
                }
            }
        }
    }

    minimumWidth: 320
    minimumHeight: 180
    maximumWidth: 600
    maximumHeight: 400

    // Animasi saat muncul
    function showAnimated() {
        opacity = 0
        scale = 0.95
        show()

        // Animasi paralel
        parallelAnimation.start()
    }

    ParallelAnimation {
        id: parallelAnimation
        NumberAnimation {
            target: warningWindowComponent
            property: "opacity"
            to: 1
            duration: 150
            easing.type: Easing.OutQuad
        }
        NumberAnimation {
            target: warningWindowComponent
            property: "scale"
            to: 1
            duration: 200
            easing.type: Easing.OutBack
        }
    }

    // Override show untuk selalu muncul di tengah
    onVisibleChanged: {
        if (visible) {
            x = (Screen.width - width) / 2
            y = (Screen.height - height) / 2
        }
    }

    // === SHORTCUTS ============================================================
    Shortcut {
        sequences: [ StandardKey.Close, "Escape", "Return", "Enter" ]
        onActivated: closeButton.clicked()
    }

    // Fokus awal ke aksi utama
    Component.onCompleted: closeButton.forceActiveFocus()
}
