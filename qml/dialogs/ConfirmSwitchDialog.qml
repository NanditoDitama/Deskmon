import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"
import "../components"

Dialog {
    id: root
    title: ""
    modal: true
    implicitWidth: 420
    implicitHeight: contentCol.implicitHeight + 48
    width: 420
    height: contentCol.implicitHeight + 48
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? Math.round((parent.height - height) / 2) : 0
    padding: 0
    topPadding: 0
    bottomPadding: 0
    leftPadding: 0
    rightPadding: 0
    topInset: 0
    bottomInset: 0
    leftInset: 0
    rightInset: 0
    property int taskId: -1
    property string warningMessage: ""

    signal confirmed(int targetTaskId)

    onClosed: warningMessage = ""

    background: Rectangle {
        color: "transparent"
        radius: Theme.radiusLarge
    }

    contentItem: Rectangle {
        implicitWidth: root.width
        implicitHeight: contentCol.implicitHeight + 48
        color: Theme.cardColor
        radius: Theme.radiusLarge
        border.width: 1
        border.color: Theme.borderColor

        ColumnLayout {
            id: contentCol
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 24
            spacing: 16

            // Icon + Title section
            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                Rectangle {
                    Layout.preferredWidth: 38
                    Layout.preferredHeight: 38
                    radius: Theme.radiusSmall
                    color: Qt.alpha(Theme.warningColor, 0.14)
                    border.width: 1
                    border.color: Qt.alpha(Theme.warningColor, 0.30)

                    Image {
                        anchors.centerIn: parent
                        source: "image://icon/danger.svg?" + Theme.warningColor
                        width: 18
                        height: 18
                        sourceSize.width: 64
                        sourceSize.height: 64
                        fillMode: Image.PreserveAspectFit
                        smooth: true
                        mipmap: true
                    }
                }

                Label {
                    text: Lang.t("Switch to Project?")
                    font {
                        pixelSize: 18
                        family: "Segoe UI"
                        weight: Font.DemiBold
                    }
                    color: Theme.textColor
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }

            // Message section
            Label {
                text: Lang.t("You have an active task. Do you want to switch to this task?")
                font {
                    pixelSize: Theme.fontSizeSmall
                    family: "Segoe UI"
                }
                color: Theme.lightTextColor
                wrapMode: Text.Wrap
                Layout.fillWidth: true
                lineHeight: 1.4
            }

            // Combined pending-task warning banner (shown when warningMessage is provided)
            Rectangle {
                visible: root.warningMessage !== ""
                Layout.fillWidth: true
                implicitHeight: warningRow.implicitHeight + 24
                radius: Theme.radiusSmall
                color: Qt.alpha(Theme.warningColor, 0.10)
                border.width: 1
                border.color: Qt.alpha(Theme.warningColor, 0.28)

                RowLayout {
                    id: warningRow
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 10

                    Image {
                        Layout.preferredWidth: 16
                        Layout.preferredHeight: 16
                        Layout.alignment: Qt.AlignTop
                        Layout.topMargin: 2
                        source: "image://icon/danger.svg?" + Theme.warningColor
                        sourceSize.width: 48
                        sourceSize.height: 48
                        fillMode: Image.PreserveAspectFit
                        smooth: true
                        mipmap: true
                    }

                    Label {
                        text: root.warningMessage
                        font {
                            pixelSize: 12
                            family: "Segoe UI"
                        }
                        color: Theme.textColor
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                        lineHeight: 1.4
                    }
                }
            }

            // Button section
            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: 4
                spacing: 12

                CustomButton {
                    id: cancelButton
                    text: Lang.t("Cancel")
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    Layout.preferredHeight: 36
                    btnColor: "transparent"
                    btnHoverColor: Qt.alpha(Theme.textColor, 0.08)
                    btnTextColor: Theme.textColor
                    btnBorderColor: Theme.borderColor
                    btnBorderWidth: 1
                    font.pixelSize: Theme.fontSizeSmall
                    font.family: "Segoe UI"
                    font.weight: Font.Medium

                    onClicked: root.reject()
                }

                CustomButton {
                    id: confirmButton
                    text: Lang.t("Switch Project")
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    Layout.preferredHeight: 36
                    btnColor: Theme.primaryColor
                    btnHoverColor: Theme.primaryHoverColor
                    btnTextColor: Theme.isDarkMode ? "#0F172A" : "#FFFFFF"
                    font.pixelSize: Theme.fontSizeSmall
                    font.family: "Segoe UI"
                    font.weight: Font.DemiBold

                    onClicked: {
                        root.confirmed(root.taskId)
                        root.accept()
                    }
                }
            }
        }
    }

    enter: Transition {
        ParallelAnimation {
            NumberAnimation {
                property: "opacity"
                from: 0
                to: 1
                duration: 200
                easing.type: Easing.OutCubic
            }
            NumberAnimation {
                property: "scale"
                from: 0.92
                to: 1.0
                duration: 250
                easing.type: Easing.OutCubic
            }
        }
    }

    exit: Transition {
        ParallelAnimation {
            NumberAnimation {
                property: "opacity"
                from: 1
                to: 0
                duration: 150
                easing.type: Easing.InCubic
            }
            NumberAnimation {
                property: "scale"
                from: 1.0
                to: 0.96
                duration: 150
                easing.type: Easing.InCubic
            }
        }
    }
}
