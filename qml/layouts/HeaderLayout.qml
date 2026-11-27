import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import "../theme"
import "../dialogs"
import "../components"

Rectangle {
    id: root
    Layout.fillWidth: true
    Layout.preferredHeight: 52
    Layout.minimumHeight: 52
    Layout.maximumHeight: 52
    height: 52
    color: Theme.headers
    border.color: Theme.dividerColor
    border.width: 1
    bottomLeftRadius: Theme.radiusMedium
    bottomRightRadius: Theme.radiusMedium

    property string username: ""
    readonly property bool isCompact: Window.window ? Window.window.width < 1060 : false

    signal profileClicked()
    signal logoutClicked()
    signal refreshClicked()

    // Drag & double-click empty header area to move or maximize/restore the window
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        onPressed: function(mouse) {
            if (Window.window && mouse.button === Qt.LeftButton)
                Window.window.startSystemMove()
        }
        onDoubleClicked: {
            var win = Window.window
            if (!win)
                return
            if (win.visibility === Window.Maximized)
                win.showNormal()
            else
                win.showMaximized()
        }
    }

    // True horizontal center across the screen, clamped between leftBrandRow and rightControlsRow
    Label {
        id: centeredUsernameLabel
        visible: root.username.length > 0 && maxAvailWidth > 20
        readonly property real leftBound: leftBrandRow.width + 24
        readonly property real rightBound: root.width - rightControlsRow.width - 22
        readonly property real maxAvailWidth: Math.max(0, rightBound - leftBound)
        width: Math.min(implicitWidth, maxAvailWidth)
        x: Math.max(leftBound, Math.min((root.width - width) / 2, rightBound - width))
        anchors.verticalCenter: parent.verticalCenter
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        text: root.username
        font.pixelSize: Theme.fontSizeBody
        font.weight: Font.Medium
        font.family: "Segoe UI"
        color: Theme.lightTextColor
        elide: Text.ElideRight
    }

    RowLayout {
        id: headerMainRow
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 10
        spacing: 10

        RowLayout {
            id: leftBrandRow
            spacing: 7
            Layout.alignment: Qt.AlignVCenter

            // Reserve space on macOS for native traffic-light window controls
            Item {
                visible: Qt.platform.os === "osx"
                Layout.preferredWidth: Qt.platform.os === "osx" ? 68 : 0
            }

            Image {
                source: "qrc:/icon.ico"
                Layout.preferredWidth: 18
                Layout.preferredHeight: 18
                fillMode: Image.PreserveAspectFit
                smooth: true
                mipmap: true
                Layout.alignment: Qt.AlignVCenter
            }

            Label {
                text: "Deskmon"
                font { bold: true; pixelSize: 16; family: "Segoe UI" }
                color: Theme.textColor
                Layout.alignment: Qt.AlignBaseline
            }

            Label {
                readonly property string appVer: (typeof logger !== "undefined" && logger.appVersion)
                                                 ? logger.appVersion
                                                 : (Qt.application.version || "")
                visible: appVer.length > 0
                text: "v" + appVer
                font.pixelSize: 11
                font.weight: Font.Medium
                font.family: "Segoe UI"
                color: Theme.mutedTextColor
                Layout.alignment: Qt.AlignBaseline
            }
        }

        Item { Layout.fillWidth: true }

        RowLayout {
            id: rightControlsRow
            spacing: 8
            Layout.alignment: Qt.AlignVCenter

            Row {
                id: headerButtonsRow
                spacing: 8
                layoutDirection: Qt.RightToLeft

            // Settings button with Dropdown
            Rectangle {
                id: settingsBtn
                anchors.verticalCenter: parent.verticalCenter
                width: Theme.controlHeight
                height: Theme.controlHeight
                radius: Theme.radiusSmall
                color: settingsArea.containsMouse || settingsMenu.visible
                       ? (Theme.isDarkMode ? Qt.rgba(1, 1, 1, 0.14) : Qt.rgba(0, 0, 0, 0.06))
                       : "transparent"

                Behavior on color { ColorAnimation { duration: 120 } }

                Image {
                    id: settingsIcon
                    source: "image://icon/settings.svg?" + Theme.textColor
                    width: 16
                    height: 16
                    sourceSize.width: 48
                    sourceSize.height: 48
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                    mipmap: true
                    anchors.centerIn: parent
                    opacity: 0.92
                }

                MouseArea {
                    id: settingsArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        settingsRotationAnim.start()
                        if (settingsMenu.visible) {
                            settingsMenu.close()
                        } else {
                            settingsMenu.open()
                        }
                    }
                }

                RotationAnimation {
                    id: settingsRotationAnim
                    target: settingsIcon
                    from: 0
                    to: 90
                    duration: 250
                    easing.type: Easing.OutCubic
                }

                CustomToolTip {
                    text: Lang.t("Settings")
                    shown: settingsArea.containsMouse && !settingsMenu.visible
                }

                Menu {
                    id: settingsMenu
                    y: settingsBtn.height + 8
                    x: -width + settingsBtn.width
                    width: 224
                    padding: 6
                    transformOrigin: Item.TopRight

                    background: Rectangle {
                        implicitWidth: 224
                        color: Theme.cardColor
                        radius: Theme.radiusMedium
                        border.color: Theme.borderColor
                        border.width: 1
                    }

                    // --- Appearance Mode Item ---
                    MenuItem {
                        id: modeMenuItem
                        height: 38
                        background: Rectangle {
                            color: modeMenuItem.hovered ? (Theme.isDarkMode ? Qt.rgba(1,1,1,0.08) : Qt.rgba(0,0,0,0.05)) : "transparent"
                            radius: Theme.radiusSmall
                        }
                        contentItem: RowLayout {
                            spacing: 10
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12

                            Image {
                                source: Theme.isDarkMode ? ("image://icon/light_mode.svg?" + Theme.textColor) : ("image://icon/dark_mode.svg?" + Theme.textColor)
                                Layout.preferredWidth: 16
                                Layout.preferredHeight: 16
                                Layout.maximumWidth: 16
                                Layout.maximumHeight: 16
                                sourceSize.width: 48
                                sourceSize.height: 48
                                fillMode: Image.PreserveAspectFit
                                smooth: true
                                mipmap: true
                                opacity: 0.85
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
                            Theme.toggleDarkMode()
                        }
                    }

                    MenuSeparator {
                        topPadding: 4
                        bottomPadding: 4
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
                                color: Theme.textColor
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
                                source: "image://icon/check.svg?" + Theme.primaryColor
                                Layout.preferredWidth: 14
                                Layout.preferredHeight: 14
                                Layout.maximumWidth: 14
                                Layout.maximumHeight: 14
                                sourceSize.width: 48
                                sourceSize.height: 48
                                fillMode: Image.PreserveAspectFit
                                smooth: true
                                mipmap: true
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
                                color: Theme.textColor
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
                                source: "image://icon/check.svg?" + Theme.primaryColor
                                Layout.preferredWidth: 14
                                Layout.preferredHeight: 14
                                Layout.maximumWidth: 14
                                Layout.maximumHeight: 14
                                sourceSize.width: 48
                                sourceSize.height: 48
                                fillMode: Image.PreserveAspectFit
                                smooth: true
                                mipmap: true
                                visible: Lang.isIndonesian
                            }
                        }
                        onTriggered: Lang.setLanguage("id")
                    }

                    MenuSeparator {
                        topPadding: 4
                        bottomPadding: 4
                        contentItem: Rectangle {
                            implicitHeight: 1
                            color: Theme.dividerColor
                        }
                    }

                    // --- Logout Item ---
                    MenuItem {
                        id: logoutMenuItem
                        height: 38
                        background: Rectangle {
                            color: logoutMenuItem.hovered ? Qt.alpha(Theme.dangerColor, 0.12) : "transparent"
                            radius: Theme.radiusSmall
                        }
                        contentItem: RowLayout {
                            spacing: 10
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12

                            Text {
                                text: Lang.t("Logout")
                                font.pixelSize: Theme.fontSizeSmall
                                font.weight: Font.Medium
                                color: Theme.dangerColor
                                Layout.fillWidth: true
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        onTriggered: root.logoutClicked()
                    }
                }
            }

            // Refresh button
            Rectangle {
                id: refreshBtn
                anchors.verticalCenter: parent.verticalCenter
                width: Theme.controlHeight
                height: Theme.controlHeight
                radius: Theme.radiusSmall
                color: refreshArea.containsMouse
                       ? (Theme.isDarkMode ? Qt.rgba(1, 1, 1, 0.14) : Qt.rgba(0, 0, 0, 0.06))
                       : "transparent"

                Behavior on color { ColorAnimation { duration: 120 } }

                function trigger() {
                    root.refreshClicked()
                    if (typeof logger !== "undefined") {
                        logger.refreshAll()
                    }
                    console.log("Refresh button clicked")
                    refreshRotationAnimation.start()
                }

                Image {
                    id: refreshIcon
                    source: "image://icon/refresh.svg?" + Theme.textColor
                    width: 18
                    height: 18
                    sourceSize.width: 48
                    sourceSize.height: 48
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                    mipmap: true
                    anchors.centerIn: parent
                    opacity: 0.92
                }

                MouseArea {
                    id: refreshArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: refreshBtn.trigger()
                }

                RotationAnimation {
                    id: refreshRotationAnimation
                    target: refreshIcon
                    from: 0
                    to: 360
                    duration: 600
                    easing.type: Easing.OutBack
                }

                CustomToolTip {
                    text: Lang.t("Refresh")
                    shown: refreshArea.containsMouse
                }
            }

            Shortcut {
                sequences: ["Ctrl+R"]
                enabled: refreshBtn.enabled
                onActivated: {
                    if (Qt.platform.os === "windows" || Qt.platform.os === "linux" || Qt.platform.os === "osx") {
                        refreshBtn.trigger()
                    }
                }
            }

            // Dynamic Auto-Update Pill Button (Check Update -> Updating XX% -> Restart to Update)
            Rectangle {
                id: updateBtn
                anchors.verticalCenter: parent.verticalCenter

                readonly property string updState: (typeof logger !== "undefined" && logger.updateState) ? logger.updateState : "idle"
                readonly property int updProgress: (typeof logger !== "undefined" && logger.updateProgress !== undefined) ? logger.updateProgress : 0
                readonly property bool isWorking: updState === "checking" || updState === "downloading" || updState === "verifying" || updState === "extracting"
                readonly property bool isReady: updState === "ready"
                readonly property bool isManual: updState === "manual"

                implicitHeight: Theme.controlHeight
                implicitWidth: Math.max(updateContentRow.implicitWidth + (root.isCompact ? 20 : 24), 96)
                height: Theme.controlHeight
                width: implicitWidth
                radius: Theme.radiusSmall
                clip: true

                color: isReady
                       ? (updateArea.containsMouse ? Qt.lighter(Theme.primaryColor, 1.12) : Theme.primaryColor)
                       : (updateArea.containsMouse
                          ? (Theme.isDarkMode ? Qt.rgba(1, 1, 1, 0.14) : Qt.rgba(0, 0, 0, 0.06))
                          : "transparent")
                border.color: isReady
                              ? Theme.primaryColor
                              : (isWorking
                                 ? Theme.primaryColor
                                 : (isManual
                                    ? Theme.accentColor
                                    : (Theme.isDarkMode ? Qt.rgba(1, 1, 1, 0.22) : Theme.borderColor)))
                border.width: 1

                Behavior on color { ColorAnimation { duration: 150 } }
                Behavior on border.color { ColorAnimation { duration: 150 } }
                Behavior on width { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }

                // Smooth progress fill layer while downloading/verifying/extracting
                Rectangle {
                    visible: updateBtn.isWorking && updateBtn.updProgress > 0
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    width: Math.round(parent.width * (Math.min(100, Math.max(0, updateBtn.updProgress)) / 100.0))
                    color: Qt.alpha(Theme.primaryColor, Theme.isDarkMode ? 0.28 : 0.18)

                    Behavior on width { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }
                }

                Row {
                    id: updateContentRow
                    anchors.centerIn: parent
                    spacing: 6

                    Rectangle {
                        id: updatePulseDot
                        visible: updateBtn.isWorking
                        width: 7
                        height: 7
                        radius: 3.5
                        color: Theme.primaryColor
                        anchors.verticalCenter: parent.verticalCenter

                        SequentialAnimation on opacity {
                            running: updatePulseDot.visible
                            loops: Animation.Infinite
                            NumberAnimation { from: 1.0; to: 0.3; duration: 600 }
                            NumberAnimation { from: 0.3; to: 1.0; duration: 600 }
                        }
                    }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: {
                            if (updateBtn.isReady) {
                                return Lang.t("Restart Application")
                            }
                            if (updateBtn.updState === "downloading" || updateBtn.updState === "verifying" || updateBtn.updState === "extracting") {
                                return Lang.t("Downloading") + " " + updateBtn.updProgress + "%"
                            }
                            if (updateBtn.updState === "checking") {
                                return Lang.t("Checking...")
                            }
                            if (updateBtn.isManual) {
                                return Lang.t("Update Available")
                            }
                            return Lang.t("Check Update")
                        }
                        color: updateBtn.isReady ? "#FFFFFF" : Theme.textColor
                        font.pixelSize: 12
                        font.family: "Segoe UI"
                        font.weight: updateBtn.isReady ? Font.DemiBold : Font.Medium
                    }
                }

                MouseArea {
                    id: updateArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (typeof logger === "undefined") {
                            return
                        }
                        if (updateBtn.isReady) {
                            logger.restartToApplyUpdate()
                        } else if (updateBtn.isWorking) {
                            // Keep progress silently inside the header button without blocking modal dialog
                            return
                        } else if (updateBtn.isManual) {
                            logger.openUpdateReleasePage()
                        } else {
                            logger.checkForUpdates()
                        }
                    }
                }

                CustomToolTip {
                    text: {
                        if (updateBtn.isReady) {
                            return Lang.t("Restart Application") + " (v" + (logger.availableUpdateVersion || "") + ")"
                        }
                        if (updateBtn.isWorking) {
                            return Lang.t("Downloading") + " " + updateBtn.updProgress + "%"
                        }
                        return Lang.t("Check Update")
                    }
                    shown: updateArea.containsMouse
                }
            }

            // Website link button
            Rectangle {
                id: deskmonButton
                anchors.verticalCenter: parent.verticalCenter
                width: Theme.controlHeight
                height: Theme.controlHeight
                radius: Theme.radiusSmall
                color: websiteArea.containsMouse
                       ? (Theme.isDarkMode ? Qt.rgba(1, 1, 1, 0.14) : Qt.rgba(0, 0, 0, 0.06))
                       : "transparent"
                border.color: Theme.isDarkMode ? Qt.rgba(1, 1, 1, 0.22) : Theme.borderColor
                border.width: 1

                Behavior on color { ColorAnimation { duration: 120 } }

                Image {
                    source: "image://icon/website.svg?" + Theme.textColor
                    width: 18
                    height: 18
                    sourceSize.width: 48
                    sourceSize.height: 48
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                    mipmap: true
                    anchors.centerIn: parent
                }

                MouseArea {
                    id: websiteArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: Qt.openUrlExternally("https://deskmon.pranala-dt.co.id/")
                }

                CustomToolTip {
                    text: Lang.t("Deskmon Website")
                    shown: websiteArea.containsMouse
                }
            }
            }

            Rectangle {
                visible: Qt.platform.os !== "osx"
                Layout.preferredWidth: 1
                Layout.preferredHeight: 20
                color: Theme.dividerColor
            }

            WindowControls {}
        }
    }

    // Status Message Label
    Label {
        id: statusLabel
        anchors.centerIn: parent
        padding: 12
        text: Lang.t("This is a status message")
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

    Connections {
        target: (typeof logger !== "undefined") ? logger : null

        function onShowStatusMessage(message) {
            statusLabel.text = message
            statusLabel.visible = true
            statusLabel.opacity = 1
            statusTimer.restart()
        }
    }
}
