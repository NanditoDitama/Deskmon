import QtQuick
import QtQuick.Controls
import "../theme"

Button {
    id: control

    property color btnColor: Theme.primaryColor
    property color btnHoverColor: Qt.lighter(Theme.primaryColor, 1.15)
    property color btnTextColor: "#FFFFFF"
    property real btnRadius: Theme.radiusSmall
    property string iconPath: ""
    property int iconSize: 18

    implicitHeight: 38
    implicitWidth: Math.max(contentItem.implicitWidth + 32, 80)

    font.pixelSize: Theme.fontSizeBody
    font.weight: Font.DemiBold

    background: Rectangle {
        implicitWidth: control.implicitWidth
        implicitHeight: control.implicitHeight
        radius: control.btnRadius
        color: control.down ? Qt.darker(control.btnColor, 1.15)
                            : (control.hovered ? control.btnHoverColor : control.btnColor)
        opacity: control.enabled ? 1.0 : 0.5

        Behavior on color {
            ColorAnimation { duration: 150 }
        }
    }

    contentItem: Row {
        spacing: 8
        anchors.centerIn: parent

        Image {
            id: btnIcon
            visible: control.iconPath !== ""
            source: control.iconPath
            width: control.iconSize
            height: control.iconSize
            anchors.verticalCenter: parent.verticalCenter
            smooth: true
            antialiasing: true
        }

        Text {
            text: control.text
            font: control.font
            color: control.btnTextColor
            anchors.verticalCenter: parent.verticalCenter
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }
}
