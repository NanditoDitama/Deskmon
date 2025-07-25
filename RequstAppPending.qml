import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material
import QtQuick.Dialogs
import QtQuick.Effects
import QtQuick.Window


Dialog {
    id: requestDialog
    title: "Application Requests"
    modal: true
    width: Math.min(parent.width * 0.8, 800)
    height: Math.min(parent.height * 0.8, 600)
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2
    padding: 16
    dim: true

    property var pendingRequests: logger.getPendingApplicationRequests()

    background: Rectangle {
        color: cardColor
        radius: 12
        border.color: dividerColor
        border.width: 1

        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            gradient: Gradient {
                GradientStop { position: 0.0; color: Qt.rgba(0,0,0,0.05) }
                GradientStop { position: 1.0; color: "transparent" }
            }
        }
    }

    contentItem: ColumnLayout {
        spacing: 16

        Label {
            text: "Pending Application Requests"
            font {
                pixelSize: 18
                bold: true
                family: "Segoe UI"
            }
            color: primaryColor
            Layout.alignment: Qt.AlignHCenter
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: dividerColor
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            ListView {
                id: requestListView
                model: requestDialog.pendingRequests
                spacing: 8
                boundsBehavior: Flickable.StopAtBounds

                delegate: Rectangle {
                    width: requestListView.width
                    height: 100  // Increased height to accommodate more info
                    radius: 8
                    color: index % 2 === 0 ? Qt.lighter(cardColor, 1.1) : cardColor
                    border.color: dividerColor
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 16

                        // App icon placeholder
                        Rectangle {
                            Layout.preferredWidth: 48
                            Layout.preferredHeight: 48
                            radius: 8
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
                                color: primaryColor
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
                                        pixelSize: 16
                                        weight: Font.Medium
                                    }
                                    color: textColor
                                    elide: Text.ElideRight
                                }

                                // Productivity type badge
                                Rectangle {
                                    visible: modelData.productivity_text
                                    radius: 4
                                    color: {
                                        if (modelData.productivity === 1) return "#4CAF50"; // Green for productive
                                        if (modelData.productivity === 2) return "#F44336"; // Red for non-productive
                                        return "#9E9E9E"; // Gray for neutral
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
                                text: "URL: " + (modelData.url || "Not specified")
                                font {
                                    family: "Segoe UI"
                                    pixelSize: 12
                                }
                                color: lightTextColor
                                elide: Text.ElideMiddle
                                Layout.fillWidth: true
                            }

                            // For users display
                            Label {
                                text: "For: " + modelData.for_users
                                font {
                                    family: "Segoe UI"
                                    pixelSize: 11
                                }
                                color: Qt.lighter(lightTextColor, 1.2)
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                        }
                    }
                }
            }
        }

        Button {
            text: "Close"
            Layout.alignment: Qt.AlignRight
            Layout.preferredWidth: 120
            Layout.preferredHeight: 40
            onClicked: requestDialog.close()

            background: Rectangle {
                radius: 8
                color: parent.hovered ? Qt.lighter(secondaryColor, 1.1) : secondaryColor
            }

            contentItem: Text {
                text: parent.text
                font.pixelSize: 14
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
