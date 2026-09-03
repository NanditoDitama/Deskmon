import QtQuick
import QtQuick.Controls
import "../theme"

Rectangle {
    id: root

    property string message: ""
    property string ntype: "info"
    property int duration: 3500
    property string iconSource: ""
    property string closeIcon: "qrc:/icons/close.svg"
    property bool leaving: false

    anchors.top: parent ? parent.top : undefined
    anchors.right: parent ? parent.right : undefined
    anchors.topMargin: 60
    anchors.rightMargin: 20

    width: Math.min(Math.max(contentRow.implicitWidth + 32, 240), 360)
    height: contentRow.implicitHeight + 34
    radius: Theme.radiusMedium
    color: Theme.cardColor
    border.width: 1.5
    border.color: {
        switch(ntype) {
        case "success": return Theme.successColor
        case "warning": return Theme.warningColor
        case "error": return Theme.dangerColor
        default: return Theme.dividerColor
        }
    }

    opacity: 0.0
    visible: opacity > 0

    Row {
        id: contentRow
        anchors.centerIn: parent
        spacing: 12

        Image {
            id: typeIcon
            width: 20
            height: 20
            anchors.verticalCenter: parent.verticalCenter
            source: root.iconSource
            smooth: true
            antialiasing: true
        }

        Text {
            id: messageText
            text: root.message
            width: root.width - 36 - 32 - 36 - 12
            anchors.verticalCenter: parent.verticalCenter
            color: Theme.textColor
            font.pixelSize: Theme.fontSizeTitle
            font.weight: Font.Medium
            wrapMode: Text.WordWrap
            elide: Text.ElideRight
            maximumLineCount: 2
        }

        MouseArea {
            id: closeArea
            width: 32
            height: 32
            anchors.verticalCenter: parent.verticalCenter
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: root.hide()

            Rectangle {
                anchors.centerIn: parent
                width: 28
                height: 28
                radius: Theme.radiusSmall
                color: closeArea.containsMouse ? (Theme.isDarkMode ? Qt.rgba(1,1,1,0.1) : Qt.rgba(0,0,0,0.06)) : "transparent"

                Behavior on color { ColorAnimation { duration: 150 } }

                Image {
                    anchors.centerIn: parent
                    width: 16
                    height: 16
                    source: root.closeIcon
                    smooth: true
                    antialiasing: true
                    opacity: closeArea.containsMouse ? 1.0 : 0.6
                    Behavior on opacity { NumberAnimation { duration: 150 } }
                }
            }
        }
    }

    Rectangle {
        id: progressBar
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.leftMargin: 1.5
        height: 2.5
        radius: 2
        width: parent.width - 3
        color: {
            switch(root.ntype) {
            case "success": return Theme.successColor
            case "warning": return Theme.warningColor
            case "error": return Theme.dangerColor
            default: return Theme.infoColor
            }
        }
        opacity: 0.4
    }

    Behavior on opacity {
        NumberAnimation { duration: 200; easing.type: Easing.OutCubic }
    }
    Behavior on anchors.rightMargin {
        NumberAnimation { duration: 300; easing.type: Easing.OutCubic }
    }
    scale: opacity
    Behavior on scale {
        NumberAnimation { duration: 200; easing.type: Easing.OutBack }
    }

    Timer {
        id: lifetime
        interval: root.duration
        repeat: false
        onTriggered: root.hide()
    }

    PropertyAnimation {
        id: progressAnim
        target: progressBar
        property: "width"
        from: root.width - 3
        to: 0
        duration: root.duration
        easing.type: Easing.Linear
    }

    function show(type, msg) {
        ntype = type
        message = msg
        leaving = false

        switch(type) {
        case "success":
            iconSource = "qrc:/icons/check.svg"
            break
        case "warning":
            iconSource = "qrc:/icons/danger.svg"
            break
        case "error":
            iconSource = "qrc:/icons/danger.svg"
            break
        default:
            iconSource = "qrc:/icons/review.svg"
            break
        }

        var wordCount = msg.split(' ').length
        if (wordCount > 15) {
            duration = 5000
        } else {
            duration = 3500
        }
        lifetime.interval = duration
        progressAnim.duration = duration

        opacity = 1.0
        scale = 1.0
        progressBar.width = root.width - 3
        progressAnim.restart()
        lifetime.restart()
    }

    function hide() {
        if (leaving) return
        leaving = true
        lifetime.stop()
        progressAnim.stop()
        opacity = 0.0
        scale = 0.95
    }

    onOpacityChanged: {
        if (opacity === 0.0 && leaving) {
            leaving = false
        }
    }
}
