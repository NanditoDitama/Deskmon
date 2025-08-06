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
            anchors.centerIn: parent
            width: 360
            height: 500
            color: cardColor
            radius: 12
            border.color: dividerColor
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 24
                spacing: 16

                Label {
                    text: "Deskmon"
                    font { bold: true; pixelSize: 24; family: "Segoe UI" }
                    color: primaryColor
                    Layout.alignment: Qt.AlignHCenter
                }

                Label {
                    text: "Sign in to track your activity"
                    font { pixelSize: 16; family: "Segoe UI" }
                    color: lightTextColor
                    Layout.alignment: Qt.AlignHCenter
                }

                TextField {
                    id: usernameField
                    placeholderText: "Username"
                    Layout.fillWidth: true
                    font.pixelSize: 16
                    padding: 12
                    background: Rectangle {
                        color: window.Material.theme === Material.Dark ? "#282828" : "#F9FAFB"
                        radius: 8
                    }
                    onAccepted: loginButton.clicked()
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    TextField {
                        id: passwordField
                        placeholderText: "Password"
                        echoMode: showPassword ? TextInput.Normal : TextInput.Password
                        Layout.fillWidth: true
                        font.pixelSize: 16
                        padding: 12
                        background: Rectangle {
                            color: window.Material.theme === Material.Dark ? "#282828" : "#F9FAFB"
                            radius: 8
                        }
                        onAccepted: loginButton.clicked()
                    }

                    Button {
                        id: showPasswordButton
                        icon.source: showPassword ? visibilityIcon : visibilityOffIcon
                        icon.color: primaryColor
                        icon.width: 24
                        icon.height: 24
                        flat: true
                        Layout.preferredWidth: 48
                        Layout.preferredHeight: 48
                        onClicked: showPassword = !showPassword
                        background: Rectangle {
                            color: "transparent"
                        }
                    }
                }

                Button {
                    id: loginButton
                    text: "Login"
                    Layout.fillWidth: true
                    Layout.preferredHeight: 48
                    Material.background: secondaryColor
                    Material.foreground: "white"
                    font.pixelSize: 16
                    onClicked: {
                        if (logger.authenticate(usernameField.text, passwordField.text)) {
                            console.log("Login successful")

                            // Set login state
                            isLoggedIn = true

                            // Ambil data user dari Logger properties
                            currentUsername = logger.currentUsername
                            console.log("Current username from logger:", currentUsername)
                            console.log("Current email from logger:", logger.currentUserEmail)

                            // Update temp variables
                            tempUsername = currentUsername
                            tempPassword = ""
                            tempDepartment = logger.getUserDepartment(currentUsername)

                            // Debug log
                            console.log("Username:", currentUsername)
                            console.log("Email:", logger.getUserEmail(currentUsername))
                            console.log("Department:", tempDepartment)

                            // Ambil profile image path
                            var savedImagePath = logger.getProfileImagePath(currentUsername)
                            profileImagePath = savedImagePath !== "" ? savedImagePath + "?t=" + new Date().getTime() : ":/profilImage.png"
                            refreshProfileImage()

                            // Set date range
                            var today = new Date()
                            startSelectedDate = today
                            endSelectedDate = today
                            isDateSelected = true
                            applyDateRange()

                            // Clear form fields
                            usernameField.text = ""
                            passwordField.text = ""
                            error_Label.text = ""

                        } else {
                            console.log("Login failed")
                            error_Label.text = "Invalid username or password"
                        }
                    }



                    Behavior on Material.background {
                        ColorAnimation { duration: 200 }
                    }
                }
                function refreshProfileImage() {
                    console.log("Refreshing profile image for user:", currentUsername, "path:", profileImagePath)
                    profileImage.source = ""
                    profileImage.source = profileImagePath
                }

                Label {
                    id: error_Label
                    text: ""
                    color: "red"
                    font.pixelSize: 14
                    Layout.alignment: Qt.AlignHCenter
                }
            }
        }
    }
}
