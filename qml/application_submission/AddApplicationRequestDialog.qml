import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material
import QtQuick.Dialogs
import QtQuick.Effects
import QtQuick.Window
import "../theme"
import "../components"

Dialog {
    id: addAppDialog
    title: Lang.t("Add Productivity Application")
    property int selectedProductivityType: 1
    signal cancelRequested()
    modal: true
    width: parent ? Math.min(parent.width * 0.8, 1000) : 800
    height: parent ? Math.min(parent.height * 0.8, 600) : 600
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? Math.round((parent.height - height) / 2) : 0
    padding: 0
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
                text: Lang.t("Add Productivity Application")
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
                onClicked: addAppDialog.close()
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
            text: Lang.t("Cancel")
            implicitHeight: 38
            leftPadding: 16
            rightPadding: 16
            onClicked: {
                addAppDialog.close()
                addAppDialog.cancelRequested()
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
            id: addButton
            text: Lang.t("Add")
            implicitHeight: 38
            leftPadding: 24
            rightPadding: 24
            font.pixelSize: Theme.fontSizeSmall
            font.weight: Font.DemiBold
            enabled: {
                if (appList.currentItem && appList.model[appList.currentIndex] === "Other") {
                    return txtAppName.text.trim().length > 0
                }
                if (txtWebsite.visible) {
                    return txtWebsite.text.trim().length > 0
                }
                return appList.currentItem !== null
            }
            background: Rectangle {
                color: addButton.enabled ? (parent.hovered ? Theme.primaryHoverColor : Theme.primaryColor) : Theme.dividerColor
                radius: Theme.radiusSmall
            }
            contentItem: Text {
                text: parent.text
                color: addButton.enabled ? "#ffffff" : Theme.mutedTextColor
                font: parent.font
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            onClicked: {
                const prodType = addAppDialog.selectedProductivityType
                const appName = appList.model[appList.currentIndex] === "Other" ?
                                  txtAppName.text.trim() : appList.model[appList.currentIndex]
                const url = txtWebsite.visible ? txtWebsite.text.trim() : ""

                // Panggil fungsi dengan parameter URL baru
                logger.addProductivityApp(appName, "", url, prodType)
                addAppDialog.close()
                if (typeof logger !== "undefined" && logger.notify) {
                    logger.notify("success", Lang.t("Application request submitted successfully and is under review."))
                }
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 15
        anchors.margins: 15

        // Left: App Selection List
        Rectangle {
            color: Theme.cardColor
            clip: true
            Layout.preferredWidth: parent.width * 0.45
            Layout.fillHeight: true
            radius: 4
            border.color: Theme.dividerColor
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                Label {
                    text: Lang.t("Application List:")
                    font.pixelSize: Theme.fontSizeBody
                    font.bold: true
                    color: Theme.textColor
                    padding: 10
                    Layout.fillWidth: true
                    background: Rectangle {
                        color: Qt.lighter(Theme.cardColor, 1.1)
                    }
                }

                TextField {
                    id: searchField
                    Layout.fillWidth: true
                    Layout.margins: 10
                    placeholderText: Lang.t("Search applications...")
                    font.pixelSize: Theme.fontSizeBody
                    background: Rectangle {
                        radius: 4
                        border.color: Theme.dividerColor
                        border.width: 1
                        color: Theme.cardColor
                        implicitHeight: 36
                    }
                    onTextChanged: {
                        appList.currentIndex = 0
                    }
                }

                ListView {
                    id: appList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: 4
                    model: {
                        let apps = logger.getAvailableApps()
                        return apps.filter(app => app.toLowerCase().includes(searchField.text.toLowerCase()))
                    }
                    currentIndex: 0
                    ScrollBar.vertical: CustomScrollBar {}

                    delegate: Rectangle {
                        width: appList.width
                        height: 50
                        color: Theme.cardColor

                        property bool hover: false
                        property bool isSelected: ListView.isCurrentItem

                        Rectangle {
                            width: 2
                            height: parent.height
                            color: hover ? Qt.lighter(Theme.primaryColor, 1.5) : "transparent"
                            visible: hover && !isSelected
                        }

                        Rectangle {
                            width: 2
                            height: parent.height
                            visible: isSelected
                            gradient: Gradient {
                                orientation: Gradient.Vertical
                                GradientStop { position: 0.0; color: Theme.primaryColor }
                                GradientStop { position: 1.0; color: Qt.darker(Theme.primaryColor, 1.5) }
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            onEntered: parent.hover = true
                            onExited: parent.hover = false
                            onClicked: {
                                appList.currentIndex = index
                            }
                        }

                        Row {
                            spacing: 12
                            anchors.fill: parent
                            anchors.verticalCenter: parent.verticalCenter
                            leftPadding: 10

                            Rectangle {
                                width: 34
                                height: 34
                                radius: 8
                                color: Qt.alpha(Theme.primaryColor, 0.14)
                                anchors.verticalCenter: parent.verticalCenter

                                Image {
                                    id: reqAppIconImg
                                    anchors.centerIn: parent
                                    width: 22
                                    height: 22
                                    asynchronous: true
                                    fillMode: Image.PreserveAspectFit
                                    source: {
                                        var key = (modelData || "").toLowerCase().trim()
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
                                            "adobe": "adobe.com",
                                            "antigravity": "antigravity.google"
                                        }
                                        for (var k in map) {
                                            if (key.indexOf(k) !== -1) {
                                                return "https://www.google.com/s2/favicons?domain=" + map[k] + "&sz=64"
                                            }
                                        }
                                        return ""
                                    }
                                    visible: status === Image.Ready
                                }

                                Text {
                                    visible: reqAppIconImg.status !== Image.Ready
                                    text: modelData.charAt(0).toUpperCase()
                                    font.bold: true
                                    font.pixelSize: Theme.fontSizeBody
                                    color: Theme.primaryColor
                                    anchors.centerIn: parent
                                }
                            }

                            Column {
                                width: parent.width - 60
                                spacing: 2
                                anchors.verticalCenter: parent.verticalCenter

                                Text {
                                    text: modelData
                                    font.bold: true
                                    font.pixelSize: Theme.fontSizeSmall
                                    color: hover || isSelected ? (hover ? Theme.primaryColor : Theme.textColor) : Theme.textColor
                                    width: parent.width
                                    elide: Text.ElideRight
                                }

                                Text {
                                    text: {
                                        if (modelData === "Other") return Lang.t("Other Application")
                                        if (["Chrome", "Firefox", "Edge", "Safari", "Opera"].includes(modelData))
                                             return Lang.t("Web Browser")
                                        return Lang.t("Application")
                                    }
                                    font.pixelSize: 10
                                    color: hover || isSelected ? (hover ? Theme.primaryColor : Theme.lightTextColor) : Theme.lightTextColor
                                    width: parent.width
                                    elide: Text.ElideRight
                                }
                            }
                        }
                    }
                }
            }
        }

        // Divider
        Rectangle {
            width: 1
            Layout.fillHeight: true
            color: Theme.dividerColor
        }

        // Right: Inputs and Controls
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 15
            Layout.leftMargin: 10
            Layout.rightMargin: 5

            // Nama Aplikasi Custom
            Label {
                text: Lang.t("Application Name:")
                font.pixelSize: Theme.fontSizeBody
                color: Theme.textColor
                Layout.fillWidth: true
                visible: txtAppName.visible
            }

            TextField {
                id: txtAppName
                Layout.fillWidth: true
                placeholderText: Lang.t("Enter application name, make sure the name is correct")
                font.pixelSize: Theme.fontSizeBody
                color: Theme.textColor
                placeholderTextColor: Theme.mutedTextColor
                leftPadding: 12
                rightPadding: 12
                visible: appList.currentIndex >= 0 && appList.model && appList.model[appList.currentIndex] === "Other"
                background: Rectangle {
                    radius: Theme.radiusSmall
                    border.color: txtAppName.activeFocus ? Theme.primaryColor : Theme.borderColor
                    border.width: txtAppName.activeFocus ? 2 : 1
                    color: Theme.surfaceColor
                    implicitHeight: 40
                }
            }

            // URL/Domain
            Label {
                text: Lang.t("Website Domain:")
                font.pixelSize: Theme.fontSizeBody
                color: Theme.textColor
                Layout.fillWidth: true
                visible: txtWebsite.visible
            }

            TextField {
                id: txtWebsite
                Layout.fillWidth: true
                placeholderText: Lang.t("Enter website domain (example: youtube.com, google.com, etc)")
                font.pixelSize: Theme.fontSizeBody
                color: Theme.textColor
                placeholderTextColor: Theme.mutedTextColor
                leftPadding: 12
                rightPadding: 12
                visible: appList.currentIndex >= 0 && appList.model && ["Chrome", "Firefox", "Edge", "Safari", "Opera"].indexOf(appList.model[appList.currentIndex]) !== -1
                background: Rectangle {
                    radius: Theme.radiusSmall
                    border.color: txtWebsite.activeFocus ? Theme.primaryColor : Theme.borderColor
                    border.width: txtWebsite.activeFocus ? 2 : 1
                    color: Theme.surfaceColor
                    implicitHeight: 40
                }
                validator: RegularExpressionValidator {
                    regularExpression: /^(https?:\/\/)?([\da-z\.-]+)\.([a-z\.]{2,6})([\/\w \.-]*)*\/?$/
                }
            }

            Rectangle {
                Layout.fillWidth: true
                color: Theme.dividerColor
                Layout.topMargin: 4
            }

            // Tipe Produktivitas
            ColumnLayout {
                spacing: 16
                Layout.fillWidth: true

                // Header with improved typography
                Label {
                    id: headerLabel
                    text: Lang.t("Productivity Type")
                    font.family: "Segoe UI, Arial, sans-serif"
                    font.pixelSize: 16
                    font.weight: Font.DemiBold
                    color: Theme.textColor
                    leftPadding: 4

                    Behavior on color {
                        ColorAnimation { duration: 300 }
                    }
                }

                // Radio button group with modern styling
                ColumnLayout {
                    spacing: 12
                    Layout.leftMargin: 8
                    Layout.fillWidth: true

                    // Produktif option
                    RadioButton {
                        id: produktifRadio
                        text: Lang.t("Productive")
                        checked: true
                        font.family: "Segoe UI, Arial, sans-serif"
                        font.pixelSize: 14
                        Layout.fillWidth: true

                        onCheckedChanged: if (checked) addAppDialog.selectedProductivityType = 1

                        contentItem: Text {
                            text: produktifRadio.text
                            font: produktifRadio.font
                            color: Theme.textColor
                            verticalAlignment: Text.AlignVCenter
                            leftPadding: produktifRadio.indicator.width + produktifRadio.spacing

                            Behavior on color {
                                ColorAnimation { duration: 300 }
                            }
                        }

                        indicator: Rectangle {
                            implicitWidth: 22
                            implicitHeight: 22
                            x: 0
                            y: (produktifRadio.height - height) / 2
                            radius: 11
                            color: "transparent"
                            border.color: produktifRadio.checked ? Theme.accentColor : Theme.borderColor
                            border.width: produktifRadio.checked ? 2 : 1

                            Behavior on border.color {
                                ColorAnimation { duration: 200 }
                            }

                            Behavior on border.width {
                                NumberAnimation { duration: 200 }
                            }

                            Rectangle {
                                anchors.centerIn: parent
                                width: 10
                                height: 10
                                radius: 5
                                color: Theme.accentColor
                                visible: produktifRadio.checked

                                // Scale animation for dot
                                scale: produktifRadio.checked ? 1.0 : 0.0
                                Behavior on scale {
                                    NumberAnimation {
                                        duration: 200
                                        easing.type: Easing.OutBack
                                        easing.overshoot: 0.3
                                    }
                                }
                            }

                            // Ripple effect on click
                            Rectangle {
                                id: produktifRipple
                                anchors.centerIn: parent
                                width: 0
                                height: 0
                                radius: width / 2
                                color: Qt.alpha(Theme.accentColor, 0.3)
                                visible: false

                                PropertyAnimation {
                                    id: produktifRippleAnim
                                    target: produktifRipple
                                    properties: "width,height"
                                    to: 40
                                    duration: 300
                                    onStarted: produktifRipple.visible = true
                                    onFinished: {
                                        produktifRipple.width = 0
                                        produktifRipple.height = 0
                                        produktifRipple.visible = false
                                    }
                                }
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            onPressed: produktifRippleAnim.start()
                            onClicked: produktifRadio.checked = true
                        }
                    }

                    // Non-Produktif option
                    RadioButton {
                        id: nonProduktifRadio
                        text: Lang.t("Non-Productive")
                        font.family: "Segoe UI, Arial, sans-serif"
                        font.pixelSize: Theme.fontSizeBody
                        Layout.fillWidth: true

                        onCheckedChanged: if (checked) addAppDialog.selectedProductivityType = 2

                        contentItem: Text {
                            text: nonProduktifRadio.text
                            font: nonProduktifRadio.font
                            color: Theme.textColor
                            verticalAlignment: Text.AlignVCenter
                            leftPadding: nonProduktifRadio.indicator.width + nonProduktifRadio.spacing

                            Behavior on color {
                                ColorAnimation { duration: 300 }
                            }
                        }

                        indicator: Rectangle {
                            implicitWidth: 22
                            implicitHeight: 22
                            x: 0
                            y: (nonProduktifRadio.height - height) / 2
                            radius: 11
                            color: "transparent"
                            border.color: nonProduktifRadio.checked ? Theme.accentColor : Theme.borderColor
                            border.width: nonProduktifRadio.checked ? 2 : 1

                            Behavior on border.color {
                                ColorAnimation { duration: 200 }
                            }

                            Rectangle {
                                anchors.centerIn: parent
                                width: 10
                                height: 10
                                radius: 5
                                color: Theme.accentColor
                                visible: nonProduktifRadio.checked

                                scale: nonProduktifRadio.checked ? 1.0 : 0.0
                                Behavior on scale {
                                    NumberAnimation {
                                        duration: 200
                                        easing.type: Easing.OutBack
                                        easing.overshoot: 0.3
                                    }
                                }
                            }
                        }

                        Rectangle {
                            width: 16
                            height: 16
                            radius: 3
                            color: Theme.dangerColor
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.indicator.verticalCenter
                            anchors.rightMargin: 8
                            visible: nonProduktifRadio.checked

                            Text {
                                anchors.centerIn: parent
                                text: "×"
                                color: "white"
                                font.pixelSize: 12
                                font.bold: true
                            }

                            scale: nonProduktifRadio.checked ? 1.0 : 0.0
                            Behavior on scale {
                                NumberAnimation {
                                    duration: 300
                                    easing.type: Easing.OutBack
                                }
                            }
                        }
                    }

                    // Netral option
                    RadioButton {
                        id: netralRadio
                        text: Lang.t("Neutral")
                        font.family: "Segoe UI, Arial, sans-serif"
                        font.pixelSize: Theme.fontSizeBody
                        Layout.fillWidth: true

                        onCheckedChanged: if (checked) addAppDialog.selectedProductivityType = 0

                        contentItem: Text {
                            text: netralRadio.text
                            font: netralRadio.font
                            color: Theme.textColor
                            verticalAlignment: Text.AlignVCenter
                            leftPadding: netralRadio.indicator.width + netralRadio.spacing

                            Behavior on color {
                                ColorAnimation { duration: 300 }
                            }
                        }

                        indicator: Rectangle {
                            implicitWidth: 22
                            implicitHeight: 22
                            x: 0
                            y: (netralRadio.height - height) / 2
                            radius: 11
                            color: "transparent"
                            border.color: netralRadio.checked ? Theme.accentColor : Theme.borderColor
                            border.width: netralRadio.checked ? 2 : 1

                            Behavior on border.color {
                                ColorAnimation { duration: 200 }
                            }

                            Rectangle {
                                anchors.centerIn: parent
                                width: 10
                                height: 10
                                radius: 5
                                color: Theme.accentColor
                                visible: netralRadio.checked

                                scale: netralRadio.checked ? 1.0 : 0.0
                                Behavior on scale {
                                    NumberAnimation {
                                        duration: 200
                                        easing.type: Easing.OutBack
                                        easing.overshoot: 0.3
                                    }
                                }
                            }
                        }

                        Rectangle {
                            width: 16
                            height: 16
                            radius: 3
                            color: Theme.neutralColor
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.indicator.verticalCenter
                            anchors.rightMargin: 8
                            visible: netralRadio.checked

                            Text {
                                anchors.centerIn: parent
                                text: "–"
                                color: "white"
                                font.pixelSize: 12
                                font.bold: true
                            }

                            scale: netralRadio.checked ? 1.0 : 0.0
                            Behavior on scale {
                                NumberAnimation {
                                    duration: 300
                                    easing.type: Easing.OutBack
                                }
                            }
                        }
                    }
                }
            }

            Item {
                Layout.fillHeight: true
            }
        }

        Connections {
            target: appList
            function onCurrentIndexChanged() {
                if (appList.currentItem) {
                    const appName = appList.model[appList.currentIndex]
                    txtWebsite.visible = ["Chrome", "Firefox", "Edge", "Safari", "Opera"].includes(appName)
                    txtAppName.visible = appName === "Other"
                }
            }
        }
    }
}
