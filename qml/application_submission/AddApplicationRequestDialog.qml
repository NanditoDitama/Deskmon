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
    id: addAppDialog
    dialogTitle: Lang.t("Add Productivity Application")
    property int selectedProductivityType: 1
    property var availableApps: (typeof logger !== "undefined" && logger) ? logger.getAvailableApps() : []
    signal cancelRequested()
    width: parent ? Math.min(parent.width * 0.8, 1000) : 800
    height: parent ? Math.min(parent.height * 0.8, 600) : 600
    padding: 0

    onOpened: {
        if (typeof logger !== "undefined" && logger) {
            availableApps = logger.getAvailableApps()
        }
        searchField.text = ""
        appList.currentIndex = 0
    }

    footer: DialogButtonBox {
        alignment: Qt.AlignRight
        leftPadding: Theme.dialogPadding
        rightPadding: Theme.dialogPadding
        topPadding: 12
        bottomPadding: 12
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
            text: Lang.t("Cancel")
            btnColor: Theme.cardHeaderColor
            btnHoverColor: Qt.alpha(Theme.textColor, 0.06)
            btnTextColor: Theme.textColor
            btnBorderColor: Theme.borderColor
            btnBorderWidth: 1
            font.pixelSize: Theme.fontSizeSmall
            font.weight: Font.Medium
            onClicked: {
                addAppDialog.close()
                addAppDialog.cancelRequested()
            }
        }
        CustomButton {
            id: addButton
            text: Lang.t("Add")
            btnColor: addButton.enabled ? Theme.primaryColor : Theme.dividerColor
            btnHoverColor: Theme.primaryHoverColor
            btnTextColor: addButton.enabled ? "#ffffff" : Theme.mutedTextColor
            font.pixelSize: Theme.fontSizeSmall
            enabled: {
                if (appList.currentItem && appList.model[appList.currentIndex] === "Other") {
                    return txtAppName.text.trim().length > 0
                }
                if (txtWebsite.visible) {
                    return txtWebsite.text.trim().length > 0
                }
                return appList.currentItem !== null
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
        spacing: Theme.spacingLarge
        anchors.margins: Theme.dialogPadding

        // Left: App Selection List
        Rectangle {
            color: Theme.cardColor
            clip: true
            Layout.preferredWidth: parent.width * 0.45
            Layout.fillHeight: true
            radius: Theme.radiusMedium
            border.color: Theme.dividerColor
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                Label {
                    text: Lang.t("Application List:")
                    font.pixelSize: Theme.fontSizeBody
                    font.weight: Font.DemiBold
                    color: Theme.textColor
                    padding: 12
                    Layout.fillWidth: true
                    background: Rectangle {
                        color: Theme.cardHeaderColor
                        radius: Theme.radiusMedium
                    }
                }

                CustomTextField {
                    id: searchField
                    Layout.fillWidth: true
                    Layout.margins: 10
                    placeholderText: Lang.t("Search applications...")
                    leftIconSource: "image://icon/search.svg?" + Theme.lightTextColor
                    onTextChanged: {
                        appList.currentIndex = 0
                    }
                }

                ListView {
                    id: appList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.leftMargin: 10
                    Layout.rightMargin: 10
                    Layout.bottomMargin: 10
                    clip: true
                    spacing: 6
                    model: {
                        let apps = addAppDialog.availableApps || []
                        return apps.filter(app => app.toLowerCase().includes(searchField.text.toLowerCase()))
                    }
                    currentIndex: 0
                    ScrollBar.vertical: CustomScrollBar {}

                    delegate: CustomCard {
                        width: appList.width - 10
                        height: 54
                        cardRadius: Theme.radiusSmall
                        cardColor: isSelected
                                   ? Qt.alpha(Theme.primaryColor, 0.12)
                                   : (hover ? Qt.alpha(Theme.primaryColor, 0.06) : Theme.cardHeaderColor)
                        cardBorderColor: isSelected
                                         ? Theme.primaryColor
                                         : (hover ? Theme.borderColor : Theme.dividerColor)

                        property bool hover: false
                        property bool isSelected: ListView.isCurrentItem

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
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12

                            AppIconBadge {
                                appName: modelData || ""
                                accentColor: Theme.primaryColor
                                anchors.verticalCenter: parent.verticalCenter
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

            CustomTextField {
                id: txtAppName
                Layout.fillWidth: true
                placeholderText: Lang.t("Enter application name, make sure the name is correct")
                visible: appList.currentIndex >= 0 && appList.model && appList.model[appList.currentIndex] === "Other"
            }

            // URL/Domain
            Label {
                text: Lang.t("Website Domain:")
                font.pixelSize: Theme.fontSizeBody
                color: Theme.textColor
                Layout.fillWidth: true
                visible: txtWebsite.visible
            }

            CustomTextField {
                id: txtWebsite
                Layout.fillWidth: true
                placeholderText: Lang.t("Enter website domain (example: youtube.com, google.com, etc)")
                visible: appList.currentIndex >= 0 && appList.model && ["Chrome", "Firefox", "Edge", "Safari", "Opera"].indexOf(appList.model[appList.currentIndex]) !== -1
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
                            border.color: produktifRadio.checked ? Theme.primaryColor : Theme.borderColor
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
                                color: Theme.primaryColor
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
                                color: Qt.alpha(Theme.primaryColor, 0.3)
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
                            border.color: nonProduktifRadio.checked ? Theme.primaryColor : Theme.borderColor
                            border.width: nonProduktifRadio.checked ? 2 : 1

                            Behavior on border.color {
                                ColorAnimation { duration: 200 }
                            }

                            Rectangle {
                                anchors.centerIn: parent
                                width: 10
                                height: 10
                                radius: 5
                                color: Theme.primaryColor
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
                            border.color: netralRadio.checked ? Theme.primaryColor : Theme.borderColor
                            border.width: netralRadio.checked ? 2 : 1

                            Behavior on border.color {
                                ColorAnimation { duration: 200 }
                            }

                            Rectangle {
                                anchors.centerIn: parent
                                width: 10
                                height: 10
                                radius: 5
                                color: Theme.primaryColor
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
