import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Effects
import "../theme"
import "../dialogs"
import "../components"

Item {
    anchors.fill: parent
    MessageDialog {
        id: warningDialog
        title: Lang.t("Warning")
        buttons: MessageDialog.Ok
    }

    TimeUpPopup {
        id: customWarningDialog
        titleText: Lang.t("Warning")
    }

    property bool isTimeUpPopupOpen: false
    property bool isTimeUpWarningOpen: false

    function checkTaskTimeWarning(timeUsage, maxTime) {
        if (!maxTime || maxTime <= 0) return;
        var diffSeconds = maxTime - timeUsage;
        if (diffSeconds <= 0) {
            if (!isTimeUpPopupOpen) {
                customWarningDialog.newText = "Waktu Task anda Sudah Habis";
                isTimeUpPopupOpen = true;
                customWarningDialog.show();
            }
        } else if (diffSeconds <= 600) {
            if (!isTimeUpWarningOpen) {
                customWarningDialog.newText = "Waktu Task anda tersisa kurang dari 10 menit!";
                isTimeUpWarningOpen = true;
                customWarningDialog.show();
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        ColumnLayout {
            id: taskControlRow
            Layout.fillWidth: true
            spacing: 10

            property bool isLoading: false

            // Main Control Panel
            Rectangle {
                Layout.fillWidth: true
                height: 50
                color: "transparent"

                Behavior on height {
                    NumberAnimation { duration: 250; easing.type: Easing.OutCubic }
                }

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 12

                    // Active Task Layout
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 10
                        visible: logger.activeTaskId !== -1
                        opacity: logger.activeTaskId !== -1 ? 1 : 0

                        Behavior on opacity {
                            NumberAnimation { duration: 200 }
                        }

                        // Top Row: Status Badge + Time Info + Button
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 14

                            // Status Badge
                            Rectangle {
                                width: 76
                                height: 30
                                radius: 15
                                visible: {
                                    if (logger.activeTaskId === -1) return false
                                    for (let i = 0; i < logger.taskList.length; i++) {
                                        if (logger.taskList[i].id === logger.activeTaskId) {
                                            return logger.taskList[i].status !== "Review"
                                        }
                                    }
                                    return false
                                }

                                color: {
                                    var activeTask = logger.taskList.find(task => task.id === logger.activeTaskId)
                                    if (activeTask && activeTask.status === "Review") {
                                        return Qt.rgba(1, 0.6, 0, 0.12)
                                    }
                                    return logger.isTaskPaused ? Qt.rgba(0.4, 0.4, 0.7, 0.12) : Qt.rgba(0.2, 0.8, 0.4, 0.12)
                                }

                                RowLayout {
                                    anchors.centerIn: parent
                                    spacing: 6

                                    Rectangle {
                                        width: 8
                                        height: 8
                                        radius: 4
                                        color: {
                                            var activeTask = logger.taskList.find(task => task.id === logger.activeTaskId)
                                            if (activeTask && activeTask.status === "Review") return Theme.warningColor
                                            return logger.isTaskPaused ? Theme.accentColor : Theme.productiveColor
                                        }

                                        SequentialAnimation on opacity {
                                            running: {
                                                var activeTask = logger.taskList.find(task => task.id === logger.activeTaskId)
                                                return !logger.isTaskPaused && !(activeTask && activeTask.status === "Review")
                                            }
                                            loops: Animation.Infinite
                                            NumberAnimation { from: 0.3; to: 1; duration: 700; easing.type: Easing.InOutQuad }
                                            NumberAnimation { from: 1; to: 0.3; duration: 700; easing.type: Easing.InOutQuad }
                                        }
                                    }

                                    Label {
                                        text: {
                                            var activeTask = logger.taskList.find(task => task.id === logger.activeTaskId)
                                            if (activeTask && activeTask.status === "Review") return Lang.t("Review")
                                            return logger.isTaskPaused ? Lang.t("Paused") : Lang.t("Active")
                                        }
                                        font.pixelSize: 13
                                        font.weight: Font.DemiBold
                                        font.family: "Segoe UI"
                                        color: {
                                            var activeTask = logger.taskList.find(task => task.id === logger.activeTaskId)
                                            if (activeTask && activeTask.status === "Review") return Theme.warningColor
                                            return logger.isTaskPaused ? Theme.accentColor : Theme.productiveColor
                                        }
                                    }
                                }
                            }

                            // Current Time
                            Label {
                                text: {
                                    var activeTask = logger.taskList.find(task => task.id === logger.activeTaskId)
                                    if (activeTask) {
                                        checkTaskTimeWarning(activeTask.time_usage, activeTask.max_time)
                                        var hours = Math.floor(activeTask.time_usage / 3600)
                                        var minutes = Math.floor((activeTask.time_usage % 3600) / 60)
                                        var timeText = ""
                                        if (hours > 0) timeText += hours + "h "
                                        timeText += minutes + "m"
                                        return timeText
                                    }
                                    return "0m"
                                }
                                font.pixelSize: Theme.fontSizeBody
                                font.weight: Font.Bold
                                font.family: "Segoe UI"
                                color: {
                                    var activeTask = logger.taskList.find(task => task.id === logger.activeTaskId)
                                    if (activeTask && activeTask.time_usage > activeTask.max_time) {
                                        return Theme.nonProductiveColor
                                    }
                                    return Theme.primaryColor
                                }
                            }

                            // Progress Bar - Centered
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignVCenter
                                height: 8
                                radius: 4
                                color: Qt.rgba(0, 0, 0, 0.06)

                                Rectangle {
                                    width: {
                                        var activeTask = logger.taskList.find(task => task.id === logger.activeTaskId)
                                        if (activeTask) {
                                            return parent.width * Math.min(1, activeTask.time_usage / activeTask.max_time)
                                        }
                                        return 0
                                    }
                                    height: parent.height
                                    radius: 4

                                    gradient: Gradient {
                                        orientation: Gradient.Horizontal
                                        GradientStop {
                                            position: 0.0
                                            color: {
                                                var activeTask = logger.taskList.find(task => task.id === logger.activeTaskId)
                                                if (activeTask && activeTask.time_usage > activeTask.max_time) {
                                                    return Theme.nonProductiveColor
                                                }
                                                return Theme.secondaryColor
                                            }
                                        }
                                        GradientStop {
                                            position: 1.0
                                            color: {
                                                var activeTask = logger.taskList.find(task => task.id === logger.activeTaskId)
                                                if (activeTask && activeTask.time_usage > activeTask.max_time) {
                                                    return Qt.lighter(Theme.nonProductiveColor, 1.2)
                                                }
                                                return Qt.lighter(Theme.secondaryColor, 1.2)
                                            }
                                        }
                                    }

                                    Behavior on width {
                                        enabled: {
                                            var activeTask = logger.taskList.find(task => task.id === logger.activeTaskId)
                                            return !(activeTask && activeTask.status === "Review")
                                        }
                                        NumberAnimation { duration: 300; easing.type: Easing.OutCubic }
                                    }
                                }
                            }

                            // Loading Indicator
                            BusyIndicator {
                                visible: taskControlRow.isLoading
                                running: taskControlRow.isLoading
                                Layout.preferredWidth: 22
                                Layout.preferredHeight: 22
                            }

                            // Percentage
                            Label {
                                text: {
                                    var activeTask = logger.taskList.find(task => task.id === logger.activeTaskId)
                                    if (activeTask) {
                                        var percentage = Math.round((activeTask.time_usage / activeTask.max_time) * 100)
                                        return percentage + "%"
                                    }
                                    return "0%"
                                }
                                font.pixelSize: 13
                                font.weight: Font.Medium
                                font.family: "Segoe UI"
                                color: Theme.primaryColor
                                opacity: 0.7
                            }

                            // Max Time
                            Label {
                                text: {
                                    var activeTask = logger.taskList.find(task => task.id === logger.activeTaskId)
                                    if (activeTask) {
                                        var hours = Math.floor(activeTask.max_time / 3600)
                                        var minutes = Math.floor((activeTask.max_time % 3600) / 60)
                                        var timeText = ""
                                        if (hours > 0) timeText += hours + "h "
                                        timeText += minutes + "m"
                                        return timeText
                                    }
                                    return "0m"
                                }
                                font.pixelSize: 13
                                font.family: "Segoe UI"
                                color: Theme.lightTextColor
                            }

                            // Pause/Resume Button
                            Button {
                                id: pauseResumeButton
                                visible: {
                                    if (logger.activeTaskId === -1) return false
                                    for (let i = 0; i < logger.taskList.length; i++) {
                                        if (logger.taskList[i].id === logger.activeTaskId) {
                                            return logger.taskList[i].status !== "Review"
                                        }
                                    }
                                    return false
                                }

                                text: logger.isTaskPaused ? Lang.t("Resume") : Lang.t("Pause")
                                Layout.preferredWidth: 92
                                Layout.preferredHeight: 40
                                font.pixelSize: 13
                                font.weight: Font.DemiBold

                                background: Rectangle {
                                    radius: 17
                                    color: pauseResumeButton.hovered ?
                                        (logger.isTaskPaused ? Qt.lighter(Theme.accentColor, 1.15) : Qt.lighter(Theme.productiveColor, 1.15)) :
                                        (logger.isTaskPaused ? Theme.accentColor : Theme.productiveColor)

                                    Behavior on color {
                                        ColorAnimation { duration: 150 }
                                    }
                                }

                                contentItem: Text {
                                    text: pauseResumeButton.text
                                    font: pauseResumeButton.font
                                    color: "white"
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }

                                onClicked: {
                                    taskControlRow.isLoading = true
                                    logger.toggleTaskPause()
                                }
                            }
                        }
                    }

                    // No Active Task Message
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignVCenter
                        visible: logger.activeTaskId === -1

                        Item { Layout.fillWidth: true }

                        Label {
                            text: Lang.t("No active task")
                            font.pixelSize: Theme.fontSizeBody
                            font.family: "Segoe UI"
                            color: Theme.lightTextColor
                            opacity: 0.6
                        }

                        Item { Layout.fillWidth: true }
                    }
                }
            }

            // Divider
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Theme.dividerColor
                opacity: 0.5
                visible: taskListView.count > 0
            }

            Connections {
                target: logger

                function onTaskPausedChanged() {
                    taskControlRow.isLoading = false
                }

                function onAuthTokenError(message) {
                    taskControlRow.isLoading = false
                    console.error("API Error:", message)
                }
            }
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
                    if (!logger || !logger.taskList) return []

                    var sorted = logger.taskList.slice() // Copy array
                    sorted.sort(function(a, b) {
                        // Urutan prioritas berdasarkan status
                        function getPriority(task) {
                            if (task.active) return 0;                 // On Progress
                            if (task.status === "Pending") return 1;   // Pending
                            if (task.status === "Need Revise") return 2;
                            if (task.status === "Need Review") return 3; // Need Review/Revise
                            return 3; // status lain
                        }

                        var pa = getPriority(a)
                        var pb = getPriority(b)
                        if (pa !== pb) return pa - pb

                        // Kalau sama prioritas → urutkan berdasarkan id (descending)
                        return b.id - a.id
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
                            isExpired: task.isExpired
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
                    readonly property bool isExpired: modelData.isExpired
                    color: {
                        if (isExpired) return Qt.alpha(Theme.neutralColor, 0.15)
                        if (isReview) {
                            return Qt.alpha(Theme.warningColor, 0.08)
                        }
                        if (isNeedReview) return Qt.alpha(Theme.infoColor, 0.15);
                        if (isNeedRevise) return Qt.alpha(Theme.dangerColor, 0.15);
                        return isActive ? Qt.lighter(Theme.cardColor, 1.6) : Theme.cardColor
                    }
                    border.color: {
                        if (isExpired) return Qt.alpha(Theme.neutralColor, 0.4)
                        if (isReview) {
                            return Qt.alpha(Theme.warningColor, 0.3)
                        }
                        if (isNeedReview) return Qt.alpha(Theme.infoColor, 0.4);
                        if (isNeedRevise) return Qt.alpha(Theme.dangerColor, 0.4);

                        return isActive ? Theme.secondaryColor : Qt.alpha(Theme.dividerColor, 0.3)
                    }
                    border.width: 1
                    opacity: isReview ? 0.85 : 1 ||  isExpired ? 0.7 : (isReview ? 0.85 : 1)


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
                        enabled: !(delegateRoot.isReview || delegateRoot.isNeedReview)

                        onClicked: {
                            // === LOGIKA 1: CEK TASK EXPIRED (Bulan Lalu) ===
                            if (logger.isTaskExpired(modelData.id)) {
                                customWarningDialog.titleText = "Task Expired"
                                customWarningDialog.newText = "Task dari bulan lalu tidak dapat dijalankan. Silahkan Need Review."
                                customWarningDialog.showAnimated()
                                return;
                            }

                            // === LOGIKA 2: CEK WARNING TASK PENDING ===
                            if (!delegateRoot.isActive) {
                                var pendingStartedCount = logger.getPendingStartedTaskCount();

                                if (pendingStartedCount >= 3) {
                                    customWarningDialog.titleText = "Warning"
                                    customWarningDialog.newText = "Terdapat " + pendingStartedCount + " task pending yang sudah berjalan. Mohon segera di-review."
                                    customWarningDialog.showAnimated()
                                }
                            }

                            // === LOGIKA 3: JALANKAN TASK (Jika lolos validasi) ===
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
                                font { bold: true; pixelSize: Theme.fontSizeBody }
                                color: delegateRoot.isReview ? Theme.warningColor : Theme.textColor
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                                maximumLineCount: 1
                            }
                            Rectangle {
                                Layout.preferredWidth: 32
                                Layout.preferredHeight: 12
                                radius: 16
                                color: menuMouseArea.containsMouse ? Qt.rgba(0, 0, 0, 0.1) : "transparent"
                                visible: !(delegateRoot.isReview || delegateRoot.isNeedReview)

                                Rectangle {
                                    anchors.centerIn: parent
                                    width: 20
                                    height: 10
                                    radius: 10
                                    color: "transparent"
                                    border.color: Qt.alpha(Theme.lightTextColor, 0.6)
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
                                                color: Qt.alpha(Theme.lightTextColor, 0.8)
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
                                        stableTaskMenu.taskId = modelData.id
                                        stableTaskMenu.userId = logger.currentUserId
                                        stableTaskMenu.authToken = logger.authToken
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
                                font.pixelSize: Theme.fontSizeSmall
                                color: delegateRoot.isReview ? Qt.rgba(255/255, 152/255, 0/255, 0.8) : Theme.lightTextColor
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
                                color: viewAllArea.pressed ? Qt.alpha(Theme.primaryColor, 0.2) :
                                                             viewAllArea.containsMouse ? Qt.alpha(Theme.primaryColor, 0.1) :
                                                                                         Qt.alpha(Theme.primaryColor, 0.05)

                                border.color: Qt.alpha(Theme.primaryColor, 0.3)
                                border.width: 1

                                Text {
                                    anchors.centerIn: parent
                                    text: Lang.t("View All")
                                    font.pixelSize: 9
                                    font.bold: true
                                    color: Theme.primaryColor
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
                                    if (isExpired) return Qt.alpha(Theme.neutralColor, 0.2);
                                    if (modelData.status === "Review") return Qt.alpha(Theme.warningColor, 0.15);
                                    if (modelData.status === "Need Review") return Qt.alpha(Theme.infoColor, 0.15);
                                    if (modelData.status === "Need Revise") return Qt.alpha(Theme.dangerColor, 0.15);
                                    if (modelData.active) return modelData.isTaskPaused ? Qt.alpha(Theme.warningColor, 0.15) : Qt.alpha(Theme.successColor, 0.15);
                                    return Qt.alpha(Theme.lightTextColor, 0.1);
                                }

                                border.color: {
                                    if (isExpired) return Qt.alpha(Theme.neutralColor, 0.5);
                                    if (modelData.status === "Review") return Qt.alpha(Theme.warningColor, 0.4);
                                    if (modelData.status === "Need Review") return Qt.alpha(Theme.infoColor, 0.4);
                                    if (modelData.status === "Need Revise") return Qt.alpha(Theme.dangerColor, 0.4);
                                    if (modelData.active) return modelData.isTaskPaused ? Qt.alpha(Theme.warningColor, 0.4) : Qt.alpha(Theme.successColor, 0.4);
                                    return Qt.alpha(Theme.lightTextColor, 0.2);
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
                                        if (modelData.status === "Review") return Theme.warningColor;
                                        if (modelData.status === "Need Review") return Theme.infoColor;
                                        if (modelData.status === "Need Revise") return Theme.dangerColor;
                                        if (modelData.active) return modelData.isTaskPaused ? Theme.warningColor : Theme.successColor;
                                        return Theme.lightTextColor;
                                    }
                                }
                            }

                            Item { Layout.fillWidth: true }

                            // Time display with subtle background
                            Rectangle {
                                Layout.preferredWidth: timeLabel.implicitWidth + 8
                                Layout.preferredHeight: 18
                                radius: 4
                                color: Qt.alpha(Theme.lightTextColor, 0.05)

                                Label {
                                    id: timeLabel
                                    anchors.centerIn: parent
                                    text: logger.formatDuration(modelData.time_usage)
                                    font.pixelSize: 11
                                    font.bold: true
                                    color: delegateRoot.isReview ? Theme.warningColor : Theme.lightTextColor
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
