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
    id: applicationsDialog
    dialogTitle: Lang.t("Monitored Applications")
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
        var domain = url.replace(/^https?:\/\//, '').split('/')[0]
        return domain.replace(/^www\./, '')
    }
    // Initialize filtered models when dialog opens
    onOpened: {
        filterApps()
    }

    footer: DialogButtonBox {
        alignment: Qt.AlignRight
        leftPadding: Theme.dialogPadding
        rightPadding: Theme.dialogPadding
        topPadding: 14
        bottomPadding: 14
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
        CustomButton {
            text: Lang.t("Add Application")
            btnColor: Theme.cardHeaderColor
            btnHoverColor: Qt.alpha(Theme.textColor, 0.06)
            btnTextColor: Theme.textColor
            btnBorderColor: Theme.borderColor
            btnBorderWidth: 1
            font.weight: Font.Medium
            onClicked: {
                applicationsDialog.close()
                addAppDialog.open()
            }
        }

        CustomButton {
            text: Lang.t("OK")
            btnColor: Theme.primaryColor
            btnHoverColor: Theme.primaryHoverColor
            onClicked: applicationsDialog.accept()
        }
    }

    width: 900
    height: 700

    Column {
        spacing: 14
        anchors.fill: parent
        anchors.margins: Theme.dialogPadding

        // Search and Filter Row
        Row {
            width: parent.width
            spacing: 12
            height: Theme.controlHeight

            // Search Field
            CustomTextField {
                id: search_Field
                width: parent.width * 0.6
                height: Theme.controlHeight
                placeholderText: Lang.t("Search applications...")
                leftIconSource: "image://icon/search.svg?" + Theme.lightTextColor

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
            }

            // Clear search button
            CustomButton {
                width: Theme.controlHeight
                height: Theme.controlHeight
                leftPadding: 0
                rightPadding: 0
                visible: search_Field.text.length > 0
                text: "✕"
                btnColor: "transparent"
                btnHoverColor: Qt.lighter(Theme.cardColor, 1.2)
                btnTextColor: Theme.lightTextColor
                btnBorderColor: Theme.dividerColor
                btnBorderWidth: 1
                font.pixelSize: Theme.fontSizeTitle
                onClicked: {
                    search_Field.text = ""
                    search_Field.focus = true
                }
            }

            // Request Button
            CustomButton {
                id: requestButton
                text: Lang.t("Request")
                height: Theme.controlHeight
                btnColor: Theme.cardHeaderColor
                btnHoverColor: Qt.alpha(Theme.textColor, 0.06)
                btnTextColor: Theme.textColor
                btnBorderColor: Theme.borderColor
                btnBorderWidth: 1
                font.weight: Font.Medium
                onClicked: requestDialog.open()
            }
        }

        // Main Content Row
        Row {
            width: parent.width
            height: parent.height - Theme.controlHeight - 14
            spacing: 16

            // Productive Apps Column
            Column {
                width: parent.width * 0.48
                height: parent.height
                spacing: 8

                Rectangle {
                    width: parent.width
                    height: 32
                    color: "transparent"
                    radius: Theme.radiusSmall

                    Text {
                        text: Lang.t("Productive Apps")
                        font.weight: Font.DemiBold
                        font.pixelSize: Theme.fontSizeTitle
                        color: Theme.productiveColor
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                    }
                }

                ListView {
                    id: productiveListView
                    width: parent.width
                    height: parent.height - 40
                    clip: true
                    spacing: 6
                    model: filteredProductiveApps // Use filtered array
                    ScrollBar.vertical: CustomScrollBar {}

                    delegate: CustomCard {
                        width: productiveListView.width - 10
                        height: 54
                        cardRadius: Theme.radiusSmall
                        cardColor: hover ? Qt.alpha(Theme.productiveColor, 0.08) : Theme.cardHeaderColor
                        cardBorderColor: hover ? Theme.productiveColor : Theme.dividerColor

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

                            AppIconBadge {
                                appName: modelData.appName || ""
                                url: modelData.url || ""
                                accentColor: Theme.productiveColor
                                anchors.verticalCenter: parent.verticalCenter
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
                    height: 32
                    color: "transparent"
                    radius: Theme.radiusSmall

                    Text {
                        text: Lang.t("Non-Productive Apps")
                        font.weight: Font.DemiBold
                        font.pixelSize: Theme.fontSizeTitle
                        color: Theme.nonProductiveColor
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                    }
                }

                ListView {
                    id: nonProductiveListView
                    width: parent.width
                    height: parent.height - 40
                    clip: true
                    spacing: 6
                    model: filteredNonProductiveApps // Use filtered array
                    ScrollBar.vertical: CustomScrollBar {}

                    delegate: CustomCard {
                        width: nonProductiveListView.width - 10
                        height: 54
                        cardRadius: Theme.radiusSmall
                        cardColor: hover ? Qt.alpha(Theme.nonProductiveColor, 0.08) : Theme.cardHeaderColor
                        cardBorderColor: hover ? Theme.nonProductiveColor : Theme.dividerColor

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

                            AppIconBadge {
                                appName: modelData.appName || ""
                                url: modelData.url || ""
                                accentColor: Theme.nonProductiveColor
                                anchors.verticalCenter: parent.verticalCenter
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

