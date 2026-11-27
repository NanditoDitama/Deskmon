import QtQuick
import QtQuick.Templates as T
import "../theme"

T.TextField {
    id: control

    property string leftIconSource: ""
    property real fieldRadius: Theme.radiusSmall
    property color fieldBackgroundColor: Theme.surfaceColor
    property color fieldBorderColor: Theme.borderColor
    property color fieldFocusBorderColor: Theme.primaryColor

    implicitWidth: 200
    implicitHeight: Theme.controlHeight
    leftPadding: leftIconSource !== "" ? 40 : 12
    rightPadding: 12
    topPadding: 0
    bottomPadding: 0
    verticalAlignment: TextInput.AlignVCenter
    font.pixelSize: Theme.fontSizeBody
    font.family: "Segoe UI"
    color: Theme.textColor
    placeholderTextColor: Theme.mutedTextColor
    selectionColor: Qt.alpha(Theme.primaryColor, 0.3)
    selectedTextColor: Theme.textColor
    selectByMouse: true
    clip: true

    background: Rectangle {
        radius: control.fieldRadius
        color: control.fieldBackgroundColor
        border.color: control.activeFocus ? control.fieldFocusBorderColor : control.fieldBorderColor
        border.width: control.activeFocus ? 2 : 1

        Image {
            visible: control.leftIconSource !== ""
            source: control.leftIconSource
            width: 16
            height: 16
            sourceSize.width: 48
            sourceSize.height: 48
            fillMode: Image.PreserveAspectFit
            smooth: true
            mipmap: true
            anchors.left: parent.left
            anchors.leftMargin: 12
            anchors.verticalCenter: parent.verticalCenter
        }

        Text {
            visible: control.text.length === 0 && control.placeholderText.length > 0
            text: control.placeholderText
            font: control.font
            color: control.placeholderTextColor
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: control.leftPadding
            anchors.rightMargin: control.rightPadding
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}
