import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Effects
import "../theme"

Item {
    anchors.fill: parent

    function parseTimeSeconds(t) {
        if (!t) return -1
        const p = t.split(':')
        if (p.length < 2) return -1
        const h = parseInt(p[0], 10)
        const m = parseInt(p[1], 10)
        const s = p.length >= 3 ? parseInt(p[2], 10) : 0
        if (isNaN(h) || isNaN(m) || isNaN(s)) return -1
        return h * 3600 + m * 60 + s
    }

    function getTopRecentLogs(content, maxCount) {
        if (!content) return []
        const result = []
        let pos = 0
        const len = content.length
        while (pos < len && result.length < maxCount) {
            const nl = content.indexOf('\n', pos)
            const line = (nl === -1) ? content.substring(pos) : content.substring(pos, nl)
            pos = (nl === -1) ? len : nl + 1
            if (line.trim() === '') continue
            const parts = line.split(',')
            if (parts.length >= 4) {
                const start = parts[0].trim()
                const end = parts[1].trim()
                const startSec = parseTimeSeconds(start)
                const endSec = parseTimeSeconds(end)
                let durationText = ""
                if (startSec >= 0 && endSec >= 0) {
                    const durationSec = endSec - startSec
                    durationText = Math.floor(durationSec / 60) + "m " + (durationSec % 60) + "s"
                }
                result.push({
                    time: start,
                    durationText: durationText,
                    app: parts[2].trim(),
                    title: parts[3].trim()
                })
            } else {
                result.push({
                    time: "Unknown",
                    durationText: "",
                    app: "Unknown",
                    title: ""
                })
            }
        }
        return result
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        Label {
            text: Lang.t("Activity Monitor")
            font { bold: true; pixelSize: Theme.fontSizeTitle; family: "Segoe UI" }
            color: Theme.primaryColor
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Theme.dividerColor
        }

        // Current Activity Section
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 8

            Label {
                text: Lang.t("Current Window")
                font { bold: true; pixelSize: Theme.fontSizeBody }
                color: Theme.textColor
            }

            GridLayout {
                columns: 2
                columnSpacing: 12
                rowSpacing: 8
                Layout.fillWidth: true

                Label {
                    text: Lang.t("Application") + ":"
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.lightTextColor
                }
                Label {
                    text: logger.currentAppName || Lang.t("Unknown")
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.textColor
                    elide: Text.ElideRight
                }

                Label {
                    text: Lang.t("Window Title") + ":"
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.lightTextColor
                }
                Label {
                    text: logger.currentWindowTitle || Lang.t("Unknown")
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.textColor
                    elide: Text.ElideRight
                }
                Label {
                    text: Lang.t("Total Logs") + ":"
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.lightTextColor
                }
                Label {
                    text: logger.logCount
                    color: Theme.lightTextColor
                    font.pixelSize: Theme.fontSizeSmall
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Theme.dividerColor
        }

        // Recent Activity Section
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 8

            Label {
                text: Lang.t("Recent Activity")
                font {
                    family: "Segoe UI"
                    weight: Font.Medium
                    pixelSize: Theme.fontSizeBody
                }
                color: Theme.textColor
            }

            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true

                ListView {
                    id: activityListView
                    model: getTopRecentLogs(logger.logContent, 15)
                    spacing: 4
                    width: parent.width

                    header: RowLayout {
                        width: activityListView.width
                        spacing: 8

                        Label {
                            text: Lang.t("Time")
                            font {
                                family: "Segoe UI"
                                weight: Font.DemiBold
                                pixelSize: Theme.fontSizeSmall
                            }
                            color: Theme.textColor
                            Layout.preferredWidth: 100
                        }

                        Label {
                            text: Lang.t("Duration")
                            font {
                                family: "Segoe UI"
                                weight: Font.DemiBold
                                pixelSize: Theme.fontSizeSmall
                            }
                            color: Theme.textColor
                            Layout.preferredWidth: 80
                        }

                        Label {
                            text: Lang.t("Application")
                            font {
                                family: "Segoe UI"
                                weight: Font.DemiBold
                                pixelSize: Theme.fontSizeSmall
                            }
                            color: Theme.textColor
                            Layout.preferredWidth: 150
                        }

                        Label {
                            text: Lang.t("Title")
                            font {
                                family: "Segoe UI"
                                weight: Font.DemiBold
                                pixelSize: Theme.fontSizeSmall
                            }
                            color: Theme.textColor
                            Layout.fillWidth: true
                        }
                    }

                    delegate: Rectangle {
                        width: activityListView.width
                        height: 36
                        color: index % 2 === 0 ? Theme.cardColor : Qt.lighter(Theme.cardColor, 1.1)
                        radius: 4

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 8
                            anchors.rightMargin: 8
                            spacing: 8

                            Label {
                                text: modelData.time
                                Layout.preferredWidth: 100
                                font {
                                    family: "Segoe UI"
                                    pixelSize: Theme.fontSizeSmall
                                }
                                color: Theme.lightTextColor
                                elide: Text.ElideRight
                            }

                            Label {
                                text: modelData.durationText
                                Layout.preferredWidth: 80
                                font {
                                    family: "Segoe UI"
                                    pixelSize: Theme.fontSizeSmall
                                }
                                color: Theme.lightTextColor
                            }

                            Label {
                                text: modelData.app
                                Layout.preferredWidth: 150
                                font {
                                    family: "Segoe UI"
                                    pixelSize: Theme.fontSizeSmall
                                }
                                color: Theme.lightTextColor
                                elide: Text.ElideRight
                            }

                            Label {
                                text: modelData.title
                                font {
                                    family: "Segoe UI"
                                    pixelSize: Theme.fontSizeSmall
                                }
                                color: Theme.lightTextColor
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                        }
                    }
                }
            }
        }
    }
}
