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


        RowLayout {
            id: taskControlRow
            Layout.fillWidth: true
            spacing: 8

            property bool isLoading: false

            Label {
                text: "Current Task"
                font {
                    bold: true
                    pixelSize: 16
                    family: "Segoe UI"
                }
                color: primaryColor
            }

            Item { Layout.fillWidth: true }

            BusyIndicator {
                visible: taskControlRow.isLoading
                running: taskControlRow.isLoading
                Layout.preferredWidth: 24
                Layout.preferredHeight: 24
            }

            Button {
                id: pauseResumeButton
                visible: {
                    if (logger.activeTaskId === -1) return false;

                    // Find active task
                    for (let i = 0; i < logger.taskList.length; i++) {
                        if (logger.taskList[i].id === logger.activeTaskId) {
                            return logger.taskList[i].status !== "Review";
                        }
                    }
                    return false;
                }

                text: logger.isTaskPaused ? "Play" : "Pause"
                Layout.preferredWidth: 100
                Layout.preferredHeight: 34
                font.pixelSize: 14

                background: Rectangle {
                    radius: 8
                    color: logger.isTaskPaused ? accentColor : productiveColor
                }

                contentItem: Text {
                    text: pauseResumeButton.text
                    font: pauseResumeButton.font
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: {
                    taskControlRow.isLoading = true;

                    if (logger.isTaskPaused) {
                        // Resume task
                        logger.toggleTaskPause(); // Use the existing toggle function
                    } else {
                        // Pause task
                        logger.toggleTaskPause(); // Use the existing toggle function
                    }
                }
            }
        }

        Connections {
            target: logger

            function onTaskPausedChanged() {
                taskControlRow.isLoading = false;
            }

            function onAuthTokenError(message) {
                taskControlRow.isLoading = false;
                console.error("API Error:", message);
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: dividerColor
            Layout.topMargin: 4
        }

        // Active Task Info
        ColumnLayout {
            visible: logger.activeTaskId !== -1
            spacing: 8

            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Label {
                    text: "Status:"
                    font { family: "Segoe UI"; pixelSize: 14 }
                    color: lightTextColor
                }

                RowLayout {
                    spacing: 6

                    Rectangle {
                        width: 10
                        height: 10
                        radius: 5
                        color: {
                            var activeTask = logger.taskList.find(task => task.id === logger.activeTaskId)
                            if (activeTask && activeTask.status === "Review") {
                                return "#FF9800" // Orange for review status
                            }
                            return logger.isTaskPaused ? accentColor : productiveColor
                        }
                        opacity: {
                            var activeTask = logger.taskList.find(task => task.id === logger.activeTaskId)
                            if (activeTask && activeTask.status === "Review") {
                                return 0.7 // Static opacity for review
                            }
                            return logger.isTaskPaused ? 0.8 : 1
                        }

                        // Animasi hanya untuk task non-Review yang aktif
                        SequentialAnimation on opacity {
                            running: {
                                var activeTask = logger.taskList.find(task => task.id === logger.activeTaskId)
                                return !logger.isTaskPaused && !(activeTask && activeTask.status === "Review")
                            }
                            loops: Animation.Infinite
                            NumberAnimation { from: 0.3; to: 1; duration: 800; easing.type: Easing.InOutQuad }
                            NumberAnimation { from: 1; to: 0.3; duration: 800; easing.type: Easing.InOutQuad }
                        }
                    }

                    Label {
                        text: {
                            var activeTask = logger.taskList.find(task => task.id === logger.activeTaskId)
                            if (activeTask && activeTask.status === "Review") {
                                return "Review"
                            }
                            return logger.isTaskPaused ? "Paused" : "Active"
                        }
                        font { family: "Segoe UI"; pixelSize: 14 }
                        color: {
                            var activeTask = logger.taskList.find(task => task.id === logger.activeTaskId)
                            if (activeTask && activeTask.status === "Review") {
                                return "#FF9800"
                            }
                            return logger.isTaskPaused ? accentColor : productiveColor
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Label {
                            text: {
                                var activeTask = logger.taskList.find(task => task.id === logger.activeTaskId)
                                if (activeTask) {
                                    // Format waktu ke format "Xh Ym" (contoh: 1h 30m)
                                    var hours = Math.floor(activeTask.time_usage / 3600)
                                    var minutes = Math.floor((activeTask.time_usage % 3600) / 60)
                                    var timeText = ""
                                    if (hours > 0) timeText += hours + "h "
                                    timeText += minutes + "m"

                                    public_curent_time = timeText
                                    return timeText
                                }
                                return "0h 0m"
                            }
                            font.pixelSize: 14
                            color: {
                                var activeTask = logger.taskList.find(task => task.id === logger.activeTaskId)
                                if (activeTask && activeTask.time_usage > activeTask.max_time) {
                                    return nonProductiveColor
                                }
                                return lightTextColor
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 6
                            radius: 3
                            color: Qt.rgba(dividerColor.r, dividerColor.g, dividerColor.b, 0.3)

                            Rectangle {
                                width: {
                                    var activeTask = logger.taskList.find(task => task.id === logger.activeTaskId)
                                    if (activeTask) {
                                        // Calculate percentage (can be >100%)
                                        var percentage = Math.min(1.5, activeTask.time_usage / activeTask.max_time) // Cap at 150% for visibility
                                        return parent.width  * Math.min(1, activeTask.time_usage / activeTask.max_time)
                                    }
                                    return parent.width * Math.min(1, logger.globalTimeUsage / (8 * 3600))
                                }
                                height: parent.height
                                radius: 3
                                color: {
                                    var activeTask = logger.taskList.find(task => task.id === logger.activeTaskId)
                                    if (activeTask && activeTask.time_usage > activeTask.max_time) {
                                        return nonProductiveColor
                                    }
                                    return secondaryColor
                                }

                                // Nonaktifkan animasi untuk task Review
                                Behavior on width {
                                    enabled: {
                                        var activeTask = logger.taskList.find(task => task.id === logger.activeTaskId)
                                        return !(activeTask && activeTask.status === "Review")
                                    }
                                    NumberAnimation { duration: 500 }
                                }
                            }

                            // Percentage label inside progress bar
                            Label {
                                anchors {
                                    right: parent.right
                                    rightMargin: 4
                                    verticalCenter: parent.verticalCenter
                                }
                                text: {
                                    var activeTask = logger.taskList.find(task => task.id === logger.activeTaskId)
                                    if (activeTask) {
                                        var percentage = Math.round((activeTask.time_usage / activeTask.max_time) * 100)
                                        return percentage + "%"
                                    }
                                    return ""
                                }
                                font.pixelSize: 9
                                font.bold: true
                                color: "white"
                            }
                        }

                        Label {
                            text: {
                                var activeTask = logger.taskList.find(task => task.id === logger.activeTaskId)
                                if (activeTask) {
                                    // Format waktu maksimal ke format "Xh Ym" (contoh: 2h 0m)
                                    var hours = Math.floor(activeTask.max_time / 3600)
                                    var minutes = Math.floor((activeTask.max_time % 3600) / 60)
                                    var timeText_max = ""
                                    if (hours > 0) timeText_max += hours + "h "
                                    timeText_max += minutes + "m"


                                    public_max_time = timeText_max
                                    return timeText_max
                                }
                                return "0h 0m"
                            }
                            font.pixelSize: 14
                            color: {
                                var activeTask = logger.taskList.find(task => task.id === logger.activeTaskId)
                                if (activeTask && activeTask.time_usage > activeTask.max_time) {
                                    return nonProductiveColor
                                }
                                return lightTextColor
                            }
                        }
                    }
                }
            }
        }

        Label {
            visible: logger.activeTaskId === -1
            text: "No active task"
            font.pixelSize: 14
            color: lightTextColor
            Layout.alignment: Qt.AlignHCenter
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: dividerColor
            visible: taskListView.count > 0
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            visible: taskListView.count > 0

            ScrollBar.vertical.policy: ScrollBar.AsNeeded
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            ListView {
                id: taskListView

                // Properti untuk menyimpan posisi scroll
                property real savedContentY: 0
                property bool preservePosition: false

                model: {
                    if (!logger.taskList) return []

                    // Simpan posisi sebelum update model
                    if (taskListView.count > 0) {
                        taskListView.savedContentY = taskListView.contentY
                        taskListView.preservePosition = true
                    }

                    var sorted = logger.taskList.slice() // Copy array
                    sorted.sort(function(a, b) {
                        // Active task selalu di atas
                        if (a.active && !b.active) return -1
                        if (b.active && !a.active) return 1
                        return a.id - b.id
                    })


                    return sorted.map(function(task) {
                        var isActive = task.id === logger.activeTaskId
                        return {
                            id: task.id,
                            project_name: task.project_name,
                            task: task.task,
                            max_time: task.max_time,
                            time_usage: task.time_usage,
                            active: isActive,
                            status: task.status,
                            isTaskPaused: isActive ? logger.isTaskPaused : false,
                            // Tambahkan properti lain yang diperlukan
                        }
                    })
                }

                spacing: 8
                width: parent.width

                boundsBehavior: Flickable.StopAtBounds
                flickableDirection: Flickable.VerticalFlick
                highlightFollowsCurrentItem: false
                keyNavigationEnabled: false

                // Restore posisi setelah model berubah
                onCountChanged: {
                    if (preservePosition && count > 0) {
                        Qt.callLater(function() {
                            taskListView.contentY = taskListView.savedContentY
                            taskListView.preservePosition = false
                        })
                    }
                }

                // Alternative: menggunakan onModelChanged jika onCountChanged tidak bekerja
                onModelChanged: {
                    if (preservePosition && model && model.length > 0) {
                        Qt.callLater(function() {
                            taskListView.contentY = taskListView.savedContentY
                            taskListView.preservePosition = false
                        })
                    }
                }

                delegate: Rectangle {
                    id: delegateRoot
                    width: taskListView.width
                    height: column.implicitHeight + 20
                    radius: 8

                    readonly property bool isReview: modelData.status === "Review"
                    readonly property bool isNeedReview: modelData.status === "Need Review"
                    readonly property bool isNeedRevise: modelData.status === "Need Revise"
                    readonly property bool isActive: modelData.active

                    color: {
                        if (isReview) {
                            return Qt.rgba(255/255, 152/255, 0/255, 0.08) // Subtle orange for review
                        }
                        if (isNeedReview) return Qt.rgba(33/255, 150/255, 243/255, 0.15);
                        if (isNeedRevise) return Qt.rgba(244/255, 67/255, 54/255, 0.15);
                        return isActive ? Qt.lighter(cardColor, 1.6) : cardColor
                    }
                    border.color: {
                        if (isReview) {
                            return Qt.rgba(255/255, 152/255, 0/255, 0.3) // Soft orange border
                        }
                        if (isNeedReview) return Qt.rgba(33/255, 150/255, 243/255, 0.4);
                        if (isNeedRevise) return Qt.rgba(244/255, 67/255, 54/255, 0.4);

                        return isActive ? secondaryColor : Qt.rgba(dividerColor.r, dividerColor.g, dividerColor.b, 0.3)
                    }
                    border.width: 1
                    opacity: isReview ? 0.85 : 1

                    // Subtle shadow effect
                    Rectangle {
                        anchors.fill: parent
                        anchors.topMargin: 1
                        radius: parent.radius
                        color: Qt.rgba(0, 0, 0, 0.02)
                        z: -1
                    }

                    MouseArea {
                        anchors.fill: parent
                        enabled:  !(delegateRoot.isReview || delegateRoot.isNeedReview || delegateRoot.isNeedRevise)
                        onClicked: {
                            if (!delegateRoot.isActive && logger.activeTaskId !== -1) {
                                confirmSwitchDialog.taskId = modelData.id
                                confirmSwitchDialog.open()
                            } else if (!delegateRoot.isActive) {
                                logger.setActiveTask(modelData.id)
                            }
                        }
                    }

                    ColumnLayout {
                        id: column
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 6

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Label {
                                id: projectLabel
                                text: modelData.project_name
                                font { bold: true; pixelSize: 14 }
                                color: delegateRoot.isReview ? "#FF9800" : textColor
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                                maximumLineCount: 1
                            }
                            Rectangle {
                                Layout.preferredWidth: 32
                                Layout.preferredHeight: 12
                                radius: 16
                                color: menuMouseArea.containsMouse ? Qt.rgba(0, 0, 0, 0.1) : "transparent"
                                visible: !(delegateRoot.isReview || delegateRoot.isNeedReview || delegateRoot.isNeedRevise)

                                Rectangle {
                                    anchors.centerIn: parent
                                    width: 20
                                    height: 10
                                    radius: 10
                                    color: "transparent"
                                    border.color: Qt.rgba(lightTextColor.r, lightTextColor.g, lightTextColor.b, 0.6)
                                    border.width: 1

                                    // Three dots
                                    Row {
                                        anchors.centerIn: parent
                                        spacing: 2

                                        Repeater {
                                            model: 3
                                            Rectangle {
                                                width: 2
                                                height: 2
                                                radius: 1
                                                color: Qt.rgba(lightTextColor.r, lightTextColor.g, lightTextColor.b, 0.8)
                                            }
                                        }
                                    }
                                }

                                MouseArea {
                                    id: menuMouseArea
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor

                                    onClicked: {
                                        // Simpan data yang diperlukan
                                        stableTaskMenu.taskId = modelData.id
                                        stableTaskMenu.userId = logger.currentUserId
                                        stableTaskMenu.authToken = logger.authToken

                                        // Popup menu (ini akan bekerja karena Menu memiliki method popup)
                                        stableTaskMenu.popup()
                                    }
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Label {
                                id: taskLabel
                                text: modelData.task
                                font.pixelSize: 12
                                color: delegateRoot.isReview ? Qt.rgba(255/255, 152/255, 0/255, 0.8) : lightTextColor
                                elide: Text.ElideRight
                                maximumLineCount: 1
                                Layout.fillWidth: true
                            }

                            // Modern "View All" button
                            Rectangle {
                                visible: projectLabel.truncated || taskLabel.truncated
                                Layout.preferredWidth: 56
                                Layout.preferredHeight: 20
                                radius: 10
                                color: viewAllArea.pressed ? Qt.rgba(primaryColor.r, primaryColor.g, primaryColor.b, 0.2) :
                                                             viewAllArea.containsMouse ? Qt.rgba(primaryColor.r, primaryColor.g, primaryColor.b, 0.1) :
                                                                                         Qt.rgba(primaryColor.r, primaryColor.g, primaryColor.b, 0.05)

                                border.color: Qt.rgba(primaryColor.r, primaryColor.g, primaryColor.b, 0.3)
                                border.width: 1

                                Text {
                                    anchors.centerIn: parent
                                    text: "View All"
                                    font.pixelSize: 9
                                    font.bold: true
                                    color: primaryColor
                                }

                                MouseArea {
                                    id: viewAllArea
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: taskDetailPopup.show(modelData.project_name, modelData.task)
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true

                            // Status badge
                            Rectangle {
                                Layout.preferredHeight: 18
                                Layout.preferredWidth: statusText.implicitWidth + 12
                                radius: 9
                                color: {
                                    if (modelData.status === "Review") return Qt.rgba(255/255, 152/255, 0/255, 0.15);
                                    if (modelData.status === "Need Review") return Qt.rgba(33/255, 150/255, 243/255, 0.15);
                                    if (modelData.status === "Need Revise") return Qt.rgba(244/255, 67/255, 54/255, 0.15);
                                    if (modelData.active) return modelData.isTaskPaused ? Qt.rgba(255/255, 152/255, 0/255, 0.15) : Qt.rgba(76/255, 175/255, 80/255, 0.15);
                                    return Qt.rgba(lightTextColor.r, lightTextColor.g, lightTextColor.b, 0.1);
                                }

                                border.color: {
                                    if (modelData.status === "Review") return Qt.rgba(255/255, 152/255, 0/255, 0.4);
                                    if (modelData.status === "Need Review") return Qt.rgba(33/255, 150/255, 243/255, 0.4);
                                    if (modelData.status === "Need Revise") return Qt.rgba(244/255, 67/255, 54/255, 0.4);
                                    if (modelData.active) return modelData.isTaskPaused ? Qt.rgba(255/255, 152/255, 0/255, 0.4) : Qt.rgba(76/255, 175/255, 80/255, 0.4);
                                    return Qt.rgba(lightTextColor.r, lightTextColor.g, lightTextColor.b, 0.2);
                                }
                                border.width: 1

                                Label {
                                    id: statusText
                                    anchors.centerIn: parent
                                    text: {
                                        if (modelData.status === "Review") return "Review";
                                        if (modelData.status === "Need Review") return "Need Review";
                                        if (modelData.status === "Need Revise") return "Need Revise";
                                        if (modelData.active) return modelData.isTaskPaused ? "Paused" : "Running";
                                        return "Pending";
                                    }
                                    font.pixelSize: 10
                                    font.bold: true
                                    color: {
                                        if (modelData.status === "Review") return "#FF9800";
                                        if (modelData.status === "Need Review") return "#2196F3";
                                        if (modelData.status === "Need Revise") return "#F44336";
                                        if (modelData.active) return modelData.isTaskPaused ? "#FF9800" : "#4CAF50";
                                        return lightTextColor;
                                    }
                                }
                            }

                            Item { Layout.fillWidth: true }

                            // Time display with subtle background
                            Rectangle {
                                Layout.preferredWidth: timeLabel.implicitWidth + 8
                                Layout.preferredHeight: 18
                                radius: 4
                                color: Qt.rgba(lightTextColor.r, lightTextColor.g, lightTextColor.b, 0.05)

                                Label {
                                    id: timeLabel
                                    anchors.centerIn: parent
                                    text: logger.formatDuration(modelData.time_usage)
                                    font.pixelSize: 11
                                    font.bold: true
                                    color: delegateRoot.isReview ? "#FF9800" : lightTextColor
                                }
                            }
                        }
                    }
                }
            }
        }
    }

}
