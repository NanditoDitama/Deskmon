import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material
import QtQuick.Dialogs
import QtQuick.Effects
import QtQuick.Window
import "../theme"
import "../components"

CustomDialog {
    id: requestDialog
    dialogTitle: Lang.t("Application Requests")
    width: parent ? Math.min(parent.width * 0.8, 800) : 800
    height: parent ? Math.min(parent.height * 0.8, 600) : 600
    padding: 16
    dim: true

    property var pendingRequests: logger.getPendingApplicationRequests()

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

            HoverHandler {
                id: requestScrollHover
            }

            ScrollView {
                id: requestScrollView
                anchors.fill: parent
                clip: true
                visible: requestListView.count > 0
                rightPadding: requestScrollBar.isScrollable ? 14 : 0
                ScrollBar.vertical: CustomScrollBar {
                    id: requestScrollBar
                    containerHovered: requestScrollHover.hovered
                }
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            ListView {
                id: requestListView
                model: requestDialog.pendingRequests
                spacing: 8
                boundsBehavior: Flickable.StopAtBounds

                delegate: CustomCard {
                    width: requestListView.width
                    height: 92
                    cardRadius: Theme.radiusSmall
                    cardColor: Theme.cardHeaderColor

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 14

                        AppIconBadge {
                            appName: modelData.app_name || ""
                            url: modelData.url || ""
                            accentColor: modelData.productivity === 2 ? Theme.nonProductiveColor : Theme.primaryColor
                            badgeSize: 42
                            iconSize: 24
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
    }

    footer: DialogButtonBox {
        alignment: Qt.AlignRight
        leftPadding: Theme.dialogPadding
        rightPadding: Theme.dialogPadding
        topPadding: 12
        bottomPadding: 12
        background: Rectangle {
            color: Theme.cardColor
            radius: Theme.radiusLarge
            Rectangle {
                anchors.top: parent.top
                width: parent.width
                height: 1
                color: Theme.dividerColor
            }
        }
        CustomButton {
            text: Lang.t("Close")
            btnColor: Theme.primaryColor
            btnHoverColor: Theme.primaryHoverColor
            font.pixelSize: Theme.fontSizeSmall
            onClicked: requestDialog.close()
        }
    }

    onOpened: {
        pendingRequests = logger.getPendingApplicationRequests()
    }
}
