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

                ToolTip.text: Theme.isDarkMode ? "Switch to Light Mode" : "Switch to Dark Mode"
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

                ToolTip.text: "Refresh"
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
                text: "Profile"
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
                text: "Logout"
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
                text: "Cek Update"
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
