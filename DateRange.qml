import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material
import QtQuick.Dialogs
import QtQuick.Effects
import QtQuick.Window

Dialog {
    id: dateRangeDialog
    modal: true
    anchors.centerIn: parent
    width: Math.min(600, parent.width * 0.9)
    height: Math.min(600, parent.height * 0.8)
    padding: 0
    dim: true

    background: Rectangle {
        color: cardColor
        radius: 12
        border.color: dividerColor
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
            radius: 16
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
                text: "Select Date Range"
                font {
                    bold: true;
                    pixelSize: 20;
                    family: "Segoe UI"
                }
                color: primaryColor
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
                        text: "Quick Select"
                        font {
                            pixelSize: 14;
                            bold: true
                        }
                        color: textColor
                        opacity: 0.8
                        Layout.bottomMargin: 5
                    }

                    Repeater {
                        model: [
                            { text: "Today", range: 0 },
                            { text: "Yesterday", range: 1 },
                            { text: "This Week", range: 2 },
                            { text: "Last Week", range: 3 },
                            { text: "This Month", range: 4 },
                            { text: "Last Month", range: 5 }
                        ]

                        Button {
                            text: modelData.text
                            Layout.fillWidth: true
                            Layout.preferredHeight: 40
                            Material.background: index % 2 === 0 ? Qt.lighter(cardColor, 1.1) : cardColor
                            Material.foreground: textColor
                            font.pixelSize: 14
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
                                    var daysSinceMonday = (date.getDay() + 6) % 7
                                    startSelectedDate = new Date(date)
                                    startSelectedDate.setDate(date.getDate() - daysSinceMonday)
                                    endSelectedDate = new Date(date)
                                    break
                                case 3: // Last Week
                                    var daysSinceMonday = (date.getDay() + 6) % 7
                                    startSelectedDate = new Date(date)
                                    startSelectedDate.setDate(date.getDate() - daysSinceMonday - 7)
                                    endSelectedDate = new Date(startSelectedDate)
                                    endSelectedDate.setDate(startSelectedDate.getDate() + 6)
                                    break
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
                            icon.color: textColor
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
                            color: textColor
                        }

                        Button {
                            icon.source: "qrc:/icons/chevron-right.svg"
                            icon.color: textColor
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
                                    pixelSize: 14;
                                    bold: true
                                }
                                color: lightTextColor
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
                                color: {
                                    if (!day) return "transparent"
                                    if (isSelected) return secondaryColor
                                    if (isInRange) return Qt.rgba(secondaryColor.r, secondaryColor.g, secondaryColor.b, 0.2)
                                    if (isToday) return Qt.rgba(secondaryColor.r, secondaryColor.g, secondaryColor.b, 0.1)
                                    return "transparent"
                                }

                                property int day: {
                                    var dayIndex = index - calendarGrid.firstDay + 1
                                    if (dayIndex <= 0 || dayIndex > calendarGrid.daysInMonth) return 0
                                    return dayIndex
                                }

                                property date dayDate: new Date(currentYear, currentMonth, day)
                                property bool isToday: dayDate.toDateString() === new Date().toDateString()
                                property bool isSelected: {
                                    if (isNaN(startSelectedDate.getTime()) || !day) return false
                                    return dayDate.toDateString() === startSelectedDate.toDateString() ||
                                            (!isNaN(endSelectedDate.getTime()) && dayDate.toDateString() === endSelectedDate.toDateString())
                                }
                                property bool isInRange: {
                                    if (isNaN(startSelectedDate.getTime()) || isNaN(endSelectedDate.getTime()) || !day) return false
                                    var start = startSelectedDate
                                    var end = endSelectedDate
                                    if (start > end) [start, end] = [end, start]
                                    return dayDate >= start && dayDate <= end
                                }

                                Label {
                                    anchors.centerIn: parent
                                    text: day || ""
                                    color: {
                                        if (!day) return "transparent"
                                        if (isSelected) return "white"
                                        if (new Date(currentYear, currentMonth, day).getDay() === 0) return "#FF5252" // Red for Sundays
                                        return textColor
                                    }
                                    font.pixelSize: 14
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
                        color: Qt.rgba(secondaryColor.r, secondaryColor.g, secondaryColor.b, 0.1)
                        radius: 8
                        border.color: Qt.rgba(secondaryColor.r, secondaryColor.g, secondaryColor.b, 0.3)
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
                            color: textColor
                            font.pixelSize: 14
                        }
                    }
                }
            }

            // Footer Buttons
            RowLayout {
                Layout.alignment: Qt.AlignRight
                spacing: 12

                Button {
                    text: "Clear"
                    Layout.preferredWidth: 100
                    Layout.preferredHeight: 40
                    Material.background: "transparent"
                    Material.foreground: accentColor
                    font.pixelSize: 14
                    onClicked: {
                        startSelectedDate = new Date(NaN)
                        endSelectedDate = new Date(NaN)
                        isDateSelected = false
                    }
                }

                Button {
                    text: "Cancel"
                    Layout.preferredWidth: 100
                    Layout.preferredHeight: 40
                    Material.background: "transparent"
                    Material.foreground: accentColor
                    font.pixelSize: 14
                    onClicked: dateRangeDialog.reject()
                }

                Button {
                    text: "Apply"
                    Layout.preferredWidth: 100
                    Layout.preferredHeight: 40
                    Material.background: secondaryColor
                    Material.foreground: "white"
                    font.pixelSize: 14
                    enabled: !isNaN(startSelectedDate.getTime())
                    onClicked: {
                        if (!isNaN(startSelectedDate.getTime())) {
                            if (isNaN(endSelectedDate.getTime())) {
                                endSelectedDate = new Date(startSelectedDate)
                            }
                            applyDateRange()
                            dateRangeDialog.accept()
                        }
                    }
                }
            }
        }
    }
}
