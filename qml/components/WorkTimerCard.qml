import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"

Rectangle {
    id: root
    Layout.fillWidth: true
    Layout.preferredHeight: 90
    radius: Theme.radiusMedium
    color: Qt.alpha(Theme.cardColor, 0.03)

    property string clockIn: "--:--"
    property string clockOut: "--:--"

    QtObject {
        id: workTimer
        property int elapsedSeconds: (typeof logger !== "undefined") ? logger.workTimeElapsedSeconds : 0
        property int totalWorkSeconds: 33120 // 9 jam dalam detik (9 * 3600)

        function getFormattedElapsed() {
            if (elapsedSeconds < 0) return "0h 0m"
            var hours = Math.floor(elapsedSeconds / 3600)
            var minutes = Math.floor((elapsedSeconds % 3600) / 60)
            return hours + "h " + minutes + "m"
        }

        function getProgress() {
            return Math.min(1.0, elapsedSeconds / totalWorkSeconds)
        }
    }

    Connections {
        target: (typeof logger !== "undefined") ? logger : null
        function onWorkTimeElapsedSecondsChanged() {
            if (typeof logger !== "undefined")
                workTimer.elapsedSeconds = logger.workTimeElapsedSeconds
        }
    }

    ColumnLayout {
        anchors.fill: parent

        // Header dengan elapsed time
        RowLayout {
            Layout.fillWidth: true
            spacing: 5

            Rectangle {
                width: 3
                height: 14
                radius: 1.5
                color: Theme.primaryColor
            }

            Label {
                text: "Time At Work"
                font {
                    pixelSize: 11
                    weight: Font.DemiBold
                    letterSpacing: 0.3
                }
                color: Qt.alpha(Theme.primaryColor, 0.9)
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            // Elapsed Time
            Label {
                text: workTimer.getFormattedElapsed()
                font {
                    pixelSize: 15
                    weight: Font.Bold
                    letterSpacing: 1
                    family: "Consolas, Monaco, monospace"
                }
                color: workTimer.elapsedSeconds >= 33120 ? Theme.successColor : Qt.alpha(Theme.textColor, 0.9)
                Layout.alignment: Qt.AlignVCenter
            }

            // Percentage badge
            Rectangle {
                Layout.preferredWidth: 50
                Layout.preferredHeight: 18
                radius: Theme.radiusSmall
                color: workTimer.elapsedSeconds >= 32400 ?
                           Qt.alpha(Theme.successColor, 0.12) :
                           Qt.alpha(Theme.primaryColor, 0.08)
                border.width: 1
                border.color: workTimer.elapsedSeconds >= 32400 ?
                                   Qt.alpha(Theme.successColor, 0.25) :
                                   Qt.alpha(Theme.primaryColor, 0.2)

                Label {
                    anchors.centerIn: parent
                    text: Math.round(workTimer.getProgress() * 100) + "%"
                    font {
                        pixelSize: 10
                        weight: Font.Bold
                    }
                    color: workTimer.elapsedSeconds >= 32400 ? Theme.successColor : Theme.primaryColor
                }
                Layout.alignment: Qt.AlignVCenter
            }
        }

        // Clock In/Out row di bawah
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            // Clock In
            Rectangle {
                Layout.preferredWidth: 60
                Layout.preferredHeight: 24
                radius: Theme.radiusSmall
                color: Qt.alpha(Theme.primaryColor, 0.06)
                border.width: 1
                border.color: Qt.alpha(Theme.primaryColor, 0.15)

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    spacing: 4

                    Label {
                        text: "↓"
                        font.pixelSize: 10
                        color: Theme.primaryColor
                        rotation: 45
                    }

                    Label {
                        text: root.clockIn
                        font {
                            pixelSize: 10
                            weight: Font.Bold
                            family: "Consolas, Monaco, monospace"
                        }
                        color: Theme.primaryColor
                        Layout.fillWidth: true
                    }
                }

                MouseArea {
                    id: clockInArea
                    anchors.fill: parent
                    hoverEnabled: true
                }

                ToolTip.visible: clockInArea.containsMouse
                ToolTip.text: {
                    if (root.clockIn === "--:--") {
                        return "Menunggu data jam masuk...";
                    } else {
                        return "Anda tercatat masuk pada jam " + root.clockIn;
                    }
                }
            }

            Item { Layout.fillWidth: true }

            Label {
                text: "Target: 9h"
                font {
                    pixelSize: 9
                    weight: Font.Medium
                    letterSpacing: 0.1
                }
                color: Qt.alpha(Theme.textColor, 0.5)
            }

            Item { Layout.fillWidth: true }

            // Clock Out
            Rectangle {
                Layout.preferredWidth: root.clockOut === "Online" ? 65 : 60
                Layout.preferredHeight: 24
                radius: Theme.radiusSmall
                color: root.clockOut === "Online" ?
                           Qt.alpha(Theme.successColor, 0.08) :
                           Qt.alpha(Theme.primaryColor, 0.06)
                border.width: 1
                border.color: root.clockOut === "Online" ?
                                  Qt.alpha(Theme.successColor, 0.2) :
                                  Qt.alpha(Theme.primaryColor, 0.15)

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    spacing: 4

                    Rectangle {
                        visible: root.clockOut === "Online"
                        width: 5
                        height: 5
                        radius: 2.5
                        color: Theme.successColor

                        SequentialAnimation on opacity {
                            running: root.clockOut === "Online"
                            loops: Animation.Infinite
                            NumberAnimation { from: 1; to: 0.3; duration: 800; easing.type: Easing.InOutSine }
                            NumberAnimation { from: 0.3; to: 1; duration: 800; easing.type: Easing.InOutSine }
                        }

                        Rectangle {
                            anchors.centerIn: parent
                            width: parent.width + 4
                            height: parent.height + 4
                            radius: width/2
                            color: "transparent"
                            border.width: 1
                            border.color: Theme.successColor
                            opacity: 0

                            SequentialAnimation on opacity {
                                running: root.clockOut === "Online"
                                loops: Animation.Infinite
                                NumberAnimation { from: 0.6; to: 0; duration: 1200; easing.type: Easing.OutCubic }
                            }

                            SequentialAnimation on scale {
                                running: root.clockOut === "Online"
                                loops: Animation.Infinite
                                NumberAnimation { from: 0.8; to: 1.4; duration: 1200; easing.type: Easing.OutCubic }
                            }
                        }
                    }

                    Label {
                        text: root.clockOut === "Online" ? "Online" : root.clockOut
                        font {
                            pixelSize: 10
                            weight: Font.Bold
                            family: root.clockOut === "Online" ? "Segoe UI" : "Consolas, Monaco, monospace"
                        }
                        color: root.clockOut === "Online" ? Theme.successColor : Theme.primaryColor
                        Layout.fillWidth: true
                    }

                    Label {
                        visible: root.clockOut !== "Online" && root.clockOut !== "--:--" && root.clockOut !== "Error"
                        text: "↑"
                        font.pixelSize: 10
                        color: Theme.primaryColor
                        rotation: 45
                    }
                }

                MouseArea {
                    id: clockOutArea
                    anchors.fill: parent
                    hoverEnabled: true
                }

                ToolTip.visible: clockOutArea.containsMouse
                ToolTip.text: {
                    if (root.clockOut === "Online") {
                        return "Anda sedang dalam jam kerja";
                    } else if (root.clockOut === "Error") {
                        return "Koneksi gagal, tidak bisa mengambil data";
                    } else if (root.clockOut === "--:--") {
                        return "Menunggu data jam keluar...";
                    } else {
                        return "Anda tercatat keluar pada jam " + root.clockOut;
                    }
                }
            }
        }
    }
}
