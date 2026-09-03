import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Effects
import "../theme"

Dialog {
    id: cropDialog
    title: Lang.t("Crop Profile Picture")
    modal: true
    width: parent ? Math.min(parent.width * 0.9, 700) : 600
    height: parent ? Math.min(parent.height * 0.9, 800) : 600
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? Math.round((parent.height - height) / 2) : 0
    padding: 0

    property string imagePath: ""
    property real imageScale: 1.0
    property real imageX: 0
    property real imageY: 0

    background: Rectangle {
        color: Theme.cardColor
        radius: 12
        border.color: Theme.dividerColor
        border.width: 1
        layer.enabled: true
    }

    contentItem: Rectangle {
        color: Theme.cardColor
        radius: 12
        anchors.fill: parent

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 24
            spacing: 24

            Label {
                text: Lang.t("Adjust your profile picture")
                font {
                    pixelSize: Theme.fontSizeHeader
                    family: "Segoe UI"
                    weight: Font.Medium
                }
                color: Theme.textColor
                Layout.alignment: Qt.AlignHCenter
            }

            Label {
                text: Lang.t("Move and zoom the image to fit the circular frame")
                font.pixelSize: Theme.fontSizeBody
                color: Qt.darker(Theme.textColor, 1.4)
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
                    border.color: Qt.lighter(Theme.dividerColor, 1.2)
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
                        source: cropDialog.imagePath
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
                                // Center the image initially
                                cropDialog.imageX = (cropArea.width - cropImage.width) / 2
                                cropDialog.imageY = (cropArea.height - cropImage.height) / 2
                            }
                        }
                    }

                    // Circular mask overlay
                    Rectangle {
                        id: cropMask
                        anchors.fill: parent
                        radius: width/2
                        color: "transparent"
                        border.color: Theme.primaryColor
                        border.width: 2
                    }

                    // Drag area for moving image
                    MouseArea {
                        anchors.fill: parent
                        drag.target: null // Disable default drag behavior

                        property real lastX: 0
                        property real lastY: 0

                        onPressed: function(mouse) {
                            lastX = mouse.x
                            lastY = mouse.y
                        }

                        onPositionChanged: function(mouse) {
                            if (pressed) {
                                var deltaX = mouse.x - lastX
                                var deltaY = mouse.y - lastY

                                var newX = cropDialog.imageX + deltaX
                                var newY = cropDialog.imageY + deltaY

                                // Constrain image within bounds
                                var scaledWidth = cropImage.width
                                var scaledHeight = cropImage.height
                                var maxX = (scaledWidth - cropArea.width) / 2
                                var maxY = (scaledHeight - cropArea.height) / 2

                                if (scaledWidth > cropArea.width) {
                                    cropDialog.imageX = Math.min(maxX, Math.max(-maxX, newX))
                                } else {
                                    cropDialog.imageX = (cropArea.width - scaledWidth) / 2
                                }

                                if (scaledHeight > cropArea.height) {
                                    cropDialog.imageY = Math.min(maxY, Math.max(-maxY, newY))
                                } else {
                                    cropDialog.imageY = (cropArea.height - scaledHeight) / 2
                                }

                                lastX = mouse.x
                                lastY = mouse.y
                            }
                        }

                        // Wheel zoom support
                        onWheel: function(wheel) {
                            if (wheel.angleDelta.y > 0) {
                                zoomSlider.value = Math.min(zoomSlider.to, zoomSlider.value + 0.1)
                            } else {
                                zoomSlider.value = Math.max(zoomSlider.from, zoomSlider.value - 0.1)
                            }
                        }
                    }
                }
            }

            // Zoom controls
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 8

                Label {
                    text: Lang.t("Zoom") + ": " + Math.round(zoomSlider.value * 100) + "%"
                    font.pixelSize: Theme.fontSizeSmall
                    color: Qt.darker(Theme.textColor, 1.4)
                    Layout.alignment: Qt.AlignHCenter
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    Button {
                        text: "−"
                        Layout.preferredWidth: 48
                        Layout.preferredHeight: 48
                        Material.background: Theme.secondaryColor
                        Material.foreground: "white"
                        font {
                            pixelSize: Theme.fontSizeLargeHeader
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
                        Material.background: Theme.secondaryColor
                        Material.foreground: "white"
                        font {
                            pixelSize: Theme.fontSizeLargeHeader
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
                    text: Lang.t("Cancel")
                    Layout.preferredWidth: 120
                    Layout.preferredHeight: 48
                    Material.background: "transparent"
                    Material.foreground: Theme.textColor
                    font {
                        pixelSize: Theme.fontSizeBody
                        weight: Font.Medium
                    }
                    onClicked: {
                        cropDialog.reject()
                        profileErrorLabel.text = ""
                    }
                }

                Button {
                    text: Lang.t("Save")
                    Layout.preferredWidth: 120
                    Layout.preferredHeight: 48
                    Material.background: Theme.accentColor
                    Material.foreground: "white"
                    font {
                        pixelSize: Theme.fontSizeBody
                        weight: Font.Medium
                    }
                    onClicked: {
                        var croppedPath = logger.cropProfileImage(
                                    cropDialog.imagePath,
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
                                profileErrorLabel.color = Theme.successColor
                            } else {
                                console.log("Failed to update profile image in database")
                                profileErrorLabel.text = "Failed to update profile image"
                                profileErrorLabel.color = Theme.dangerColor
                            }
                        } else {
                            console.log("Failed to crop image")
                            profileErrorLabel.text = "Failed to crop image"
                            profileErrorLabel.color = Theme.dangerColor
                        }
                    }
                }
            }
        }
    }
}
