import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Popup {
    id: taskDetailPopup
    width: 400
    height: 300
    anchors.centerIn: parent
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    // Properties that need to be set from Main.qml
    property color cardColor: "#FFFFFF"
    property color textColor: "#1F2937"
    property color lightTextColor: "#6B7280"
    property color dividerColor: "#E5E7EB"
    property color primaryColor: "#00e0a8"

    // Function to show task details
    function showTaskDetail(title, description) {
        popupTitle.text = title
        popupDescription.text = description
        open()
    }

    background: Rectangle {
        color: cardColor
        radius: 12
        border.color: dividerColor
        border.width: 1

        // Shadow effect
        Rectangle {
            anchors.fill: parent
            anchors.margins: -3
            color: "transparent"
            radius: parent.radius + 3
            border.color: Qt.rgba(0, 0, 0, 0.1)
            border.width: 1
            z: -1
        }
    }

    contentItem: ColumnLayout {
        spacing: 20
        anchors.fill: parent
        anchors.margins: 20

        // Header
        RowLayout {
            Layout.fillWidth: true

            Label {
                text: "Task Details"
                font {
                    pixelSize: 18
                    bold: true
                    family: "Segoe UI"
                }
                color: primaryColor
                Layout.fillWidth: true
            }

            Button {
                text: "×"
                Layout.preferredWidth: 30
                Layout.preferredHeight: 30
                
                background: Rectangle {
                    color: parent.hovered ? Qt.rgba(0, 0, 0, 0.1) : "transparent"
                    radius: 15
                }
                
                contentItem: Text {
                    text: parent.text
                    font.pixelSize: 18
                    color: textColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                
                onClicked: taskDetailPopup.close()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: dividerColor
        }

        // Task Title
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 8

            Label {
                text: "Title"
                font {
                    pixelSize: 12
                    bold: true
                }
                color: lightTextColor
            }

            Label {
                id: popupTitle
                text: ""
                font {
                    pixelSize: 16
                    bold: true
                    family: "Segoe UI"
                }
                color: textColor
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }
        }

        // Task Description
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 8

            Label {
                text: "Description"
                font {
                    pixelSize: 12
                    bold: true
                }
                color: lightTextColor
            }

            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true

                Label {
                    id: popupDescription
                    text: ""
                    font {
                        pixelSize: 14
                        family: "Segoe UI"
                    }
                    color: textColor
                    wrapMode: Text.Wrap
                    width: parent.width
                }
            }
        }

        // Close Button
        Button {
            text: "Close"
            Layout.alignment: Qt.AlignRight
            Layout.preferredWidth: 100
            Layout.preferredHeight: 40
            
            background: Rectangle {
                color: parent.hovered ? Qt.lighter(primaryColor, 1.1) : primaryColor
                radius: 8
            }
            
            contentItem: Text {
                text: parent.text
                color: "white"
                font.pixelSize: 14
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            
            onClicked: taskDetailPopup.close()
        }
    }

    // Animation
    enter: Transition {
        NumberAnimation {
            property: "opacity"
            from: 0
            to: 1
            duration: 200
            easing.type: Easing.OutQuad
        }
        NumberAnimation {
            property: "scale"
            from: 0.9
            to: 1
            duration: 200
            easing.type: Easing.OutQuad
        }
    }

    exit: Transition {
        NumberAnimation {
            property: "opacity"
            from: 1
            to: 0
            duration: 150
            easing.type: Easing.InQuad
        }
        NumberAnimation {
            property: "scale"
            from: 1
            to: 0.95
            duration: 150
            easing.type: Easing.InQuad
        }
    }
}