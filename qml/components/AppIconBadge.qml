import QtQuick
import "../theme"

Rectangle {
    id: badgeRoot

    property string appName: ""
    property string url: ""
    property color accentColor: Theme.primaryColor
    property int badgeSize: 34
    property int iconSize: 22

    width: badgeSize
    height: badgeSize
    radius: Math.min(8, Math.round(badgeSize * 0.25))
    color: Qt.alpha(accentColor, 0.14)

    Image {
        id: iconImg
        anchors.centerIn: parent
        width: badgeRoot.iconSize
        height: badgeRoot.iconSize
        sourceSize.width: badgeRoot.iconSize * 2
        sourceSize.height: badgeRoot.iconSize * 2
        asynchronous: true
        fillMode: Image.PreserveAspectFit
        smooth: true
        mipmap: true
        source: (badgeRoot.appName || badgeRoot.url)
                ? ("image://appicon/resolve?app=" + encodeURIComponent(badgeRoot.appName) +
                   "&url=" + encodeURIComponent(badgeRoot.url))
                : ""
        visible: status === Image.Ready
    }

    Text {
        visible: iconImg.status !== Image.Ready
        text: (badgeRoot.appName || "?").charAt(0).toUpperCase()
        font.bold: true
        font.pixelSize: Math.max(11, Math.round(badgeRoot.badgeSize * 0.42))
        color: badgeRoot.accentColor
        anchors.centerIn: parent
    }
}
