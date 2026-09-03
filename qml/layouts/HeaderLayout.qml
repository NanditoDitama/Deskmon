import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"
import "../dialogs"

Rectangle {
    id: root
    Layout.fillWidth: true
    height: 60
    color: Theme.headers
    border.color: Theme.dividerColor
    border.width: 1
    bottomLeftRadius: Theme.radiusMedium
    bottomRightRadius: Theme.radiusMedium

    property string username: ""

    signal profileClicked()
    signal logoutClicked()
    signal refreshClicked()

    RowLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 16

        Label {
            text: "Deskmon"
            font { bold: true; pixelSize: 20; family: "Segoe UI" }
            color: "white"
        }

        Label {
            text: root.username
            font.pixelSize: Theme.fontSizeBody
            color: "white"
            opacity: 0.8
        }

        Item { Layout.fillWidth: true }

        Row {
            id: headerButtonsRow
            spacing: 12
            layoutDirection: Qt.RightToLeft

            // Settings button with Dropdown
            RoundButton {
                id: settingsBtn
                width: 40
                height: 40
                radius: 20
                hoverEnabled: true
                background: Rectangle {
                    radius: 20
                    color: parent.hovered || settingsMenu.visible ? Qt.rgba(1, 1, 1, 0.2) : "transparent"
                }

                contentItem: Image {
                    source: "qrc:/icons/settings.svg"
                    sourceSize.width: 22
                    sourceSize.height: 22
                    anchors.centerIn: parent
                    opacity: 0.95
                }

                onClicked: {
                    settingsRotationAnim.start()
                    if (settingsMenu.visible) {
                        settingsMenu.close()
                    } else {
                        settingsMenu.open()
                    }
                }

                RotationAnimation {
                    id: settingsRotationAnim
                    target: settingsBtn.contentItem
                    from: 0
                    to: 90
                    duration: 250
                    easing.type: Easing.OutCubic
                }

                ToolTip.text: Lang.t("Settings")
                ToolTip.visible: hovered && !settingsMenu.visible
                ToolTip.delay: 500

                Menu {
                    id: settingsMenu
                    y: settingsBtn.height + 8
                    x: -width + settingsBtn.width
                    width: 210
                    transformOrigin: Item.TopRight

                    background: Rectangle {
                        implicitWidth: 210
                        color: Theme.cardColor
                        radius: Theme.radiusMedium
                        border.color: Theme.dividerColor
                        border.width: 1
                    }

                    // --- Profile Item ---
                    MenuItem {
                        id: profileMenuItem
                        height: 40
                        background: Rectangle {
                            color: profileMenuItem.hovered ? (Theme.isDarkMode ? Qt.rgba(1,1,1,0.08) : Qt.rgba(0,0,0,0.05)) : "transparent"
                            radius: Theme.radiusSmall
                        }
                        contentItem: RowLayout {
                            spacing: 12
                            anchors.fill: parent
                            anchors.leftMargin: 14
                            anchors.rightMargin: 14

                            Image {
                                source: "qrc:/icons/edit.svg"
                                sourceSize.width: 16
                                sourceSize.height: 16
                                opacity: 0.8
                            }
                            Text {
                                text: Lang.t("My Profile")
                                font.pixelSize: Theme.fontSizeSmall
                                font.weight: Font.Medium
                                color: Theme.textColor
                                Layout.fillWidth: true
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        onTriggered: root.profileClicked()
                    }

                    MenuSeparator {
                        contentItem: Rectangle {
                            implicitHeight: 1
                            color: Theme.dividerColor
                        }
                    }

                    // --- Appearance Mode Item ---
                    MenuItem {
                        id: modeMenuItem
                        height: 40
                        background: Rectangle {
                            color: modeMenuItem.hovered ? (Theme.isDarkMode ? Qt.rgba(1,1,1,0.08) : Qt.rgba(0,0,0,0.05)) : "transparent"
                            radius: Theme.radiusSmall
                        }
                        contentItem: RowLayout {
                            spacing: 12
                            anchors.fill: parent
                            anchors.leftMargin: 14
                            anchors.rightMargin: 14

                            Image {
                                source: Theme.isDarkMode ? "qrc:/icons/light_mode.svg" : "qrc:/icons/dark_mode.svg"
                                sourceSize.width: 16
                                sourceSize.height: 16
                                opacity: 0.8
                            }
                            Text {
                                text: Theme.isDarkMode ? Lang.t("Light Mode") : Lang.t("Dark Mode")
                                font.pixelSize: Theme.fontSizeSmall
                                font.weight: Font.Medium
                                color: Theme.textColor
                                Layout.fillWidth: true
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        onTriggered: {
                            Theme.isDarkMode = !Theme.isDarkMode
                        }
                    }

                    MenuSeparator {
                        contentItem: Rectangle {
                            implicitHeight: 1
                            color: Theme.dividerColor
                        }
                    }

                    // --- Language Section Header ---
                    Item {
                        height: 26
                        width: parent.width

                        Text {
                            anchors.left: parent.left
                            anchors.leftMargin: 14
                            anchors.verticalCenter: parent.verticalCenter
                            text: Lang.t("Language").toUpperCase()
                            font.pixelSize: 10
                            font.bold: true
                            font.letterSpacing: 0.5
                            color: Theme.lightTextColor
                        }
                    }

                    // --- Language: English ---
                    MenuItem {
                        id: enItem
                        height: 38
                        background: Rectangle {
                            color: enItem.hovered ? (Theme.isDarkMode ? Qt.rgba(1,1,1,0.08) : Qt.rgba(0,0,0,0.05)) : "transparent"
                            radius: Theme.radiusSmall
                        }
                        contentItem: RowLayout {
                            spacing: 12
                            anchors.fill: parent
                            anchors.leftMargin: 14
                            anchors.rightMargin: 14

                            Text {
                                text: "🇺🇸"
                                font.pixelSize: 14
                            }
                            Text {
                                text: "English"
                                font.pixelSize: Theme.fontSizeSmall
                                font.weight: Lang.isEnglish ? Font.Bold : Font.Normal
                                color: Lang.isEnglish ? Theme.primaryColor : Theme.textColor
                                Layout.fillWidth: true
                                verticalAlignment: Text.AlignVCenter
                            }
                            Image {
                                source: "qrc:/icons/check.svg"
                                sourceSize.width: 14
                                sourceSize.height: 14
                                visible: Lang.isEnglish
                            }
                        }
                        onTriggered: Lang.setLanguage("en")
                    }

                    // --- Language: Bahasa Indonesia ---
                    MenuItem {
                        id: idItem
                        height: 38
                        background: Rectangle {
                            color: idItem.hovered ? (Theme.isDarkMode ? Qt.rgba(1,1,1,0.08) : Qt.rgba(0,0,0,0.05)) : "transparent"
                            radius: Theme.radiusSmall
                        }
                        contentItem: RowLayout {
                            spacing: 12
                            anchors.fill: parent
                            anchors.leftMargin: 14
                            anchors.rightMargin: 14

                            Text {
                                text: "🇮🇩"
                                font.pixelSize: 14
                            }
                            Text {
                                text: "Bahasa Indonesia"
                                font.pixelSize: Theme.fontSizeSmall
                                font.weight: Lang.isIndonesian ? Font.Bold : Font.Normal
                                color: Lang.isIndonesian ? Theme.primaryColor : Theme.textColor
                                Layout.fillWidth: true
                                verticalAlignment: Text.AlignVCenter
                            }
                            Image {
                                source: "qrc:/icons/check.svg"
                                sourceSize.width: 14
                                sourceSize.height: 14
                                visible: Lang.isIndonesian
                            }
                        }
                        onTriggered: Lang.setLanguage("id")
                    }
                }
            }

            // Dark mode toggle button
            RoundButton {
                id: themeToggle
                width: 40
                height: 40
                radius: 20
                hoverEnabled: true
                background: Rectangle {
                    radius: 20
                    color: parent.hovered ? Qt.rgba(1, 1, 1, 0.2) : "transparent"
                }

                contentItem: Image {
                    source: Theme.isDarkMode ? "qrc:/icons/light_mode.svg" : "qrc:/icons/dark_mode.svg"
                    sourceSize.width: 24
                    sourceSize.height: 24
                    anchors.centerIn: parent
                    opacity: 0.9
                }

                onClicked: {
                    Theme.isDarkMode = !Theme.isDarkMode
                    rotationAnim.start()
                }

                RotationAnimation {
                    id: rotationAnim
                    target: themeToggle.contentItem
                    from: 0
                    to: 360
                    duration: 600
                    easing.type: Easing.OutBack
                }

                ToolTip.text: Theme.isDarkMode ? Lang.t("Switch to Light Mode") : Lang.t("Switch to Dark Mode")
                ToolTip.visible: hovered
                ToolTip.delay: 500
            }

            // Refresh button
            RoundButton {
                id: refreshBtn
                width: 40
                height: 40
                radius: 20
                hoverEnabled: true
                background: Rectangle {
                    radius: 20
                    color: parent.hovered ? Qt.rgba(1, 1, 1, 0.2) : "transparent"
                }

                contentItem: Image {
                    source: "qrc:/icons/refresh.svg"
                    sourceSize.width: 24
                    sourceSize.height: 24
                    anchors.centerIn: parent
                    opacity: 0.9
                }

                onClicked: {
                    root.refreshClicked()
                    if (typeof logger !== "undefined") {
                        logger.refreshTasks()
                        logger.fetchAndStoreTasks()
                        logger.fetchAndStoreProductivityApps()
                    }
                    console.log("Refresh button clicked")
                    refreshRotationAnimation.start()
                }

                RotationAnimation {
                    id: refreshRotationAnimation
                    target: refreshBtn.contentItem
                    from: 0
                    to: 360
                    duration: 600
                    easing.type: Easing.OutBack
                }

                ToolTip.text: Lang.t("Refresh")
                ToolTip.visible: hovered
                ToolTip.delay: 500
            }

            Shortcut {
                sequences: ["Ctrl+R"]
                enabled: refreshBtn.enabled
                onActivated: {
                    if (Qt.platform.os === "windows" || Qt.platform.os === "linux" || Qt.platform.os === "osx") {
                        refreshBtn.clicked()
                    }
                }
            }

            // Profile button
            Button {
                id: profileBtn
                text: Lang.t("Profile")
                height: 40
                padding: 12
                font {
                    family: "Segoe UI"
                    pixelSize: 14
                    weight: Font.Medium
                }
                background: Rectangle {
                    radius: Theme.radiusSmall
                    color: parent.hovered ? Qt.rgba(1, 1, 1, 0.2) : "transparent"
                    border.color: Qt.rgba(1, 1, 1, 0.3)
                    border.width: 1
                }
                contentItem: Text {
                    text: profileBtn.text
                    font: profileBtn.font
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: root.profileClicked()
            }

            // Logout button
            Button {
                id: logoutBtn
                text: Lang.t("Logout")
                height: 40
                padding: 12
                font {
                    family: "Segoe UI"
                    pixelSize: 14
                    weight: Font.Medium
                }
                background: Rectangle {
                    radius: Theme.radiusSmall
                    color: parent.hovered ? Qt.rgba(1, 1, 1, 0.2) : "transparent"
                    border.color: Qt.rgba(1, 1, 1, 0.3)
                    border.width: 1
                }
                contentItem: Text {
                    text: logoutBtn.text
                    font: logoutBtn.font
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: root.logoutClicked()
            }

            // Cek Update button
            Button {
                id: updateBtn
                text: Lang.t("Check Update")
                height: 40
                padding: 12
                font { family: "Segoe UI"; pixelSize: 14; weight: Font.Medium }
                background: Rectangle {
                    radius: Theme.radiusSmall
                    color: parent.hovered ? Qt.rgba(1, 1, 1, 0.2) : "transparent"
                    border.color: Qt.rgba(1, 1, 1, 0.3)
                    border.width: 1
                }
                contentItem: Text {
                    text: parent.text
                    font: parent.font
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: {
                    if (typeof logger !== "undefined")
                        logger.checkForUpdates()
                }
            }

            // Website link button
            Button {
                id: deskmonButton
                width: 40
                height: 40
                background: Rectangle {
                    radius: Theme.radiusSmall
                    color: parent.hovered ? Qt.rgba(1, 1, 1, 0.2) : "transparent"
                    border.color: Qt.rgba(1, 1, 1, 0.3)
                    border.width: 1
                }

                contentItem: Item {
                    anchors.fill: parent
                    Image {
                        source: "qrc:/icons/website.svg"
                        sourceSize.width: 20
                        sourceSize.height: 20
                        anchors.centerIn: parent
                    }
                }

                onClicked: Qt.openUrlExternally("https://deskmon.pranala-dt.co.id/")
                ToolTip.text: "Deskmon Website"
                ToolTip.visible: hovered
                ToolTip.delay: 500
            }
        }
    }

    // Status Message Label
    Label {
        id: statusLabel
        anchors.centerIn: parent
        padding: 12
        text: "Ini adalah pesan status"
        visible: false
        z: 9999

        background: Rectangle {
            color: Qt.rgba(0, 0, 0, 0.7)
            radius: Theme.radiusSmall
        }

        font.pixelSize: 16
        color: Theme.primaryColor

        opacity: 0
        Behavior on opacity { NumberAnimation { duration: 300 } }
    }

    Timer {
        id: statusTimer
        interval: 3000
        repeat: false
        onTriggered: {
            statusLabel.opacity = 0
        }
    }

    UpdateDialog {
        id: updateDialog
        onUpdateAccepted: {
            if (typeof logger !== "undefined")
                logger.launchMaintenanceTool()
        }
    }

    Connections {
        target: (typeof logger !== "undefined") ? logger : null

        function onUpdateAvailable(version, releaseNotes, downloadUrl) {
            console.log("QML: Update tersedia!", version)
            updateDialog.openWith(version, releaseNotes)
        }

        function onShowStatusMessage(message) {
            statusLabel.text = message
            statusLabel.visible = true
            statusLabel.opacity = 1
            statusTimer.restart()
        }
    }
}
