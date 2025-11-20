import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15
import QtQuick.Controls.Material
import QtQuick
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Effects

Item {
    id: loginPageRoot
    anchors.fill: parent
    property bool isLoading: false

    Component.onCompleted: {
        usernameField.text = logger.savedUsername()
        passwordField.text = logger.savedPassword()
    }

    // Background dengan gradient modern yang responsif terhadap tema
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop {
                position: 0.0
                color: isDarkMode ? "#1a1a2e" : "#667eea"
            }
            GradientStop {
                position: 1.0
                color: isDarkMode ? "#16213e" : "#764ba2"
            }
        }

        // Animated background circles untuk efek modern
        Repeater {
            model: 3
            Rectangle {
                property real animationOffset: index * 120
                width: 200 + index * 50
                height: width
                radius: width / 2
                color: isDarkMode ? Qt.rgba(1, 1, 1, 0.03) : Qt.rgba(1, 1, 1, 0.05)
                x: parent.width * 0.7 + Math.sin((animationTimer.currentTime + animationOffset) / 2000) * 100
                y: parent.height * 0.3 + Math.cos((animationTimer.currentTime + animationOffset) / 1500) * 50

                Timer {
                    id: animationTimer
                    property real currentTime: 0
                    interval: 16
                    running: true
                    repeat: true
                    onTriggered: currentTime += interval
                }
            }
        }
    }

    // Glass morphism card
    Rectangle {
        anchors.centerIn: parent
        width: 420
        height: 580
        radius: 20
        color: isDarkMode ? Qt.rgba(0.2, 0.2, 0.2, 0.3) : Qt.rgba(1, 1, 1, 0.1)
        border.color: isDarkMode ? Qt.rgba(1, 1, 1, 0.1) : Qt.rgba(1, 1, 1, 0.2)
        border.width: 1

        // Backdrop blur effect simulation
        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            color: isDarkMode ? Qt.rgba(0, 0, 0, 0.2) : Qt.rgba(0, 0, 0, 0.1)
        }

        // Shadow effect
        Rectangle {
            anchors.fill: parent
            anchors.margins: -5
            radius: parent.radius + 5
            color: "transparent"
            border.color: isDarkMode ? Qt.rgba(1, 1, 1, 0.05) : Qt.rgba(0, 0, 0, 0.1)
            border.width: 1
            z: -1
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 40
            spacing: 30

            // Logo dan title section
            ColumnLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 16

                // Logo placeholder dengan design modern
                Rectangle {
                    width: 80
                    height: 80
                    radius: 40
                    color: isDarkMode ? Qt.rgba(0.2, 0.2, 0.2, 0.3) : Qt.rgba(1, 1, 1, 0.1)
                    border.color: isDarkMode ? Qt.rgba(1, 1, 1, 0.2) : Qt.rgba(1, 1, 1, 0.3)
                    border.width: 2
                    Layout.alignment: Qt.AlignHCenter

                    Image {
                        id: icon
                        source: "qrc:icon.ico"
                        sourceSize.width: 50
                        sourceSize.height: 50
                        anchors.centerIn: parent
                    }
                }

                Label {
                    text: "Deskmon"
                    font {
                        bold: true
                        pixelSize: 32
                        family: "Segoe UI"
                        weight: Font.Light
                    }
                    color: isDarkMode ? textColor : "white"
                    Layout.alignment: Qt.AlignHCenter
                }

                Label {
                    text: "Sign in to track your activity"
                    font {
                        pixelSize: 16
                        family: "Segoe UI"
                        weight: Font.Normal
                    }
                    color: isDarkMode ? lightTextColor : Qt.rgba(1, 1, 1, 0.8)
                    Layout.alignment: Qt.AlignHCenter
                }
            }

            // Form section
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 20

                // Username field dengan design modern
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Label {
                        text: "Username"
                        font {
                            pixelSize: 14
                            family: "Segoe UI"
                            weight: Font.Medium
                        }
                        color: isDarkMode ? textColor : Qt.rgba(1, 1, 1, 0.9)
                    }

                    TextField {
                        id: usernameField
                        placeholderText: "Enter your username"
                        text: logger.savedUsername
                        enabled: !loginPageRoot.isLoading
                        Layout.fillWidth: true
                        Layout.preferredHeight: 50
                        font.pixelSize: 16
                        font.family: "Segoe UI"
                        leftPadding: 16
                        rightPadding: 16
                        color: isDarkMode ? textColor : "white"
                        placeholderTextColor: isDarkMode ? Qt.rgba(textColor.r, textColor.g, textColor.b, 0.6) : Qt.rgba(1, 1, 1, 0.6)

                        background: Rectangle {
                            radius: 12
                            color: isDarkMode ? Qt.rgba(textColor.r, textColor.g, textColor.b, 0.1) : Qt.rgba(1, 1, 1, 0.1)
                            border.color: usernameField.activeFocus ? primaryColor : (isDarkMode ? dividerColor : Qt.rgba(1, 1, 1, 0.2))
                            border.width: usernameField.activeFocus ? 2 : 1

                            Behavior on border.color {
                                ColorAnimation { duration: 200 }
                            }
                        }
                        onAccepted: if (!loginPageRoot.isLoading) loginButton.clicked()
                    }
                }

                // Password field dengan design modern
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Label {
                        text: "Password"
                        font {
                            pixelSize: 14
                            family: "Segoe UI"
                            weight: Font.Medium
                        }
                        color: isDarkMode ? textColor : Qt.rgba(1, 1, 1, 0.9)
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 0

                        TextField {
                            id: passwordField
                            placeholderText: "Enter your password"
                            enabled: !loginPageRoot.isLoading
                            echoMode: showPassword ? TextInput.Normal : TextInput.Password
                            Layout.fillWidth: true
                            Layout.preferredHeight: 50
                            font.pixelSize: 16
                            font.family: "Segoe UI"
                            leftPadding: 16
                            rightPadding: 50
                            color: isDarkMode ? textColor : "white"
                            placeholderTextColor: isDarkMode ? Qt.rgba(textColor.r, textColor.g, textColor.b, 0.6) : Qt.rgba(1, 1, 1, 0.6)

                            background: Rectangle {
                                radius: 12
                                color: isDarkMode ? Qt.rgba(textColor.r, textColor.g, textColor.b, 0.1) : Qt.rgba(1, 1, 1, 0.1)
                                border.color: passwordField.activeFocus ? primaryColor : (isDarkMode ? dividerColor : Qt.rgba(1, 1, 1, 0.2))
                                border.width: passwordField.activeFocus ? 2 : 1

                                Behavior on border.color {
                                    ColorAnimation { duration: 200 }
                                }
                            }
                            onAccepted: if (!loginPageRoot.isLoading) loginButton.clicked()
                        }

                        Button {
                            id: showPasswordButton
                            icon.source: showPassword ? visibilityIcon : visibilityOffIcon
                            icon.color: isDarkMode ? lightTextColor : Qt.rgba(1, 1, 1, 0.7)
                            icon.width: 20
                            icon.height: 20
                            width: 40
                            height: 40
                            anchors.right: passwordField.right
                            anchors.rightMargin: 8
                            anchors.verticalCenter: passwordField.verticalCenter
                            flat: true
                            enabled: !loginPageRoot.isLoading
                            onClicked: showPassword = !showPassword
                            background: Rectangle {
                                color: "transparent"
                                radius: 6
                            }

                            MouseArea {
                                anchors.fill: parent
                                hoverEnabled: true
                                enabled: !loginPageRoot.isLoading
                                onEntered: parent.background.color = isDarkMode ? Qt.rgba(1, 1, 1, 0.05) : Qt.rgba(1, 1, 1, 0.1)
                                onExited: parent.background.color = "transparent"
                                onClicked: showPassword = !showPassword
                            }
                        }
                    }
                }

                // Login button dengan design modern dan loading state
                Button {
                    id: loginButton
                    text: loginPageRoot.isLoading ? "" : "Sign In"
                    enabled: !loginPageRoot.isLoading
                    Layout.fillWidth: true
                    Layout.preferredHeight: 54
                    font.pixelSize: 16
                    font.family: "Segoe UI"
                    font.weight: Font.Medium

                    background: Rectangle {
                        radius: 12
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: loginPageRoot.isLoading ? Qt.rgba(primaryColor.r, primaryColor.g, primaryColor.b, 0.7) : primaryColor }
                            GradientStop { position: 1.0; color: loginPageRoot.isLoading ? Qt.rgba(primaryColor.r, primaryColor.g, primaryColor.b, 0.7) : primaryColor }
                        }

                        // Hover effect
                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            enabled: !loginPageRoot.isLoading
                            onEntered: parent.color = Qt.darker(parent.color, 1.1)
                            onExited: parent.color = Qt.lighter(parent.color, 1.1)
                            onClicked: loginButton.clicked()
                        }

                        // Press animation
                        scale: loginButton.pressed ? 0.98 : 1.0
                        Behavior on scale {
                            NumberAnimation { duration: 100 }
                        }

                        Behavior on gradient {
                            ColorAnimation { duration: 200 }
                        }
                    }

                    contentItem: Item {
                        anchors.fill: parent

                        // Loading spinner
                        BusyIndicator {
                            id: loadingSpinner
                            anchors.centerIn: parent
                            width: 24
                            height: 24
                            running: loginPageRoot.isLoading
                            visible: loginPageRoot.isLoading

                            contentItem: Item {
                                width: 24
                                height: 24

                                Rectangle {
                                    width: 24
                                    height: 24
                                    radius: 12
                                    color: "transparent"
                                    border.width: 3
                                    border.color: "white"
                                    opacity: 0.3
                                }

                                Rectangle {
                                    width: 24
                                    height: 24
                                    radius: 12
                                    color: "transparent"
                                    border.width: 3
                                    border.color: "transparent"

                                    Rectangle {
                                        width: parent.width / 2
                                        height: 3
                                        color: "white"
                                        radius: 1.5
                                        anchors.left: parent.left
                                        anchors.leftMargin: parent.width / 2
                                        anchors.verticalCenter: parent.verticalCenter
                                    }

                                    RotationAnimator on rotation {
                                        running: loginPageRoot.isLoading
                                        from: 0
                                        to: 360
                                        duration: 1000
                                        loops: Animation.Infinite
                                    }
                                }
                            }
                        }

                        // Button text
                        Text {
                            text: loginButton.text
                            font: loginButton.font
                            color: "white"
                            anchors.centerIn: parent
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            visible: !loginPageRoot.isLoading
                        }
                    }

                    onClicked: {
                        if (loginPageRoot.isLoading) return

                        error_Label.text = ""
                        loginPageRoot.isLoading = true

                        // Simulate network delay for smooth UX
                        Qt.callLater(function() {
                            var result = logger.authenticate(usernameField.text, passwordField.text)

                            if (result === "") {
                                console.log("Login successful")
                                loginPageRoot.isLoading = false
                                isLoggedIn = true
                                currentUsername = logger.currentUsername
                                console.log("Current username from logger:", currentUsername)
                                console.log("Current email from logger:", logger.currentUserEmail)

                                tempUsername = currentUsername
                                tempPassword = ""
                                tempDepartment = logger.getUserDepartment(currentUsername)

                                console.log("Username:", currentUsername)
                                console.log("Email:", logger.getUserEmail(currentUsername))
                                console.log("Department:", tempDepartment)

                                var savedImagePath = logger.getProfileImagePath(currentUsername)
                                profileImagePath = savedImagePath !== "" ? savedImagePath + "?t=" + new Date().getTime() : ":/profilImage.png"
                                refreshProfileImage()

                                var today = new Date()
                                startSelectedDate = today
                                endSelectedDate = today
                                isDateSelected = true
                                applyDateRange()

                                usernameField.text = ""
                                passwordField.text = ""
                                error_Label.text = ""



                            } else {
                                console.log("Login failed:", result)
                                error_Label.text = result
                                loginPageRoot.isLoading = false
                            }
                        })
                    }
                }

                // Error message dengan design modern
                Label {
                    id: error_Label
                    text: ""
                    color: nonProductiveColor
                    font.pixelSize: 14
                    font.family: "Segoe UI"
                    Layout.alignment: Qt.AlignHCenter
                    Layout.topMargin: 8

                    // Fade in animation
                    opacity: text === "" ? 0 : 1
                    Behavior on opacity {
                        NumberAnimation { duration: 300 }
                    }
                }
            }

            // Footer spacer
            Item {
                Layout.fillHeight: true
            }
        }

        function refreshProfileImage() {
            console.log("Refreshing profile image for user:", currentUsername, "path:", profileImagePath)
            profileImage.source = ""
            profileImage.source = profileImagePath
        }
    }

    // Theme Toggle Button - positioned at bottom right
    RoundButton {
        id: themeToggle
        width: 46
        height: 46
        radius: 28
        hoverEnabled: true
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: 32
        anchors.bottomMargin: 32
        z: 1000

        background: Rectangle {
            radius: 28
            color: isDarkMode ? Qt.rgba(1, 1, 1, 0.1) : Qt.rgba(0, 0, 0, 0.1)
            border.color: isDarkMode ? Qt.rgba(1, 1, 1, 0.2) : Qt.rgba(0, 0, 0, 0.2)
            border.width: 1

            // Glow effect on hover
            Rectangle {
                anchors.fill: parent
                radius: parent.radius
                color: "transparent"
                border.color: isDarkMode ? Qt.rgba(1, 1, 1, 0.3) : Qt.rgba(0, 0, 0, 0.3)
                border.width: parent.parent.hovered ? 2 : 0
                opacity: parent.parent.hovered ? 0.6 : 0

                Behavior on opacity {
                    NumberAnimation { duration: 200 }
                }
                Behavior on border.width {
                    NumberAnimation { duration: 200 }
                }
            }

            // Ripple effect background
            Rectangle {
                anchors.fill: parent
                radius: parent.radius
                color: isDarkMode ? Qt.rgba(1, 1, 1, 0.1) : Qt.rgba(0, 0, 0, 0.1)
                opacity: parent.parent.pressed ? 0.3 : 0

                Behavior on opacity {
                    NumberAnimation { duration: 100 }
                }
            }

            Behavior on color {
                ColorAnimation { duration: 200 }
            }
        }

        contentItem: Rectangle {
            anchors.fill: parent
            radius: 28
            color: "transparent"

            Image {
                id: themeIcon
                source: isDarkMode ? "qrc:/icons/light_mode.svg" : "qrc:/icons/dark_mode.svg"
                sourceSize.width: 18
                sourceSize.height: 18
                anchors.centerIn: parent
                opacity: 0.9

                // Simple color tinting without ColorOverlay
                Rectangle {
                    anchors.fill: parent
                    radius: 14
                    color: isDarkMode ? "#FFF9C4" : "#37474F"
                    opacity: 0.1
                    visible: parent.source.toString().length > 0
                }
            }
        }

        onClicked: {
            isDarkMode = !isDarkMode
            rotationAnim.start()
            scaleAnim.start()
        }

        // Pulse animation with glow effect
        SequentialAnimation {
            id: pulseAnim
            loops: 2
            NumberAnimation {
                target: themeToggle
                property: "scale"
                from: 1.0
                to: 1.2
                duration: 150
                easing.type: Easing.OutQuad
            }
            NumberAnimation {
                target: themeToggle
                property: "scale"
                from: 1.2
                to: 1.0
                duration: 150
                easing.type: Easing.InQuad
            }
        }

        // 3D flip animation for the icon
        SequentialAnimation {
            id: flipAnim
            NumberAnimation {
                target: themeIcon
                property: "rotation"
                from: 0
                to: 90
                duration: 200
                easing.type: Easing.InQuad
            }
            ScriptAction {
                script: {
                    // Change opacity during flip for smooth transition
                    themeIcon.opacity = 0.3
                }
            }
            PauseAnimation { duration: 50 }
            ScriptAction {
                script: {
                    themeIcon.opacity = isDarkMode ? 0.9 : 0.8
                }
            }
            NumberAnimation {
                target: themeIcon
                property: "rotation"
                from: -90
                to: 0
                duration: 200
                easing.type: Easing.OutQuad
            }
        }

        // Bouncy scale animation on hover
        Behavior on scale {
            enabled: !pulseAnim.running
            SpringAnimation {
                spring: 3
                damping: 0.5
                modulus: 1.0
            }
        }

        // Smooth glow animation
        SequentialAnimation {
            id: glowAnim
            running: themeToggle.hovered
            loops: Animation.Infinite
            NumberAnimation {
                target: themeToggle.background
                property: "border.width"
                from: 1
                to: 3
                duration: 1000
                easing.type: Easing.InOutSine
            }
            NumberAnimation {
                target: themeToggle.background
                property: "border.width"
                from: 3
                to: 1
                duration: 1000
                easing.type: Easing.InOutSine
            }
        }

        // Enhanced tooltip
        ToolTip {
            id: themeTooltip
            text: isDarkMode ? "Switch to Light Mode" : "Switch to Dark Mode"
            visible: themeToggle.hovered
            delay: 800
            timeout: 3000

            background: Rectangle {
                color: isDarkMode ? "#2D2D2D" : "#F5F5F5"
                radius: 8
                border.color: isDarkMode ? "#404040" : "#E0E0E0"
                border.width: 1

                // Shadow effect
                Rectangle {
                    anchors.fill: parent
                    anchors.margins: -2
                    color: "transparent"
                    radius: 10
                    border.color: Qt.rgba(0, 0, 0, 0.1)
                    border.width: 1
                    z: -1
                }
            }

            contentItem: Text {
                text: themeTooltip.text
                font.pixelSize: 12
                font.family: "Segoe UI"
                color: isDarkMode ? "#FFFFFF" : "#333333"
            }
        }
    }
}
