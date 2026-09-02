import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Dialog {
    id: applicationsDialog
    title: "<b>Monitored Applications</b>"
    modal: true
    width: 900
    height: 700
    anchors.centerIn: parent

    // Properties that need to be set from Main.qml
    property var productiveAppsModel
    property var nonProductiveAppsModel
    property var filteredProductiveAppsModel
    property var filteredNonProductiveAppsModel
    property color cardColor: "#FFFFFF"
    property color textColor: "#1F2937"
    property color dividerColor: "#E5E7EB"
    property color primaryColor: "#00e0a8"

    // Signals
    signal addAppRequested()

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

    // Re-filter when original models change
    Connections {
        target: productiveAppsModel
        function onCountChanged() {
            applicationsDialog.filterApps()
        }
    }

    Connections {
        target: nonProductiveAppsModel
        function onCountChanged() {
            applicationsDialog.filterApps()
        }
    }

    background: Rectangle {
        color: cardColor
        radius: 8
    }

    footer: DialogButtonBox {
        alignment: Qt.AlignRight
        background: Rectangle {
            color: cardColor
            radius: 8
        }
        Button {
            text: "Tambah Aplikasi"
            flat: true
            onClicked: {
                applicationsDialog.close()
                addAppRequested()
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
            text: "OK"
            flat: true
            onClicked: applicationsDialog.accept()
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
                placeholderText: "Search applications..."
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
                    color: dividerColor
                    radius: 8
                    border.color: search_Field.activeFocus ? "#1976d2" : "#e0e0e0"
                    border.width: 1

                    Image {
                        source: "qrc:/icons/search.svg"
                        width: 20
                        height: 20
                        anchors.left: parent.left
                        anchors.leftMargin: 10
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }
        }

        // Productive Apps Section
        ColumnLayout {
            width: parent.width
            spacing: 10

            Label {
                text: "Productive Applications"
                font.bold: true
                font.pixelSize: 16
                color: primaryColor
            }

            ListView {
                Layout.fillWidth: true
                Layout.preferredHeight: 250
                model: filteredProductiveAppsModel
                clip: true

                delegate: Rectangle {
                    width: parent.width
                    height: 60
                    color: index % 2 === 0 ? Qt.lighter(cardColor, 1.02) : cardColor
                    border.color: dividerColor
                    border.width: 0.5
                    radius: 4

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 12

                        Rectangle {
                            Layout.preferredWidth: 36
                            Layout.preferredHeight: 36
                            radius: 18
                            color: primaryColor
                            opacity: 0.1

                            Text {
                                text: model.appName ? model.appName.charAt(0).toUpperCase() : "?"
                                anchors.centerIn: parent
                                font.bold: true
                                font.pixelSize: 16
                                color: primaryColor
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            Text {
                                text: model.appName || ""
                                font.bold: true
                                font.pixelSize: 14
                                color: textColor
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }

                            Text {
                                text: model.url ? extractDomain(model.url) : "System Application"
                                font.pixelSize: 12
                                color: Qt.lighter(textColor, 1.4)
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                        }

                        Rectangle {
                            Layout.preferredWidth: 80
                            Layout.preferredHeight: 24
                            radius: 12
                            color: primaryColor
                            opacity: 0.8

                            Text {
                                text: "Productive"
                                anchors.centerIn: parent
                                font.pixelSize: 10
                                font.bold: true
                                color: "white"
                            }
                        }
                    }
                }
            }
        }

        // Non-Productive Apps Section
        ColumnLayout {
            width: parent.width
            spacing: 10

            Label {
                text: "Non-Productive Applications"
                font.bold: true
                font.pixelSize: 16
                color: "#ff5100"
            }

            ListView {
                Layout.fillWidth: true
                Layout.preferredHeight: 250
                model: filteredNonProductiveAppsModel
                clip: true

                delegate: Rectangle {
                    width: parent.width
                    height: 60
                    color: index % 2 === 0 ? Qt.lighter(cardColor, 1.02) : cardColor
                    border.color: dividerColor
                    border.width: 0.5
                    radius: 4

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 12

                        Rectangle {
                            Layout.preferredWidth: 36
                            Layout.preferredHeight: 36
                            radius: 18
                            color: "#ff5100"
                            opacity: 0.1

                            Text {
                                text: model.appName ? model.appName.charAt(0).toUpperCase() : "?"
                                anchors.centerIn: parent
                                font.bold: true
                                font.pixelSize: 16
                                color: "#ff5100"
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            Text {
                                text: model.appName || ""
                                font.bold: true
                                font.pixelSize: 14
                                color: textColor
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }

                            Text {
                                text: model.url ? extractDomain(model.url) : "System Application"
                                font.pixelSize: 12
                                color: Qt.lighter(textColor, 1.4)
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                        }

                        Rectangle {
                            Layout.preferredWidth: 100
                            Layout.preferredHeight: 24
                            radius: 12
                            color: "#ff5100"
                            opacity: 0.8

                            Text {
                                text: "Non-Productive"
                                anchors.centerIn: parent
                                font.pixelSize: 10
                                font.bold: true
                                color: "white"
                            }
                        }
                    }
                }
            }
        }
    }
}