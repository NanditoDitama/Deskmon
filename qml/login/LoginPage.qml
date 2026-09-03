import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Effects
import "../theme"

Item {
    id: loginPageRoot
    anchors.fill: parent
    property bool isLoading: false
    property bool showPassword: false
    readonly property string visibilityIcon: "qrc:/icons/visibility.svg"
    readonly property string visibilityOffIcon: "qrc:/icons/visibility_off.svg"

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
                color: Theme.isDarkMode ? "#1a1a2e" : "#E0FFF8"
            }
            GradientStop {
                position: 1.0
                color: Theme.isDarkMode ? "black" : "#e0e5ec"
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

    Item {
        id: cardContainer
        anchors.centerIn: parent
        width: 420
        height: 580

        Rectangle {
            id: sourceCard
            anchors.fill: parent
            radius: Theme.radiusLarge
            color: Theme.isDarkMode ? Qt.rgba(0.2, 0.2, 0.2, 0.3) : Qt.rgba(1, 1, 1, 0.9)
            visible: false
        }

        MultiEffect {
            source: sourceCard
            anchors.fill: sourceCard
            autoPaddingEnabled: true
            shadowEnabled: !Theme.isDarkMode
            shadowColor: Qt.rgba(0, 0, 0, 0.2)
            shadowBlur: 1.0
            shadowVerticalOffset: 10
            shadowHorizontalOffset: 0
            shadowScale: 1.02
        }

        Rectangle {
            anchors.fill: parent
            radius: Theme.radiusLarge
            color: "transparent"
            border.color: Theme.isDarkMode ? Qt.rgba(1, 1, 1, 0.1) : Qt.rgba(0, 0, 0, 0.1)
            border.width: 1
        }

        Rectangle {
            anchors.fill: parent
            radius: Theme.radiusLarge
            color: Theme.isDarkMode ? Qt.rgba(1, 1, 1, 0.05) : "transparent"
            border.color: Theme.isDarkMode ? "transparent" : Qt.rgba(1, 1, 1, 0.5)
            border.width: 1
            anchors.margins: 1
            z: 1
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 40
            spacing: 30

            // Logo dan title section
            ColumnLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 16

                Rectangle {
                    width: 80
                    height: 80
                    radius: 40
                    color: Theme.dividerColor
                    border.color: Theme.isDarkMode ? Qt.rgba(1, 1, 1, 0.2) : Qt.rgba(1, 1, 1, 0.3)
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
                        weight: Font.Bold
                    }
                    color: Theme.textColor
                    Layout.alignment: Qt.AlignHCenter
                }

                Label {
                    text: Lang.t("Sign in to track your activity")
                    font {
                        pixelSize: Theme.fontSizeTitle
                        family: "Segoe UI"
                        weight: Font.Normal
                    }
                    color: Qt.alpha(Theme.textColor, 0.6)
                    Layout.alignment: Qt.AlignHCenter
                }
            }

            // Form section
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 20

                // Username field
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Label {
                        text: Lang.t("Username")
                        font {
                            pixelSize: Theme.fontSizeBody
                            family: "Segoe UI"
                            weight: Font.Medium
                        }
                        color: Theme.textColor
                    }

                    TextField {
                        id: usernameField
                        placeholderText: Lang.t("Enter your username")
                        text: logger.savedUsername
                        enabled: !loginPageRoot.isLoading
                        Layout.fillWidth: true
                        Layout.preferredHeight: 50
                        font.pixelSize: Theme.fontSizeTitle
                        font.family: "Segoe UI"
                        leftPadding: 16
                        rightPadding: 16
                        color: Theme.textColor
                        placeholderTextColor: Qt.alpha(Theme.textColor, 0.6)

                        background: Rectangle {
                            radius: Theme.radiusMedium
                            color: Theme.isDarkMode ? Qt.alpha(Theme.textColor, 0.1) : Qt.rgba(1, 1, 1, 0.1)
                            border.color: usernameField.activeFocus ? Theme.primaryColor : Theme.dividerColor
                            border.width: usernameField.activeFocus ? 2 : 1

                            Behavior on border.color {
                                ColorAnimation { duration: 200 }
                            }
                        }
                        onAccepted: if (!loginPageRoot.isLoading) loginButton.clicked()
                    }
                }

                // Password field
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Label {
                        text: Lang.t("Password")
                        font {
                            pixelSize: Theme.fontSizeBody
                            family: "Segoe UI"
                            weight: Font.Medium
                        }
                        color: Theme.textColor
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 50
                        radius: Theme.radiusMedium
                        color: Theme.isDarkMode ? Qt.alpha(Theme.textColor, 0.1) : Qt.rgba(1, 1, 1, 0.1)
                        border.color: passwordField.activeFocus ? Theme.primaryColor : Theme.dividerColor
                        border.width: passwordField.activeFocus ? 2 : 1

                        Behavior on border.color {
                            ColorAnimation { duration: 200 }
                        }

                        RowLayout {
                            anchors.fill: parent
                            spacing: 8

                            TextField {
                                id: passwordField
                                placeholderText: Lang.t("Enter your password")
                                enabled: !loginPageRoot.isLoading
                                echoMode: showPassword ? TextInput.Normal : TextInput.Password
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                font.pixelSize: Theme.fontSizeTitle
                                font.family: "Segoe UI"
                                leftPadding: 16
                                rightPadding: 16
                                color: Theme.textColor
                                placeholderTextColor: Qt.alpha(Theme.textColor, 0.6)

                                background: Rectangle {
                                    color: "transparent"
                                }

                                onAccepted: if (!loginPageRoot.isLoading) loginButton.clicked()
                            }

                            Button {
                                id: showPasswordButton
                                icon.source: showPassword ? visibilityIcon : visibilityOffIcon
                                icon.color: Qt.alpha(Theme.textColor, 0.6)
                                icon.width: 35
                                icon.height: 35
                                Layout.preferredWidth: 50
                                Layout.preferredHeight: 50
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
                                    onEntered: parent.background.color = Theme.isDarkMode ? Qt.rgba(1, 1, 1, 0.05) : Qt.rgba(1, 1, 1, 0.1)
                                    onExited: parent.background.color = "transparent"
                                    onClicked: showPassword = !showPassword
                                }
                            }
                        }
                    }
                }
                // Login button
                Button {
                    id: loginButton
                    text: loginPageRoot.isLoading ? "" : Lang.t("Sign In")
                    enabled: !loginPageRoot.isLoading
                    Layout.fillWidth: true
                    Layout.preferredHeight: 54
                    font.pixelSize: Theme.fontSizeTitle
                    font.family: "Segoe UI"
                    font.weight: Font.Medium

                    background: Rectangle {
                        radius: Theme.radiusMedium
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: loginPageRoot.isLoading ? Qt.alpha(Theme.primaryColor, 0.7) : Theme.primaryColor }
                            GradientStop { position: 1.0; color: loginPageRoot.isLoading ? Qt.alpha(Theme.primaryColor, 0.7) : Theme.primaryColor }
                        }

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            enabled: !loginPageRoot.isLoading
                            onEntered: parent.color = Qt.darker(parent.color, 1.1)
                            onExited: parent.color = Qt.lighter(parent.color, 1.1)
                            onClicked: loginButton.clicked()
                        }

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

                        Qt.callLater(function() {
                            var result = logger.authenticate(usernameField.text, passwordField.text)

                            if (result === "") {
                                console.log("Login successful")
                                loginPageRoot.isLoading = false
                                isLoggedIn = true
                                currentUsername = logger.currentUsername
                                console.log("Current username from logger:", currentUsername)
                                console.log("Current email from logger:", logger.currentUserEmail)

                                var dept = logger.getUserDepartment(currentUsername)
                                console.log("Username:", currentUsername)
                                console.log("Email:", logger.getUserEmail(currentUsername))
                                console.log("Department:", dept)

                                var savedImagePath = logger.getProfileImagePath(currentUsername)
                                profileImagePath = savedImagePath !== "" ? savedImagePath + "?t=" + new Date().getTime() : ":/profilImage.png"

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

                // Error message
                Label {
                    id: error_Label
                    text: ""
                    color: Theme.nonProductiveColor
                    font.pixelSize: Theme.fontSizeBody
                    font.family: "Segoe UI"
                    Layout.alignment: Qt.AlignHCenter
                    Layout.topMargin: 8

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
            color: Theme.isDarkMode ? Qt.rgba(1, 1, 1, 0.1) : Qt.rgba(0, 0, 0, 0.1)
            border.color: Theme.isDarkMode ? Qt.rgba(1, 1, 1, 0.2) : Qt.rgba(0, 0, 0, 0.2)
            border.width: 1

            // Glow effect on hover
            Rectangle {
                anchors.fill: parent
                radius: parent.radius
                color: "transparent"
                border.color: Theme.isDarkMode ? Qt.rgba(1, 1, 1, 0.3) : Qt.rgba(0, 0, 0, 0.3)
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
                color: Theme.isDarkMode ? Qt.rgba(1, 1, 1, 0.1) : Qt.rgba(0, 0, 0, 0.1)
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
                source: Theme.isDarkMode ? ("image://icon/light_mode.svg?" + Theme.textColor) : ("image://icon/dark_mode.svg?" + Theme.textColor)
                sourceSize.width: 18
                sourceSize.height: 18
                anchors.centerIn: parent
                opacity: 0.9

                Rectangle {
                    anchors.fill: parent
                    radius: 14
                    color: Theme.isDarkMode ? "#FFF9C4" : "#37474F"
                    opacity: 0.1
                    visible: parent.source.toString().length > 0
                }
            }
        }

        onClicked: {
            Theme.isDarkMode = !Theme.isDarkMode
            pulseAnim.start()
            flipAnim.start()
        }

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
                    themeIcon.opacity = 0.3
                }
            }
            PauseAnimation { duration: 50 }
            ScriptAction {
                script: {
                    themeIcon.opacity = Theme.isDarkMode ? 0.9 : 0.8
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

        Behavior on scale {
            enabled: !pulseAnim.running
            SpringAnimation {
                spring: 3
                damping: 0.5
                modulus: 1.0
            }
        }

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

        ToolTip {
            id: themeTooltip
            text: Theme.isDarkMode ? "Switch to Light Mode" : "Switch to Dark Mode"
            visible: themeToggle.hovered
            delay: 800
            timeout: 3000

            background: Rectangle {
                color: Theme.isDarkMode ? "#2D2D2D" : "#F5F5F5"
                radius: Theme.radiusSmall
                border.color: Theme.borderColor
                border.width: 1

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
                font.pixelSize: Theme.fontSizeSmall
                font.family: "Segoe UI"
                color: Theme.textColor
            }
        }
    }
}
