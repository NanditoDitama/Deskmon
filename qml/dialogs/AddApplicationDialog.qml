import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Dialog {
    id: addAppDialog
    title: "Tambah Aplikasi Produktivitas"
    modal: true
    anchors.centerIn: parent
    width: 500
    height: 600
    padding: 20

    // Properties that need to be set from Main.qml
    property int selectedProductivityType: 1
    property var logger
    property color cardColor: "#FFFFFF"
    property color textColor: "#1F2937"
    property color lightTextColor: "#6B7280"
    property color dividerColor: "#E5E7EB"
    property color primaryColor: "#00e0a8"
    property color secondaryColor: "#3B82F6"
    property color accentColor: "#F59E0B"

    // Signals
    signal backToApplications()

    background: Rectangle {
        color: cardColor
        radius: 12
        border.color: dividerColor
        border.width: 1
    }

    footer: DialogButtonBox {
        alignment: Qt.AlignRight
        background: Rectangle {
            color: cardColor
            radius: 8
        }

        Button {
            text: "Kembali"
            flat: true
            onClicked: {
                addAppDialog.close()
                backToApplications()
            }
            background: Rectangle {
                radius: 14
                color: parent.hovered ? Qt.lighter(cardColor, 1.5) : "transparent"
                border.color: dividerColor
                border.width: 1
            }
            contentItem: Text {
                text: parent.text
                color: textColor
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }

        Button {
            text: "Simpan"
            flat: true
            onClicked: {
                if (logger) {
                    // Get selected values
                    var selectedApp = applicationCombo.currentText
                    var website = websiteField.text.trim()
                    var prodType = addAppDialog.selectedProductivityType

                    console.log("Saving app:", selectedApp, "Website:", website, "Type:", prodType)

                    // Add the application
                    const success = logger.addProductivityApp(selectedApp, website, prodType)
                    if (success) {
                        addAppDialog.close()
                        backToApplications()
                    }
                }
            }
            background: Rectangle {
                radius: 14
                color: parent.hovered ? Qt.lighter(primaryColor, 1.1) : primaryColor
            }
            contentItem: Text {
                text: parent.text
                color: "white"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.bold: true
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 20

        // Header
        Label {
            text: "Add New Productivity Application"
            font {
                bold: true
                pixelSize: 18
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

        // Application Selection
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 8

            Label {
                text: "Select Application"
                font {
                    pixelSize: 14
                    bold: true
                }
                color: textColor
            }

            ComboBox {
                id: applicationCombo
                Layout.fillWidth: true
                Layout.preferredHeight: 50
                model: logger ? logger.getAvailableApps() : []

                background: Rectangle {
                    color: dividerColor
                    radius: 8
                    border.color: applicationCombo.activeFocus ? primaryColor : dividerColor
                    border.width: 2
                }

                contentItem: Text {
                    text: applicationCombo.displayText
                    font.pixelSize: 14
                    color: textColor
                    verticalAlignment: Text.AlignVCenter
                    leftPadding: 12
                    rightPadding: 12
                }

                popup: Popup {
                    y: applicationCombo.height - 1
                    width: applicationCombo.width
                    implicitHeight: contentItem.implicitHeight
                    padding: 1

                    contentItem: ListView {
                        clip: true
                        implicitHeight: Math.min(contentHeight, 200)
                        model: applicationCombo.popup.visible ? applicationCombo.delegateModel : null
                        currentIndex: applicationCombo.highlightedIndex

                        ScrollIndicator.vertical: ScrollIndicator { }
                    }

                    background: Rectangle {
                        color: cardColor
                        border.color: dividerColor
                        radius: 8
                    }
                }
            }
        }

        // Website Field (for browsers)
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 8
            visible: isBrowserApp()

            Label {
                text: "Website/Domain (Optional for browsers)"
                font {
                    pixelSize: 14
                    bold: true
                }
                color: textColor
            }

            TextField {
                id: websiteField
                Layout.fillWidth: true
                Layout.preferredHeight: 50
                placeholderText: "e.g., github.com, stackoverflow.com"
                font.pixelSize: 14

                background: Rectangle {
                    color: dividerColor
                    radius: 8
                    border.color: websiteField.activeFocus ? primaryColor : dividerColor
                    border.width: 2
                }
            }
        }

        // Productivity Type Selection
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 12

            Label {
                text: "Productivity Type"
                font {
                    pixelSize: 14
                    bold: true
                }
                color: textColor
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 8

                RadioButton {
                    text: "Productive - Helps with work/learning"
                    checked: addAppDialog.selectedProductivityType === 1
                    onCheckedChanged: if (checked) addAppDialog.selectedProductivityType = 1

                    indicator: Rectangle {
                        implicitWidth: 20
                        implicitHeight: 20
                        x: parent.leftPadding
                        y: parent.height / 2 - height / 2
                        radius: 10
                        border.color: parent.checked ? "#4CAF50" : dividerColor
                        border.width: 2
                        color: "transparent"

                        Rectangle {
                            width: 10
                            height: 10
                            radius: 5
                            anchors.centerIn: parent
                            color: "#4CAF50"
                            visible: parent.parent.checked
                        }
                    }

                    contentItem: Text {
                        text: parent.text
                        font.pixelSize: 14
                        color: textColor
                        verticalAlignment: Text.AlignVCenter
                        leftPadding: parent.indicator.width + parent.spacing
                    }
                }

                RadioButton {
                    text: "Non-Productive - Entertainment/distraction"
                    checked: addAppDialog.selectedProductivityType === 2
                    onCheckedChanged: if (checked) addAppDialog.selectedProductivityType = 2

                    indicator: Rectangle {
                        implicitWidth: 20
                        implicitHeight: 20
                        x: parent.leftPadding
                        y: parent.height / 2 - height / 2
                        radius: 10
                        border.color: parent.checked ? "#F44336" : dividerColor
                        border.width: 2
                        color: "transparent"

                        Rectangle {
                            width: 10
                            height: 10
                            radius: 5
                            anchors.centerIn: parent
                            color: "#F44336"
                            visible: parent.parent.checked
                        }
                    }

                    contentItem: Text {
                        text: parent.text
                        font.pixelSize: 14
                        color: textColor
                        verticalAlignment: Text.AlignVCenter
                        leftPadding: parent.indicator.width + parent.spacing
                    }
                }

                RadioButton {
                    text: "Neutral - Neither productive nor distracting"
                    checked: addAppDialog.selectedProductivityType === 0
                    onCheckedChanged: if (checked) addAppDialog.selectedProductivityType = 0

                    indicator: Rectangle {
                        implicitWidth: 20
                        implicitHeight: 20
                        x: parent.leftPadding
                        y: parent.height / 2 - height / 2
                        radius: 10
                        border.color: parent.checked ? "#9E9E9E" : dividerColor
                        border.width: 2
                        color: "transparent"

                        Rectangle {
                            width: 10
                            height: 10
                            radius: 5
                            anchors.centerIn: parent
                            color: "#9E9E9E"
                            visible: parent.parent.checked
                        }
                    }

                    contentItem: Text {
                        text: parent.text
                        font.pixelSize: 14
                        color: textColor
                        verticalAlignment: Text.AlignVCenter
                        leftPadding: parent.indicator.width + parent.spacing
                    }
                }
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }

    function isBrowserApp() {
        var browserApps = ["Chrome", "Firefox", "Edge", "Safari", "Opera", "Brave"]
        var currentApp = applicationCombo.currentText
        return browserApps.includes(currentApp)
    }
}