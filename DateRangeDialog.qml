import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Dialog {
    id: dateRangeDialog
    modal: true
    anchors.centerIn: parent
    width: Math.min(600, parent ? parent.width * 0.9 : 600)
    height: Math.min(600, parent ? parent.height * 0.8 : 600)
    padding: 0
    dim: true

    // Properties that need to be set from Main.qml
    property var startSelectedDate: new Date(NaN)
    property var endSelectedDate: new Date(NaN)
    property bool isDateSelected: false
    property int currentMonth: new Date().getMonth()
    property int currentYear: new Date().getFullYear()
    property color cardColor: "#FFFFFF"
    property color textColor: "#1F2937"
    property color lightTextColor: "#6B7280"
    property color dividerColor: "#E5E7EB"
    property color primaryColor: "#00e0a8"
    property color selectedColor: "#3B82F6"
    property color rangeColor: "#DBEAFE"

    // Signals
    signal dateRangeApplied()

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
                            
                            background: Rectangle {
                                color: parent.hovered ? Qt.lighter(cardColor, 1.2) : Qt.lighter(cardColor, 1.1)
                                radius: 8
                                border.color: dividerColor
                                border.width: 1
                            }
                            
                            contentItem: Text {
                                text: parent.text
                                color: textColor
                                font.pixelSize: 14
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

                        Button {
                            text: "‹"
                            font.pixelSize: 20
                            font.bold: true
                            Layout.preferredWidth: 40
                            Layout.preferredHeight: 40
                            
                            background: Rectangle {
                                color: parent.hovered ? Qt.lighter(primaryColor, 1.2) : primaryColor
                                radius: 20
                            }
                            
                            contentItem: Text {
                                text: parent.text
                                color: "white"
                                font: parent.font
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            
                            onClicked: {
                                if (currentMonth === 0) {
                                    currentMonth = 11
                                    currentYear--
                                } else {
                                    currentMonth--
                                }
                                calendar.updateCalendar()
                            }
                        }

                        Text {
                            text: Qt.locale().monthName(currentMonth) + " " + currentYear
                            font.pixelSize: 18
                            font.bold: true
                            color: textColor
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignHCenter
                        }

                        Button {
                            text: "›"
                            font.pixelSize: 20
                            font.bold: true
                            Layout.preferredWidth: 40
                            Layout.preferredHeight: 40
                            
                            background: Rectangle {
                                color: parent.hovered ? Qt.lighter(primaryColor, 1.2) : primaryColor
                                radius: 20
                            }
                            
                            contentItem: Text {
                                text: parent.text
                                color: "white"
                                font: parent.font
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            
                            onClicked: {
                                if (currentMonth === 11) {
                                    currentMonth = 0
                                    currentYear++
                                } else {
                                    currentMonth++
                                }
                                calendar.updateCalendar()
                            }
                        }
                    }

                    // Calendar Grid
                    GridLayout {
                        id: calendar
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        columns: 7
                        rowSpacing: 4
                        columnSpacing: 4

                        function updateCalendar() {
                            // Clear existing items
                            for (var i = children.length - 1; i >= 0; i--) {
                                children[i].destroy()
                            }

                            // Add day headers
                            var dayHeaders = ["Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"]
                            for (var h = 0; h < dayHeaders.length; h++) {
                                var header = headerComponent.createObject(calendar, {"text": dayHeaders[h]})
                            }

                            // Calculate first day of month and days in month
                            var firstDay = new Date(currentYear, currentMonth, 1)
                            var lastDay = new Date(currentYear, currentMonth + 1, 0)
                            var firstDayOfWeek = (firstDay.getDay() + 6) % 7 // Make Monday = 0
                            var daysInMonth = lastDay.getDate()

                            // Add empty cells for days before first day of month
                            for (var e = 0; e < firstDayOfWeek; e++) {
                                var empty = emptyDayComponent.createObject(calendar)
                            }

                            // Add days of the month
                            for (var d = 1; d <= daysInMonth; d++) {
                                var day = dayComponent.createObject(calendar, {
                                    "dayNumber": d,
                                    "currentMonth": currentMonth,
                                    "currentYear": currentYear
                                })
                            }
                        }

                        Component.onCompleted: updateCalendar()
                    }
                }
            }

            // Selected Range Display
            RowLayout {
                Layout.fillWidth: true
                spacing: 20

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    Text {
                        text: "Start Date:"
                        font.pixelSize: 12
                        color: lightTextColor
                    }
                    Text {
                        text: isNaN(startSelectedDate.getTime()) ? "Not selected" : Qt.formatDate(startSelectedDate, "MMM d, yyyy")
                        font.pixelSize: 14
                        font.bold: true
                        color: textColor
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    Text {
                        text: "End Date:"
                        font.pixelSize: 12
                        color: lightTextColor
                    }
                    Text {
                        text: isNaN(endSelectedDate.getTime()) ? "Not selected" : Qt.formatDate(endSelectedDate, "MMM d, yyyy")
                        font.pixelSize: 14
                        font.bold: true
                        color: textColor
                    }
                }
            }

            // Action Buttons
            RowLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignRight
                spacing: 12

                Button {
                    text: "Clear"
                    Layout.preferredWidth: 80
                    Layout.preferredHeight: 40
                    
                    background: Rectangle {
                        color: parent.hovered ? Qt.lighter(dividerColor, 1.1) : dividerColor
                        radius: 8
                        border.color: dividerColor
                        border.width: 1
                    }
                    
                    contentItem: Text {
                        text: parent.text
                        color: textColor
                        font.pixelSize: 14
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
                    text: "Cancel"
                    Layout.preferredWidth: 80
                    Layout.preferredHeight: 40
                    
                    background: Rectangle {
                        color: parent.hovered ? Qt.lighter(dividerColor, 1.1) : dividerColor
                        radius: 8
                        border.color: dividerColor
                        border.width: 1
                    }
                    
                    contentItem: Text {
                        text: parent.text
                        color: textColor
                        font.pixelSize: 14
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    
                    onClicked: dateRangeDialog.reject()
                }

                Button {
                    text: "Apply"
                    Layout.preferredWidth: 80
                    Layout.preferredHeight: 40
                    enabled: !isNaN(startSelectedDate.getTime()) && !isNaN(endSelectedDate.getTime())
                    
                    background: Rectangle {
                        color: parent.enabled ? (parent.hovered ? Qt.lighter(primaryColor, 1.1) : primaryColor) : dividerColor
                        radius: 8
                    }
                    
                    contentItem: Text {
                        text: parent.text
                        color: parent.enabled ? "white" : lightTextColor
                        font.pixelSize: 14
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    
                    onClicked: {
                        if (!isNaN(startSelectedDate.getTime()) && !isNaN(endSelectedDate.getTime())) {
                            isDateSelected = true
                            applyDateRange()
                            dateRangeDialog.accept()
                        }
                    }
                }
            }
        }
    }

    // Components for calendar
    Component {
        id: headerComponent
        Text {
            text: ""
            font.pixelSize: 12
            font.bold: true
            color: lightTextColor
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            Layout.preferredHeight: 30
            Layout.fillWidth: true
        }
    }

    Component {
        id: emptyDayComponent
        Item {
            Layout.preferredHeight: 30
            Layout.fillWidth: true
        }
    }

    Component {
        id: dayComponent
        Rectangle {
            property int dayNumber: 1
            property int currentMonth
            property int currentYear
            
            Layout.preferredHeight: 35
            Layout.fillWidth: true
            radius: 4
            
            property var dayDate: new Date(currentYear, currentMonth, dayNumber)
            property bool isInRange: {
                if (isNaN(startSelectedDate.getTime()) || isNaN(endSelectedDate.getTime())) {
                    return false
                }
                return dayDate >= startSelectedDate && dayDate <= endSelectedDate
            }
            property bool isSelected: {
                if (isNaN(startSelectedDate.getTime()) && isNaN(endSelectedDate.getTime())) {
                    return false
                }
                return (dayDate.getTime() === startSelectedDate.getTime()) || 
                       (dayDate.getTime() === endSelectedDate.getTime())
            }
            
            color: isSelected ? selectedColor : (isInRange ? rangeColor : "transparent")
            border.color: isSelected ? selectedColor : (mouseArea.containsMouse ? selectedColor : "transparent")
            border.width: 1
            
            Text {
                text: dayNumber
                anchors.centerIn: parent
                font.pixelSize: 14
                color: parent.isSelected ? "white" : textColor
                font.bold: parent.isSelected
            }
            
            MouseArea {
                id: mouseArea
                anchors.fill: parent
                hoverEnabled: true
                
                onClicked: {
                    var clickedDate = new Date(currentYear, currentMonth, dayNumber)
                    
                    if (isNaN(startSelectedDate.getTime()) || (!isNaN(startSelectedDate.getTime()) && !isNaN(endSelectedDate.getTime()))) {
                        // Start new selection
                        startSelectedDate = clickedDate
                        endSelectedDate = new Date(NaN)
                    } else if (!isNaN(startSelectedDate.getTime()) && isNaN(endSelectedDate.getTime())) {
                        // Set end date
                        if (clickedDate >= startSelectedDate) {
                            endSelectedDate = clickedDate
                        } else {
                            endSelectedDate = startSelectedDate
                            startSelectedDate = clickedDate
                        }
                    }
                }
            }
        }
    }

    function applyDateRange() {
        dateRangeApplied()
    }
}