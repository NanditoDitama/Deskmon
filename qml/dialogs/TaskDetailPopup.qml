import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"

Popup {
    id: root
    width: Math.min(parent ? parent.width * 0.9 : 600, 600)
    height: Math.min(parent ? parent.height * 0.7 : 500, 500)
    x: parent ? (parent.width - width) / 2 : 0
    y: parent ? (parent.height - height) / 2 : 0
    modal: true
    focus: true
    padding: 16
    dim: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    background: Rectangle {
        color: Theme.cardColor
        radius: Theme.radiusSmall
        border.color: Theme.dividerColor
        border.width: 1
    }

    function show(title, description) {
        popupTitle.text = title
        popupDescription.text = description
        open()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        Label {
            id: popupTitle
            Layout.fillWidth: true
            font { bold: true; pixelSize: 18; family: "Segoe UI" }
            color: Theme.primaryColor
            wrapMode: Text.Wrap
            maximumLineCount: 2
            elide: Text.ElideRight
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Theme.dividerColor
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            ScrollBar.horizontal.policy: ScrollBar.AsNeeded

            TextArea {
                id: popupDescription
                width: parent.width
                wrapMode: Text.Wrap
                readOnly: true
                selectByMouse: true
                font.pixelSize: Theme.fontSizeBody
                color: Theme.textColor
                background: null
                padding: 0
                textFormat: Text.PlainText
            }
        }

        Button {
            id: closeBtn
            text: Lang.t("Close")
            Layout.alignment: Qt.AlignRight
            leftPadding: 20
            rightPadding: 20
            topPadding: 8
            bottomPadding: 8

            background: Rectangle {
                radius: Theme.radiusSmall
                color: closeBtn.hovered ? Qt.lighter(Theme.secondaryColor, 1.1) : Theme.secondaryColor
                Behavior on color { ColorAnimation { duration: 150 } }
            }

            contentItem: Text {
                text: closeBtn.text
                font.pixelSize: Theme.fontSizeBody
                font.weight: Font.Medium
                color: "white"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            onClicked: root.close()
        }
    }
}
