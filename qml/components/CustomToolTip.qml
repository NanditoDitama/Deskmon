import QtQuick
import QtQuick.Controls
import QtQuick.Window

Popup {
    id: root

    property string text: ""
    property Item target: parent
    property bool shown: false
    property int delay: 350
    property int timeout: 4000
    // "auto", "top" (arrow di atas menunjuk ke atas), "bottom" (arrow di bawah menunjuk ke bawah)
    property string arrowSide: "auto"

    readonly property int arrowWidth: 12
    readonly property int arrowHeight: 6
    readonly property int cornerRadius: 8
    readonly property int padX: 12
    readonly property int padY: 7

    property bool isTopArrow: true
    property real arrowCenterX: width / 2

    padding: 0
    margins: 0
    modal: false
    focus: false
    dim: false
    closePolicy: Popup.NoAutoClose
    enabled: false

    implicitWidth: Math.min(Math.max(Math.ceil(labelText.implicitWidth) + padX * 2, 48), 280)
    implicitHeight: Math.ceil(labelText.implicitHeight) + padY * 2 + arrowHeight

    function updatePosition() {
        if (!target) return

        var win = target.Window ? target.Window.window : null
        var winW = win ? win.width : 1280
        var winH = win ? win.height : 800

        var globalPos = target.mapToItem(null, 0, 0)
        var targetW = target.width
        var targetH = target.height

        if (arrowSide === "top") {
            isTopArrow = true
        } else if (arrowSide === "bottom") {
            isTopArrow = false
        } else {
            // Jika objek berada di area atas window (mis. Header), tampilkan tooltip di bawah objek
            // dengan panah di atas menunjuk ke objek. Selain itu tampilkan di atas objek.
            isTopArrow = (globalPos.y < implicitHeight + 24) || (globalPos.y + targetH / 2 < winH * 0.15)
        }

        var idealX = (targetW - implicitWidth) / 2
        var minLocalX = 8 - globalPos.x
        var maxLocalX = winW - 8 - globalPos.x - implicitWidth
        var clampedX = Math.round(Math.max(minLocalX, Math.min(maxLocalX, idealX)))

        root.x = clampedX
        root.y = isTopArrow ? (targetH + 6) : (-implicitHeight - 6)

        // Hitung titik tengah objek target dalam koordinat lokal tooltip agar panah selalu tepat menunjuk objek
        var localTargetCenterX = (targetW / 2) - clampedX
        var minArrowX = cornerRadius + arrowWidth / 2 + 2
        var maxArrowX = implicitWidth - cornerRadius - arrowWidth / 2 - 2
        arrowCenterX = Math.max(minArrowX, Math.min(maxArrowX, localTargetCenterX))
        bubbleCanvas.requestPaint()
    }

    onShownChanged: {
        if (shown && root.text !== "") {
            delayTimer.restart()
        } else {
            delayTimer.stop()
            hideTimer.stop()
            if (root.visible) root.close()
        }
    }

    onTextChanged: {
        if (root.visible) {
            Qt.callLater(updatePosition)
        }
    }

    onWidthChanged: if (root.visible) Qt.callLater(updatePosition)
    onHeightChanged: if (root.visible) Qt.callLater(updatePosition)

    Timer {
        id: delayTimer
        interval: Math.max(0, root.delay)
        repeat: false
        onTriggered: {
            if (root.shown && root.text !== "") {
                root.updatePosition()
                root.open()
                if (root.timeout > 0) {
                    hideTimer.restart()
                }
            }
        }
    }

    Timer {
        id: hideTimer
        interval: Math.max(500, root.timeout)
        repeat: false
        onTriggered: root.close()
    }

    enter: Transition {
        ParallelAnimation {
            NumberAnimation {
                property: "opacity"
                from: 0.0
                to: 1.0
                duration: 160
                easing.type: Easing.OutCubic
            }
            NumberAnimation {
                property: "scale"
                from: 0.92
                to: 1.0
                duration: 180
                easing.type: Easing.OutBack
                easing.overshoot: 1.1
            }
        }
    }

    exit: Transition {
        NumberAnimation {
            property: "opacity"
            from: 1.0
            to: 0.0
            duration: 110
            easing.type: Easing.InCubic
        }
    }

    background: Item {
        Canvas {
            id: bubbleCanvas
            anchors.fill: parent
            antialiasing: true

            onPaint: {
                var ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                if (width <= 0 || height <= 0) return

                var r = root.cornerRadius
                var aw = root.arrowWidth
                var ah = root.arrowHeight
                var ax = Math.max(r + aw / 2, Math.min(width - r - aw / 2, root.arrowCenterX))

                var left = 1
                var right = width - 1
                var top = root.isTopArrow ? ah + 1 : 1
                var bottom = root.isTopArrow ? height - 1 : height - ah - 1

                ctx.beginPath()
                ctx.moveTo(left + r, top)

                // Sisi atas (+ panah atas jika isTopArrow == true)
                if (root.isTopArrow) {
                    ctx.lineTo(ax - aw / 2, top)
                    ctx.lineTo(ax, 1)
                    ctx.lineTo(ax + aw / 2, top)
                }
                ctx.lineTo(right - r, top)
                ctx.quadraticCurveTo(right, top, right, top + r)

                // Sisi kanan
                ctx.lineTo(right, bottom - r)
                ctx.quadraticCurveTo(right, bottom, right - r, bottom)

                // Sisi bawah (+ panah bawah jika isTopArrow == false)
                if (!root.isTopArrow) {
                    ctx.lineTo(ax + aw / 2, bottom)
                    ctx.lineTo(ax, height - 1)
                    ctx.lineTo(ax - aw / 2, bottom)
                }
                ctx.lineTo(left + r, bottom)
                ctx.quadraticCurveTo(left, bottom, left, bottom - r)

                // Sisi kiri
                ctx.lineTo(left, top + r)
                ctx.quadraticCurveTo(left, top, left + r, top)
                ctx.closePath()

                // Background hitam polos tanpa border
                ctx.fillStyle = "#000000"
                ctx.fill()
            }
        }
    }

    contentItem: Item {
        implicitWidth: root.implicitWidth
        implicitHeight: root.implicitHeight

        Text {
            id: labelText
            text: root.text
            width: Math.min(implicitWidth, 256)
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: root.isTopArrow ? (root.arrowHeight / 2) : (-root.arrowHeight / 2)
            color: "#FFFFFF"
            font.family: "Segoe UI"
            font.pixelSize: 12
            font.weight: Font.Medium
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
