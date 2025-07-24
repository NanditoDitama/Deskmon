import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15
import QtQuick.Controls.Material
import QtQuick
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Effects


Item {
    anchors.fill: parent
    Rectangle {
        anchors.fill: parent
        color: "transparent"

        Rectangle {
            width: parent.width
            height: 280
            gradient: Gradient {
                GradientStop {
                    position: 0.0
                    color: Qt.rgba(primaryColor.r,
                                   primaryColor.g,
                                   primaryColor.b,
                                   1.0)
                }
                GradientStop {
                    position: 1.0
                    color: Qt.rgba(primaryColor.r,
                                   primaryColor.g,
                                   primaryColor.b,
                                   0.0)
                }
            }
            anchors.top: parent.top
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
                        icon.source: "qrc:/icons/arrow_back.svg"
                        icon.width: 24
                        icon.height: 24
                        onClicked: isProfileVisible = false

                        HoverHandler {
                            cursorShape: Qt.PointingHandCursor
                        }
                    }

                    Label {
                        text: "My Profile"
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
                Layout.preferredWidth: Math.min(500, parent.width - 32)
                Layout.fillHeight: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 8
                Layout.bottomMargin: 16
                color: window.cardColor
                radius: 24
                Layout.alignment: Qt.AlignHCenter
                layer.enabled: true

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
                                    border.color: window.dividerColor
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
                                            color: primaryColor
                                            radius: width/2

                                            Image {
                                                anchors.centerIn: parent
                                                source: "qrc:/icons/camera.svg"
                                                width: 60
                                                height: 60
                                                opacity: 0.7
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
                                    Material.background: cardColor
                                    opacity: 0.7
                                    Material.foreground: primaryColor
                                    icon.source: "qrc:/icons/edit.svg"
                                    icon.width: 28
                                    icon.height: 28
                                    onClicked: fileDialog.open()

                                    background: Rectangle {
                                        radius: parent.radius
                                        color: cardColor
                                        border.color: primaryColor
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
                                        pixelSize: 20;
                                        bold: true;
                                        family: "Segoe UI Semibold"
                                    }
                                    color: window.textColor
                                    Layout.alignment: Qt.AlignHCenter

                                    // Debug connection
                                    Component.onCompleted: {
                                        console.log("Profile page - Username label:", text)
                                        console.log("Logger currentUsername:", logger.currentUsername)
                                    }

                                    // Update when logger properties change
                                    Connections {
                                        target: logger
                                        function onCurrentUsernameChanged() {
                                            console.log("Username changed to:", logger.currentUsername)
                                        }
                                    }
                                }

                                Label {
                                    text: logger.currentUserEmail || "Email not set"
                                    font {
                                        pixelSize: 14;
                                        family: "Segoe UI"
                                    }
                                    color: window.lightTextColor
                                    Layout.alignment: Qt.AlignHCenter

                                    // Debug connection
                                    Component.onCompleted: {
                                        console.log("Profile page - Email label:", text)
                                        console.log("Logger currentUserEmail:", logger.currentUserEmail)
                                    }

                                    // Update when logger properties change
                                    Connections {
                                        target: logger
                                        function onCurrentUserEmailChanged() {
                                            console.log("Email changed to:", logger.currentUserEmail)
                                        }
                                    }
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
                                    text: "Username"
                                    font {
                                        pixelSize: 13;
                                        bold: true;
                                        family: "Segoe UI"
                                    }
                                    color: window.lightTextColor
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    height: 48
                                    radius: 12
                                    color: window.Material.theme === Material.Dark ? "#282828" : "#F9FAFB"
                                    border.color: window.dividerColor
                                    border.width: 1

                                    Label {
                                        anchors.fill: parent
                                        anchors.leftMargin: 16
                                        anchors.rightMargin: 16
                                        verticalAlignment: Text.AlignVCenter
                                        text: logger.currentUsername || "Username not set"
                                        font.pixelSize: 15
                                        color: window.textColor
                                        elide: Text.ElideRight
                                    }
                                }
                            }

                            // Email Field
                            ColumnLayout {
                                spacing: 6
                                Layout.fillWidth: true

                                Label {
                                    text: "Email"
                                    font {
                                        pixelSize: 13;
                                        bold: true;
                                        family: "Segoe UI"
                                    }
                                    color: window.lightTextColor
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    height: 48
                                    radius: 12
                                    color: window.Material.theme === Material.Dark ? "#282828" : "#F9FAFB"
                                    border.color: window.dividerColor
                                    border.width: 1

                                    Label {
                                        anchors.fill: parent
                                        anchors.leftMargin: 16
                                        anchors.rightMargin: 16
                                        verticalAlignment: Text.AlignVCenter
                                        text: logger.currentUserEmail || "Email not set"
                                        font.pixelSize: 15
                                        color: window.textColor
                                        elide: Text.ElideRight
                                    }
                                }
                            }

                            // Role Field
                            ColumnLayout {
                                spacing: 6
                                Layout.fillWidth: true

                                Label {
                                    text: "Role"
                                    font {
                                        pixelSize: 13;
                                        bold: true;
                                        family: "Segoe UI"
                                    }
                                    color: window.lightTextColor
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    height: 48
                                    radius: 12
                                    color: window.Material.theme === Material.Dark ? "#282828" : "#F9FAFB"
                                    border.color: window.dividerColor
                                    border.width: 1

                                    Label {
                                        anchors.fill: parent
                                        anchors.leftMargin: 16
                                        anchors.rightMargin: 16
                                        verticalAlignment: Text.AlignVCenter
                                        text: logger.getUserDepartment(logger.currentUsername) || "Role not set"
                                        font.pixelSize: 15
                                        color: window.textColor
                                        elide: Text.ElideRight
                                    }
                                }
                            }

                            // Password Field
                            ColumnLayout {
                                spacing: 6
                                Layout.fillWidth: true

                                Label {
                                    text: "Password"
                                    font { pixelSize: 13; bold: true; family: "Segoe UI" }
                                    color: window.lightTextColor
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    height: 48
                                    radius: 12
                                    color: window.Material.theme === Material.Dark ? "#282828" : "#F9FAFB"
                                    border.color: window.dividerColor
                                    border.width: 1

                                    Label {
                                        anchors.fill: parent
                                        anchors.leftMargin: 16
                                        anchors.rightMargin: 16
                                        verticalAlignment: Text.AlignVCenter
                                        text: "••••••••"
                                        font.pixelSize: 15
                                        color: window.textColor
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
        title: "Choose a profile picture"
        nameFilters: ["Image files (*.png *.jpg *.jpeg)"]
        onAccepted: {
            var fileUrl = fileDialog.selectedFile.toString()
            console.log("FileDialog accepted - selectedFile:", fileUrl)

            if (!logger.validateFilePath(fileUrl)) {
                console.log("File validation failed: url=", fileUrl)
                profileErrorLabel.text = "Selected file is invalid or cannot be accessed"
                return
            }

            tempImagePath = fileUrl
            console.log("Opening crop dialog with tempImagePath:", tempImagePath)
            saveTombolCropImage_.open()
        }
        onRejected: {
            profileErrorLabel.text = ""
            console.log("FileDialog rejected")
        }
    }
    SaveTombolCropImage{
        id: saveTombolCropImage_
        parent: Overlay.overlay

    }
}

