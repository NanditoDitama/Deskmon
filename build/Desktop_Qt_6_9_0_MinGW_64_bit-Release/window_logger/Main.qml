import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material
import QtQuick.Dialogs
import QtQuick.Effects
import QtQuick.Window
import QtQuick 2.15

Window {
    id: window
    title: qsTr("Deskmon - v" + appVersion)
    visibility: Window.Maximized
    minimumWidth: 900
    minimumHeight: 700
    property string appVersion: "1.0.3.2"
    color: cardColor



    // Fungsi ini akan berjalan saat sinyal 'closing' terdeteksi
    function onClosing(close) {
        console.log("Tombol 'X' ditekan. Menyembunyikan window ke tray.")

        // 1. Perintahkan agar window tidak benar-benar ditutup
        close.accepted = false

        // 2. Akses window utama dan sembunyikan
        // Kita perlu mencari window utama dari C++. Ini cara yang aman:
        if (window.parent) {
            window.parent.hide()
        }
    }
    onClosing: {
        // 'close' adalah argumen event yang berisi informasi
        console.log("Tombol 'X' ditekan. Menyembunyikan window ke tray.")

        // 1. Perintahkan agar window tidak benar-benar ditutup
        close.accepted = false

        // 2. Sembunyikan window secara manual
        window.hide()
    }


    Rectangle {
        id: notification
        parent: win.overlay

        // Posisi: pojok kanan bawah dengan margin dinamis
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: 60
        anchors.rightMargin: 20

        // Compact size - max width 360px
        width: Math.min(Math.max(contentRow.implicitWidth + 32, 240), 360)
        height: contentRow.implicitHeight + 34

        radius: 12
        color: {
            switch(ntype) {
            case "success": return cardColor
            case "warning": return cardColor
            case "error": return cardColor
            default: return cardColor
            }
        }

        // Subtle border dengan warna sesuai type
        border.width: 1.5
        border.color: {
            switch(ntype) {
            case "success": return primaryColor
            case "warning": return "#F59E0B20"
            case "error": return "#EF444420"
            default: return "#E5E7EB"
            }
        }

        opacity: 0.0
        visible: opacity > 0 || entering || leaving
        z: 99999

        // Smooth rendering
        layer.enabled: true
        layer.smooth: true
        layer.samples: 4

        // Properties
        property string ntype: "success"
        property string message: ""
        property int duration: 3500
        property bool entering: false
        property bool leaving: false

        // SVG Icons sebagai string
        property string successIcon: "data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='20' height='20' viewBox='0 0 24 24' fill='none' stroke='%2310B981' stroke-width='2.5' stroke-linecap='round' stroke-linejoin='round'%3E%3Cpolyline points='20 6 9 17 4 12'%3E%3C/polyline%3E%3C/svg%3E"

        property string warningIcon: "data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='20' height='20' viewBox='0 0 24 24' fill='none' stroke='%23F59E0B' stroke-width='2.5' stroke-linecap='round' stroke-linejoin='round'%3E%3Ccircle cx='12' cy='12' r='10'%3E%3C/circle%3E%3Cline x1='12' y1='8' x2='12' y2='12'%3E%3C/line%3E%3Cline x1='12' y1='16' x2='12.01' y2='16'%3E%3C/line%3E%3C/svg%3E"

        property string errorIcon: "data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='20' height='20' viewBox='0 0 24 24' fill='none' stroke='%23EF4444' stroke-width='2.5' stroke-linecap='round' stroke-linejoin='round'%3E%3Ccircle cx='12' cy='12' r='10'%3E%3C/circle%3E%3Cline x1='15' y1='9' x2='9' y2='15'%3E%3C/line%3E%3Cline x1='9' y1='9' x2='15' y2='15'%3E%3C/line%3E%3C/svg%3E"

        property string closeIcon: "data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='16' height='16' viewBox='0 0 24 24' fill='none' stroke='%239CA3AF' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'%3E%3Cline x1='18' y1='6' x2='6' y2='18'%3E%3C/line%3E%3Cline x1='6' y1='6' x2='18' y2='18'%3E%3C/line%3E%3C/svg%3E"

        // Content
        Row {
            id: contentRow
            anchors.fill: parent
            anchors.margins: 12
            spacing: 12

            // Icon Container dengan background subtle
            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: 36
                height: 36
                radius: 8
                color: {
                    switch(notification.ntype) {
                    case "success": return "#10B98110"
                    case "warning": return "#F59E0B10"
                    case "error": return "#EF444410"
                    default: return "#F3F4F6"
                    }
                }

                Image {
                    anchors.centerIn: parent
                    width: 20
                    height: 20
                    source: {
                        switch(notification.ntype) {
                        case "success": return notification.successIcon
                        case "warning": return notification.warningIcon
                        case "error": return notification.errorIcon
                        default: return notification.successIcon
                        }
                    }
                    smooth: true
                    antialiasing: true
                }
            }

            // Message
            Text {
                id: messageText
                text: notification.message
                width: notification.width - 36 - 32 - 36 - 12 // icon - margins - close - spacing
                anchors.verticalCenter: parent.verticalCenter
                color: textColor
                font.pixelSize: 16
                font.weight: Font.Medium
                wrapMode: Text.WordWrap
                elide: Text.ElideRight
                maximumLineCount: 2
            }

            // Close Button
            MouseArea {
                id: closeArea
                width: 32
                height: 32
                anchors.verticalCenter: parent.verticalCenter
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: notification.hide()

                Rectangle {
                    anchors.centerIn: parent
                    width: 28
                    height: 28
                    radius: 6
                    color: closeArea.containsMouse ? "#F3F4F6" : "transparent"

                    Behavior on color { ColorAnimation { duration: 150 } }

                    Image {
                        anchors.centerIn: parent
                        width: 16
                        height: 16
                        source: notification.closeIcon
                        smooth: true
                        antialiasing: true
                        opacity: closeArea.containsMouse ? 1.0 : 0.6

                        Behavior on opacity { NumberAnimation { duration: 150 } }
                    }
                }
            }
        }

        // Progress bar di bawah (thin accent line)
        Rectangle {
            id: progressBar
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.leftMargin: 1.5
            height: 2.5
            radius: 2
            width: parent.width - 3
            color: {
                switch(notification.ntype) {
                case "success": return primaryColor
                case "warning": return "#F59E0B"
                case "error": return "#EF4444"
                default: return "#10B981"
                }
            }
            opacity: 0.4
        }

        // Animasi
        Behavior on opacity {
            NumberAnimation { duration: 200; easing.type: Easing.OutCubic }
        }

        Behavior on anchors.rightMargin {
            NumberAnimation { duration: 300; easing.type: Easing.OutCubic }
        }

        Behavior on anchors.bottomMargin {
            NumberAnimation { duration: 300; easing.type: Easing.OutCubic }
        }

        // Scale animation untuk entrance
        scale: opacity

        Behavior on scale {
            NumberAnimation { duration: 200; easing.type: Easing.OutBack }
        }

        // Timers
        Timer {
            id: lifetime
            interval: notification.duration
            repeat: false
            onTriggered: notification.hide()
        }

        // Progress animation
        PropertyAnimation {
            id: progressAnim
            target: progressBar
            property: "width"
            from: notification.width - 3
            to: 0
            duration: notification.duration
            easing.type: Easing.Linear
        }

        // Functions
        function show(type, msg) {
            ntype = type || "success"
            message = msg || ""

            // Reset state
            entering = true
            leaving = false
            opacity = 1.0
            scale = 1.0

            // Start animations
            lifetime.restart()
            progressBar.width = notification.width - 3
            progressAnim.stop()
            progressAnim.start()

            // Auto-adjust duration based on message length
            var wordCount = message.split(' ').length
            if (wordCount > 15) {
                duration = 5000
                lifetime.interval = 5000
                progressAnim.duration = 5000
            } else {
                duration = 3500
                lifetime.interval = 3500
                progressAnim.duration = 3500
            }
        }

        function hide() {
            if (leaving) return
            leaving = true
            lifetime.stop()
            progressAnim.stop()
            opacity = 0.0
            scale = 0.95
        }

        // Reset on opacity complete
        onOpacityChanged: {
            if (opacity === 0.0 && leaving) {
                leaving = false
            }
        }
    }

    // Connections (tambahkan di luar Rectangle notification)
    Connections {
        target: logger
        function onShowNotification(type, message) {
            notification.show(type, message)
        }
    }



    property var appDurations: ({})
    property var sortedApps: []
    property var sortedDomains: []
    property bool showAllPercentages: false
    property string startDate: ""
    property string endDate: ""
    property bool isLoggedIn: false
    property string currentUsername: ""
    property date startSelectedDate: new Date(NaN)
    property date endSelectedDate: new Date(NaN)
    property bool isDateSelected: false
    property bool isProfileVisible: false
    property string profileImagePath: ":/profilImage.png"
    property string tempUsername: ""
    property string tempPassword: ""
    property string tempImagePath: ""
    property string tempDepartment: ""

    property bool showPassword: false
    property bool showRegPassword: false
    property bool showRegisterPage: false

    property string visibilityIcon: "qrc:/icons/visibility.svg"
    property string visibilityOffIcon: "qrc:/icons/visibility_off.svg"



    property color primaryColor: "#00e0a8"
    property color secondaryColor: "#3B82F6"
    property color accentColor: "#F59E0B"

    property var public_curent_time: ""
    property var public_max_time: ""
    property bool isPop_up_waktuhabis_open: false
    property bool isPop_up_waktuhabis_kurangdari_open: false


    property bool isDarkMode: false
    property color backgroundColor: isDarkMode ? "#121212" : "#F1F5F9"
    property color cardColor: isDarkMode ? "#1E1E1E" : "#FFFFFF"
    property color textColor: isDarkMode ? "#FFFFFF" : "#1F2937"
    property color lightTextColor: isDarkMode ? "#B0B0B0" : "#6B7280"
    property color dividerColor: isDarkMode ? "#333333" : "#E5E7EB"
    property color headers : isDarkMode ? "#1E1E1E" : "#00e0a8"
    // Coba deteksi tema sistem saat startup
    Component.onCompleted: {

        if (typeof Qt.styleHints !== "undefined") {
            isDarkMode = Qt.styleHints.colorScheme === Qt.Dark
        }
        else {
            isDarkMode = Material.theme === Material.Dark
        }
        if (logger.currentUserId === -1) {
            stackView.clear()
            stackView.push("Login_page.qml")
            console.log("ke halaman login untuk login ulang")
        }
        window.visibility = Window.Maximized

    }

    function goToLoginPage() {
        stackView.clear()
        stackView.push(Qt.resolvedUrl("Login_page.qml"))
        console.log("ke halaman login untuk login ulang2")
    }


    Connections {
        target: logger
        function onRequestLoginPage() {
            isLoggedIn = false
            isProfileVisible = false
            showRegisterPage = false
            showIdleNotification = false
            console.log("mencoba untuk login ulang")
        }
    }


    // Sync Material theme dengan mode kita
    Material.theme: isDarkMode ? Material.Dark : Material.Light

    property color selectedColor: "#3B82F6"
    property color rangeColor: "#DBEAFE"
    property color productiveColor: primaryColor
    property color nonProductiveColor: "#ff5100"
    property color neutralColor: "#bdbdbd"

    property int currentMonth: new Date().getMonth()
    property int currentYear: new Date().getFullYear()



    property bool showIdleNotification: false
    property string idleNotificationText: ""




    Pop_up_waktuhabis {
        id: warningWindowComponent
    }

    function timeStringToMinutes(timeStr) {
        // Handle format "Xh Ym" atau "XX:XX"
        if (timeStr.includes("h") || timeStr.includes("m")) {
            // Format "Xh Ym" (contoh: 1h 30m)
            var hours = 0;
            var minutes = 0;

            // Ekstrak jam
            var hourIndex = timeStr.indexOf("h");
            if (hourIndex !== -1) {
                hours = parseInt(timeStr.substring(0, hourIndex).trim()) || 0;
            }

            // Ekstrak menit
            var minIndex = timeStr.indexOf("m");
            if (minIndex !== -1) {
                var minPart = timeStr.substring(hourIndex !== -1 ? hourIndex + 1 : 0, minIndex).trim();
                minutes = parseInt(minPart) || 0;
            }

            return hours * 60 + minutes;
        } else {
            // Format "XX:XX" (asumsi jam:menit)
            var parts = timeStr.split(":");
            if (parts.length !== 2) return 0;
            return (parseInt(parts[0]) || 0) * 60 + (parseInt(parts[1]) || 0);
        }
    }

    onPublic_curent_timeChanged: {
        console.log("Max:", public_max_time, "Current:", public_curent_time)

        // Konversi ke menit
        var maxMinutes = timeStringToMinutes(public_max_time)
        var currentMinutes = timeStringToMinutes(public_curent_time)
        var diffMinutes = maxMinutes - currentMinutes

        console.log("Selisih menit:", diffMinutes)

        if (diffMinutes <= 0) {

            console.log ("chek 1", isPop_up_waktuhabis_open)
            if(isPop_up_waktuhabis_open == false){
                warningWindowComponent.newText = "Waktu Task anda Sudah Habis"
                isPop_up_waktuhabis_open = true
                warningWindowComponent.show()
                console.log("Waktu Task anda sudah habis!")
            }
        }
        else if(diffMinutes <= 10) {
            console.log ("chek 2",isPop_up_waktuhabis_kurangdari_open)
            if(isPop_up_waktuhabis_kurangdari_open == false){
                warningWindowComponent.newText = "Waktu Task anda tersisa kurang dari 10 menit!"
                isPop_up_waktuhabis_kurangdari_open = true
                warningWindowComponent.show()
                console.log("Waktu Task anda tersisa kurang dari 10 menit!")
            }
            // Tampilkan peringatan

        }
    }

    function formatDuration(seconds) {
        if (seconds < 60) {
            return seconds + "s"
        } else if (seconds < 3600) {
            var minutes = Math.floor(seconds / 60)
            var secs = seconds % 60
            return minutes + "m " + secs + "s"
        } else {
            var hours = Math.floor(seconds / 3600)
            var mins = Math.floor((seconds % 3600) / 60)
            var secs = seconds % 60
            return hours + "h " + mins + "m " + secs + "s"
        }
    }

    function updateAppDurations() {
        if (!isLoggedIn) return;
        var appDurations = {};
        var logs = logger.logContent.split('\n');
        var totalDuration = 0;

        for (var i = 0; i < logs.length; i++) {
            var parts = logs[i].split(',');
            if (parts.length >= 4 && parts[2].trim() !== '' && parts[3].trim() !== '') {
                var appName = parts[2].trim();
                // Abaikan entri 'Idle'
                if (appName === "Idle") continue;

                var start = parts[0].trim();
                var end = parts[1].trim();
                var startTime = new Date("2000-01-01 " + start);
                var endTime = new Date("2000-01-01 " + end);
                var durationSec = (endTime - startTime) / 1000;

                if (durationSec > 0) {
                    if (appDurations[appName] === undefined) {
                        appDurations[appName] = 0;
                    }
                    appDurations[appName] += durationSec;
                    totalDuration += durationSec;
                }
            }
        }

        var appArray = [];
        for (var app in appDurations) {
            var percentage = totalDuration > 0 ? (appDurations[app] / totalDuration) * 100 : 0;
            // Tambahkan productivityType ke objek
            // Dalam updateAppDurations()
            appArray.push({
                              name: app,
                              duration: appDurations[app],
                              percentage: percentage,
                              productivityType: getProductivityType(app, "") // URL kosong untuk app
                          });


        }



        appArray.sort((a, b) => b.duration - a.duration);
        sortedApps = appArray;
    }

    function updateDomainDurations() {
        if (!isLoggedIn) return;
        var domainDurations = {};
        var logs = logger.logContent.split('\n').filter(line => line.trim() !== '');
        var totalDuration = 0;

        for (var i = 0; i < logs.length; i++) {
            var parts = logs[i].split(',');
            // Pastikan ada 5 bagian untuk menyertakan URL
            if (parts.length >= 5 && parts[4] && parts[4].trim() !== '') {
                var url = parts[4].trim();
                var domain = extractDomain(url);
                if (domain) {
                    var start = parts[0].trim();
                    var end = parts[1].trim();
                    var startTime = new Date("2000-01-01 " + start);
                    var endTime = new Date("2000-01-01 " + end);
                    var durationSec = (endTime - startTime) / 1000;

                    if (durationSec > 0) {
                        if (domainDurations[domain] === undefined) {
                            domainDurations[domain] = 0;
                        }
                        domainDurations[domain] += durationSec;
                        totalDuration += durationSec;
                    }
                }
            }
        }

        var domainArray = [];
        for (var domain in domainDurations) {
            var percentage = totalDuration > 0 ? (domainDurations[domain] / totalDuration) * 100 : 0;
            // Dalam updateDomainDurations()
            domainArray.push({
                                 name: domain,
                                 duration: domainDurations[domain],
                                 percentage: percentage,
                                 productivityType: getProductivityType(domain, domain)
                             });
        }

        domainArray.sort((a, b) => b.duration - a.duration);
        sortedDomains = domainArray;
    }

    Connections {
        target: logger
        function onLogContentChanged() {
            updateAppDurations();
            updateDomainDurations(); // Tambahkan ini
        }
    }


    function getProductivityType(name, url) {
        var typeInt = logger.getAppProductivityType(name, url);

        switch(typeInt) {
        case 1: return "productive";
        case 2: return "non-productive";
        default: return "neutral";


        }
    }

    function extractDomain(urlString) {
        if (!urlString || urlString.trim() === "") {
            return "";
        }
        try {
            // Tambahkan "https://" jika URL tidak memiliki protokol, karena ini diperlukan oleh parser.
            let fullUrl = urlString.startsWith("http") ? urlString : "https://" + urlString;

            // Gunakan parser URL bawaan untuk keamanan dan keandalan
            let url = new URL(fullUrl);
            let hostname = url.hostname;

            // Hapus subdomain "www." jika ada
            if (hostname.startsWith("www.")) {
                return hostname.substring(4);
            }
            return hostname;

        } catch (e) {
            // Jika parsing gagal (misalnya, string bukanlah URL yang valid),
            // coba ambil bagian pertama sebelum garis miring sebagai fallback.

            let domain = urlString.split('/')[0];
            if (domain.startsWith("www.")) {
                return domain.substring(4);
            }
            return domain;
        }
    }


    // Di bagian JavaScript (logger.js atau file model):
    function fetchAndStoreTasks() {
        const sortedTasks = rawTasks.sort((a, b) => {
                                              if (a.active === b.active) return 0;
                                              return a.active ? -1 : 1; // Active tasks first
                                          });
        this.taskList = sortedTasks; // Perbarui model
    }
    function applyDateRange() {
        if (isNaN(startSelectedDate.getTime()) || isNaN(endSelectedDate.getTime())) {
            errorLabel.text = "Please select both start and end dates"
            return
        }

        startDate = Qt.formatDate(startSelectedDate, "yyyy-MM-dd")
        endDate = Qt.formatDate(endSelectedDate, "yyyy-MM-dd")

        console.log("Setting date filter - Start:", startDate, "End:", endDate)

        // Cukup panggil setLogFilter. Pembaruan data akan ditangani oleh sinyal onLogContentChanged.
        logger.setLogFilter(startDate, endDate)

        errorLabel.text = ""

        // Logika untuk memperbarui teks tombol tetap ada.
        var rangeText = Qt.formatDate(startSelectedDate, "MMM d, yyyy")
        if (Qt.formatDate(startSelectedDate, "yyyy-MM-dd") !== Qt.formatDate(endSelectedDate, "yyyy-MM-dd")) {
            rangeText += " - " + Qt.formatDate(endSelectedDate, "MMM d, yyyy")
        }
        dateRangeButton.text = rangeText
    }



    // Modify the profileImageChanged signal handler
    Connections {
        target: logger
        function onProfileImageChanged(username, newPath) {
            if (username === currentUsername) {
                console.log("Profile image changed for", username, "to", newPath)
                profileImagePath = newPath // Update with the new path (includes timestamp)
                refreshProfileImage()
            }
        }
    }


    Connections {
        target: sortedApps
        function onModelUpdated() {
            // Trigger re-animation saat data diupdate
            percentageListView.model = showAllPercentages ? sortedApps : sortedApps.slice(0, 4)
        }
    }


    Connections {
        target: Logger
        onTaskListChanged: {
            console.log("Task list changed, fetching tasks...")
            taskModel = Logger.taskList()
        }
    }


    Connections {
        target: logger

        function onTaskReviewNotification(message) {
            console.log("Review notification:", message);

            // Tampilkan notifikasi popup
            if (typeof reviewNotificationPopup !== 'undefined') {
                reviewNotificationPopup.showNotification(message);
            }

            // Tampilkan notifikasi system tray
            if (typeof SystemTrayIcon !== 'undefined' && SystemTrayIcon.supportsMessages) {
                SystemTrayIcon.showMessage("Task Review", message);
            }

            // Tampilkan notifikasi di UI
            notification.idleNotificationText = message;
            notification.show();
        }

        function onTaskStatusChanged(taskId, newStatus) {
            if (newStatus === "review") {
                // Anda bisa tambahkan logika tambahan di sini
                console.log("Task status changed to review:", taskId);
            }
        }
    }

    function showEarlyLeaveDialog() {
        earlyLeaveReasonDialog.open();
    }


    Connections {
        target: logger
        function onRequestTaskDetails(taskId, action, nextTaskId) {
            taskDetailsDialog.show(taskId, action, nextTaskId);
        }
        function onTaskDetailsSubmissionSuccess() {
            taskDetailsDialog.loadingIndicator.running = false;
            taskDetailsDialog.close();
        }

        // Hubungkan sinyal GAGAL yang BARU
        function onTaskDetailsSubmissionFailedWithRetry(errorMessage, taskId, details, action, nextTaskId) {
            // Sembunyikan loading di dialog utama dan tutup
            taskDetailsDialog.loadingIndicator.running = false;
            taskDetailsDialog.close();

            // Tampilkan dialog kegagalan dengan data yang relevan
            failureDialog.show(errorMessage, taskId, details, action, nextTaskId);
        }
    }

    function showTaskDetailsDialog(taskId, action, nextTaskId) {
        taskDetailsDialog.show(taskId, action, nextTaskId);
    }

    TaskDetailsDialog {
        id: taskDetailsDialog
    }

    FailureDialog {
        id: failureDialog
    }


    // TAMBAHKAN KOMPONEN DIALOG INI DI DALAM ApplicationWindow
    EarlyLeaveDialog {
        id: earlyLeaveReasonDialog
    }

    NeedReviewDialog{
        id: needReviewReasonDialog
    }


    Popup {
        id: reviewNotificationPopup
        width: 340
        height: 120
        x: (parent.width - width) / 2
        y: 50
        modal: false
        closePolicy: Popup.NoAutoClose
        padding: 0
        topInset: 0
        leftInset: 0
        rightInset: 0
        bottomInset: 0

        // Theme colors
        readonly property bool isDarkMode: Qt.application.styleHints.colorScheme === Qt.Dark
        property color cardColor: isDarkMode ? "#2d2d2d" : "#ffffff"
        property color textColor: isDarkMode ? "#f0f0f0" : "#1a1a1a"
        property color lightTextColor: isDarkMode ? "#b0b0b0" : "#666666"
        property color warningColor: isDarkMode ? "#ff9500" : "#ff6b00"

        // Entrance animation
        enter: Transition {
            ParallelAnimation {
                NumberAnimation {
                    property: "opacity"
                    from: 0
                    to: 1
                    duration: 250
                    easing.type: Easing.OutCubic
                }
                NumberAnimation {
                    property: "y"
                    from: 20
                    to: 50
                    duration: 300
                    easing.type: Easing.OutBack
                    easing.overshoot: 1.2
                }
            }
        }

        exit: Transition {
            NumberAnimation {
                property: "opacity"
                to: 0
                duration: 200
                easing.type: Easing.InCubic
            }
        }

        background: Rectangle {
            color: cardColor
            radius: 12
            border.color: Qt.rgba(warningColor.r, warningColor.g, warningColor.b, 0.15)
            border.width: 1
        }

        contentItem: RowLayout {
            spacing: 12
            anchors.fill: parent
            anchors.margins: 16

            // Icon modern dengan pulse
            Rectangle {
                width: 36
                height: 36
                radius: 18
                color: Qt.rgba(warningColor.r, warningColor.g, warningColor.b, 0.12)
                Layout.alignment: Qt.AlignVCenter

                // Pulse animation
                SequentialAnimation on scale {
                    running: reviewNotificationPopup.visible
                    loops: Animation.Infinite
                    NumberAnimation { from: 1.0; to: 1.08; duration: 1200; easing.type: Easing.InOutQuad }
                    NumberAnimation { from: 1.08; to: 1.0; duration: 1200; easing.type: Easing.InOutQuad }
                }

                Image {
                    anchors.centerIn: parent
                    width: 20
                    height: 20
                    source: "qrc:/icon.ico"
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                }
            }

            // Content
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 4

                Label {
                    text: "Task Review Reminder"
                    font.pixelSize: 15
                    font.weight: Font.DemiBold
                    color: textColor
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }

                Label {
                    id: reviewNotificationText
                    text: "You have tasks pending review. Please check them before the deadline."
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                    color: Qt.rgba(textColor.r, textColor.g, textColor.b, 0.7)
                    font.pixelSize: 12
                    lineHeight: 1.3
                    maximumLineCount: 2
                    elide: Text.ElideRight
                }
            }

            // Close button
            Button {
                Layout.alignment: Qt.AlignTop
                Layout.preferredWidth: 28
                Layout.preferredHeight: 28
                padding: 0

                background: Rectangle {
                    radius: 6
                    color: parent.hovered ? Qt.rgba(textColor.r, textColor.g, textColor.b, 0.1) : "transparent"

                    Behavior on color { ColorAnimation { duration: 150 } }
                }

                contentItem: Text {
                    text: "×"
                    font.pixelSize: 20
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: Qt.rgba(textColor.r, textColor.g, textColor.b, 0.5)
                }

                onClicked: reviewNotificationPopup.close()
            }
        }

        function showNotification(message) {
            reviewNotificationText.text = message
            open()
            notificationTimer.restart()
        }

        Timer {
            id: notificationTimer
            interval: 10000
            onTriggered: reviewNotificationPopup.close()
        }
    }

    // Function to show review notification from C++
    function showReviewNotification(message) {
        if (visibility === Window.Windowed || visibility === Window.Maximized) {
            reviewNotificationPopup.showNotification(message)
        }
    }





    ApplicationWindow {
        id: idleNotificationWindow
        width: 340
        height: 180
        title: "Idle Detected"
        modality: Qt.ApplicationModal
        flags: Qt.Dialog | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
        color: "transparent"

        // Pusatkan window
        x: (Screen.width  - width)  / 2
        y: (Screen.height - height) / 2

        // Tampilkan/hidden via flag eksternal
        visible: showIdleNotification
        onVisibleChanged: if (!visible) showIdleNotification = false

        // === KARTU MODERN ====================================================
        Rectangle {
            id: card
            anchors.fill: parent
            radius: 16
            color: cardColor
            border.color: Qt.rgba(primaryColor.r, primaryColor.g, primaryColor.b, 0.15)
            border.width: 1
            opacity: showIdleNotification ? 1 : 0
            scale: showIdleNotification ? 1 : 0.96

            Behavior on opacity { NumberAnimation { duration: 250; easing.type: Easing.OutCubic } }
            Behavior on scale { NumberAnimation { duration: 250; easing.type: Easing.OutBack; easing.overshoot: 1.2 } }

            // === KONTEN =======================================================
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 24
                spacing: 20

                // HEADER: ikon + judul
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 14

                    // ikon modern dengan gradient
                    Rectangle {
                        width: 40
                        height: 40
                        radius: 20
                        color: Qt.rgba(primaryColor.r, primaryColor.g, primaryColor.b, 0.12)
                        Layout.alignment: Qt.AlignVCenter

                        // animasi pulse halus
                        SequentialAnimation on scale {
                            running: idleNotificationWindow.visible
                            loops: Animation.Infinite
                            NumberAnimation { from: 1.0; to: 1.08; duration: 1200; easing.type: Easing.InOutQuad }
                            NumberAnimation { from: 1.08; to: 1.0; duration: 1200; easing.type: Easing.InOutQuad }
                        }

                        Image {
                            anchors.centerIn: parent
                            width: 24
                            height: 24
                            source: "qrc:/icon.ico"
                            fillMode: Image.PreserveAspectFit
                            smooth: true
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Label {
                            text: "Idle Terdeteksi"
                            font.pixelSize: 17
                            font.weight: Font.DemiBold
                            color: textColor
                            Layout.fillWidth: true
                        }

                        Label {
                            text: "Aktivitas dihentikan sementara"
                            font.pixelSize: 13
                            color: Qt.rgba(textColor.r, textColor.g, textColor.b, 0.6)
                            Layout.fillWidth: true
                        }
                    }
                }

                // SPACER
                Item { Layout.fillHeight: true }

                // TOMBOL AKSI MODERN
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Item { Layout.fillWidth: true }

                    Button {
                        id: btnDismiss
                        text: "Dismiss"

                        leftPadding: 18
                        rightPadding: 18
                        topPadding: 10
                        bottomPadding: 10

                        background: Rectangle {
                            radius: 8
                            color: btnDismiss.hovered ? Qt.rgba(textColor.r, textColor.g, textColor.b, 0.08) : "transparent"
                            border.width: 0

                            Behavior on color { ColorAnimation { duration: 150 } }
                        }

                        contentItem: Text {
                            text: btnDismiss.text
                            font.pixelSize: 13
                            font.weight: Font.Medium
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            color: Qt.rgba(textColor.r, textColor.g, textColor.b, 0.7)
                        }

                        onClicked: {
                            showIdleNotification = false
                            idleNotificationWindow.close()
                        }
                    }

                    Button {
                        id: btnResume
                        text: "Resume"

                        leftPadding: 24
                        rightPadding: 24
                        topPadding: 10
                        bottomPadding: 10

                        background: Rectangle {
                            radius: 8
                            color: btnResume.pressed ? Qt.darker(primaryColor, 1.1) :
                                                       btnResume.hovered ? Qt.lighter(primaryColor, 1.05) : primaryColor
                            border.width: 0

                            Behavior on color { ColorAnimation { duration: 150 } }
                        }

                        contentItem: Text {
                            text: btnResume.text
                            font.pixelSize: 13
                            font.weight: Font.DemiBold
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            color: "white"
                        }

                        onClicked: {
                            showIdleNotification = false
                            idleNotificationWindow.close()
                            if (logger && logger.isTaskPaused)
                                logger.toggleTaskPause()
                        }
                    }
                }
            }
        }

        // === SHORTCUTS ============================================================
        Shortcut {
            sequences: [ StandardKey.Close, "Escape" ]
            onActivated: {
                showIdleNotification = false
                idleNotificationWindow.close()
            }
        }
        Shortcut {
            sequences: [ StandardKey.Accept, "Return", "Enter" ]
            onActivated: btnResume.clicked()
        }

        // Fokus awal ke aksi utama
        Component.onCompleted: btnResume.forceActiveFocus()
    }


    // Tambahkan connection untuk menangani notifikasi idle
    Connections {
        target: idleChecker
        function onShowIdleNotification(message) {
            idleNotificationText = message
            showIdleNotification = true

            // Jika window minimized, tampilkan notifikasi system tray
            if (window.visibility === Window.Minimized) {
                showSystemTrayNotification("Idle Detected", message)
            }
        }
        function onHideIdleNotification() {
            showIdleNotification = false
        }
    }

    // Fungsi untuk system tray notification
    function showSystemTrayNotification(title, message) {
        if (typeof SystemTrayIcon !== 'undefined' && SystemTrayIcon.supportsMessages) {
            SystemTrayIcon.showMessage(title, message)
        }
    }




    ApplicationWindow {
        id: authErrorWindow
        width: 340
        height: 200
        visible: false
        color: "transparent"
        title: "Sesi Berakhir"
        modality: Qt.ApplicationModal
        flags: Qt.Dialog | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint


        // Center pada screen
        x: (Screen.width - width) / 2
        y: (Screen.height - height) / 2

        // Main card
        Rectangle {
            id: cardError
            anchors.fill: parent
            radius: 10
            color: cardColor
            border.color: Qt.rgba(primaryColor.r, primaryColor.g, primaryColor.b, 0.15)
            border.width: 1
            opacity: authErrorWindow.visible ? 1 : 0
            scale: authErrorWindow.visible ? 1 : 0.96

            Behavior on opacity { NumberAnimation { duration: 250; easing.type: Easing.OutCubic } }
            Behavior on scale { NumberAnimation { duration: 250; easing.type: Easing.OutBack; easing.overshoot: 1.2 } }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 24
                spacing: 20

                // Header: Icon + Title
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 14

                    // Modern icon with subtle animation
                    Rectangle {
                        width: 40
                        height: 40
                        radius: 20
                        color: Qt.rgba(primaryColor.r, primaryColor.g, primaryColor.b, 0.12)
                        Layout.alignment: Qt.AlignTop

                        // Pulse animation
                        SequentialAnimation on scale {
                            running: root.visible
                            loops: Animation.Infinite
                            NumberAnimation { from: 1.0; to: 1.08; duration: 1200; easing.type: Easing.InOutQuad }
                            NumberAnimation { from: 1.08; to: 1.0; duration: 1200; easing.type: Easing.InOutQuad }
                        }

                        Image {
                            anchors.centerIn: parent
                            width: 24
                            height: 24
                            source: "qrc:/icon.ico"
                            fillMode: Image.PreserveAspectFit
                            smooth: true
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Label {
                            text: "Sesi Berakhir"
                            font.pixelSize: 17
                            font.weight: Font.DemiBold
                            color: textColor
                            Layout.fillWidth: true
                        }

                        Label {
                            text: "Silakan login kembali"
                            font.pixelSize: 13
                            color: Qt.rgba(textColor.r, textColor.g, textColor.b, 0.6)
                            Layout.fillWidth: true
                        }
                    }
                }

                // Message content
                Label {
                    id: authErrorText
                    text: ""
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    font.pixelSize: 13
                    color: Qt.rgba(textColor.r, textColor.g, textColor.b, 0.75)
                    lineHeight: 1.5
                    verticalAlignment: Text.AlignTop

                    // Fade in animation
                    opacity: 0
                    NumberAnimation on opacity {
                        running: authErrorWindow.visible
                        from: 0
                        to: 1
                        duration: 300
                        easing.type: Easing.OutQuart
                    }
                }

                // Spacer
                Item { Layout.fillHeight: true }

                // Action buttons
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Item { Layout.fillWidth: true }

                    Button {
                        id: okButton
                        text: "Kembali ke halaman Login"

                        leftPadding: 24
                        rightPadding: 24
                        topPadding: 10
                        bottomPadding: 10

                        background: Rectangle {
                            radius: 8
                            color: primaryColor
                            border.width: 0

                            Behavior on color { ColorAnimation { duration: 150 } }
                        }

                        contentItem: Text {
                            text: okButton.text
                            font.pixelSize: 13
                            font.weight: Font.DemiBold
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            color: textColor
                        }

                        onClicked: {
                            closeAnimation.start()
                        }
                    }
                }
            }
        }

        // Closing animation
        SequentialAnimation {
            id: closeAnimation
            NumberAnimation {
                target: cardError
                property: "scale"
                from: 1.0
                to: 0.96
                duration: 150
                easing.type: Easing.InQuart
            }
            NumberAnimation {
                target: cardError
                property: "opacity"
                from: 1.0
                to: 0.0
                duration: 100
            }
            ScriptAction {
                script: {
                    authErrorWindow.visible = false
                    cardError.opacity = 1.0
                    cardError.scale = 1.0
                    // Reset ke login page
                    isLoggedIn = false
                    isProfileVisible = false
                    showRegisterPage = false
                    logger.logout()
                }
            }
        }

        // Keyboard shortcuts
        Shortcut {
            sequences: [ StandardKey.Close, "Escape" ]
            onActivated: closeAnimation.start()
        }

        Shortcut {
            sequences: [ StandardKey.Accept, "Return", "Enter" ]
            onActivated: okButton.clicked()
        }

        // Focus on button when shown
        Component.onCompleted: okButton.forceActiveFocus()
    }

    Connections {
        target: logger
        function onShowAuthTokenErrorWindow(message) {
            // Tutup idleNotificationWindow jika sedang terbuka
            showIdleNotification = false
            if (idleNotificationWindow.visible) {
                idleNotificationWindow.close()
            }
            if (isLoggedIn) {
                console.log("Token expired, showing auth error window.")
                authErrorText.text = message
                authErrorWindow.visible = true
                authErrorWindow.raise()
            } else {
                console.log("Ignored auth token error because user is already logged out.")
            }
        }
        function onReadyToProceedWithLogout() {
            console.log("Processing logout cleanup...")

            // Tutup paksa window error jika sedang terbuka
            taskDetailsDialog.close()
            taskDetailsDialog.visible = false
            authErrorWindow.visible = false
            authErrorWindow.close()


            // Lakukan logout sistemlogoutBtn
            logger.logout()

            // Ubah state UI ke halaman Login
            isLoggedIn = false // <- Ini yang akan mencegah pop-up muncul berkat pengecekan di atas

            // Reset UI lainnya...
            currentUsername = ""
            profileImagePath = ":/profilImage.png"
            showRegisterPage = false
            logger.clearLogFilter()
        }
    }



    //Login Page
    Rectangle {
        anchors.fill: parent
        color: backgroundColor
        visible: !isLoggedIn && !isProfileVisible && !showRegisterPage

        Login_page {
            anchors.fill: parent
        }

    }



    // Profile Page
    Rectangle{
        anchors.fill: parent
        color: window.backgroundColor
        visible: isProfileVisible
        opacity: isProfileVisible ? 1 : 0
        Behavior on opacity {
            NumberAnimation { duration: 300 }
        }

        Profil_page{
            anchors.fill: parent
        }
        Label {
            id: profileErrorLabel
            text: ""
            color: profileErrorLabel.text.includes("success") ? "#10B981" : "#EF4444"
            font.pixelSize: 13
            Layout.alignment: Qt.AlignHCenter
            visible: text !== ""
        }

    }


    Dialog {
        id: pingErrorDialog
        modal: true
        visible: false
        anchors.centerIn: Overlay.overlay
        width: Math.min(340, parent.width - 40)
        height: 240

        padding: 0
        margins: 0
        topPadding: 0
        bottomPadding: 0
        leftPadding: 0
        rightPadding: 0

        onAccepted: visible = false

        background: Rectangle {
            color: cardColor
            radius: 16
            border.color: Qt.rgba(palette.highlight.r, palette.highlight.g, palette.highlight.b, 0.15)
            border.width: 1
        }

        contentItem: Rectangle {
            color: cardColor
            radius: 16
            implicitHeight: contentColumn.implicitHeight

            ColumnLayout {
                id: contentColumn
                anchors.fill: parent
                anchors.margins: 24
                spacing: 16

                // Header: Icon + Title
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 14

                    // Modern icon with pulse animation
                    Rectangle {
                        width: 40
                        height: 40
                        radius: 20
                        color: Qt.rgba(primaryColor.r, primaryColor.g, primaryColor.b, 0.12)
                        Layout.alignment: Qt.AlignTop

                        // Pulse animation
                        SequentialAnimation on scale {
                            running: root.visible
                            loops: Animation.Infinite
                            NumberAnimation { from: 1.0; to: 1.08; duration: 1200; easing.type: Easing.InOutQuad }
                            NumberAnimation { from: 1.08; to: 1.0; duration: 1200; easing.type: Easing.InOutQuad }
                        }

                        Image {
                            anchors.centerIn: parent
                            width: 24
                            height: 24
                            source: "qrc:/icon.ico"
                            fillMode: Image.PreserveAspectFit
                            smooth: true
                        }
                    }


                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Label {
                            text: "Kesalahan Koneksi"
                            font.pixelSize: 17
                            font.weight: Font.DemiBold
                            color: textColor
                            Layout.fillWidth: true
                        }

                        Label {
                            text: "Terjadi masalah dengan koneksi"
                            font.pixelSize: 13
                            color: secondaryColor
                            Layout.fillWidth: true
                        }
                    }
                }

                // Error message content
                ScrollView {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Math.min(errorText.implicitHeight, 100)
                    Layout.maximumHeight: 120
                    clip: true

                    Label {
                        id: errorText
                        text: pingErrorText.text
                        wrapMode: Text.Wrap
                        width: pingErrorDialog.width - 48
                        font.pixelSize: 13
                        color: textColor
                        lineHeight: 1.5

                        // Fade in animation
                        opacity: 0
                        NumberAnimation on opacity {
                            running: pingErrorDialog.visible
                            from: 0
                            to: 1
                            duration: 300
                            easing.type: Easing.OutQuart
                        }
                    }
                }

                // Action button
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Item { Layout.fillWidth: true }

                    Button {
                        id: okButton1
                        text: "OK"

                        leftPadding: 32
                        rightPadding: 32
                        topPadding: 10
                        bottomPadding: 10

                        background: Rectangle {
                            radius: 8
                            color: okButton1.pressed ? Qt.darker(primaryColor, 1.1) :
                                                       okButton1.hovered ? Qt.lighter(primaryColor, 1.05) : primaryColor
                            border.width: 0

                            Behavior on color { ColorAnimation { duration: 150 } }
                        }

                        contentItem: Text {
                            text: okButton1.text
                            font.pixelSize: 13
                            font.weight: Font.DemiBold
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            color: "white"
                        }
                        onClicked: pingErrorDialog.accept()
                    }
                }
            }
        }

        // Smooth animations
        enter: Transition {
            ParallelAnimation {
                NumberAnimation {
                    property: "scale"
                    from: 0.96
                    to: 1.0
                    duration: 250
                    easing.type: Easing.OutCubic
                }
                NumberAnimation {
                    property: "opacity"
                    from: 0.0
                    to: 1.0
                    duration: 250
                    easing.type: Easing.OutCubic
                }
            }
        }

        exit: Transition {
            ParallelAnimation {
                NumberAnimation {
                    property: "scale"
                    from: 1.0
                    to: 0.96
                    duration: 150
                    easing.type: Easing.InCubic
                }
                NumberAnimation {
                    property: "opacity"
                    from: 1.0
                    to: 0.0
                    duration: 150
                    easing.type: Easing.InCubic
                }
            }
        }

        // Hidden text element untuk data binding
        Text {
            id: pingErrorText
            text: ""
            visible: false
        }

        // Keyboard shortcuts
        Shortcut {
            sequences: [ StandardKey.Close, "Escape" ]
            onActivated: pingErrorDialog.accept()
        }

        Shortcut {
            sequences: [ StandardKey.Accept, "Return", "Enter" ]
            onActivated: okButton.clicked()
        }

        // Focus on button when shown
        onVisibleChanged: if (visible) okButton.forceActiveFocus()
    }

    Connections {
        target: logger
        onShowPingErrorDialog: {
            pingErrorText.text = message
            pingErrorDialog.open()
        }
        onHidePingErrorDialog: {
            pingErrorDialog.close()
        }
    }




    // Dashboard
    Rectangle {
        anchors.fill: parent
        color: backgroundColor
        visible: isLoggedIn && !isProfileVisible

        onVisibleChanged: {
            if (visible) { // Pastikan kode hanya berjalan saat dashboard menjadi terlihat
                console.log("Dashboard is now visible. Resetting date range to today.");
                var today = new Date();
                startSelectedDate = today;
                endSelectedDate = today;
                isDateSelected = true;
                applyDateRange();
            }
        }

        // Component.onCompleted bisa tetap ada untuk memastikan tanggal diatur pada pemuatan pertama kali
        Component.onCompleted: {
            console.log("Dashboard component completed. Setting date range to today.");
            var today = new Date();
            startSelectedDate = today;
            endSelectedDate = today;
            isDateSelected = true;
            applyDateRange();
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // Header
            Rectangle {
                Layout.fillWidth: true
                height: 60
                color: headers
                border.color: dividerColor
                border.width: 1
                bottomLeftRadius: 10
                bottomRightRadius: 10

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 16

                    Label {
                        text: "Deskmon"
                        font { bold: true; pixelSize: 20; family: "Segoe UI" }
                        color: "white"
                    }

                    Label {
                        text: currentUsername
                        font.pixelSize: 14
                        color: "white"
                        opacity: 0.8
                    }


                    Item { Layout.fillWidth: true }

                    Row {
                        id: headerButtonsRow
                        spacing: 12
                        layoutDirection: Qt.RightToLeft

                        property string update_newVersion: ""
                        property string update_newReleaseNotes: ""
                        property string update_newDownloadUrl: ""

                        // Dark mode toggle button
                        RoundButton {
                            id: themeToggle
                            width: 40
                            height: 40
                            radius: 20
                            hoverEnabled: true
                            background: Rectangle {
                                radius: 20
                                color: parent.hovered ? Qt.rgba(1,1,1,0.2) : "transparent"
                            }

                            contentItem: Image {
                                source: window.Material.theme === Material.Dark ? "qrc:/icons/light_mode.svg" : "qrc:/icons/dark_mode.svg"
                                sourceSize.width: 24
                                sourceSize.height: 24
                                anchors.centerIn: parent
                                opacity: 0.9
                            }

                            onClicked: {
                                isDarkMode = !isDarkMode
                                rotationAnim.start()
                            }

                            RotationAnimation {
                                id: rotationAnim
                                target: themeToggle.contentItem
                                from: 0
                                to: 360
                                duration: 600
                                easing.type: Easing.OutBack
                            }

                            ToolTip.text: window.Material.theme === Material.Dark ? "Switch to Light Mode" : "Switch to Dark Mode"
                            ToolTip.visible: hovered
                            ToolTip.delay: 500
                        }

                        RoundButton {
                            id: refresh
                            width: 40
                            height: 40
                            radius: 20
                            hoverEnabled: true
                            background: Rectangle {
                                radius: 20
                                color: parent.hovered ? Qt.rgba(1,1,1,0.2) : "transparent"
                            }

                            contentItem: Image {
                                source: "qrc:/icons/refresh.svg"
                                sourceSize.width: 24
                                sourceSize.height: 24
                                anchors.centerIn: parent
                                opacity: 0.9
                            }

                            onClicked: {
                                logger.refreshAll()
                                console.log("Refresh button clicked")
                                rotationAnimation.start()

                            }

                            RotationAnimation {
                                id: rotationAnimation
                                target: refresh.contentItem
                                from: 0
                                to: 360
                                duration: 600
                                easing.type: Easing.OutBack
                            }

                            ToolTip.text: "Refresh"
                            ToolTip.visible: hovered
                            ToolTip.delay: 500
                        }

                        Shortcut {
                            sequences: ["Ctrl+R"]
                            enabled: refresh.enabled
                            onActivated: {
                                if (Qt.platform.os === "windows" || Qt.platform.os === "linux" || Qt.platform.os === "osx") {
                                    refresh.clicked()
                                }
                            }
                        }

                        // Profile button
                        Button {
                            id: profileBtn
                            text: "Profile"
                            height: 40
                            padding: 12
                            font {
                                family: "Segoe UI"
                                pixelSize: 14
                                weight: Font.Medium
                            }
                            background: Rectangle {
                                radius: 8
                                color: parent.hovered ? Qt.rgba(1,1,1,0.2) : "transparent"
                                border.color: Qt.rgba(1,1,1,0.3)
                                border.width: 1
                            }
                            contentItem: Text {
                                text: profileBtn.text
                                font: profileBtn.font
                                color: "white"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            onClicked: {
                                tempUsername = currentUsername
                                tempPassword = ""
                                tempDepartment = logger.getUserDepartment(currentUsername)
                                profileErrorLabel.text = ""
                                isProfileVisible = true
                            }
                        }

                        // Logout button
                        Button {
                            id: logoutBtn
                            text: "Logout"
                            height: 40
                            padding: 12
                            font {
                                family: "Segoe UI"
                                pixelSize: 14
                                weight: Font.Medium
                            }
                            background: Rectangle {
                                radius: 8
                                color: parent.hovered ? Qt.rgba(1,1,1,0.2) : "transparent"
                                border.color: Qt.rgba(1,1,1,0.3)
                                border.width: 1
                            }
                            contentItem: Text {
                                text: logoutBtn.text
                                font: logoutBtn.font
                                color: "white"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            onClicked: {
                                    if (logger.activeTaskId && logger.activeTaskId > 0) {
                                        console.log("Task aktif terdeteksi (ID: " + logger.activeTaskId + "). Membuka dialog detail.");
                                        taskDetailsDialog.show(logger.activeTaskId, "logout", -1);

                                    } else {
                                        console.log("Tidak ada task aktif. Langsung logout.");
                                        logger.taskDetailsDialogClosed("logout");
                                    }
                                }
                        }

                        Button {
                            id: updateBtn
                            text: "Cek Update"
                            height: 40

                            onClicked: logger.checkForUpdates()

                            // Styling (sama seperti tombol lain)
                            padding: 12; font { family: "Segoe UI"; pixelSize: 14; weight: Font.Medium }
                            background: Rectangle { radius: 8; color: parent.hovered ? Qt.rgba(1,1,1,0.2) : "transparent"; border.color: Qt.rgba(1,1,1,0.3); border.width: 1 }
                            contentItem: Text { text: parent.text; font: parent.font; color: "white"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        }

                        Button {
                            id: deskmonButton
                            width: 40 // Set both width and height to make it a square
                            height: 40

                            // Set the button's background and text color
                            background: Rectangle {
                                radius: 8
                                color: parent.hovered ? Qt.rgba(1, 1, 1, 0.2) : "transparent"
                                border.color: Qt.rgba(1, 1, 1, 0.3)
                                border.width: 1
                            }

                            contentItem: Item {
                                anchors.fill: parent

                                Image {
                                    id: externalLinkIcon
                                    source: "qrc:/icons/website.svg"
                                    sourceSize.width: 20
                                    sourceSize.height: 20
                                    anchors.centerIn: parent // Center the icon inside the button
                                }
                            }

                            // Set cursor and handle click event
                            onClicked: Qt.openUrlExternally("https://deskmon.pranala-dt.co.id/")
                            ToolTip.text: "Deskmon Website"
                            ToolTip.visible: hovered
                            ToolTip.delay: 500
                        }

                    }
                }
                Label {
                    id: statusLabel
                    anchors.centerIn: parent // Tampilkan di tengah layar
                    padding: 12
                    text: "Ini adalah pesan status"
                    visible: false // Awalnya sembunyi
                    z: 9999 // Pastikan selalu di atas elemen lain

                    background: Rectangle {
                        color: Qt.rgba(0, 0, 0, 0.7) // Latar belakang gelap semi-transparan
                        radius: 8
                    }

                    font.pixelSize: 16
                    color: primaryColor

                    // Animasi untuk muncul dan hilang dengan halus
                    opacity: 0
                    Behavior on opacity { NumberAnimation { duration: 300 } }
                }

                // 2. Timer untuk menyembunyikan Label setelah 3 detik
                Timer {
                    id: statusTimer
                    interval: 3000 // 3 detik
                    repeat: false
                    onTriggered: {
                        // Sembunyikan label dengan mengubah opacity-nya
                        statusLabel.opacity = 0
                    }
                }
            }
            Connections {
                target: logger

                function onUpdateAvailable(version, releaseNotes, downloadUrl) {
                    console.log("QML: Update tersedia!", version)

                    // Simpan info ke properti yang sudah ada di header
                    headerButtonsRow.update_newVersion = version
                    headerButtonsRow.update_newReleaseNotes = releaseNotes

                    // Buka dialog yang sudah kita siapkan
                    updateDialog.open()
                }

                function onShowStatusMessage(message) {
                    statusLabel.text = message    // Atur teks label
                    statusLabel.visible = true    // Tampilkan label
                    statusLabel.opacity = 1       // Set opacity agar terlihat
                    statusTimer.restart()         // Mulai timer 3 detik
                }
            }

            // Letakkan di level atas ApplicationWindow Anda

            Dialog {
                id: updateDialog
                title: ""
                modal: true
                width: 480
                height: 520
                anchors.centerIn: parent

                // Modern styling properties
                background: Rectangle {
                    color: "#1e1e1e"
                    radius: 12
                    border.width: 1
                    border.color: "#3d3d3d"

                    // Subtle gradient overlay
                    Rectangle {
                        anchors.fill: parent
                        radius: parent.radius
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: "#252525" }
                            GradientStop { position: 1.0; color: "#1a1a1a" }
                        }
                        opacity: 0.8
                    }
                }

                // Content container
                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 1
                    color: "transparent"
                    radius: 11

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 24
                        spacing: 0

                        // Header section
                        Item {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 80

                            Rectangle {
                                width: 56
                                height: 56
                                radius: 12
                                color: "#007ACC"
                                anchors.left: parent.left
                                anchors.verticalCenter: parent.verticalCenter

                                // Modern update icon using simple shapes
                                Rectangle {
                                    width: 24
                                    height: 24
                                    radius: 12
                                    color: "#ffffff"
                                    anchors.centerIn: parent

                                    Rectangle {
                                        width: 8
                                        height: 8
                                        radius: 4
                                        color: "#007ACC"
                                        anchors.centerIn: parent
                                    }
                                }
                            }

                            Column {
                                anchors.left: parent.left
                                anchors.leftMargin: 72
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: 4

                                Label {
                                    text: "Pembaruan Tersedia"
                                    font.pixelSize: 20
                                    font.weight: Font.DemiBold
                                    color: "#ffffff"
                                }

                                Label {
                                    text: "Versi " + headerButtonsRow.update_newVersion
                                    font.pixelSize: 14
                                    color: "#007ACC"
                                    font.weight: Font.Medium
                                }
                            }
                        }

                        // Separator line
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 1
                            Layout.topMargin: 16
                            Layout.bottomMargin: 20
                            color: "#3d3d3d"
                        }

                        // Release notes section
                        ScrollView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.maximumHeight: 120
                            clip: true

                            background: Rectangle {
                                color: "#2d2d2d"
                                radius: 8
                                border.width: 1
                                border.color: "#404040"
                            }

                            Label {
                                width: parent.width
                                text: headerButtonsRow.update_newReleaseNotes
                                wrapMode: Text.Wrap
                                font.pixelSize: 13
                                color: "#cccccc"
                                lineHeight: 1.4
                                padding: 16
                            }
                        }

                        // Spacer
                        Item {
                            Layout.fillHeight: true
                            Layout.minimumHeight: 20
                        }

                        // Action buttons
                        RowLayout {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 44
                            spacing: 12

                            Item { Layout.fillWidth: true } // Push buttons to right

                            Button {
                                Layout.preferredWidth: 100
                                Layout.preferredHeight: 36
                                text: "Nanti"

                                background: Rectangle {
                                    radius: 8
                                    color: parent.hovered ? "#404040" : "#2d2d2d"
                                    border.width: 1
                                    border.color: parent.hovered ? "#555555" : "#404040"

                                    Behavior on color {
                                        ColorAnimation { duration: 150 }
                                    }
                                    Behavior on border.color {
                                        ColorAnimation { duration: 150 }
                                    }
                                }

                                contentItem: Text {
                                    text: parent.text
                                    font.pixelSize: 13
                                    font.weight: Font.Medium
                                    color: "#cccccc"
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }

                                onClicked: updateDialog.close()
                            }

                            Button {
                                Layout.preferredWidth: 140
                                Layout.preferredHeight: 36
                                text: "Update Sekarang"

                                background: Rectangle {
                                    radius: 8
                                    color: parent.hovered ? "#0086d4" : "#007ACC"

                                    // Subtle inner glow
                                    Rectangle {
                                        anchors.fill: parent
                                        radius: parent.radius
                                        color: "transparent"
                                        border.width: 1
                                        border.color: "#4da6d9"
                                        opacity: 0.3
                                    }

                                    Behavior on color {
                                        ColorAnimation { duration: 150 }
                                    }
                                }

                                contentItem: Text {
                                    text: parent.text
                                    font.pixelSize: 13
                                    font.weight: Font.DemiBold
                                    color: "#ffffff"
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }

                                onClicked: {
                                    logger.launchMaintenanceTool()
                                    updateDialog.close()
                                }
                            }
                        }
                    }
                }
            }

            // Main Content
            GridLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.margins: 10
                columns: 1
                columnSpacing: 16
                rowSpacing: 16


                // Application Usage Card
                Frame {
                    id: combinedCard
                    Layout.fillWidth: true
                    Layout.preferredHeight: 320
                    padding: 16

                    background: Rectangle {
                        color: cardColor
                        radius: 16
                        layer.enabled: true
                        border.color: dividerColor
                        border.width: 1

                    }
                    Rectangle{
                        anchors.fill: parent
                        color: "transparent"
                        Productivty{
                            anchors.fill: parent
                        }
                    }
                }

                //Monitored_Applications
                ListModel {
                    id: productiveAppsModel
                }

                ListModel {
                    id: nonProductiveAppsModel
                }

                // Filtered models for search functionality
                ListModel {
                    id: filteredProductiveAppsModel
                }

                ListModel {
                    id: filteredNonProductiveAppsModel
                }


                //RequestAppPending
                //AddAplicatioRequest
                //DateRange





                //Monitored application
                GridLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    columns: 2
                    columnSpacing: 14
                    rowSpacing: 14

                    Frame {
                        Layout.fillWidth: true
                        // Layout.minimumWidth: 100
                        // Layout.maximumWidth: 800
                        Layout.fillHeight: true
                        padding: 16
                        background: Rectangle {
                            color: cardColor
                            radius: 8
                            border.color: dividerColor
                            border.width: 1
                        }
                        Current_Task{
                            anchors.fill: parent
                        }


                    }

                    Frame {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        padding: 16
                        background: Rectangle {
                            color: cardColor
                            radius: 8
                            border.color: dividerColor
                            border.width: 1
                        }
                        Activity_Monitor{
                            anchors.fill: parent
                        }
                    }

                }

                // Cari komponen Menu ini di Main.qml
                Menu {
                    id: stableTaskMenu
                    property int taskId: -1
                    property int userId: -1
                    property string authToken: ""

                    MenuItem {
                        text: "Mark as Need Review"
                        font.pixelSize: 13

                        background: Rectangle {
                            color: parent.hovered ? Qt.rgba(255/255, 152/255, 0/255, 0.1) : "transparent"
                            radius: 4
                        }

                        // UBAH BAGIAN INI
                        onTriggered: {
                            // Hapus semua logika XMLHttpRequest yang lama dari sini

                            // Panggil dialog baru untuk meminta alasan
                            needReviewReasonDialog.openWithTaskId(stableTaskMenu.taskId, logger)
                        }
                    }
                }
                // Tambahkan komponen Dialog ini di dalam file Main.qml








                // Add these popups at the root level of your QML file
                Popup {
                    id: taskDetailPopup
                    width: Math.min(parent.width * 0.9, 600)
                    height: Math.min(parent.height * 0.7, 500)
                    x: (parent.width - width) / 2
                    y: (parent.height - height) / 2
                    modal: true
                    focus: true
                    padding: 16
                    dim: true
                    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

                    background: Rectangle {
                        color: cardColor
                        radius: 8
                        border.color: dividerColor
                        border.width: 1
                    }

                    function show(title, description) {
                        popupTitle.text = title
                        popupDescription.text = description
                        open()
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 12

                        Label {
                            id: popupTitle
                            Layout.fillWidth: true
                            font { bold: true; pixelSize: 18; family: "Segoe UI" }
                            color: primaryColor
                            wrapMode: Text.Wrap
                            maximumLineCount: 2
                            elide: Text.ElideRight
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: dividerColor
                        }

                        ScrollView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            ScrollBar.horizontal.policy: ScrollBar.AsNeeded

                            TextArea {
                                id: popupDescription
                                width: parent.width
                                wrapMode: Text.Wrap
                                readOnly: true
                                selectByMouse: true
                                font.pixelSize: 14
                                color: textColor
                                background: null
                                padding: 0
                                textFormat: Text.PlainText
                            }
                        }

                        Button {
                            text: "Close"
                            Layout.alignment: Qt.AlignRight
                            Material.background: secondaryColor
                            Material.foreground: "white"
                            onClicked: taskDetailPopup.close()
                        }
                    }
                }


                Dialog {
                    id: confirmSwitchDialog
                    title: ""
                    modal: true
                    anchors.centerIn: parent
                    width: 380
                    height: 240
                    padding: 0
                    property int taskId: -1

                    background: Item {}

                    contentItem: Rectangle {
                        color: cardColor
                        radius: 12
                        border.width: 1
                        border.color: Qt.rgba(255, 255, 255, 0.08)

                        // Subtle gradient overlay
                        Rectangle {
                            anchors.fill: parent
                            radius: 12
                            gradient: Gradient {
                                GradientStop { position: 0.0; color: Qt.rgba(255, 255, 255, 0.03) }
                                GradientStop { position: 1.0; color: "transparent" }
                            }
                        }

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 24
                            spacing: 20

                            // Icon + Title section (horizontal)
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 12

                                Rectangle {
                                    Layout.preferredWidth: 40
                                    Layout.preferredHeight: 40
                                    radius: 8
                                    color: Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 0.12)
                                    border.width: 1
                                    border.color: Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 0.2)

                                    Text {
                                        anchors.centerIn: parent
                                        text: "⚠"
                                        font.pixelSize: 20
                                        color: accentColor
                                    }
                                }

                                Label {
                                    text: "Switch Project?"
                                    font {
                                        pixelSize: 18
                                        family: "Segoe UI"
                                        weight: Font.DemiBold
                                    }
                                    color: textColor
                                    Layout.fillWidth: true
                                }
                            }

                            // Message section
                            Label {
                                text: "You're about to switch to another project. Your current progress will be saved automatically."
                                font {
                                    pixelSize: 13
                                    family: "Segoe UI"
                                }
                                color: Qt.rgba(textColor.r, textColor.g, textColor.b, 0.65)
                                wrapMode: Text.Wrap
                                Layout.fillWidth: true
                                lineHeight: 1.5
                            }

                            Item { Layout.fillHeight: true }

                            // Button section
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 12

                                Button {
                                    id: cancelButton
                                    text: "Cancel"
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 40

                                    background: Rectangle {
                                        radius: 8
                                        color: cancelButton.pressed ? Qt.rgba(textColor.r, textColor.g, textColor.b, 0.08) :
                                                                      cancelButton.hovered ? Qt.rgba(textColor.r, textColor.g, textColor.b, 0.04) : "transparent"
                                        border.width: 1
                                        border.color: Qt.rgba(textColor.r, textColor.g, textColor.b, 0.15)

                                        Behavior on color {
                                            ColorAnimation { duration: 150 }
                                        }
                                    }

                                    contentItem: Text {
                                        text: cancelButton.text
                                        font {
                                            pixelSize: 13
                                            family: "Segoe UI"
                                            weight: Font.Medium
                                        }
                                        color: textColor
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }

                                    onClicked: confirmSwitchDialog.reject()
                                }

                                Button {
                                    id: confirmButton
                                    text: "Switch Project"
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 40

                                    background: Rectangle {
                                        radius: 8
                                        color: confirmButton.pressed ? Qt.darker(accentColor, 1.15) :
                                                                       confirmButton.hovered ? Qt.lighter(accentColor, 1.08) : accentColor

                                        Behavior on color {
                                            ColorAnimation { duration: 150 }
                                        }
                                    }

                                    contentItem: Text {
                                        text: confirmButton.text
                                        font {
                                            pixelSize: 13
                                            family: "Segoe UI"
                                            weight: Font.Medium
                                        }
                                        color: "white"
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }

                                    onClicked: {
                                        logger.setActiveTask(confirmSwitchDialog.taskId)
                                        confirmSwitchDialog.accept()
                                        isPop_up_waktuhabis_open = false
                                        isPop_up_waktuhabis_kurangdari_open = false
                                        console.log("ok 1", isPop_up_waktuhabis_open)
                                        console.log("ok 2", isPop_up_waktuhabis_kurangdari_open)
                                    }
                                }
                            }
                        }
                    }

                    // Entry animation
                    enter: Transition {
                        ParallelAnimation {
                            NumberAnimation {
                                property: "opacity"
                                from: 0
                                to: 1
                                duration: 200
                                easing.type: Easing.OutCubic
                            }
                            NumberAnimation {
                                property: "scale"
                                from: 0.92
                                to: 1.0
                                duration: 250
                                easing.type: Easing.OutCubic
                            }
                        }
                    }

                    // Exit animation
                    exit: Transition {
                        ParallelAnimation {
                            NumberAnimation {
                                property: "opacity"
                                from: 1
                                to: 0
                                duration: 150
                                easing.type: Easing.InCubic
                            }
                            NumberAnimation {
                                property: "scale"
                                from: 1.0
                                to: 0.96
                                duration: 150
                                easing.type: Easing.InCubic
                            }
                        }
                    }
                }

                // Dialog Konfirmasi Penyelesaian Tugas
                Dialog {
                    id: confirmFinishDialog
                    title: "Confirm Task Completion"
                    modal: true
                    anchors.centerIn: parent
                    width: 400
                    height: 280
                    padding: 0

                    property int taskId: -1

                    background: Rectangle {
                        color: cardColor
                        radius: 12
                        border.color: dividerColor
                    }

                    contentItem: Rectangle {
                        color: cardColor
                        radius: 12
                        anchors.fill: parent

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 24
                            spacing: 16

                            Label {
                                text: "Are you sure you want to mark this task as completed?"
                                font { pixelSize: 16; family: "Segoe UI" }
                                color: textColor
                                wrapMode: Text.Wrap
                                Layout.fillWidth: true
                            }

                            RowLayout {
                                Layout.alignment: Qt.AlignRight
                                spacing: 16

                                Button {
                                    text: "Cancel"
                                    Layout.preferredWidth: 120
                                    Layout.preferredHeight: 40
                                    Material.background: "transparent"
                                    Material.foreground: accentColor
                                    font.pixelSize: 14
                                    onClicked: confirmFinishDialog.reject()
                                }

                                Button {
                                    text: "OK"
                                    Layout.preferredWidth: 120
                                    Layout.preferredHeight: 40
                                    Material.background: secondaryColor
                                    Material.foreground: "white"
                                    font.pixelSize: 14
                                    onClicked: {
                                        logger.finishTask(confirmFinishDialog.taskId)
                                        confirmFinishDialog.accept()

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
