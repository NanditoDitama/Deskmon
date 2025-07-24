import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15
import QtQuick.Controls.Material
import QtQuick
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Effects

Item {
    anchors.fill: parent
    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        Label {
            text: "Activity Monitor"
            font { bold: true; pixelSize: 16; family: "Segoe UI" }
            color: primaryColor
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: dividerColor
        }

        // Current Activity Section
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 8

            Label {
                text: "Current Window"
                font { bold: true; pixelSize: 14 }
                color: textColor
            }

            GridLayout {
                columns: 2
                columnSpacing: 12
                rowSpacing: 8
                Layout.fillWidth: true

                Label {
                    text: "Application:"
                    font.pixelSize: 12
                    color: lightTextColor
                }
                Label {
                    text: logger.currentAppName || "Unknown"
                    font.pixelSize: 12
                    color: textColor
                    elide: Text.ElideRight
                }

                Label {
                    text: "Window Title:"
                    font.pixelSize: 12
                    color: lightTextColor
                }
                Label {
                    text: logger.currentWindowTitle || "Unknown"
                    font.pixelSize: 12
                    color: textColor
                    elide: Text.ElideRight
                }
                Label {
                    text: "Total Logs:"
                    font.pixelSize: 12
                    color: lightTextColor
                }
                Label {
                    text: logger.logCount
                    color: lightTextColor
                    font.pixelSize: 12
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: dividerColor
        }

        // Recent Activity Section
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 8

            Label {
                text: "Recent Activity"
                font {
                    family: "Segoe UI"
                    weight: Font.Medium
                    pixelSize: 14
                }
                color: textColor
            }

            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true

                ListView {
                    id: activityListView
                    model: logger.logContent.split('\n').filter(line => line.trim() !== '').slice(0, 15)
                    spacing: 4
                    width: parent.width

                    header: RowLayout {
                        width: activityListView.width
                        spacing: 8

                        Label {
                            text: "Time"
                            font {
                                family: "Segoe UI"
                                weight: Font.DemiBold
                                pixelSize: 12
                            }
                            color: textColor
                            Layout.preferredWidth: 100
                        }

                        Label {
                            text: "Duration"
                            font {
                                family: "Segoe UI"
                                weight: Font.DemiBold
                                pixelSize: 12
                            }
                            color: textColor
                            Layout.preferredWidth: 80
                        }

                        Label {
                            text: "Application"
                            font {
                                family: "Segoe UI"
                                weight: Font.DemiBold
                                pixelSize: 12
                            }
                            color: textColor
                            Layout.preferredWidth: 150
                        }

                        Label {
                            text: "Title"
                            font {
                                family: "Segoe UI"
                                weight: Font.DemiBold
                                pixelSize: 12
                            }
                            color: textColor
                            Layout.fillWidth: true
                        }
                    }

                    delegate: Rectangle {
                        width: activityListView.width
                        height: 36
                        color: index % 2 === 0 ? cardColor : Qt.lighter(cardColor, 1.1)
                        radius: 4

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 8
                            anchors.rightMargin: 8
                            spacing: 8

                            Label {
                                text: {
                                    const parts = modelData.split(',')
                                    if (parts.length >= 4) {
                                        return parts[0].trim()
                                    }
                                    return "Unknown"
                                }
                                Layout.preferredWidth: 100
                                font {
                                    family: "Segoe UI"
                                    pixelSize: 11
                                }
                                color: lightTextColor
                                elide: Text.ElideRight
                            }

                            Label {
                                text: {
                                    const parts = modelData.split(',')
                                    if (parts.length >= 4) {
                                        const start = parts[0].trim()
                                        const end = parts[1].trim()
                                        const startTime = new Date("2000-01-01 " + start)
                                        const endTime = new Date("2000-01-01 " + end)
                                        const durationSec = (endTime - startTime) / 1000
                                        return Math.floor(durationSec/60) + "m " + (durationSec%60) + "s"
                                    }
                                    return ""
                                }
                                Layout.preferredWidth: 80
                                font {
                                    family: "Segoe UI"
                                    pixelSize: 11
                                }
                                color: lightTextColor
                            }

                            Label {
                                text: {
                                    const parts = modelData.split(',')
                                    if (parts.length >= 4) {
                                        return parts[2].trim()
                                    }
                                    return "Unknown"
                                }
                                Layout.preferredWidth: 150
                                font {
                                    family: "Segoe UI"
                                    pixelSize: 11
                                }
                                color: lightTextColor
                                elide: Text.ElideRight
                            }

                            Label {
                                text: {
                                    const parts = modelData.split(',')
                                    if (parts.length >= 4) {
                                        return parts[3].trim()
                                    }
                                    return ""
                                }
                                font {
                                    family: "Segoe UI"
                                    pixelSize: 11
                                }
                                color: lightTextColor
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
