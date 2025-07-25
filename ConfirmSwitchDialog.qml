import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Dialog {
    id: confirmSwitchDialog
    title: "Confirm Task Switch"
    modal: true
    anchors.centerIn: parent
    width: 400
    height: 280
    padding: 0

    // Properties that need to be set from Main.qml
    property int taskId: -1
    property var logger
    property color cardColor: "#FFFFFF"
    property color textColor: "#1F2937"
    property color dividerColor: "#E5E7EB"
    property color primaryColor: "#00e0a8"
    property color secondaryColor: "#3B82F6"
    property color accentColor: "#F59E0B"

    // Signals
    signal taskSwitchConfirmed(int taskId)
    signal taskSwitchCancelled()

    background: Rectangle {
        color: cardColor
        radius: 12
        border.color: dividerColor
    }

    contentItem: Rectangle {
        color: cardColor
        radius: 12
        anchors.fill: parent

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 24
            spacing: 16

            // Icon
            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 60
                Layout.preferredHeight: 60
                radius: 30
                color: Qt.rgba(secondaryColor.r, secondaryColor.g, secondaryColor.b, 0.1)

                Text {
                    text: "⚠"
                    font.pixelSize: 28
                    color: secondaryColor
                    anchors.centerIn: parent
                }
            }

            Label {
                text: "Switch Task"
                font { 
                    pixelSize: 20
                    bold: true
                    family: "Segoe UI" 
                }
                color: textColor
                Layout.alignment: Qt.AlignHCenter
            }

            Label {
                text: "Are you sure you want to switch to another project?"
                font { pixelSize: 16; family: "Segoe UI" }
                color: textColor
                wrapMode: Text.Wrap
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
            }

            Item {
                Layout.fillHeight: true
            }

            RowLayout {
                Layout.alignment: Qt.AlignRight
                spacing: 16

                Button {
                    text: "Cancel"
                    Layout.preferredWidth: 120
                    Layout.preferredHeight: 40
                    
                    background: Rectangle {
                        color: parent.hovered ? Qt.lighter(cardColor, 0.9) : "transparent"
                        radius: 8
                        border.color: dividerColor
                        border.width: 1
                    }
                    
                    contentItem: Text {
                        text: parent.text
                        color: accentColor
                        font.pixelSize: 14
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    
                    onClicked: {
                        confirmSwitchDialog.reject()
                        taskSwitchCancelled()
                    }
                }

                Button {
                    text: "Switch Task"
                    Layout.preferredWidth: 120
                    Layout.preferredHeight: 40
                    
                    background: Rectangle {
                        color: parent.hovered ? Qt.lighter(secondaryColor, 1.1) : secondaryColor
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
                    
                    onClicked: {
                        if (logger && taskId !== -1) {
                            logger.setActiveTask(confirmSwitchDialog.taskId)
                        }
                        confirmSwitchDialog.accept()
                        taskSwitchConfirmed(taskId)
                    }
                }
            }
        }
    }
}