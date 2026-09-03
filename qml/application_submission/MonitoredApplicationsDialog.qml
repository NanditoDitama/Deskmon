import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material
import QtQuick.Dialogs
import QtQuick.Effects
import QtQuick.Window
import "../theme"

Dialog {
    id: applicationsDialog
    title: "<b>" + Lang.t("Monitored Applications") + "</b>"
    modal: true

    property alias productiveAppsModel: productiveAppsModel
    property alias nonProductiveAppsModel: nonProductiveAppsModel

    ListModel { id: productiveAppsModel }
    ListModel { id: nonProductiveAppsModel }
    ListModel { id: filteredProductiveAppsModel }
    ListModel { id: filteredNonProductiveAppsModel }

    // Search function
    function filterApps() {
        var searchText = search_Field.text.toLowerCase().trim()

        filteredProductiveAppsModel.clear()
        filteredNonProductiveAppsModel.clear()

        // Filter productive apps
        for (var i = 0; i < productiveAppsModel.count; i++) {
            var item = productiveAppsModel.get(i)
            var appName = item.appName ? item.appName.toLowerCase() : ""
            var url = item.url ? item.url.toLowerCase() : ""

            if (searchText === "" ||
                    appName.indexOf(searchText) !== -1 ||
                    url.indexOf(searchText) !== -1) {
                filteredProductiveAppsModel.append(item)
            }
        }

        // Filter non-productive apps
        for (var j = 0; j < nonProductiveAppsModel.count; j++) {
            var item2 = nonProductiveAppsModel.get(j)
            var appName2 = item2.appName ? item2.appName.toLowerCase() : ""
            var url2 = item2.url ? item2.url.toLowerCase() : ""

            if (searchText === "" ||
                    appName2.indexOf(searchText) !== -1 ||
                    url2.indexOf(searchText) !== -1) {
                filteredNonProductiveAppsModel.append(item2)
            }
        }
    }

    function extractDomain(url) {
        if (!url) return ""
        // Remove protocol and path
        var domain = url.replace(/^https?:\/\//, '').split('/')[0]
        // Remove www. if present
        return domain.replace(/^www\./, '')
    }
    // Initialize filtered models when dialog opens
    onOpened: {
        filterApps()
    }

    footer: DialogButtonBox {
        alignment: Qt.AlignRight
        background: Rectangle {
            color: Theme.cardColor
            radius: Theme.radiusSmall
        }
        Button {
            text: Lang.t("Add Application")
            flat: true
            onClicked: {
                applicationsDialog.close()
                addAppDialog.open()
            }
            background: Rectangle {
                radius: 14
                color: parent.hovered ? Qt.lighter(Theme.cardColor, 1.5) : "transparent"
                border.color: Theme.dividerColor
                border.width: 1
            }
            contentItem: Text {
                text: parent.text
                color: Theme.textColor
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }


        Button {
            text: Lang.t("OK")
            flat: true
            onClicked: applicationsDialog.accept()
            background: Rectangle {
                radius: 14
                color: parent.hovered ? Qt.lighter(Theme.cardColor, 1.5) : "transparent"
                border.color: Theme.dividerColor
                border.width: 1
            }
            contentItem: Text {
                text: parent.text
                color: Theme.textColor
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
    }

    width: 900
    height: 700
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? Math.round((parent.height - height) / 2) : 0
    background: Rectangle {
        color: Theme.cardColor
        radius: Theme.radiusSmall
    }

    Column {
        spacing: 15
        anchors.fill: parent
        anchors.margins: 20

        // Search and Filter Row
        Row {
            width: parent.width
            spacing: 15
            height: 50

            // Search Field
            TextField {
                id: search_Field
                width: parent.width * 0.6
                height: 40
                placeholderText: Lang.t("Search applications...")
                leftPadding: 40

                // Trigger search on text change
                onTextChanged: {
                    searchTimer.restart()
                }

                // Add timer to prevent excessive filtering while typing
                Timer {
                    id: searchTimer
                    interval: 500 // 500ms delay
                    onTriggered: applicationsDialog.filterApps()
                }

                background: Rectangle {
                    color: Theme.dividerColor
                    radius: Theme.radiusSmall
                    border.color: search_Field.activeFocus ? Theme.infoColor : Theme.borderColor
                    border.width: 1

                    Image {
                        source: "image://icon/search.svg?" + Theme.lightTextColor
                        width: 20
                        height: 20
                        anchors.left: parent.left
                        anchors.leftMargin: 12
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }

            // Clear search button
            Button {
                width: 40
                height: 40
                visible: search_Field.text.length > 0
                flat: true
                onClicked: {
                    search_Field.text = ""
                    search_Field.focus = true
                }

                background: Rectangle {
                    radius: Theme.radiusSmall
                    color: parent.hovered ? Qt.lighter(Theme.cardColor, 1.2) : "transparent"
                    border.color: Theme.dividerColor
                    border.width: 1
                }

                contentItem: Text {
                    text: "✕"
                    color: Theme.lightTextColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: Theme.fontSizeTitle
                }
            }
            Rectangle {
                Layout.fillWidth: true
                height: 0
                color: Theme.dividerColor
                Layout.topMargin: 4
            }

            // Request Button
            Button {
                id: requestButton
                text: Lang.t("Request")
                height: 40
                leftPadding: 16
                rightPadding: 16
                font.pixelSize: Theme.fontSizeBody
                onClicked: requestDialog.open()

                background: Rectangle {
                    radius: Theme.radiusSmall
                    color: parent.hovered ? Qt.lighter(Theme.cardColor, 1.5) : "transparent"
                }

                contentItem: Text {
                    text: parent.text
                    font: parent.font
                    color: Theme.textColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }

        // Main Content Row
        Row {
            width: parent.width
            height: parent.height - 70
            spacing: 15

            // Productive Apps Column
            Column {
                width: parent.width * 0.48
                height: parent.height
                spacing: 8

                Rectangle {
                    width: parent.width
                    height: 40
                    color: "transparent"
                    radius: Theme.radiusSmall

                    Text {
                        text: Lang.t("Productive Apps")
                        font.bold: true
                        font.pixelSize: Theme.fontSizeTitle
                        color: Theme.productiveColor
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: 15
                    }
                }

                ListView {
                    id: productiveListView
                    width: parent.width
                    height: parent.height - 50
                    clip: true
                    spacing: 6
                    model: filteredProductiveAppsModel // Use filtered model
                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                    delegate: Rectangle {
                        width: productiveListView.width
                        height: 50
                        gradient: Gradient {
                            orientation: Gradient.Horizontal
                            GradientStop { position: 0.1; color: hover ? Theme.productiveColor : Theme.cardColor }
                            GradientStop { position: 1.0; color: Theme.cardColor }
                        }

                        property bool hover: false

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            onEntered: parent.hover = true
                            onExited: parent.hover = false
                        }

                        Row {
                            spacing: 12
                            anchors.fill: parent
                            anchors.verticalCenter: parent.verticalCenter

                            Rectangle {
                                width: 2
                                height: parent.height
                                color: Theme.productiveColor
                            }

                            Rectangle {
                                width: 32
                                height: 32
                                radius: 6
                                color: Qt.alpha(Theme.productiveColor, 0.15)
                                anchors.verticalCenter: parent.verticalCenter

                                Text {
                                    text: model.appName.charAt(0).toUpperCase()
                                    font.bold: true
                                    font.pixelSize: Theme.fontSizeBody
                                    color: Theme.productiveColor
                                    anchors.centerIn: parent
                                }
                            }

                            Column {
                                width: parent.width - 50
                                spacing: 2
                                anchors.verticalCenter: parent.verticalCenter

                                Text {
                                    text: model.appName
                                    font.bold: true
                                    font.pixelSize: Theme.fontSizeSmall
                                    color: hover ? "white" : Theme.textColor
                                    width: parent.width
                                    elide: Text.ElideRight
                                }

                                Text {
                                    text: extractDomain(model.url) || "Aplikasi"
                                    font.pixelSize: 10
                                    color: hover ? "white" : Theme.lightTextColor
                                    width: parent.width
                                    elide: Text.ElideRight
                                }
                            }
                        }
                    }
                }
            }

            // Vertical Separator
            Rectangle {
                width: 1
                height: parent.height
                color: Theme.dividerColor
            }

            // Non-Productive Apps Column
            Column {
                width: parent.width * 0.48
                height: parent.height
                spacing: 8

                Rectangle {
                    width: parent.width
                    height: 40
                    color: "transparent"
                    radius: Theme.radiusSmall

                    Text {
                        text: Lang.t("Non-Productive Apps")
                        font.bold: true
                        font.pixelSize: 15
                        color: Theme.nonProductiveColor
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: 15
                    }
                }

                ListView {
                    id: nonProductiveListView
                    width: parent.width
                    height: parent.height - 50
                    clip: true
                    spacing: 6
                    model: filteredNonProductiveAppsModel // Use filtered model
                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                    delegate: Rectangle {
                        width: nonProductiveListView.width
                        height: 50
                        gradient: Gradient {
                            orientation: Gradient.Horizontal
                            GradientStop { position: 0.1; color: hover ? Theme.nonProductiveColor : Theme.cardColor }
                            GradientStop { position: 1.0; color: Theme.cardColor }
                        }

                        property bool hover: false

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            onEntered: parent.hover = true
                            onExited: parent.hover = false
                        }

                        Row {
                            spacing: 12
                            anchors.fill: parent
                            anchors.verticalCenter: parent.verticalCenter

                            Rectangle {
                                width: 2
                                height: parent.height
                                color: Theme.nonProductiveColor
                            }

                            Rectangle {
                                width: 32
                                height: 32
                                radius: 6
                                color: Qt.alpha(Theme.nonProductiveColor, 0.15)
                                anchors.verticalCenter: parent.verticalCenter

                                Text {
                                    text: model.appName.charAt(0).toUpperCase()
                                    font.bold: true
                                    font.pixelSize: Theme.fontSizeBody
                                    color: Theme.nonProductiveColor
                                    anchors.centerIn: parent
                                }
                            }

                            Column {
                                width: parent.width - 50
                                spacing: 2
                                anchors.verticalCenter: parent.verticalCenter

                                Text {
                                    text: model.appName
                                    font.bold: true
                                    font.pixelSize: Theme.fontSizeSmall
                                    color: hover ? "white" : Theme.textColor
                                    width: parent.width
                                    elide: Text.ElideRight
                                }

                                Text {
                                    text: model.url || "Aplikasi"
                                    font.pixelSize: 10
                                    color: hover ? "white" : Theme.lightTextColor
                                    width: parent.width
                                    elide: Text.ElideRight
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    AddApplicationRequestDialog{
        id:addAppDialog
        parent:Overlay.overlay
    }
    PendingAppRequestsDialog{
        id: requestDialog
        parent: Overlay.overlay
    }
}

