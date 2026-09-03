import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material
import QtQuick.Dialogs
import QtQuick.Effects
import QtQuick.Window
import "../theme"

Dialog {
    id: dateRangeDialog
    modal: true
    width: parent ? Math.min(600, parent.width * 0.9) : 600
    height: parent ? Math.min(600, parent.height * 0.8) : 550
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? Math.round((parent.height - height) / 2) : 0
    padding: 0
    dim: true

    property int currentMonth: new Date().getMonth()
    property int currentYear: new Date().getFullYear()

    background: Rectangle {
        color: Theme.cardColor
        radius: Theme.radiusLarge
        border.color: Theme.dividerColor
        border.width: 1
        layer.enabled: true
        Rectangle {
            anchors.fill: parent
            z: -1
            gradient: Gradient {
                GradientStop { position: 0.0; color: Qt.rgba(0,0,0,0.1) }
                GradientStop { position: 0.2; color: Qt.rgba(0,0,0,0.05) }
                GradientStop { position: 1.0; color: "transparent" }
            }
            radius: Theme.radiusLarge
        }
    }

    contentItem: Rectangle {
        color: "transparent"
        anchors.fill: parent

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 20

            // Header
            Label {
                text: Lang.t("Select Date Range")
                font {
                    bold: true;
                    pixelSize: Theme.fontSizeHeader;
                    family: "Segoe UI"
                }
                color: Theme.primaryColor
                Layout.alignment: Qt.AlignHCenter
                Layout.bottomMargin: 10
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 20

                // Quick Selection Panel
                ColumnLayout {
                    Layout.preferredWidth: 150
                    spacing: 10

                    Label {
                        text: Lang.t("Quick Select")
                        font {
                            pixelSize: Theme.fontSizeBody;
                            bold: true
                        }
                        color: Theme.textColor
                        opacity: 0.8
                        Layout.bottomMargin: 5
                    }

                    Repeater {
                        model: [
                            { text: Lang.t("Today"), range: 0 },
                            { text: Lang.t("Yesterday"), range: 1 },
                            { text: Lang.t("This Week"), range: 2 },
                            { text: Lang.t("Last Week"), range: 3 },
                            { text: Lang.t("This Month"), range: 4 },
                            { text: Lang.t("Last Month"), range: 5 }
                        ]

                        Button {
                            id: presetBtn
                            text: modelData.text
                            Layout.fillWidth: true
                            Layout.preferredHeight: 40
                            background: Rectangle {
                                radius: Theme.radiusSmall
                                color: presetBtn.hovered ? Qt.alpha(Theme.primaryColor, 0.15) : (index % 2 === 0 ? Qt.lighter(Theme.cardColor, 1.05) : Theme.cardColor)
                                border.color: Theme.dividerColor
                                border.width: 1
                                Behavior on color { ColorAnimation { duration: 150 } }
                            }
                            contentItem: Text {
                                text: presetBtn.text
                                font.pixelSize: Theme.fontSizeBody
                                color: Theme.textColor
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            onClicked: {
                                var today = new Date()
                                var date = new Date(today)

                                switch(modelData.range) {
                                case 0: // Today
                                    startSelectedDate = new Date(date)
                                    endSelectedDate = new Date(date)
                                    break
                                case 1: // Yesterday
                                    date.setDate(date.getDate() - 1)
                                    startSelectedDate = new Date(date)
                                    endSelectedDate = new Date(date)
                                    break
                                case 2: // This Week
                                    var daysSinceSunday = date.getDay();
                                    startSelectedDate = new Date(date);
                                    startSelectedDate.setDate(date.getDate() - daysSinceSunday);
                                    endSelectedDate = new Date(date);
                                    break;
                                case 3: // Last Week
                                    var daysSinceSunday = date.getDay();
                                        var thisWeekSunday = new Date(date);
                                        thisWeekSunday.setDate(date.getDate() - daysSinceSunday);
                                        startSelectedDate = new Date(thisWeekSunday);
                                        startSelectedDate.setDate(thisWeekSunday.getDate() - 7);
                                        endSelectedDate = new Date(startSelectedDate);
                                        endSelectedDate.setDate(startSelectedDate.getDate() + 6);
                                        break;
                                case 4: // This Month
                                    startSelectedDate = new Date(date.getFullYear(), date.getMonth(), 1)
                                    endSelectedDate = new Date(date)
                                    break
                                case 5: // Last Month
                                    startSelectedDate = new Date(date.getFullYear(), date.getMonth() - 1, 1)
                                    endSelectedDate = new Date(date.getFullYear(), date.getMonth(), 0)
                                    break
                                }

                                isDateSelected = true
                                applyDateRange()
                                dateRangeDialog.accept()
                            }
                        }
                    }
                }

                // Calendar View
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 10

                    // Month/Year Navigation
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        Button {
                            icon.source: "qrc:/icons/chevron-left.svg"
                            icon.color: Theme.textColor
                            icon.width: 18
                            icon.height: 18
                            flat: true
                            onClicked: {
                                if (currentMonth === 0) {
                                    currentMonth = 11
                                    currentYear -= 1
                                } else {
                                    currentMonth -= 1
                                }
                            }
                        }

                        Label {
                            text: Qt.formatDate(new Date(currentYear, currentMonth), "MMMM yyyy")
                            font {
                                bold: true;
                                pixelSize: 18;
                                family: "Segoe UI"
                            }
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignHCenter
                            color: Theme.textColor
                        }

                        Button {
                            icon.source: "qrc:/icons/chevron-right.svg"
                            icon.color: Theme.textColor
                            icon.width: 18
                            icon.height: 18
                            flat: true
                            onClicked: {
                                if (currentMonth === 11) {
                                    currentMonth = 0
                                    currentYear += 1
                                } else {
                                    currentMonth += 1
                                }
                            }
                        }
                    }

                    // Day Names Header
                    GridLayout {
                        columns: 7
                        rowSpacing: 5
                        columnSpacing: 5
                        Layout.fillWidth: true

                        Repeater {
                            model: ["S", "M", "T", "W", "T", "F", "S"]
                            Label {
                                text: modelData
                                font {
                                    pixelSize: Theme.fontSizeBody;
                                    bold: true
                                }
                                color: Theme.lightTextColor
                                Layout.fillWidth: true
                                horizontalAlignment: Text.AlignHCenter
                            }
                        }
                    }

                    // Calendar Days
                    GridLayout {
                        id: calendarGrid
                        columns: 7
                        rows: 6
                        columnSpacing: 5
                        rowSpacing: 5
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        property int daysInMonth: new Date(currentYear, currentMonth + 1, 0).getDate()
                        property int firstDay: new Date(currentYear, currentMonth, 1).getDay()

                        Repeater {
                            model: 42
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                radius: 4

                                property int day: {
                                    var dayIndex = index - calendarGrid.firstDay + 1
                                    if (dayIndex <= 0 || dayIndex > calendarGrid.daysInMonth) return 0
                                    return dayIndex
                                }

                                property date dayDate: new Date(currentYear, currentMonth, day)
                                property bool isToday: day ? dayDate.toDateString() === new Date().toDateString() : false
                                property bool isSelected: {
                                    if (!day || isNaN(startSelectedDate.getTime())) return false
                                    return dayDate.toDateString() === startSelectedDate.toDateString() ||
                                            (!isNaN(endSelectedDate.getTime()) && dayDate.toDateString() === endSelectedDate.toDateString())
                                }
                                property bool isInRange: {
                                    if (!day || isNaN(startSelectedDate.getTime()) || isNaN(endSelectedDate.getTime())) return false
                                    var start = startSelectedDate
                                    var end = endSelectedDate
                                    if (start > end) [start, end] = [end, start]
                                    return dayDate >= start && dayDate <= end
                                }

                                color: {
                                    if (!day) return "transparent"
                                    if (isSelected) return Theme.selectedColor
                                    if (isInRange) return Theme.rangeColor
                                    if (isToday) return Qt.alpha(Theme.secondaryColor, 0.1)
                                    return "transparent"
                                }

                                Label {
                                    anchors.centerIn: parent
                                    text: day || ""
                                    color: {
                                        if (!day) return "transparent"
                                        if (isSelected) return "white"
                                        if (new Date(currentYear, currentMonth, day).getDay() === 0) return Theme.dangerColor
                                        return Theme.textColor
                                    }
                                    font.pixelSize: Theme.fontSizeBody
                                    font.bold: isSelected || isToday
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    enabled: day !== 0
                                    hoverEnabled: true
                                    cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor

                                    onClicked: {
                                        if (!isDateSelected || (!isNaN(startSelectedDate.getTime()) && !isNaN(endSelectedDate.getTime()))) {
                                             startSelectedDate = dayDate
                                            endSelectedDate = new Date(NaN)
                                            isDateSelected = true
                                        } else if (isDateSelected && isNaN(endSelectedDate.getTime())) {
                                            endSelectedDate = dayDate
                                            if (endSelectedDate < startSelectedDate) {
                                                [startSelectedDate, endSelectedDate] = [endSelectedDate, startSelectedDate]
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // Selected Range Display
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 40
                        color: Theme.rangeColor
                        radius: Theme.radiusSmall
                        border.color: Theme.borderColor
                        border.width: 1

                        Label {
                            anchors.centerIn: parent
                            text: {
                                if (isNaN(startSelectedDate.getTime())) return "No date selected"
                                if (isNaN(endSelectedDate.getTime())) {
                                    return "Selected: " + Qt.formatDate(startSelectedDate, "MMMM d, yyyy")
                                }
                                return Qt.formatDate(startSelectedDate, "MMM d") + " - " + Qt.formatDate(endSelectedDate, "MMM d, yyyy")
                            }
                            color: Theme.textColor
                            font.pixelSize: Theme.fontSizeBody
                        }
                    }
                }
            }

            // Footer Buttons
            RowLayout {
                Layout.alignment: Qt.AlignRight
                spacing: 12

                Button {
                    id: clearBtn
                    text: Lang.t("Clear")
                    Layout.preferredWidth: 100
                    Layout.preferredHeight: 40
                    background: Rectangle {
                        radius: Theme.radiusSmall
                        color: clearBtn.hovered ? Qt.alpha(Theme.accentColor, 0.15) : "transparent"
                        border.color: Theme.accentColor
                        border.width: 1
                    }
                    contentItem: Text {
                        text: clearBtn.text
                        font.pixelSize: Theme.fontSizeBody
                        color: Theme.accentColor
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: {
                        startSelectedDate = new Date(NaN)
                        endSelectedDate = new Date(NaN)
                        isDateSelected = false
                    }
                }

                Button {
                    id: cancelBtn
                    text: Lang.t("Cancel")
                    Layout.preferredWidth: 100
                    Layout.preferredHeight: 40
                    background: Rectangle {
                        radius: Theme.radiusSmall
                        color: cancelBtn.hovered ? Qt.alpha(Theme.textColor, 0.1) : "transparent"
                        border.color: Theme.dividerColor
                        border.width: 1
                    }
                    contentItem: Text {
                        text: cancelBtn.text
                        font.pixelSize: Theme.fontSizeBody
                        color: Theme.textColor
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: dateRangeDialog.reject()
                }

                Button {
                    id: applyBtn
                    text: Lang.t("Apply")
                    Layout.preferredWidth: 100
                    Layout.preferredHeight: 40
                    enabled: !isNaN(startSelectedDate.getTime())
                    background: Rectangle {
                        radius: Theme.radiusSmall
                        color: applyBtn.enabled ? (applyBtn.hovered ? Qt.lighter(Theme.secondaryColor, 1.1) : Theme.secondaryColor) : Theme.neutralColor
                    }
                    contentItem: Text {
                        text: applyBtn.text
                        font.pixelSize: Theme.fontSizeBody
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: {
                        if (!isNaN(startSelectedDate.getTime())) {
                            if (isNaN(endSelectedDate.getTime())) {
                                endSelectedDate = new Date(startSelectedDate)
                            }
                            isDateSelected = true
                            dateRangeDialog.accept()
                        }
                    }
                }
            }
        }
    }
}
