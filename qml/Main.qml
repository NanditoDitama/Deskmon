import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material
import QtQuick.Dialogs
import QtQuick.Effects
import QtQuick.Window
import QtQuick 2.15
import "theme"
import "components"
import "views"
import "dialogs"
import "layouts"

Window {
    id: window
    title: qsTr("Deskmon - v" + appVersion)
    visibility: Window.Maximized
    minimumWidth: 900
    minimumHeight: 700
    property string appVersion: "1.0.3.4"
    color: Theme.cardColor

    // Fungsi ini akan berjalan saat sinyal 'closing' terdeteksi
    function onClosing(close) {
        console.log("Tombol 'X' ditekan. Menyembunyikan window ke tray.")
        close.accepted = false
        if (window.parent) {
            window.parent.hide()
        }
    }
    onClosing: {
        console.log("Tombol 'X' ditekan. Menyembunyikan window ke tray.")
        close.accepted = false
        window.hide()
    }

    ToastNotification {
        id: notification
        parent: Overlay.overlay
    }

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



    // Global Theme Bindings (Single Source of Truth)
    property bool isDarkMode: Theme.isDarkMode
    property color primaryColor: Theme.primaryColor
    property color secondaryColor: Theme.secondaryColor
    property color accentColor: Theme.accentColor
    property color backgroundColor: Theme.backgroundColor
    property color cardColor: Theme.cardColor
    property color textColor: Theme.textColor
    property color lightTextColor: Theme.lightTextColor
    property color dividerColor: Theme.dividerColor
    property color headers: Theme.headers
    property color selectedColor: Theme.selectedColor
    property color rangeColor: Theme.rangeColor
    property color productiveColor: Theme.productiveColor
    property color nonProductiveColor: Theme.nonProductiveColor
    property color neutralColor: Theme.neutralColor

    property var public_curent_time: ""
    property var public_max_time: ""
    property bool isTimeUpPopupOpen: false
    property bool isTimeUpWarningOpen: false

    // Coba deteksi tema sistem saat startup
    Component.onCompleted: {
        if (typeof Qt.styleHints !== "undefined") {
            Theme.isDarkMode = Qt.styleHints.colorScheme === Qt.Dark
        }
        else {
            Theme.isDarkMode = Material.theme === Material.Dark
        }
        if (logger.currentUserId === -1) {
            isLoggedIn = false
            console.log("ke halaman login untuk login ulang")
        }
        window.visibility = Window.Maximized
    }

    function goToLoginPage() {
        isLoggedIn = false
        isProfileVisible = false
        showRegisterPage = false
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
    Material.theme: Theme.isDarkMode ? Material.Dark : Material.Light

    property int currentMonth: new Date().getMonth()
    property int currentYear: new Date().getFullYear()



    property bool showIdleNotification: false
    property string idleNotificationText: ""




    TimeUpPopup {
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

            console.log ("chek 1", isTimeUpPopupOpen)
            if(isTimeUpPopupOpen == false){
                warningWindowComponent.newText = "Waktu Task anda Sudah Habis"
                isTimeUpPopupOpen = true
                warningWindowComponent.show()
                console.log("Waktu Task anda sudah habis!")
            }
        }
        else if(diffMinutes <= 10) {
            console.log ("chek 2",isTimeUpWarningOpen)
            if(isTimeUpWarningOpen == false){
                warningWindowComponent.newText = "Waktu Task anda tersisa kurang dari 10 menit!"
                isTimeUpWarningOpen = true
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


    // Update profile image when changed
    Connections {
        target: logger
        function onProfileImageChanged(username, newPath) {
            if (username === currentUsername) {
                console.log("Profile image changed for", username, "to", newPath)
                profileImagePath = newPath
            }
        }
    }

    Connections {
        target: logger

        function onTaskReviewNotification(message) {
            console.log("Review notification:", message);

            if (typeof reviewNotificationPopup !== 'undefined') {
                reviewNotificationPopup.showNotification(message);
            }

            if (typeof SystemTrayIcon !== 'undefined' && SystemTrayIcon.supportsMessages) {
                SystemTrayIcon.showMessage("Task Review", message);
            }

            notification.show("warning", message);
        }

        function onTaskStatusChanged(taskId, newStatus) {
            if (newStatus === "review") {
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





    IdleNotificationWindow {
        id: idleNotificationWindow
        visible: showIdleNotification
        onResumeRequested: {
            showIdleNotification = false
            if (logger && logger.isTaskPaused)
                logger.toggleTaskPause()
        }
        onDismissRequested: {
            showIdleNotification = false
        }
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




    AuthErrorWindow {
        id: authErrorWindow
        onLoginRedirectRequested: {
            isLoggedIn = false
            isProfileVisible = false
            showRegisterPage = false
            logger.logout()
        }
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
                authErrorWindow.errorMessage = message
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

        LoginPage {
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

        ProfilePage{
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


    PingErrorDialog {
        id: pingErrorDialog
    }

    Connections {
        target: logger
        function onShowPingErrorDialog(message) {
            pingErrorDialog.errorMessage = message
            pingErrorDialog.open()
        }
        function onHidePingErrorDialog() {
            pingErrorDialog.close()
        }
    }




    // Dashboard
    Rectangle {
        anchors.fill: parent
        color: backgroundColor
        visible: isLoggedIn && !isProfileVisible

        onVisibleChanged: {
            if (visible) {
                console.log("Dashboard is now visible. Resetting date range to today.");
                var today = new Date();
                startSelectedDate = today;
                endSelectedDate = today;
                isDateSelected = true;
            }
        }

        Component.onCompleted: {
            console.log("Dashboard component completed. Setting date range to today.");
            var today = new Date();
            startSelectedDate = today;
            endSelectedDate = today;
            isDateSelected = true;
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // Header Layout
            HeaderLayout {
                username: currentUsername
                onProfileClicked: {
                    tempUsername = currentUsername
                    tempPassword = ""
                    tempDepartment = logger.getUserDepartment(currentUsername)
                    profileErrorLabel.text = ""
                    isProfileVisible = true
                }
                onLogoutClicked: {
                    if (logger.activeTaskId && logger.activeTaskId > 0) {
                        console.log("Task aktif terdeteksi (ID: " + logger.activeTaskId + "). Membuka dialog detail.");
                        taskDetailsDialog.show(logger.activeTaskId, "logout", -1);
                    } else {
                        console.log("Tidak ada task aktif. Langsung logout.");
                        logger.taskDetailsDialogClosed("logout");
                    }
                }
            }

            // Body Layout
            BodyLayout {
                onNeedReviewRequested: function(taskId) {
                    needReviewReasonDialog.openWithTaskId(taskId, logger)
                }
            }
        }
    }
}


