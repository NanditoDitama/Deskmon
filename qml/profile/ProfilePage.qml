import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Effects
import "../theme"

Item {
    anchors.fill: parent
    Rectangle {
        anchors.fill: parent
        color: "transparent"

        // Background dengan gradient modern yang responsif terhadap tema
        Rectangle {
            anchors.fill: parent

            gradient: Gradient {
                GradientStop {
                    position: 0.0
                    color: Theme.isDarkMode ? "#1a1a2e" : "#667eea"
                }
                GradientStop {
                    position: 1.0
                    color: Theme.isDarkMode ? Theme.cardColor : "#764ba2"
                }
            }

            // Shooting Stars Animation
            Repeater {
                model: 5

                Item {
                    id: shootingStar
                    width: parent.width
                    height: parent.height

                    property real startX: 0
                    property real startY: 0
                    property real endX: 0
                    property real endY: 0

                    property real animationDelay: index * 2000 + Math.random() * 4000
                    property real starSize: 2 + Math.random() * 3
                    property bool isVisible: false

                    visible: shootingStar.isVisible

                    Item {
                        id: starTrail
                        width: 100
                        height: 6

                        x: starBody.x + (starBody.width / 1)
                        y: starBody.y + (starBody.height / 1) - (height / 1)

                        transformOrigin: Item.Left
                        rotation: 0
                        opacity: starBody.opacity * 0.8

                        Canvas {
                            id: trailCanvas
                            anchors.fill: parent

                            onPaint: {
                                var ctx = getContext("2d")
                                ctx.clearRect(0, 0, width, height)

                                var gradient = ctx.createLinearGradient(0, 0, width, 0)
                                gradient.addColorStop(0, "rgba(255, 255, 255, 0.9)")
                                gradient.addColorStop(0.2, "rgba(255, 255, 255, 0.5)")
                                gradient.addColorStop(1.0, "rgba(255, 255, 255, 0)")

                                ctx.fillStyle = gradient

                                var h = height
                                var mid = h / 2

                                ctx.beginPath()
                                ctx.moveTo(0, mid - 2)
                                ctx.lineTo(0, mid + 2)
                                ctx.lineTo(width, mid + 0.5)
                                ctx.lineTo(width, mid - 0.5)
                                ctx.closePath()
                                ctx.fill()
                            }

                            Connections {
                                target: starTrail
                                function onOpacityChanged() { trailCanvas.requestPaint() }
                            }
                        }
                    }

                    Rectangle {
                        id: starBody
                        width: shootingStar.starSize
                        height: shootingStar.starSize
                        radius: width / 2
                        color: "white"
                    }

                    SequentialAnimation {
                        loops: Animation.Infinite
                        running: true

                        PauseAnimation { duration: shootingStar.animationDelay }

                        ScriptAction {
                            script: {
                                shootingStar.startX = (parent.width * 0.2) + Math.random() * (parent.width * 1.1)
                                shootingStar.startY = -100 - Math.random() * 150

                                var travelDistance = 300 + Math.random() * 300

                                shootingStar.endX = shootingStar.startX - travelDistance
                                shootingStar.endY = shootingStar.startY + travelDistance

                                var dx = shootingStar.endX - shootingStar.startX
                                var dy = shootingStar.endY - shootingStar.startY
                                var angle = Math.atan2(dy, dx) * 180 / Math.PI
                                starTrail.rotation = angle + 180

                                starBody.x = shootingStar.startX
                                starBody.y = shootingStar.startY
                                starBody.opacity = 0
                                shootingStar.isVisible = true
                            }
                        }

                        ParallelAnimation {
                            NumberAnimation {
                                target: starBody; property: "x"
                                from: shootingStar.startX; to: shootingStar.endX
                                duration: 1200 + Math.random() * 400
                                easing.type: Easing.OutQuad
                            }
                            NumberAnimation {
                                target: starBody; property: "y"
                                from: shootingStar.startY; to: shootingStar.endY
                                duration: 1200 + Math.random() * 400
                                easing.type: Easing.OutQuad
                            }

                            SequentialAnimation {
                                NumberAnimation { target: starBody; property: "opacity"; from: 0; to: 1; duration: 200 }
                                PauseAnimation { duration: 200 }
                                NumberAnimation { target: starBody; property: "opacity"; from: 1; to: 0; duration: 500 }
                            }
                        }

                        ScriptAction { script: { shootingStar.isVisible = false } }
                        PauseAnimation { duration: 1500 + Math.random() * 3000 }
                    }
                }
            }

            // Static stars (Background)
            Repeater {
                model: 30
                Rectangle {
                    width: 1 + Math.random() * 2
                    height: width
                    radius: width / 2
                    color: Theme.isDarkMode ? "white" : Qt.rgba(1, 1, 1, 0.8)
                    x: Math.random() * parent.width
                    y: Math.random() * parent.height
                    opacity: 0.3
                    SequentialAnimation on opacity {
                        loops: Animation.Infinite
                        running: true
                        PauseAnimation { duration: Math.random() * 2000 }
                        NumberAnimation { from: 0.3; to: 1; duration: 1000 }
                        NumberAnimation { from: 1; to: 0.3; duration: 1000 }
                    }
                }
            }
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // Header with back button and title
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 80

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 16
                    anchors.rightMargin: 16
                    spacing: 16

                    RoundButton {
                        id: backButton
                        radius: 20
                        Layout.preferredWidth: 40
                        Layout.preferredHeight: 40
                        Material.background: Qt.rgba(1,1,1,0.2)
                        Material.foreground: "white"
                        icon.source: "image://icon/arrow_back.svg?#FFFFFF"
                        icon.width: 24
                        icon.height: 24
                        onClicked: isProfileVisible = false

                        HoverHandler {
                            cursorShape: Qt.PointingHandCursor
                        }
                    }

                    Label {
                        text: Lang.t("My Profile")
                        font {
                            bold: true;
                            pixelSize: 22;
                            family: "Segoe UI Semibold"
                        }
                        color: "white"
                        Layout.leftMargin: 8
                    }

                    Item { Layout.fillWidth: true }
                }
            }

            // Profile Card with modern design
            Rectangle {
                Layout.alignment: Qt.AlignCenter
                Layout.preferredWidth: Math.min(500, parent.width - 32)
                Layout.fillHeight: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 8
                Layout.bottomMargin: 16
                color: Theme.isDarkMode ? Qt.rgba(0.2, 0.2, 0.2, 0.3) : Qt.rgba(1, 1, 1, 0.1)
                border.color: Theme.isDarkMode ? Qt.rgba(1, 1, 1, 0.1) : Qt.rgba(1, 1, 1, 0.2)
                border.width: 1

                // Backdrop blur effect simulation
                Rectangle {
                    anchors.fill: parent
                    radius: parent.radius
                    color: Theme.isDarkMode ? Qt.rgba(0, 0, 0, 0.2) : Qt.rgba(0, 0, 0, 0.1)
                }

                // Shadow effect
                Rectangle {
                    anchors.fill: parent
                    anchors.margins: -5
                    radius: parent.radius + 5
                    color: "transparent"
                    border.color: Theme.isDarkMode ? Qt.rgba(1, 1, 1, 0.05) : Qt.rgba(0, 0, 0, 0.1)
                    border.width: 1
                    z: -1
                }

                Flickable {
                    anchors.fill: parent
                    anchors.margins: 20
                    contentHeight: profileContent.height
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds

                    ColumnLayout {
                        id: profileContent
                        width: parent.width
                        spacing: 24

                        // Profile Picture Section
                        ColumnLayout {
                            Layout.alignment: Qt.AlignHCenter
                            spacing: 16

                            // Profile Picture Container
                            Item {
                                Layout.alignment: Qt.AlignHCenter
                                width: 140
                                height: 140

                                Rectangle {
                                    id: profileFrame
                                    anchors.fill: parent
                                    radius: width/2
                                    color: "transparent"
                                    border.color: Theme.dividerColor
                                    border.width: 2
                                    layer.enabled: true

                                    Image {
                                        id: profileImage
                                        anchors.fill: parent
                                        anchors.margins: 4
                                        source: profileImagePath
                                        fillMode: Image.PreserveAspectCrop
                                        layer.enabled: true
                                        cache: false
                                        onStatusChanged: {
                                            if (status === Image.Ready) {
                                                console.log("Profile image loaded successfully:", source)
                                            } else if (status === Image.Error) {
                                                console.log("Failed to load profile image:", source, "falling back to default")
                                                source = ":/profilImage.png"
                                            }
                                        }

                                        Rectangle {
                                            visible: profileImage.status !== Image.Ready
                                            anchors.fill: parent
                                            color: Theme.primaryColor
                                            radius: width/2

                                            Image {
                                                anchors.centerIn: parent
                                                source: "image://icon/camera.svg?#FFFFFF"
                                                width: 60
                                                height: 60
                                                opacity: 0.9
                                            }
                                        }
                                    }
                                }

                                RoundButton {
                                    anchors.bottom: profileFrame.bottom
                                    anchors.right: profileFrame.right
                                    radius: 16
                                    width: 46
                                    height: 46
                                    Material.background: Theme.cardColor
                                    opacity: 0.7
                                    Material.foreground: Theme.primaryColor
                                    icon.source: "image://icon/edit.svg?" + Theme.primaryColor
                                    icon.width: 28
                                    icon.height: 28
                                    onClicked: fileDialog.open()

                                    background: Rectangle {
                                        radius: parent.radius
                                        color: Theme.cardColor
                                        border.color: Theme.primaryColor
                                        border.width: 2
                                    }

                                    HoverHandler {
                                        cursorShape: Qt.PointingHandCursor
                                    }
                                }
                            }

                            // User Info Display
                            ColumnLayout {
                                Layout.alignment: Qt.AlignHCenter
                                spacing: 4

                                Label {
                                    text: logger.currentUsername || "Username not set"
                                    font {
                                        pixelSize: Theme.fontSizeHeader;
                                        bold: true;
                                        family: "Segoe UI Semibold"
                                    }
                                    color: "white"
                                    Layout.alignment: Qt.AlignHCenter
                                }

                                Label {
                                    text: logger.currentUserEmail || "Email not set"
                                    font {
                                        pixelSize: Theme.fontSizeBody;
                                        family: "Segoe UI"
                                    }
                                    color: Theme.isDarkMode ? Qt.alpha(Theme.textColor, 0.6) : Qt.rgba(1, 1, 1, 0.6)
                                    Layout.alignment: Qt.AlignHCenter
                                }
                            }
                        }

                        // Profile Details Section
                        ColumnLayout {
                            spacing: 16
                            Layout.fillWidth: true
                            Layout.leftMargin: 8
                            Layout.rightMargin: 8

                            // Username Field
                            ColumnLayout {
                                spacing: 6
                                Layout.fillWidth: true

                                Label {
                                    text: Lang.t("Username")
                                    font {
                                        pixelSize: 13;
                                        bold: true;
                                        family: "Segoe UI"
                                    }
                                    color: Theme.isDarkMode ? Qt.alpha(Theme.textColor, 0.6) : Qt.rgba(1, 1, 1, 0.6)
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    height: 48
                                    radius: Theme.radiusMedium
                                    color: Theme.isDarkMode ? Qt.alpha(Theme.textColor, 0.1) : Qt.rgba(1, 1, 1, 0.1)
                                    border.color: Theme.dividerColor
                                    border.width: 1

                                    Label {
                                        anchors.fill: parent
                                        anchors.leftMargin: 16
                                        anchors.rightMargin: 16
                                        verticalAlignment: Text.AlignVCenter
                                        text: logger.currentUsername || Lang.t("Username not set")
                                        font.pixelSize: 15
                                        color: "white"
                                        elide: Text.ElideRight
                                    }
                                }
                            }

                            // Email Field
                            ColumnLayout {
                                spacing: 6
                                Layout.fillWidth: true

                                Label {
                                    text: Lang.t("Email")
                                    font {
                                        pixelSize: 13;
                                        bold: true;
                                        family: "Segoe UI"
                                    }
                                    color: Theme.isDarkMode ? Qt.alpha(Theme.textColor, 0.6) : Qt.rgba(1, 1, 1, 0.6)
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    height: 48
                                    radius: Theme.radiusMedium
                                    color: Theme.isDarkMode ? Qt.alpha(Theme.textColor, 0.1) : Qt.rgba(1, 1, 1, 0.1)
                                    border.color: Theme.dividerColor
                                    border.width: 1

                                    Label {
                                        anchors.fill: parent
                                        anchors.leftMargin: 16
                                        anchors.rightMargin: 16
                                        verticalAlignment: Text.AlignVCenter
                                        text: logger.currentUserEmail || "Email not set"
                                        font.pixelSize: 15
                                        color: "white"
                                        elide: Text.ElideRight
                                    }
                                }
                            }

                            // Role Field
                            ColumnLayout {
                                spacing: 6
                                Layout.fillWidth: true

                                Label {
                                    text: Lang.t("Role")
                                    font {
                                        pixelSize: 13;
                                        bold: true;
                                        family: "Segoe UI"
                                    }
                                    color: Theme.isDarkMode ? Qt.alpha(Theme.textColor, 0.6) : Qt.rgba(1, 1, 1, 0.6)
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    height: 48
                                    radius: Theme.radiusMedium
                                    color: Theme.isDarkMode ? Qt.alpha(Theme.textColor, 0.1) : Qt.rgba(1, 1, 1, 0.1)
                                    border.color: Theme.dividerColor
                                    border.width: 1

                                    Label {
                                        anchors.fill: parent
                                        anchors.leftMargin: 16
                                        anchors.rightMargin: 16
                                        verticalAlignment: Text.AlignVCenter
                                        text: logger.getUserDepartment(logger.currentUsername) || Lang.t("Role not set")
                                        font.pixelSize: 15
                                        color: "white"
                                        elide: Text.ElideRight
                                    }
                                }
                            }

                            // Password Field
                            ColumnLayout {
                                spacing: 6
                                Layout.fillWidth: true

                                Label {
                                    text: Lang.t("Password")
                                    font { pixelSize: 13; bold: true; family: "Segoe UI" }
                                    color: Theme.isDarkMode ? Qt.alpha(Theme.textColor, 0.6) : Qt.rgba(1, 1, 1, 0.6)
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    height: 48
                                    radius: Theme.radiusMedium
                                    color: Theme.isDarkMode ? Qt.alpha(Theme.textColor, 0.1) : Qt.rgba(1, 1, 1, 0.1)
                                    border.color: Theme.dividerColor
                                    border.width: 1

                                    Label {
                                        anchors.fill: parent
                                        anchors.leftMargin: 16
                                        anchors.rightMargin: 16
                                        verticalAlignment: Text.AlignVCenter
                                        text: "••••••••"
                                        font.pixelSize: 15
                                        color: "white"
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    // FileDialog for selecting image
    FileDialog {
        id: fileDialog
        title: Lang.t("Choose a profile picture")
        nameFilters: ["Image files (*.png *.jpg *.jpeg)"]
        onAccepted: {
            var fileUrl = fileDialog.selectedFile.toString()
            console.log("FileDialog accepted - selectedFile:", fileUrl)

            if (!logger.validateFilePath(fileUrl)) {
                console.log("File validation failed: url=", fileUrl)
                profileErrorLabel.text = Lang.t("Selected file is invalid or cannot be accessed")
                return
            }

            saveTombolCropImage_.imagePath = fileUrl
            console.log("Opening crop dialog with imagePath:", fileUrl)
            saveTombolCropImage_.open()
        }
        onRejected: {
            profileErrorLabel.text = ""
            console.log("FileDialog rejected")
        }
    }
    CropImageDialog{
        id: saveTombolCropImage_
        parent: Overlay.overlay
    }
}

