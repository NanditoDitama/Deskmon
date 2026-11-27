import QtQuick
import QtQuick.Templates as T
import "../theme"

T.Button {
    id: control

    property color btnColor: Theme.primaryColor
    property color btnHoverColor: Qt.lighter(Theme.primaryColor, 1.15)
    property color btnTextColor: "#FFFFFF"
    property color btnBorderColor: "transparent"
    property int btnBorderWidth: 0
    property real btnRadius: Theme.radiusSmall
    property string iconPath: ""
    property int iconSize: 18

    padding: 0
    leftPadding: 14
    rightPadding: 14
    topPadding: 0
    bottomPadding: 0

    implicitHeight: Theme.controlHeight
    implicitWidth: Math.max(contentRow.implicitWidth + leftPadding + rightPadding, 80)

    font.pixelSize: Theme.fontSizeSmall
    font.weight: Font.DemiBold

    background: Rectangle {
        implicitWidth: control.implicitWidth
        implicitHeight: control.implicitHeight
        radius: control.btnRadius
        border.color: control.btnBorderColor
        border.width: control.btnBorderWidth
        color: control.down ? Qt.darker(control.btnColor, 1.15)
                            : (control.hovered ? control.btnHoverColor : control.btnColor)
        opacity: control.enabled ? 1.0 : 0.5

        Behavior on color {
            ColorAnimation { duration: 150 }
        }
    }

    contentItem: Item {
        implicitWidth: contentRow.implicitWidth
        implicitHeight: contentRow.implicitHeight

        Row {
            id: contentRow
            anchors.centerIn: parent
            spacing: control.iconPath !== "" ? 8 : 0

            Image {
                id: btnIcon
                visible: control.iconPath !== ""
                source: control.iconPath
                width: visible ? control.iconSize : 0
                height: visible ? control.iconSize : 0
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
}
