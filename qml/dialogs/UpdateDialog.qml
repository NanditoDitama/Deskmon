import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"

Dialog {
    id: root
    title: ""
    modal: true
    width: 480
    height: 520
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? Math.round((parent.height - height) / 2) : 0

    property string newVersion: ""
    property string newReleaseNotes: ""

    signal updateAccepted()

    function openWith(version, releaseNotes) {
        newVersion = version
        newReleaseNotes = releaseNotes
        open()
    }

    background: Rectangle {
        color: Theme.cardColor
        radius: Theme.radiusMedium
        border.width: 1
        border.color: Qt.rgba(255, 255, 255, 0.08)

        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            gradient: Gradient {
                GradientStop { position: 0.0; color: Qt.rgba(255, 255, 255, 0.03) }
                GradientStop { position: 1.0; color: "transparent" }
            }
            opacity: 0.8
        }
    }

    contentItem: Rectangle {
        anchors.fill: parent
        color: "transparent"
        radius: Theme.radiusMedium

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 24
            spacing: 0

            // Header section
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 80

                Rectangle {
                    width: 56
                    height: 56
                    radius: Theme.radiusMedium
                    color: Theme.accentColor
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter

                    Rectangle {
                        width: 24
                        height: 24
                        radius: 12
                        color: "#ffffff"
                        anchors.centerIn: parent

                        Rectangle {
                            width: 8
                            height: 8
                            radius: 4
                            color: Theme.accentColor
                            anchors.centerIn: parent
                        }
                    }
                }

                Column {
                    anchors.left: parent.left
                    anchors.leftMargin: 72
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 4

                    Label {
                        text: Lang.t("Update Available")
                        font.pixelSize: 20
                        font.weight: Font.DemiBold
                        color: Theme.textColor
                    }

                    Label {
                        text: Lang.t("Version") + " " + root.newVersion
                        font.pixelSize: 14
                        color: Theme.accentColor
                        font.weight: Font.Medium
                    }
                }
            }

            // Separator line
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                Layout.topMargin: 16
                Layout.bottomMargin: 20
                color: Theme.dividerColor
            }

            // Release notes section
            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.maximumHeight: 120
                clip: true

                background: Rectangle {
                    color: Qt.alpha(Theme.cardColor, 0.5)
                    radius: Theme.radiusSmall
                    border.width: 1
                    border.color: Theme.dividerColor
                }

                Label {
                    width: parent.width
                    text: root.newReleaseNotes
                    wrapMode: Text.Wrap
                    font.pixelSize: 13
                    color: Qt.alpha(Theme.textColor, 0.8)
                    lineHeight: 1.4
                    padding: 16
                }
            }

            // Spacer
            Item {
                Layout.fillHeight: true
                Layout.minimumHeight: 20
            }

            // Action buttons
            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 44
                spacing: 12

                Item { Layout.fillWidth: true } // Push buttons to right

                Button {
                    id: laterBtn
                    Layout.preferredWidth: 100
                    Layout.preferredHeight: 36
                    text: Lang.t("Later")

                    background: Rectangle {
                        radius: Theme.radiusSmall
                        color: laterBtn.hovered ? Qt.alpha(Theme.textColor, 0.12) :
                                                  Qt.alpha(Theme.textColor, 0.06)
                        border.width: 1
                        border.color: laterBtn.hovered ? Qt.alpha(Theme.textColor, 0.2) : Theme.dividerColor

                        Behavior on color { ColorAnimation { duration: 150 } }
                        Behavior on border.color { ColorAnimation { duration: 150 } }
                    }

                    contentItem: Text {
                        text: laterBtn.text
                        font.pixelSize: 13
                        font.weight: Font.Medium
                        color: Theme.textColor
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    onClicked: root.close()
                }

                Button {
                    id: updateNowBtn
                    Layout.preferredWidth: 140
                    Layout.preferredHeight: 36
                    text: Lang.t("Update Now")

                    background: Rectangle {
                        radius: Theme.radiusSmall
                        color: updateNowBtn.hovered ? Qt.lighter(Theme.accentColor, 1.1) : Theme.accentColor

                        Rectangle {
                            anchors.fill: parent
                            radius: parent.radius
                            color: "transparent"
                            border.width: 1
                            border.color: Qt.lighter(Theme.accentColor, 1.3)
                            opacity: 0.3
                        }

                        Behavior on color { ColorAnimation { duration: 150 } }
                    }

                    contentItem: Text {
                        text: updateNowBtn.text
                        font.pixelSize: 13
                        font.weight: Font.DemiBold
                        color: "#ffffff"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    onClicked: {
                        root.updateAccepted()
                        root.close()
                    }
                }
            }
        }
    }
}
