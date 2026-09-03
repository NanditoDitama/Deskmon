import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"

Dialog {
    id: root
    modal: true
    visible: false
    width: Math.min(340, parent ? parent.width - 40 : 340)
    height: 240
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? Math.round((parent.height - height) / 2) : 0

    padding: 0
    margins: 0
    topPadding: 0
    bottomPadding: 0
    leftPadding: 0
    rightPadding: 0

    property alias errorMessage: errorText.text

    onAccepted: visible = false

    background: Rectangle {
        color: Theme.cardColor
        radius: Theme.radiusLarge
        border.color: Qt.alpha(Theme.primaryColor, 0.15)
        border.width: 1
    }

    contentItem: Rectangle {
        color: Theme.cardColor
        radius: Theme.radiusLarge
        implicitHeight: contentColumn.implicitHeight

        ColumnLayout {
            id: contentColumn
            anchors.fill: parent
            anchors.margins: 24
            spacing: 16

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
                        text: Lang.t("Connection Error")
                        font.pixelSize: 17
                        font.weight: Font.DemiBold
                        color: Theme.textColor
                        Layout.fillWidth: true
                    }

                    Label {
                        text: Lang.t("A connection problem occurred")
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.secondaryColor
                        Layout.fillWidth: true
                    }
                }
            }

            // Error message content
            ScrollView {
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(errorText.implicitHeight, 100)
                Layout.maximumHeight: 120
                clip: true

                Label {
                    id: errorText
                    text: ""
                    wrapMode: Text.Wrap
                    width: root.width - 48
                    font.pixelSize: 13
                    color: Theme.textColor
                    lineHeight: 1.5

                    opacity: 0
                    NumberAnimation on opacity {
                        running: root.visible
                        from: 0
                        to: 1
                        duration: 300
                        easing.type: Easing.OutQuart
                    }
                }
            }

            // Action button
            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Item { Layout.fillWidth: true }

                Button {
                    id: okButton1
                    text: Lang.t("OK")
                    leftPadding: 32
                    rightPadding: 32
                    topPadding: 10
                    bottomPadding: 10

                    background: Rectangle {
                        radius: Theme.radiusSmall
                        color: okButton1.pressed ? Qt.darker(Theme.primaryColor, 1.1) :
                                                   okButton1.hovered ? Qt.lighter(Theme.primaryColor, 1.05) : Theme.primaryColor
                        border.width: 0
                        Behavior on color { ColorAnimation { duration: 150 } }
                    }

                    contentItem: Text {
                        text: okButton1.text
                        font.pixelSize: 13
                        font.weight: Font.DemiBold
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        color: "white"
                    }
                    onClicked: root.accept()
                }
            }
        }
    }

    enter: Transition {
        ParallelAnimation {
            NumberAnimation {
                property: "scale"
                from: 0.96
                to: 1.0
                duration: 250
                easing.type: Easing.OutCubic
            }
            NumberAnimation {
                property: "opacity"
                from: 0.0
                to: 1.0
                duration: 250
                easing.type: Easing.OutCubic
            }
        }
    }

    exit: Transition {
        ParallelAnimation {
            NumberAnimation {
                property: "scale"
                from: 1.0
                to: 0.96
                duration: 150
                easing.type: Easing.InCubic
            }
            NumberAnimation {
                property: "opacity"
                from: 1.0
                to: 0.0
                duration: 150
                easing.type: Easing.InCubic
            }
        }
    }

    Shortcut {
        sequences: [ StandardKey.Close, "Escape" ]
        onActivated: root.accept()
    }

    Shortcut {
        sequences: [ StandardKey.Accept, "Return", "Enter" ]
        onActivated: okButton1.clicked()
    }

    onVisibleChanged: if (visible) okButton1.forceActiveFocus()
}
