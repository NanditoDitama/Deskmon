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

    property var allProductiveApps: []
    property var allNonProductiveApps: []
    property var filteredProductiveApps: []
    property var filteredNonProductiveApps: []

    ListModel { id: productiveAppsModel }
    ListModel { id: nonProductiveAppsModel }
    ListModel { id: filteredProductiveAppsModel }
    ListModel { id: filteredNonProductiveAppsModel }

    function setApps(apps) {
        var prod = []
        var nonProd = []
        if (apps) {
            for (var i = 0; i < apps.length; i++) {
                var item = apps[i]
                if (item.type === 1) {
                    prod.push({ "appName": item.appName || "", "url": item.url || "" })
                } else if (item.type === 2) {
                    nonProd.push({ "appName": item.appName || "", "url": item.url || "" })
                }
            }
        }
        allProductiveApps = prod
        allNonProductiveApps = nonProd
        filterApps()
    }

    // Search function
    function filterApps() {
        var searchText = search_Field.text.toLowerCase().trim()
        var prodSource = allProductiveApps
        var nonProdSource = allNonProductiveApps

        if (prodSource.length === 0 && productiveAppsModel.count > 0) {
            prodSource = []
            for (var i = 0; i < productiveAppsModel.count; i++) {
                prodSource.push(productiveAppsModel.get(i))
            }
        }
        if (nonProdSource.length === 0 && nonProductiveAppsModel.count > 0) {
            nonProdSource = []
            for (var j = 0; j < nonProductiveAppsModel.count; j++) {
                nonProdSource.push(nonProductiveAppsModel.get(j))
            }
        }

        if (searchText === "") {
            filteredProductiveApps = prodSource
            filteredNonProductiveApps = nonProdSource
            return
        }

        var filteredProd = []
        for (var k = 0; k < prodSource.length; k++) {
            var item = prodSource[k]
            var appName = item.appName ? item.appName.toLowerCase() : ""
            var url = item.url ? item.url.toLowerCase() : ""
            if (appName.indexOf(searchText) !== -1 || url.indexOf(searchText) !== -1) {
                filteredProd.push(item)
            }
        }

        var filteredNonProd = []
        for (var m = 0; m < nonProdSource.length; m++) {
            var item2 = nonProdSource[m]
            var appName2 = item2.appName ? item2.appName.toLowerCase() : ""
            var url2 = item2.url ? item2.url.toLowerCase() : ""
            if (appName2.indexOf(searchText) !== -1 || url2.indexOf(searchText) !== -1) {
                filteredNonProd.push(item2)
            }
        }

        filteredProductiveApps = filteredProd
        filteredNonProductiveApps = filteredNonProd
    }

    function extractDomain(url) {
        if (!url) return ""
        // Remove protocol and path
        var domain = url.replace(/^https?:\/\//, '').split('/')[0]
        // Remove www. if present
        return domain.replace(/^www\./, '')
    }

    function resolveIconUrl(appName, url) {
        var domain = extractDomain(url)
        if (domain) return "https://www.google.com/s2/favicons?domain=" + domain + "&sz=64"
        if (!appName) return ""
        var key = appName.toLowerCase().trim()
        var map = {
            "chrome": "google.com/chrome",
            "firefox": "mozilla.org",
            "edge": "microsoft.com",
            "safari": "apple.com",
            "opera": "opera.com",
            "visual studio code": "code.visualstudio.com",
            "vscode": "code.visualstudio.com",
            "figma": "figma.com",
            "canva": "canva.com",
            "blender": "blender.org",
            "capcut": "capcut.com",
            "discord": "discord.com",
            "slack": "slack.com",
            "zoom": "zoom.us",
            "notion": "notion.so",
            "spotify": "spotify.com",
            "whatsapp": "whatsapp.com",
            "telegram": "telegram.org",
            "postman": "postman.com",
            "github": "github.com",
            "acrord32": "adobe.com",
            "adobe": "adobe.com",
            "antigravity": "antigravity.google",
            "anti gravity": "antigravity.google"
        }
        for (var k in map) {
            if (key.indexOf(k) !== -1) {
                return "https://www.google.com/s2/favicons?domain=" + map[k] + "&sz=64"
            }
        }
        return ""
    }
    // Initialize filtered models when dialog opens
    onOpened: {
        filterApps()
    }

    footer: DialogButtonBox {
        alignment: Qt.AlignRight
        padding: 16
        spacing: 10
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
        Button {
            text: Lang.t("Add Application")
            implicitHeight: 38
            leftPadding: 16
            rightPadding: 16
            onClicked: {
                applicationsDialog.close()
                addAppDialog.open()
            }
            background: Rectangle {
                radius: Theme.radiusSmall
                color: parent.hovered ? Qt.alpha(Theme.textColor, 0.06) : Theme.cardHeaderColor
                border.color: Theme.borderColor
                border.width: 1
            }
            contentItem: Text {
                text: parent.text
                font.pixelSize: Theme.fontSizeSmall
                font.weight: Font.Medium
                color: Theme.textColor
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }

        Button {
            text: Lang.t("OK")
            implicitHeight: 38
            leftPadding: 24
            rightPadding: 24
            onClicked: applicationsDialog.accept()
            background: Rectangle {
                radius: Theme.radiusSmall
                color: parent.hovered ? Theme.primaryHoverColor : Theme.primaryColor
            }
            contentItem: Text {
                text: parent.text
                font.pixelSize: Theme.fontSizeSmall
                font.weight: Font.DemiBold
                color: "#FFFFFF"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
    }

    header: Item {
        implicitHeight: 56
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 24
            anchors.rightMargin: 20
            Label {
                text: Lang.t("Monitored Applications")
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
                onClicked: applicationsDialog.close()
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

    width: 900
    height: 700
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? Math.round((parent.height - height) / 2) : 0
    background: Rectangle {
        color: Theme.cardColor
        radius: Theme.radiusLarge
        border.color: Theme.borderColor
        border.width: 1
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
                    color: parent.hovered ? Qt.alpha(Theme.textColor, 0.06) : Theme.cardHeaderColor
                    border.color: Theme.borderColor
                    border.width: 1
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
                    model: filteredProductiveApps // Use filtered array
                    ScrollBar.vertical: CustomScrollBar {}

                    delegate: Rectangle {
                        width: productiveListView.width - 10
                        height: 54
                        radius: Theme.radiusSmall
                        color: hover ? Qt.alpha(Theme.productiveColor, 0.08) : Theme.cardHeaderColor
                        border.color: hover ? Theme.productiveColor : Theme.dividerColor
                        border.width: 1

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
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12

                            Rectangle {
                                width: 34
                                height: 34
                                radius: 8
                                color: Qt.alpha(Theme.productiveColor, 0.14)
                                anchors.verticalCenter: parent.verticalCenter

                                Image {
                                    id: prodIconImg
                                    anchors.centerIn: parent
                                    width: 22
                                    height: 22
                                    asynchronous: true
                                    fillMode: Image.PreserveAspectFit
                                    source: applicationsDialog.resolveIconUrl(modelData.appName, modelData.url)
                                    visible: status === Image.Ready
                                }

                                Text {
                                    visible: prodIconImg.status !== Image.Ready
                                    text: (modelData.appName || "?").charAt(0).toUpperCase()
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
                                    text: modelData.appName || ""
                                    font.bold: true
                                    font.pixelSize: Theme.fontSizeSmall
                                    color: Theme.textColor
                                    width: parent.width
                                    elide: Text.ElideRight
                                }

                                Text {
                                    text: extractDomain(modelData.url) || Lang.t("Application")
                                    font.pixelSize: 10
                                    color: Theme.lightTextColor
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
                    model: filteredNonProductiveApps // Use filtered array
                    ScrollBar.vertical: CustomScrollBar {}

                    delegate: Rectangle {
                        width: nonProductiveListView.width - 10
                        height: 54
                        radius: Theme.radiusSmall
                        color: hover ? Qt.alpha(Theme.nonProductiveColor, 0.08) : Theme.cardHeaderColor
                        border.color: hover ? Theme.nonProductiveColor : Theme.dividerColor
                        border.width: 1

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
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12

                            Rectangle {
                                width: 34
                                height: 34
                                radius: 8
                                color: Qt.alpha(Theme.nonProductiveColor, 0.14)
                                anchors.verticalCenter: parent.verticalCenter

                                Image {
                                    id: nonProdIconImg
                                    anchors.centerIn: parent
                                    width: 22
                                    height: 22
                                    asynchronous: true
                                    fillMode: Image.PreserveAspectFit
                                    source: applicationsDialog.resolveIconUrl(modelData.appName, modelData.url)
                                    visible: status === Image.Ready
                                }

                                Text {
                                    visible: nonProdIconImg.status !== Image.Ready
                                    text: (modelData.appName || "?").charAt(0).toUpperCase()
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
                                    text: modelData.appName || ""
                                    font.bold: true
                                    font.pixelSize: Theme.fontSizeSmall
                                    color: Theme.textColor
                                    width: parent.width
                                    elide: Text.ElideRight
                                }

                                Text {
                                    text: modelData.url || Lang.t("Application")
                                    font.pixelSize: 10
                                    color: Theme.lightTextColor
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
        onCancelRequested: applicationsDialog.open()
    }
    PendingAppRequestsDialog{
        id: requestDialog
        parent: Overlay.overlay
    }
}

