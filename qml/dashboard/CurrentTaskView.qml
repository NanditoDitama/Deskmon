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
    id: currentTaskRoot
    anchors.fill: parent

    function formatMaxDuration(seconds) {
        if (!seconds || seconds <= 0) return "0m"
        var hours = Math.floor(seconds / 3600)
        var minutes = Math.floor((seconds % 3600) / 60)
        var text = ""
        if (hours > 0) text += hours + "h "
        text += minutes + "m"
        return text
    }
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
    property int warnedTaskId: -1
    property int expiredTaskId: -1
    property int liveTickCounter: 0

    readonly property var activeTaskObj: {
        void(liveTickCounter);
        if (typeof logger === "undefined" || !logger || logger.activeTaskId === -1 || !logger.taskList) return null;
        return logger.taskList.find(task => task.id === logger.activeTaskId) || null;
    }

    Timer {
        id: activeTaskLiveTimer
        interval: 1000
        repeat: true
        running: typeof logger !== "undefined" && logger && logger.activeTaskId !== -1 && !logger.isTaskPaused
        onTriggered: {
            liveTickCounter = (liveTickCounter + 1) % 1000000
            if (activeTaskObj) {
                checkTaskTimeWarning(activeTaskObj.id, activeTaskObj.time_usage, activeTaskObj.max_time)
            }
        }
    }

    function checkTaskTimeWarning(taskId, timeUsage, maxTime) {
        if (!taskId || taskId <= 0 || !maxTime || maxTime <= 0) return;
        // Hanya tampilkan peringatan saat task sedang aktif berjalan (tidak Paused)
        if (typeof logger !== "undefined" && logger.isTaskPaused) return;

        var diffSeconds = maxTime - timeUsage;
        if (diffSeconds <= 0) {
            if (!isTimeUpPopupOpen && expiredTaskId !== taskId) {
                expiredTaskId = taskId;
                warnedTaskId = taskId;
                isTimeUpPopupOpen = true;
                customWarningDialog.titleText = Lang.t("Task Time Warning");
                customWarningDialog.newText = Lang.t("timeUpFinished");
                customWarningDialog.showAnimated();
            }
        } else if (diffSeconds <= 600) {
            if (!isTimeUpWarningOpen && warnedTaskId !== taskId) {
                warnedTaskId = taskId;
                isTimeUpWarningOpen = true;
                customWarningDialog.titleText = Lang.t("Task Time Warning");
                customWarningDialog.newText = Lang.t("timeUpWarning");
                customWarningDialog.showAnimated();
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
                                visible: activeTaskObj !== null && activeTaskObj.status !== "Review"

                                color: {
                                    if (activeTaskObj && activeTaskObj.status === "Review") {
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
                                            if (activeTaskObj && activeTaskObj.status === "Review") return Theme.warningColor
                                            return logger.isTaskPaused ? Theme.accentColor : Theme.productiveColor
                                        }

                                        SequentialAnimation on opacity {
                                            running: !logger.isTaskPaused && !(activeTaskObj && activeTaskObj.status === "Review")
                                            loops: Animation.Infinite
                                            NumberAnimation { from: 0.3; to: 1; duration: 700; easing.type: Easing.InOutQuad }
                                            NumberAnimation { from: 1; to: 0.3; duration: 700; easing.type: Easing.InOutQuad }
                                        }
                                    }

                                    Label {
                                        text: {
                                            if (activeTaskObj && activeTaskObj.status === "Review") return Lang.t("Review")
                                            return logger.isTaskPaused ? Lang.t("Paused") : Lang.t("Active")
                                        }
                                        font.pixelSize: 13
                                        font.weight: Font.DemiBold
                                        font.family: "Segoe UI"
                                        color: {
                                            if (activeTaskObj && activeTaskObj.status === "Review") return Theme.warningColor
                                            return logger.isTaskPaused ? Theme.accentColor : Theme.productiveColor
                                        }
                                    }
                                }
                            }

                            // Current Time
                            Label {
                                text: {
                                    if (activeTaskObj) {
                                        var hours = Math.floor(activeTaskObj.time_usage / 3600)
                                        var minutes = Math.floor((activeTaskObj.time_usage % 3600) / 60)
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
                                    if (activeTaskObj && activeTaskObj.time_usage > activeTaskObj.max_time) {
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
                                        if (activeTaskObj && activeTaskObj.max_time > 0) {
                                            return parent.width * Math.min(1, activeTaskObj.time_usage / activeTaskObj.max_time)
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
                                                if (activeTaskObj && activeTaskObj.time_usage > activeTaskObj.max_time) {
                                                    return Theme.nonProductiveColor
                                                }
                                                return Theme.secondaryColor
                                            }
                                        }
                                        GradientStop {
                                            position: 1.0
                                            color: {
                                                if (activeTaskObj && activeTaskObj.time_usage > activeTaskObj.max_time) {
                                                    return Qt.lighter(Theme.nonProductiveColor, 1.2)
                                                }
                                                return Qt.lighter(Theme.secondaryColor, 1.2)
                                            }
                                        }
                                    }

                                    Behavior on width {
                                        enabled: !(activeTaskObj && activeTaskObj.status === "Review")
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
                                    if (activeTaskObj && activeTaskObj.max_time > 0) {
                                        var percentage = Math.round((activeTaskObj.time_usage / activeTaskObj.max_time) * 100)
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
                                text: activeTaskObj ? currentTaskRoot.formatMaxDuration(activeTaskObj.max_time) : "0m"
                                font.pixelSize: 13
                                font.family: "Segoe UI"
                                color: Theme.lightTextColor
                            }

                            // Pause/Resume Button
                            CustomButton {
                                id: pauseResumeButton
                                visible: activeTaskObj !== null && activeTaskObj.status !== "Review"

                                text: logger.isTaskPaused ? Lang.t("Resume") : Lang.t("Pause")
                                Layout.preferredWidth: 92
                                Layout.preferredHeight: Theme.controlHeight
                                btnRadius: Theme.radiusSmall
                                btnColor: logger.isTaskPaused ? Theme.accentColor : Theme.productiveColor
                                btnHoverColor: logger.isTaskPaused ? Qt.lighter(Theme.accentColor, 1.15) : Qt.lighter(Theme.productiveColor, 1.15)
                                btnTextColor: "white"
                                font.pixelSize: 13
                                font.weight: Font.DemiBold

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

                function onTaskListChanged() {
                    if (activeTaskObj && !logger.isTaskPaused) {
                        checkTaskTimeWarning(activeTaskObj.id, activeTaskObj.time_usage, activeTaskObj.max_time)
                    }
                }

                function onActiveTaskChanged() {
                    if (logger.activeTaskId !== warnedTaskId) {
                        isTimeUpWarningOpen = false
                        warnedTaskId = -1
                    }
                    if (logger.activeTaskId !== expiredTaskId) {
                        isTimeUpPopupOpen = false
                        expiredTaskId = -1
                    }
                }

                function onTaskPausedChanged() {
                    taskControlRow.isLoading = false
                }

                function onAuthTokenError(message) {
                    taskControlRow.isLoading = false
                    console.error("API Error:", message)
                }
            }
        }

        HoverHandler {
            id: taskScrollHover
        }

        ScrollView {
            id: taskScrollView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            visible: taskListView.count > 0
            rightPadding: taskScrollBar.isScrollable ? 14 : 0

            ScrollBar.vertical: CustomScrollBar {
                id: taskScrollBar
                containerHovered: taskScrollHover.hovered
            }
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
                    readonly property bool isPendingNeedReview: logger && logger.pendingNeedReviewTaskIds
                                                                ? logger.pendingNeedReviewTaskIds.indexOf(modelData.id) !== -1
                                                                : false
                    readonly property bool isActive: modelData.active && !isPendingNeedReview
                    readonly property bool isExpired: modelData.isExpired
                    color: {
                        if (isPendingNeedReview) return Qt.alpha(Theme.neutralColor, 0.14)
                        if (isExpired) return Qt.alpha(Theme.neutralColor, 0.15)
                        if (isReview) {
                            return Qt.alpha(Theme.warningColor, 0.08)
                        }
                        if (isNeedReview) return Qt.alpha(Theme.infoColor, 0.15);
                        if (isNeedRevise) return Qt.alpha(Theme.dangerColor, 0.15);
                        return isActive ? Qt.lighter(Theme.cardColor, 1.6) : Theme.cardColor
                    }
                    border.color: {
                        if (isPendingNeedReview) return Qt.alpha(Theme.neutralColor, 0.4)
                        if (isExpired) return Qt.alpha(Theme.neutralColor, 0.4)
                        if (isReview) {
                            return Qt.alpha(Theme.warningColor, 0.3)
                        }
                        if (isNeedReview) return Qt.alpha(Theme.infoColor, 0.4);
                        if (isNeedRevise) return Qt.alpha(Theme.dangerColor, 0.4);

                        return isActive ? Theme.secondaryColor : Theme.borderColor
                    }
                    border.width: 1
                    opacity: isPendingNeedReview ? 0.68 : (isExpired ? 0.7 : (isReview ? 0.85 : 1.0))


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
                        enabled: !(delegateRoot.isReview || delegateRoot.isNeedReview || delegateRoot.isPendingNeedReview)

                        onClicked: {
                            // === LOGIKA 1: CEK TASK EXPIRED (Bulan Lalu) ===
                            if (logger.isTaskExpired(modelData.id)) {
                                customWarningDialog.titleText = Lang.t("Task Expired")
                                customWarningDialog.newText = Lang.t("Tasks from last month cannot be started. Please mark as Need Review.")
                                customWarningDialog.showAnimated()
                                return;
                            }

                            // === LOGIKA 2 & 3: CEK WARNING PENDING & JALANKAN TASK ===
                            var pendingWarningText = ""
                            if (!delegateRoot.isActive) {
                                var pendingStartedCount = logger.getPendingStartedTaskCount();
                                if (pendingStartedCount >= 3) {
                                    pendingWarningText = Lang.t("There are %1 pending tasks already started. Please review them soon.").arg(pendingStartedCount)
                                }
                            }

                            if (!delegateRoot.isActive && logger.activeTaskId !== -1) {
                                confirmSwitchDialog.taskId = modelData.id
                                confirmSwitchDialog.warningMessage = pendingWarningText
                                confirmSwitchDialog.open()
                            } else if (!delegateRoot.isActive) {
                                if (pendingWarningText !== "") {
                                    customWarningDialog.titleText = Lang.t("Warning")
                                    customWarningDialog.newText = pendingWarningText
                                    customWarningDialog.showAnimated()
                                }
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
                                visible: !(delegateRoot.isReview || delegateRoot.isNeedReview || delegateRoot.isPendingNeedReview)

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
                            spacing: 6

                            // Status badge
                            Rectangle {
                                Layout.preferredHeight: 18
                                Layout.preferredWidth: statusText.implicitWidth + 12
                                radius: 9
                                color: {
                                    if (delegateRoot.isPendingNeedReview || isExpired) return Qt.alpha(Theme.neutralColor, 0.2);
                                    if (modelData.status === "Review") return Qt.alpha(Theme.warningColor, 0.15);
                                    if (modelData.status === "Need Review") return Qt.alpha(Theme.infoColor, 0.15);
                                    if (modelData.status === "Need Revise") return Qt.alpha(Theme.dangerColor, 0.15);
                                    if (modelData.active) return modelData.isTaskPaused ? Qt.alpha(Theme.warningColor, 0.15) : Qt.alpha(Theme.successColor, 0.15);
                                    return Qt.alpha(Theme.lightTextColor, 0.1);
                                }

                                border.color: {
                                    if (delegateRoot.isPendingNeedReview || isExpired) return Qt.alpha(Theme.neutralColor, 0.5);
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
                                        if (delegateRoot.isPendingNeedReview) return Lang.t("Syncing Need Review...");
                                        if (modelData.status === "Review") return Lang.t("Review");
                                        if (modelData.status === "Need Review") return Lang.t("Need Review");
                                        if (modelData.status === "Need Revise") return Lang.t("Need Revise");
                                        if (modelData.active) return modelData.isTaskPaused ? Lang.t("Paused") : Lang.t("Running");
                                        return Lang.t("Pending");
                                    }
                                    font.pixelSize: 10
                                    font.bold: true
                                    color: {
                                        if (delegateRoot.isPendingNeedReview) return Theme.mutedTextColor;
                                        if (modelData.status === "Review") return Theme.warningColor;
                                        if (modelData.status === "Need Review") return Theme.infoColor;
                                        if (modelData.status === "Need Revise") return Theme.dangerColor;
                                        if (modelData.active) return modelData.isTaskPaused ? Theme.warningColor : Theme.successColor;
                                        return Theme.lightTextColor;
                                    }
                                }
                            }

                            BusyIndicator {
                                visible: delegateRoot.isPendingNeedReview
                                running: delegateRoot.isPendingNeedReview
                                Layout.preferredWidth: 16
                                Layout.preferredHeight: 16
                            }

                            Item { Layout.fillWidth: true }

                            // Time display with subtle background
                            Rectangle {
                                Layout.preferredWidth: timeRow.implicitWidth + 10
                                Layout.preferredHeight: 18
                                radius: 4
                                color: (modelData.max_time > 0 && modelData.time_usage > modelData.max_time)
                                       ? Qt.alpha(Theme.nonProductiveColor, 0.1)
                                       : Qt.alpha(Theme.lightTextColor, 0.06)

                                RowLayout {
                                    id: timeRow
                                    anchors.centerIn: parent
                                    spacing: 4

                                    Label {
                                        id: timeLabel
                                        Layout.alignment: Qt.AlignBaseline
                                        text: currentTaskRoot.formatMaxDuration(modelData.time_usage)
                                        font.pixelSize: 11
                                        font.bold: true
                                        color: {
                                            if (modelData.max_time > 0 && modelData.time_usage > modelData.max_time)
                                                return Theme.nonProductiveColor
                                            if (delegateRoot.isPendingNeedReview)
                                                return Theme.mutedTextColor
                                            if (delegateRoot.isReview)
                                                return Theme.warningColor
                                            if (modelData.active)
                                                return modelData.isTaskPaused ? Theme.warningColor : Theme.primaryColor
                                            return Theme.textColor
                                        }
                                    }

                                    Label {
                                        Layout.alignment: Qt.AlignBaseline
                                        visible: modelData.max_time > 0
                                        text: "/"
                                        font.pixelSize: 10
                                        color: Theme.mutedTextColor
                                        opacity: 0.6
                                    }

                                    Label {
                                        id: maxTimeLabel
                                        Layout.alignment: Qt.AlignBaseline
                                        visible: modelData.max_time > 0
                                        text: currentTaskRoot.formatMaxDuration(modelData.max_time)
                                        font.pixelSize: 10
                                        font.weight: Font.Normal
                                        color: Theme.mutedTextColor
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
