import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material
import QtQuick.Dialogs
import QtQuick.Effects
import QtQuick.Window
import "../theme"

Dialog {
    id: requestDialog
    title: Lang.t("Application Requests")
    modal: true
    width: parent ? Math.min(parent.width * 0.8, 800) : 800
    height: parent ? Math.min(parent.height * 0.8, 600) : 600
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? Math.round((parent.height - height) / 2) : 0
    padding: 16
    dim: true

    property var pendingRequests: logger.getPendingApplicationRequests()

    background: Rectangle {
        color: Theme.cardColor
        radius: Theme.radiusLarge
        border.color: Theme.borderColor
        border.width: 1
    }

    header: Item {
        implicitHeight: 56
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 24
            anchors.rightMargin: 20
            Label {
                text: Lang.t("Application Requests")
                font.pixelSize: 18
                font.weight: Font.DemiBold
                font.family: "Segoe UI"
                color: Theme.textColor
                Layout.fillWidth: true
            }
            Button {
                width: 32
                height: 32
                flat: true
                onClicked: requestDialog.close()
                background: Rectangle {
                    radius: 8
                    color: parent.hovered ? Qt.alpha(Theme.textColor, 0.08) : "transparent"
                }
                contentItem: Text {
                    text: "×"
                    font.pixelSize: 20
                    color: Theme.lightTextColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: 1
            color: Theme.dividerColor
        }
    }

    contentItem: ColumnLayout {
        spacing: 16

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Label {
                anchors.centerIn: parent
                visible: requestListView.count === 0
                text: Lang.t("No pending application requests")
                font.pixelSize: Theme.fontSizeBody
                color: Theme.lightTextColor
            }

            ScrollView {
                anchors.fill: parent
                clip: true
                visible: requestListView.count > 0

            ListView {
                id: requestListView
                model: requestDialog.pendingRequests
                spacing: 8
                boundsBehavior: Flickable.StopAtBounds

                delegate: Rectangle {
                    width: requestListView.width
                    height: 100
                    radius: Theme.radiusSmall
                    color: index % 2 === 0 ? Qt.lighter(Theme.cardColor, 1.1) : Theme.cardColor
                    border.color: Theme.dividerColor
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 16

                        // App icon placeholder
                        Rectangle {
                            Layout.preferredWidth: 48
                            Layout.preferredHeight: 48
                            radius: Theme.radiusSmall
                            color: Qt.rgba(
                                       Math.random() * 0.5 + 0.3,
                                       Math.random() * 0.5 + 0.3,
                                       Math.random() * 0.5 + 0.3,
                                       0.2
                                       )

                            Label {
                                text: modelData.app_name.charAt(0).toUpperCase()
                                anchors.centerIn: parent
                                font {
                                    family: "Segoe UI"
                                    weight: Font.Bold
                                    pixelSize: 18
                                }
                                color: Theme.primaryColor
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            // Application name row
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                Label {
                                    text: modelData.app_name
                                    font {
                                        family: "Segoe UI"
                                        pixelSize: Theme.fontSizeTitle
                                        weight: Font.Medium
                                    }
                                    color: Theme.textColor
                                    elide: Text.ElideRight
                                }

                                // Productivity type badge
                                Rectangle {
                                    visible: modelData.productivity_text
                                    radius: 4
                                    color: {
                                        if (modelData.productivity === 1) return Theme.productiveColor;
                                        if (modelData.productivity === 2) return Theme.nonProductiveColor;
                                        return Theme.neutralColor;
                                    }
                                    Layout.preferredHeight: 20
                                    Layout.preferredWidth: productivityText.width + 12
                                    opacity: 0.8

                                    Label {
                                        id: productivityText
                                        text: modelData.productivity_text || ""
                                        anchors.centerIn: parent
                                        font {
                                            family: "Segoe UI"
                                            pixelSize: 10
                                            weight: Font.DemiBold
                                        }
                                        color: "white"
                                    }
                                }
                            }

                            // URL display
                            Label {
                                text: Lang.t("URL") + ": " + (modelData.url || Lang.t("Not specified"))
                                font {
                                    family: "Segoe UI"
                                    pixelSize: Theme.fontSizeSmall
                                }
                                color: Theme.lightTextColor
                                elide: Text.ElideMiddle
                                Layout.fillWidth: true
                            }

                            // For users display
                            Label {
                                text: Lang.t("For") + ": " + modelData.for_users
                                font {
                                    family: "Segoe UI"
                                    pixelSize: 11
                                }
                                color: Qt.lighter(Theme.lightTextColor, 1.2)
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                        }
                    }
                }
            }
        }
        }

        Button {
            text: Lang.t("Close")
            Layout.alignment: Qt.AlignRight
            Layout.preferredWidth: 110
            Layout.preferredHeight: 38
            onClicked: requestDialog.close()

            background: Rectangle {
                radius: Theme.radiusSmall
                color: parent.hovered ? Theme.primaryHoverColor : Theme.primaryColor
            }

            contentItem: Text {
                text: parent.text
                font.pixelSize: Theme.fontSizeSmall
                font.weight: Font.DemiBold
                color: "white"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
    }

    onOpened: {
        pendingRequests = logger.getPendingApplicationRequests()
    }
}
