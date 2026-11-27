import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"
import "../components"

CustomDialog {
    id: root
    width: Math.min(parent ? parent.width * 0.9 : 600, 600)
    height: Math.min(parent ? parent.height * 0.7 : 500, 500)
    focus: true
    padding: 16
    dim: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    function show(title, description) {
        dialogTitle = title
        popupDescription.text = description
        open()
    }

    contentItem: ColumnLayout {
        spacing: 12

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            ScrollBar.vertical: CustomScrollBar {}
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

        CustomButton {
            id: closeBtn
            text: Lang.t("Close")
            Layout.alignment: Qt.AlignRight
            btnColor: Theme.secondaryColor
            btnHoverColor: Qt.lighter(Theme.secondaryColor, 1.1)
            btnTextColor: "white"
            font.pixelSize: Theme.fontSizeBody
            font.weight: Font.Medium
            onClicked: root.close()
        }
    }
}
