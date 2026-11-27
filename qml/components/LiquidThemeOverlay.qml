import QtQuick
import QtQuick.Effects
import QtQuick.Shapes

Item {
    id: overlayRoot

    property Item targetItem: null
    property real progress: 0.0
    property bool active: false
    readonly property alias running: transitionAnim.running

    visible: active
    anchors.fill: parent
    z: 9999

    readonly property real waveW: width
    readonly property real waveH: height
    readonly property real waveDiag: Math.sqrt(waveW * waveW + waveH * waveH)
    readonly property real waveNx: waveDiag > 0 ? -waveH / waveDiag : 0
    readonly property real waveNy: waveDiag > 0 ? -waveW / waveDiag : 0
    readonly property real waveK: -0.08 + progress * 1.30
    readonly property real waveCx: waveW * (1.0 - waveK)
    readonly property real waveCy: waveH * (1.0 - waveK)
    readonly property real waveFade: Math.sin(Math.min(1.0, Math.max(0.0, progress)) * Math.PI)
    readonly property real waveAmp: Math.min(waveW, waveH) * 0.14 * waveFade
    readonly property real waveShift: Math.sin(progress * Math.PI * 2.5) * 0.12
    readonly property real dt: 0.58
    readonly property real tStart: -1.16 + waveShift

    function startWithToggle(toggleFn) {
        if (!targetItem || width <= 0 || height <= 0 || running) {
            if (toggleFn) toggleFn()
            return
        }

        targetItem.grabToImage(function(result) {
            if (!result || !result.url) {
                if (toggleFn) toggleFn()
                return
            }

            snapshotImage.source = result.url
            progress = 0.0
            active = true

            if (toggleFn) toggleFn()
            transitionAnim.restart()
        })
    }

    Image {
        id: snapshotImage
        anchors.fill: parent
        visible: false
        smooth: true
    }

    Shape {
        id: liquidMaskShape
        anchors.fill: parent
        visible: false
        layer.enabled: true
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            fillColor: "#000000"
            strokeColor: "transparent"
            strokeWidth: 0

            startX: overlayRoot.waveCx + overlayRoot.tStart * overlayRoot.waveW
            startY: overlayRoot.waveCy - overlayRoot.tStart * overlayRoot.waveH

            // 4 C1-continuous cubic wave lobes spanning the diagonal
            PathCubic {
                readonly property real t0: overlayRoot.tStart
                readonly property real t1: t0 + overlayRoot.dt
                control1X: overlayRoot.waveCx + (t0 + overlayRoot.dt / 3) * overlayRoot.waveW + overlayRoot.waveNx * overlayRoot.waveAmp
                control1Y: overlayRoot.waveCy - (t0 + overlayRoot.dt / 3) * overlayRoot.waveH + overlayRoot.waveNy * overlayRoot.waveAmp
                control2X: overlayRoot.waveCx + (t0 + 2 * overlayRoot.dt / 3) * overlayRoot.waveW + overlayRoot.waveNx * overlayRoot.waveAmp
                control2Y: overlayRoot.waveCy - (t0 + 2 * overlayRoot.dt / 3) * overlayRoot.waveH + overlayRoot.waveNy * overlayRoot.waveAmp
                x: overlayRoot.waveCx + t1 * overlayRoot.waveW
                y: overlayRoot.waveCy - t1 * overlayRoot.waveH
            }
            PathCubic {
                readonly property real t0: overlayRoot.tStart + overlayRoot.dt
                readonly property real t1: t0 + overlayRoot.dt
                control1X: overlayRoot.waveCx + (t0 + overlayRoot.dt / 3) * overlayRoot.waveW - overlayRoot.waveNx * overlayRoot.waveAmp
                control1Y: overlayRoot.waveCy - (t0 + overlayRoot.dt / 3) * overlayRoot.waveH - overlayRoot.waveNy * overlayRoot.waveAmp
                control2X: overlayRoot.waveCx + (t0 + 2 * overlayRoot.dt / 3) * overlayRoot.waveW - overlayRoot.waveNx * overlayRoot.waveAmp
                control2Y: overlayRoot.waveCy - (t0 + 2 * overlayRoot.dt / 3) * overlayRoot.waveH - overlayRoot.waveNy * overlayRoot.waveAmp
                x: overlayRoot.waveCx + t1 * overlayRoot.waveW
                y: overlayRoot.waveCy - t1 * overlayRoot.waveH
            }
            PathCubic {
                readonly property real t0: overlayRoot.tStart + 2 * overlayRoot.dt
                readonly property real t1: t0 + overlayRoot.dt
                control1X: overlayRoot.waveCx + (t0 + overlayRoot.dt / 3) * overlayRoot.waveW + overlayRoot.waveNx * overlayRoot.waveAmp
                control1Y: overlayRoot.waveCy - (t0 + overlayRoot.dt / 3) * overlayRoot.waveH + overlayRoot.waveNy * overlayRoot.waveAmp
                control2X: overlayRoot.waveCx + (t0 + 2 * overlayRoot.dt / 3) * overlayRoot.waveW + overlayRoot.waveNx * overlayRoot.waveAmp
                control2Y: overlayRoot.waveCy - (t0 + 2 * overlayRoot.dt / 3) * overlayRoot.waveH + overlayRoot.waveNy * overlayRoot.waveAmp
                x: overlayRoot.waveCx + t1 * overlayRoot.waveW
                y: overlayRoot.waveCy - t1 * overlayRoot.waveH
            }
            PathCubic {
                readonly property real t0: overlayRoot.tStart + 3 * overlayRoot.dt
                readonly property real t1: t0 + overlayRoot.dt
                control1X: overlayRoot.waveCx + (t0 + overlayRoot.dt / 3) * overlayRoot.waveW - overlayRoot.waveNx * overlayRoot.waveAmp
                control1Y: overlayRoot.waveCy - (t0 + overlayRoot.dt / 3) * overlayRoot.waveH - overlayRoot.waveNy * overlayRoot.waveAmp
                control2X: overlayRoot.waveCx + (t0 + 2 * overlayRoot.dt / 3) * overlayRoot.waveW - overlayRoot.waveNx * overlayRoot.waveAmp
                control2Y: overlayRoot.waveCy - (t0 + 2 * overlayRoot.dt / 3) * overlayRoot.waveH - overlayRoot.waveNy * overlayRoot.waveAmp
                x: overlayRoot.waveCx + t1 * overlayRoot.waveW
                y: overlayRoot.waveCy - t1 * overlayRoot.waveH
            }

            PathLine { x: overlayRoot.waveW + overlayRoot.waveDiag; y: -overlayRoot.waveDiag }
            PathLine { x: overlayRoot.waveW + overlayRoot.waveDiag; y: overlayRoot.waveH + overlayRoot.waveDiag }
            PathLine { x: -overlayRoot.waveDiag; y: overlayRoot.waveH + overlayRoot.waveDiag }
        }
    }

    MultiEffect {
        anchors.fill: parent
        source: snapshotImage
        maskEnabled: true
        maskSource: liquidMaskShape
        maskInverted: true
        maskThresholdMin: 0.5
        maskSpreadAtMin: 0.015
    }

    NumberAnimation {
        id: transitionAnim
        target: overlayRoot
        property: "progress"
        from: 0.0
        to: 1.0
        duration: 680
        easing.type: Easing.InOutQuart
        onFinished: {
            overlayRoot.active = false
            snapshotImage.source = ""
        }
    }
}
