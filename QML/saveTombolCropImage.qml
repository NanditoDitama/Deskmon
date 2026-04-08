import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15
import QtQuick.Controls.Material
import QtQuick
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Effects


Dialog {
    id: cropDialog
    title: "Crop Profile Picture"
    modal: true
    anchors.centerIn: parent
    width: Math.min(parent.width * 0.9, 700)
    height: Math.min(parent.height * 0.9, 800)
    padding: 0

    property real imageScale: 1.0
    property real imageX: 0
    property real imageY: 0

    background: Rectangle {
        color: cardColor
        radius: 12
        border.color: dividerColor
        border.width: 1
        layer.enabled: true

    }

    contentItem: Rectangle {
        color: cardColor
        radius: 12
        anchors.fill: parent

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 24
            spacing: 24

            Label {
                text: "Adjust your profile picture"
                font {
                    pixelSize: 20
                    family: "Segoe UI"
                    weight: Font.Medium
                }
                color: textColor
                Layout.alignment: Qt.AlignHCenter
            }

            Label {
                text: "Move and zoom the image to fit the circular frame"
                font.pixelSize: 14
                color: Qt.darker(textColor, 1.4)
                Layout.alignment: Qt.AlignHCenter
            }

            // Main cropping area
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.maximumHeight: 400

                Rectangle {
                    id: cropArea
                    anchors.centerIn: parent
                    width: Math.min(parent.width, parent.height)
                    height: width
                    radius: width/2
                    color: "transparent"
                    border.color: Qt.lighter(dividerColor, 1.2)
                    border.width: 2
                    clip: true

                    // Semi-transparent overlay for outside area
                    Rectangle {
                        anchors.fill: parent
                        radius: parent.radius
                        color: "#99000000"
                        visible: cropMask.visible
                    }

                    Image {
                        id: cropImage
                        source: tempImagePath
                        fillMode: Image.PreserveAspectFit
                        width: cropArea.width * cropDialog.imageScale
                        height: cropArea.height * cropDialog.imageScale
                        x: cropDialog.imageX
                        y: cropDialog.imageY
                        smooth: true
                        mipmap: true
                        asynchronous: true
                        cache: false

                        onStatusChanged: {
                            if (status === Image.Ready) {
                                console.log("Image loaded successfully:", tempImagePath)
                                cropDialog.imageScale = Math.min(1.0, cropArea.width / implicitWidth)
                                cropDialog.imageX = (cropArea.width - width) / 2
                                cropDialog.imageY = (cropArea.height - height) / 2
                                zoomSlider.value = cropDialog.imageScale
                            } else if (status === Image.Error) {
                                console.log("Failed to load image:", tempImagePath)
                                profileErrorLabel.text = "Failed to load image for cropping"
                                cropDialog.close()
                            }
                        }
                    }

                    // Circular mask (visible only during editing)
                    Rectangle {
                        id: cropMask
                        anchors.fill: parent
                        radius: parent.radius
                        color: "transparent"
                        border.color: "white"
                        border.width: 2
                        visible: true
                    }
                }

                MouseArea {
                    anchors.fill: cropArea
                    acceptedButtons: Qt.LeftButton
                    cursorShape: Qt.OpenHandCursor

                    property real lastX: 0
                    property real lastY: 0

                    onPressed: {
                        lastX = mouse.x
                        lastY = mouse.y
                        cursorShape = Qt.ClosedHandCursor
                    }

                    onReleased: {
                        cursorShape = Qt.OpenHandCursor
                    }

                    onPositionChanged: {
                        if (pressed) {
                            var deltaX = mouse.x - lastX
                            var deltaY = mouse.y - lastY

                            // Calculate new position
                            var newX = cropDialog.imageX + deltaX
                            var newY = cropDialog.imageY + deltaY

                            // Get image dimensions after scaling
                            var scaledWidth = cropImage.width
                            var scaledHeight = cropImage.height

                            // Calculate maximum allowed movement
                            var maxX = (scaledWidth - cropArea.width) / 2
                            var maxY = (scaledHeight - cropArea.height) / 2

                            // Limit movement to keep image within crop area
                            if (scaledWidth > cropArea.width) {
                                newX = Math.min(maxX, Math.max(-maxX, newX))
                            } else {
                                newX = (cropArea.width - scaledWidth) / 2
                            }

                            if (scaledHeight > cropArea.height) {
                                newY = Math.min(maxY, Math.max(-maxY, newY))
                            } else {
                                newY = (cropArea.height - scaledHeight) / 2
                            }

                            cropDialog.imageX = newX
                            cropDialog.imageY = newY

                            lastX = mouse.x
                            lastY = mouse.y
                        }

                    }



                    onWheel: {
                        var delta = wheel.angleDelta.y / 1200
                        var newScale = cropDialog.imageScale + delta
                        zoomSlider.value = Math.min(Math.max(newScale, zoomSlider.from), zoomSlider.to)
                    }
                }
            }

            // Zoom controls
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 8

                Label {
                    text: "Zoom: " + Math.round(zoomSlider.value * 100) + "%"
                    font.pixelSize: 12
                    color: Qt.darker(textColor, 1.4)
                    Layout.alignment: Qt.AlignHCenter
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    Button {
                        text: "−"
                        Layout.preferredWidth: 48
                        Layout.preferredHeight: 48
                        Material.background: secondaryColor
                        Material.foreground: "white"
                        font {
                            pixelSize: 24
                            family: "Segoe UI Symbol"
                            weight: Font.Bold
                        }
                        onClicked: zoomSlider.value = Math.max(zoomSlider.from, zoomSlider.value - 0.1)
                    }

                    Slider {
                        id: zoomSlider
                        from: 0.2
                        to: 3.0
                        value: 1.0
                        stepSize: 0.05
                        snapMode: Slider.SnapAlways
                        Layout.fillWidth: true
                        onValueChanged: {
                            var oldScale = cropDialog.imageScale
                            cropDialog.imageScale = value

                            // Adjust position to keep the center in view
                            var centerX = cropDialog.imageX + (cropImage.width * 0.5)
                            var centerY = cropDialog.imageY + (cropImage.height * 0.5)

                            cropDialog.imageX = centerX - (cropImage.width * 0.5)
                            cropDialog.imageY = centerY - (cropImage.height * 0.5)

                            // Ensure image stays within bounds
                            var scaledWidth = cropImage.width
                            var scaledHeight = cropImage.height
                            var maxX = (scaledWidth - cropArea.width) / 2
                            var maxY = (scaledHeight - cropArea.height) / 2

                            if (scaledWidth > cropArea.width) {
                                cropDialog.imageX = Math.min(maxX, Math.max(-maxX, cropDialog.imageX))
                            } else {
                                cropDialog.imageX = (cropArea.width - scaledWidth) / 2
                            }

                            if (scaledHeight > cropArea.height) {
                                cropDialog.imageY = Math.min(maxY, Math.max(-maxY, cropDialog.imageY))
                            } else {
                                cropDialog.imageY = (cropArea.height - scaledHeight) / 2
                            }
                        }
                    }

                    Button {
                        text: "+"
                        Layout.preferredWidth: 48
                        Layout.preferredHeight: 48
                        Material.background: secondaryColor
                        Material.foreground: "white"
                        font {
                            pixelSize: 24
                            family: "Segoe UI Symbol"
                            weight: Font.Bold
                        }
                        onClicked: zoomSlider.value = Math.min(zoomSlider.to, zoomSlider.value + 0.1)
                    }
                }
            }

            // Action buttons
            RowLayout {
                Layout.alignment: Qt.AlignRight
                spacing: 16

                Button {
                    text: "Cancel"
                    Layout.preferredWidth: 120
                    Layout.preferredHeight: 48
                    Material.background: "transparent"
                    Material.foreground: textColor
                    font {
                        pixelSize: 14
                        weight: Font.Medium
                    }
                    onClicked: {
                        cropDialog.reject()
                        profileErrorLabel.text = ""
                    }
                }

                Button {
                    text: "Save"
                    Layout.preferredWidth: 120
                    Layout.preferredHeight: 48
                    Material.background: accentColor
                    Material.foreground: "white"
                    font {
                        pixelSize: 14
                        weight: Font.Medium
                    }
                    onClicked: {
                        var croppedPath = logger.cropProfileImage(
                                    tempImagePath,
                                    cropDialog.imageX,
                                    cropDialog.imageY,
                                    cropImage.width,
                                    cropImage.height,
                                    cropArea.width,
                                    cropArea.height
                                    )
                        if (croppedPath !== "") {
                            console.log("Cropped image path:", croppedPath)
                            if (logger.updateProfileImage(currentUsername, croppedPath)) {
                                profileImagePath = croppedPath
                                profileImage.source = ""
                                profileImage.source = profileImagePath
                                cropDialog.accept()
                                profileErrorLabel.text = "Profile picture updated successfully"
                                profileErrorLabel.color = "#4CAF50" // Green
                            } else {
                                console.log("Failed to update profile image in database")
                                profileErrorLabel.text = "Failed to update profile image"
                                profileErrorLabel.color = "#F44336" // Red
                            }
                        } else {
                            console.log("Failed to crop image")
                            profileErrorLabel.text = "Failed to crop image"
                            profileErrorLabel.color = "#F44336" // Red
                        }
                    }
                }
            }
        }
    }
}

