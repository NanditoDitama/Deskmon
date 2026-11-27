import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"

Dialog {
    id: dialogRoot

    property string dialogTitle: ""
    property bool showHeader: dialogTitle !== ""
    property bool showCloseButton: true

    modal: true
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? Math.round((parent.height - height) / 2) : 0

    background: Rectangle {
        color: Theme.cardColor
        radius: Theme.radiusLarge
        border.color: Theme.borderColor
        border.width: 1
    }

    header: Item {
        visible: dialogRoot.showHeader
        implicitHeight: dialogRoot.showHeader ? 56 : 0

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: Theme.dialogPadding
            anchors.rightMargin: Theme.dialogPadding

            Label {
                text: dialogRoot.dialogTitle
                font.pixelSize: 18
                font.weight: Font.DemiBold
                font.family: "Segoe UI"
                color: Theme.textColor
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            Button {
                id: dialogCloseBtn
                visible: dialogRoot.showCloseButton
                Layout.preferredWidth: 32
                Layout.preferredHeight: 32
                topInset: 0
                bottomInset: 0
                leftInset: 0
                rightInset: 0
                padding: 0
                flat: true
                onClicked: dialogRoot.close()
                background: Rectangle {
                    radius: 8
                    color: dialogCloseBtn.hovered ? Qt.alpha(Theme.textColor, 0.08) : "transparent"
                }
                contentItem: Text {
                    text: "×"
                    font.pixelSize: 20
                    color: Theme.lightTextColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }

        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: 1
            color: Theme.dividerColor
        }
    }
}
