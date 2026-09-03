import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"

Dialog {
    id: root
    title: ""
    modal: true
    width: 380
    height: 240
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? Math.round((parent.height - height) / 2) : 0
    padding: 0
    property int taskId: -1

    signal confirmed(int targetTaskId)

    background: Rectangle {
        color: "transparent"
        radius: Theme.radiusMedium
    }

    contentItem: Rectangle {
        color: Theme.cardColor
        radius: Theme.radiusMedium
        border.width: 1
        border.color: Qt.rgba(255, 255, 255, 0.08)

        Rectangle {
            anchors.fill: parent
            radius: Theme.radiusMedium
            gradient: Gradient {
                GradientStop { position: 0.0; color: Qt.rgba(255, 255, 255, 0.03) }
                GradientStop { position: 1.0; color: "transparent" }
            }
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 24
            spacing: 20

            // Icon + Title section
            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                Rectangle {
                    Layout.preferredWidth: 40
                    Layout.preferredHeight: 40
                    radius: Theme.radiusSmall
                    color: Qt.alpha(Theme.accentColor, 0.12)
                    border.width: 1
                    border.color: Qt.alpha(Theme.accentColor, 0.2)

                    Image {
                        anchors.centerIn: parent
                        source: "qrc:/icons/danger.svg"
                        sourceSize.width: 20
                        sourceSize.height: 20
                    }
                }

                Label {
                    text: Lang.t("Switch to Project?")
                    font {
                        pixelSize: 18
                        family: "Segoe UI"
                        weight: Font.DemiBold
                    }
                    color: Theme.textColor
                    Layout.fillWidth: true
                }
            }

            // Message section
            Label {
                text: Lang.t("You have an active task. Do you want to switch to this task?")
                font {
                    pixelSize: 13
                    family: "Segoe UI"
                }
                color: Qt.alpha(Theme.textColor, 0.65)
                wrapMode: Text.Wrap
                Layout.fillWidth: true
                lineHeight: 1.5
            }

            Item { Layout.fillHeight: true }

            // Button section
            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                Button {
                    id: cancelButton
                    text: Lang.t("Cancel")
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40

                    background: Rectangle {
                        radius: Theme.radiusSmall
                        color: cancelButton.pressed ? Qt.alpha(Theme.textColor, 0.08) :
                                                      cancelButton.hovered ? Qt.alpha(Theme.textColor, 0.04) : "transparent"
                        border.width: 1
                        border.color: Qt.alpha(Theme.textColor, 0.15)

                        Behavior on color {
                            ColorAnimation { duration: 150 }
                        }
                    }

                    contentItem: Text {
                        text: cancelButton.text
                        font {
                            pixelSize: 13
                            family: "Segoe UI"
                            weight: Font.Medium
                        }
                        color: Theme.textColor
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    onClicked: root.reject()
                }

                Button {
                    id: confirmButton
                    text: Lang.t("Switch Project")
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40

                    background: Rectangle {
                        radius: Theme.radiusSmall
                        color: confirmButton.pressed ? Qt.darker(Theme.accentColor, 1.15) :
                                                       confirmButton.hovered ? Qt.lighter(Theme.accentColor, 1.08) : Theme.accentColor

                        Behavior on color {
                            ColorAnimation { duration: 150 }
                        }
                    }

                    contentItem: Text {
                        text: confirmButton.text
                        font {
                            pixelSize: 13
                            family: "Segoe UI"
                            weight: Font.Medium
                        }
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    onClicked: {
                        root.confirmed(root.taskId)
                        root.accept()
                    }
                }
            }
        }
    }

    enter: Transition {
        ParallelAnimation {
            NumberAnimation {
                property: "opacity"
                from: 0
                to: 1
                duration: 200
                easing.type: Easing.OutCubic
            }
            NumberAnimation {
                property: "scale"
                from: 0.92
                to: 1.0
                duration: 250
                easing.type: Easing.OutCubic
            }
        }
    }

    exit: Transition {
        ParallelAnimation {
            NumberAnimation {
                property: "opacity"
                from: 1
                to: 0
                duration: 150
                easing.type: Easing.InCubic
            }
            NumberAnimation {
                property: "scale"
                from: 1.0
                to: 0.96
                duration: 150
                easing.type: Easing.InCubic
            }
        }
    }
}
