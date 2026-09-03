import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import "../theme"

ApplicationWindow {
    id: root
    width: 340
    height: 200
    visible: false
    color: "transparent"
    title: "Sesi Berakhir"
    modality: Qt.ApplicationModal
    flags: Qt.Dialog | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint

    // Center pada screen
    x: (Screen.width - width) / 2
    y: (Screen.height - height) / 2

    property alias errorMessage: authErrorText.text
    signal loginRedirectRequested()

    Rectangle {
        id: cardError
        anchors.fill: parent
        radius: Theme.radiusMedium
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

            // Header: Icon + Title
            RowLayout {
                Layout.fillWidth: true
                spacing: 14

                Rectangle {
                    width: 40
                    height: 40
                    radius: 20
                    color: Qt.alpha(Theme.primaryColor, 0.12)
                    Layout.alignment: Qt.AlignTop

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
                        text: "Sesi Berakhir"
                        font.pixelSize: 17
                        font.weight: Font.DemiBold
                        color: Theme.textColor
                        Layout.fillWidth: true
                    }

                    Label {
                        text: "Silakan login kembali"
                        font.pixelSize: Theme.fontSizeSmall
                        color: Qt.alpha(Theme.textColor, 0.6)
                        Layout.fillWidth: true
                    }
                }
            }

            // Message content
            Label {
                id: authErrorText
                text: ""
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                Layout.fillHeight: true
                font.pixelSize: 13
                color: Qt.alpha(Theme.textColor, 0.75)
                lineHeight: 1.5
                verticalAlignment: Text.AlignTop

                opacity: 0
                NumberAnimation on opacity {
                    running: root.visible
                    from: 0
                    to: 1
                    duration: 300
                    easing.type: Easing.OutQuart
                }
            }

            Item { Layout.fillHeight: true }

            // Action buttons
            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Item { Layout.fillWidth: true }

                Button {
                    id: okButton
                    text: "Kembali ke halaman Login"
                    leftPadding: 24
                    rightPadding: 24
                    topPadding: 10
                    bottomPadding: 10

                    background: Rectangle {
                        radius: Theme.radiusSmall
                        color: Theme.primaryColor
                        border.width: 0
                        Behavior on color { ColorAnimation { duration: 150 } }
                    }

                    contentItem: Text {
                        text: okButton.text
                        font.pixelSize: 13
                        font.weight: Font.DemiBold
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        color: "white"
                    }

                    onClicked: {
                        closeAnimation.start()
                    }
                }
            }
        }
    }

    SequentialAnimation {
        id: closeAnimation
        NumberAnimation {
            target: cardError
            property: "scale"
            from: 1.0
            to: 0.96
            duration: 150
            easing.type: Easing.InQuart
        }
        NumberAnimation {
            target: cardError
            property: "opacity"
            from: 1.0
            to: 0.0
            duration: 100
        }
        ScriptAction {
            script: {
                root.visible = false
                cardError.opacity = 1.0
                cardError.scale = 1.0
                root.loginRedirectRequested()
            }
        }
    }

    Shortcut {
        sequences: [ StandardKey.Close, "Escape" ]
        onActivated: closeAnimation.start()
    }

    Shortcut {
        sequences: [ StandardKey.Accept, "Return", "Enter" ]
        onActivated: okButton.clicked()
    }

    Component.onCompleted: okButton.forceActiveFocus()
}
