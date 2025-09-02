import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material
import QtQuick.Dialogs
import QtQuick.Effects
import QtQuick.Window
import QtQuick 2.15

ApplicationWindow {
    id: window
    title: qsTr("Deskmon - v" + appVersion)
    visibility: Window.Maximized
    minimumWidth: 900
    minimumHeight: 700
    property string appVersion: "1.0.2.9"


    Rectangle {
        id: notification
        width: 400
        height: 80
        color: "#ffffff"
        radius: 12
        visible: false
        anchors {
            bottom: parent.bottom
            right: parent.right
            rightMargin: -width // Awalnya tersembunyi di kanan
        }
        z: 1000

        // Left accent border
        Rectangle {
            width: 6
            height: parent.height
            color: "#4CAF50"
            radius: 3
            anchors {
                left: parent.left
                verticalCenter: parent.verticalCenter
            }
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            spacing: 12

            // Icon with background
            Rectangle {
                width: 40
                height: 40
                radius: 20
                color: "#E8F5E9"
                Layout.alignment: Qt.AlignVCenter

                Image {
                    source: notification.idleNotificationText.includes("Review") ?
                                "qrc:/icons/review.svg" : "qrc:/icons/check.svg"
                    width: 24
                    height: 24
                    anchors.centerIn: parent
                }
            }

            ColumnLayout {
                spacing: 4
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter

                Text {
                    text: notification.idleNotificationText.includes("Review") ? "Task Review" : "Review Aplikasi"
                    color: "#212121"
                    font {
                        pixelSize: 16
                        weight: Font.Medium
                        family: "Roboto"
                    }
                }

                Text {
                    text: "Aplikasi sedang dalam proses review, silahkan tunggu persetujuan."
                    color: "#616161"
                    font.pixelSize: 14
                    font.family: "Roboto"
                    wrapMode: Text.Wrap
                    maximumLineCount: 2
                    Layout.fillWidth: true
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }

        // Animasi masuk, jeda, dan keluar dalam satu SequentialAnimation
        SequentialAnimation {
            id: notificationAnimation
            running: false
            NumberAnimation {
                target: notification
                property: "anchors.rightMargin"
                from: -notification.width
                to: 20
                duration: 500
                easing.type: Easing.OutBack
            }
            PauseAnimation { duration: 5000 } // Tampilkan selama 3 detik
            NumberAnimation {
                target: notification
                property: "anchors.rightMargin"
                from: 20
                to: -notification.width
                duration: 500
                easing.type: Easing.InBack
            }
            ScriptAction {
                script: notification.visible = false
            }
        }

        function show() {
            if (!notificationAnimation.running) { // Cegah animasi tumpang tindih
                notification.visible = true
                notificationAnimation.start()
            }
        }

        function hide() {
            if (notification.visible && notificationAnimation.running) {
                notificationAnimation.stop() // Hentikan animasi jika sedang berjalan
                notification.visible = false
                notification.anchors.rightMargin = -notification.width // Kembalikan ke posisi awal
            }
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




    onClosing: function(close) {
        close.accepted = false
        window.hide()
    }
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
                warningWindowComponent.newText = "Waktu anda Sudah Habis"
                isPop_up_waktuhabis_open = true
                warningWindowComponent.show()
                console.log("Waktu sudah habis!")
            }
        }
        else if(diffMinutes <= 10) {
            console.log ("chek 2",isPop_up_waktuhabis_kurangdari_open)
            if(isPop_up_waktuhabis_kurangdari_open == false){
                warningWindowComponent.newText = "Waktu tersisa kurang dari 10 menit!"
                isPop_up_waktuhabis_kurangdari_open = true
                warningWindowComponent.show()
                console.log("Waktu tersisa kurang dari 10 menit!")
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


    // TAMBAHKAN KOMPONEN DIALOG INI DI DALAM ApplicationWindow
    Dialog {
        id: earlyLeaveReasonDialog
        title: "Alasan Keluar Lebih Awal"
        modal: true
        width: Math.min(520, parent.width * 0.9)
        height: Math.min(450, parent.height * 0.8)
        anchors.centerIn: parent
        padding: 0

        closePolicy: Popup.NoAutoClose

        // Enhanced background with subtle shadow effect
        background: Rectangle {
            color: cardColor
            radius: 16
            border.color: Qt.rgba(dividerColor.r, dividerColor.g, dividerColor.b, 0.3)
            border.width: 1

            // Subtle shadow effect
            Rectangle {
                anchors.fill: parent
                anchors.margins: -2
                color: "transparent"
                radius: 18
                border.color: Qt.rgba(0, 0, 0, 0.1)
                border.width: 1
                z: -1
            }
        }

        // Custom header with icon and better typography
        header: Rectangle {
            height: 90
            color: "transparent"

            Rectangle {
                anchors.fill: parent
                anchors.bottomMargin: 10
                color: Qt.rgba(dividerColor.r, dividerColor.g, dividerColor.b, 0.3)
                radius: 16

                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    height: parent.radius
                    color: parent.color
                }
            }

            RowLayout {
                anchors.centerIn: parent
                anchors.topMargin: 5
                spacing: 12

                // Warning icon
                Rectangle {
                    width: 40
                    height: 40
                    radius: 20
                    color: Qt.rgba(255, 152, 0, 0.1)

                    Text {
                        anchors.centerIn: parent
                        text: "⚠️"
                        font.pixelSize: 20
                    }
                }

                Column {
                    spacing: 4

                    Label {
                        text: "Keluar Lebih Awal"
                        font {
                            pixelSize: 20
                            weight: Font.Bold
                        }
                        color: textColor
                    }

                    Label {
                        text: "Mohon berikan alasan yang jelas"
                        font.pixelSize: 12
                        color: lightTextColor
                        opacity: 0.8
                    }
                }
            }
        }

        contentItem: Item {
            anchors.fill: parent
            anchors.margins: 24
            anchors.topMargin: 100

            Column {
                anchors.fill: parent
                spacing: 24

                // Enhanced info section with better visual hierarchy
                Rectangle {
                    width: parent.width
                    height: 70
                    radius: 12
                    color: Qt.rgba(33, 150, 243, 0.05)
                    border.color: Qt.rgba(33, 150, 243, 0.2)
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 12

                        Text {
                            text: "ℹ️"
                            font.pixelSize: 18
                        }

                        Label {
                            text: "Anda akan keluar sebelum waktu kerja selesai.\nAlasan ini akan dicatat dalam sistem kehadiran."
                            wrapMode: Text.Wrap
                            Layout.fillWidth: true
                            color: textColor
                            font.pixelSize: 13
                            lineHeight: 1.3
                        }
                    }
                }

                // Enhanced text input section
                Column {
                    width: parent.width
                    spacing: 8

                    Label {
                        text: "Alasan Keluar Lebih Awal *"
                        font {
                            pixelSize: 14
                            weight: Font.Medium
                        }
                        color: textColor
                    }

                    Rectangle {
                        width: parent.width
                        height: 140
                        radius: 12
                        color: Qt.rgba(textColor.r, textColor.g, textColor.b, 0.03)
                        border.color: reasonInput.activeFocus ? secondaryColor : dividerColor
                        border.width: reasonInput.activeFocus ? 2 : 1

                        Behavior on border.color {
                            ColorAnimation { duration: 200 }
                        }

                        Behavior on border.width {
                            NumberAnimation { duration: 200 }
                        }

                        ScrollView {
                            anchors.fill: parent
                            anchors.margins: 12
                            clip: true

                            TextArea {
                                id: reasonInput
                                width: parent.width
                                placeholderText: reasonInput.text.length === 0 ? "Contoh: Keperluan keluarga mendesak, jadwal dokter, dll." : ""
                                wrapMode: Text.Wrap
                                font.pixelSize: 14
                                color: textColor
                                selectByMouse: true
                                background: Item {}

                                // Character counter
                                property int maxLength: 500

                                onTextChanged: {
                                    if (text.length > maxLength) {
                                        text = text.substring(0, maxLength)
                                    }
                                }
                            }
                        }

                        // Character counter display
                        Label {
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            anchors.margins: 8
                            text: reasonInput.text.length + "/" + reasonInput.maxLength
                            font.pixelSize: 10
                            color: reasonInput.text.length > reasonInput.maxLength * 0.9 ?
                                       Material.color(Material.Red) : lightTextColor
                            opacity: 0.6
                        }
                    }
                }
            }
        }

        // Enhanced footer with better button styling
        footer: Rectangle {
            height: 80
            color: "transparent"

            Rectangle {
                anchors.fill: parent
                color: Qt.rgba(0, 0, 0, 0.02)
                radius: 16

                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    height: parent.radius
                    color: parent.color
                }
            }

            RowLayout {
                anchors.centerIn: parent
                spacing: 16

                // Cancel button with better styling
                Button {
                    text: "Batal"
                    flat: false
                    implicitWidth: 100
                    implicitHeight: 40

                    background: Rectangle {
                        radius: 8
                        color: parent.pressed ? Qt.rgba(0, 0, 0, 0.1) :
                                                parent.hovered ? Qt.rgba(0, 0, 0, 0.05) : "transparent"
                        border.color: dividerColor
                        border.width: 1

                        Behavior on color {
                            ColorAnimation { duration: 150 }
                        }
                    }

                    Material.foreground: lightTextColor
                    font.weight: Font.Medium

                    onClicked: earlyLeaveReasonDialog.reject()
                }

                // Submit button with enhanced styling and loading state
                Button {
                    id: submitButton
                    text: enabled ? "Submit dan Keluar" : "Mengirim..."
                    enabled: reasonInput.text.trim().length > 0 // Only require non-empty text
                    implicitWidth: 160
                    implicitHeight: 40

                    property bool isLoading: false

                    background: Rectangle {
                        radius: 8
                        color: parent.enabled ?
                                   (parent.pressed ? Qt.darker(secondaryColor, 1.1) :
                                                     parent.hovered ? Qt.lighter(secondaryColor, 1.1) : secondaryColor) :
                                   Qt.rgba(secondaryColor.r, secondaryColor.g, secondaryColor.b, 0.5)

                        Behavior on color {
                            ColorAnimation { duration: 150 }
                        }

                        // Loading indicator
                        Rectangle {
                            width: 16
                            height: 16
                            radius: 8
                            color: "white"
                            anchors.right: parent.right
                            anchors.rightMargin: 12
                            anchors.verticalCenter: parent.verticalCenter
                            visible: submitButton.isLoading
                            opacity: 0.8

                            RotationAnimation on rotation {
                                running: submitButton.isLoading
                                from: 0
                                to: 360
                                duration: 1000
                                loops: Animation.Infinite
                            }
                        }
                    }

                    Material.foreground: "white"
                    font.weight: Font.Medium

                    onClicked: {
                        isLoading = true
                        enabled = false
                        logger.submitEarlyLeaveReason(reasonInput.text.trim())
                    }
                }
            }
        }
    }


    Popup {
        id: reviewNotificationPopup
        width: 340
        height: 160
        x: (parent.width - width) / 2
        y: 50
        modal: false
        closePolicy: Popup.NoAutoClose
        padding: 0
        topInset: 0
        leftInset: 0
        rightInset: 0
        bottomInset: 0

        background: Rectangle {
            color: cardColor
            radius: 16
            border.color: Qt.lighter(dividerColor, 1.2)
            border.width: 1

            // Shadow effect
            layer.enabled: true


            // Gradient accent at top
            Rectangle {
                width: parent.width
                height: 4
                radius: 2
                anchors.top: parent.top
                anchors.horizontalCenter: parent.horizontalCenter
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#FF9800" }
                    GradientStop { position: 1.0; color: "#FFC107" }
                }
            }
        }

        contentItem: ColumnLayout {
            spacing: 12
            anchors.fill: parent
            anchors.margins: 20

            // Header row
            RowLayout {
                spacing: 12
                Layout.fillWidth: true

                Rectangle {
                    width: 36
                    height: 36
                    radius: 18
                    color: "#FFF3E0" // Light orange background
                    border.color: "#FFE0B2"
                    border.width: 1

                    Image {
                        source: "qrc:/icons/review.svg"
                        width: 20
                        height: 20
                        anchors.centerIn: parent
                    }
                }

                Label {
                    text: "Task Review Reminder"
                    font.bold: true
                    font.pixelSize: 18
                    color: textColor
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }
                Button {
                    icon.source: "qrc:/icons/close.svg"
                    icon.color: accentColor
                    icon.width: 26
                    icon.height: 26
                    flat: true
                    onClicked: reviewNotificationPopup.close()
                }
            }

            // Message content
            Label {
                id: reviewNotificationText
                text: "You have tasks pending review. Please check them before the deadline."
                wrapMode: Text.Wrap
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.topMargin: 4
                Layout.bottomMargin: 4
                color: lightTextColor
                font.pixelSize: 14
                lineHeight: 1.4
            }
        }


        function showNotification(message) {
            reviewNotificationText.text = message
            open()

            // Auto-close after 10 seconds
            notificationTimer.start()
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
        width: 380
        height: 280
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

        // === KARTU + BAYANGAN ====================================================
        Item {
            anchors.fill: parent

            // bayangan lembut
            Rectangle {
                anchors.fill: parent
                anchors.margins: -6
                radius: 18
                color: "#22000000"
                z: -1
            }

            // kartu utama
            Rectangle {
                id: card
                anchors.fill: parent
                radius: 12
                color: cardColor
                border.color: dividerColor
                border.width: 1
                opacity: showIdleNotification ? 1 : 0
                scale:   showIdleNotification ? 1 : 0.98
                Behavior on opacity { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
                Behavior on scale   { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }

                // overlay gradien halus
                Rectangle {
                    anchors.fill: parent
                    radius: card.radius
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: "#10ffffff" }
                        GradientStop { position: 1.0; color: "transparent" }
                    }
                }

                // === KONTEN =======================================================
                ColumnLayout {
                    id: rootLayout
                    anchors.fill: parent
                    anchors.margins: 24
                    spacing: 16

                    // HEADER: ikon + judul
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        // ikon bulat berdenyut
                        Rectangle {
                            id: iconCircle
                            width: 48; height: 48; radius: 24
                            color: nonProductiveColor
                            opacity: 0.14
                            Layout.alignment: Qt.AlignVCenter

                            // animasi denyut
                            SequentialAnimation on opacity {
                                running: idleNotificationWindow.visible
                                loops: Animation.Infinite
                                NumberAnimation { from: 0.14; to: 0.32; duration: 900; easing.type: Easing.InOutQuad }
                                NumberAnimation { from: 0.32; to: 0.14; duration: 900; easing.type: Easing.InOutQuad }
                            }

                            Image {
                                anchors.centerIn: parent
                                width: 22; height: 22
                                source: "qrc:/icons/danger.svg"
                                fillMode: Image.PreserveAspectFit
                                smooth: true
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            Label {
                                text: "Activity Paused"
                                font.pixelSize: 18
                                font.bold: true
                                color: nonProductiveColor
                                elide: Text.ElideRight
                            }
                        }
                    }

                    // PESAN DINAMIS
                    Frame {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        padding: 10

                        Label {
                            id: messageLabel
                            anchors.fill: parent
                            anchors.margins: 2
                            text: idleNotificationText + " ,Sesi kamu terdeteksi idle. Pastikan task berlanjut sesuai kebutuhan."
                            wrapMode: Text.Wrap
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            font.pixelSize: 14
                            color: textColor
                        }
                    }

                    // TOMBOL AKSI
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12
                        Layout.topMargin: 8

                        // Resume Activity Button
                        Button {
                            id: btnResume
                            text: "Resume Activity"
                            Layout.fillWidth: true
                            Layout.preferredHeight: 40

                            background: Rectangle {
                                color: btnResume.hovered ? Qt.darker(secondaryColor, 1.1) : secondaryColor
                                radius: 6
                            }

                            contentItem: Text {
                                text: btnResume.text
                                font {
                                    pixelSize: 14
                                    bold: true
                                }
                                color: "white"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
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

                // TOMBOL CLOSE (X)
                ToolButton {
                    anchors.top: parent.top
                    anchors.right: parent.right
                    anchors.margins: 8
                    width: 32; height: 32
                    text: "×"

                    focusPolicy: Qt.NoFocus
                    contentItem: Text {
                        text: control.text
                        font.pixelSize: 18
                        font.bold: true
                        color: "#ffffff"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: {
                        showIdleNotification = false
                        idleNotificationWindow.close()
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
    }

    // Fungsi untuk system tray notification
    function showSystemTrayNotification(title, message) {
        if (typeof SystemTrayIcon !== 'undefined' && SystemTrayIcon.supportsMessages) {
            SystemTrayIcon.showMessage(title, message)
        }
    }


    ApplicationWindow {
        id: authErrorWindow
        width: 380
        height: 240
        visible: false
        color: "transparent"
        title: "Sesi Berakhir"
        modality: Qt.ApplicationModal
        flags: Qt.Dialog | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
        // Center pada parent window
        Component.onCompleted: {
            x = (Screen.width - width) / 2
            y = (Screen.height - height) / 2
        }

        // Main container dengan shadow effect
        Rectangle {
            id: container
            anchors.fill: parent
            color: palette.window
            radius: 12
            border.color: palette.mid
            border.width: 1

            // Drop shadow effect
            layer.enabled: true
        }

        Column {
            anchors.fill: parent
            spacing: 0

            // Modern header dengan gradient
            Rectangle {
                width: parent.width
                height: 80
                radius: 12
                gradient: Gradient {
                    GradientStop { position: 0.0; color: palette.highlight }
                    GradientStop { position: 1.0; color: Qt.darker(palette.highlight, 1.2) }
                }

                // Square bottom corners
                Rectangle {
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: 12
                    color: Qt.darker(palette.highlight, 1.2)
                }

                Column {
                    anchors.centerIn: parent
                    spacing: 8

                    // Session expired icon
                    Rectangle {
                        width: 36
                        height: 36
                        color: "white"
                        radius: 18
                        anchors.horizontalCenter: parent.horizontalCenter

                        Text {
                            anchors.centerIn: parent
                            text: "🔒"
                            font.pixelSize: 18
                        }
                    }

                    Text {
                        text: "Sesi Berakhir"
                        color: palette.highlightedText
                        font.pixelSize: 16
                        font.weight: Font.Bold
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
            }

            // Content area
            Rectangle {
                width: parent.width
                height: parent.height - 80 - 60
                color: "transparent"

                ScrollView {
                    anchors.fill: parent
                    anchors.margins: 24
                    anchors.topMargin: 20
                    anchors.bottomMargin: 16

                    Text {
                        id: authErrorText
                        text: ""
                        wrapMode: Text.WordWrap
                        width: parent.width
                        font.pixelSize: 14
                        color: palette.windowText
                        lineHeight: 1.4
                        horizontalAlignment: Text.AlignHCenter

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
                }
            }

            // Modern footer
            Rectangle {
                width: parent.width
                height: 60
                color: palette.window
                radius: 12

                // Square top corners
                Rectangle {
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: 12
                    color: palette.window
                }

                // Subtle top border
                Rectangle {
                    width: parent.width - 32
                    height: 1
                    color: palette.mid
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top
                    anchors.topMargin: 8
                }

                Row {
                    anchors.centerIn: parent
                    spacing: 12

                    Button {
                        id: okButton
                        text: "Keluar"
                        width: 100
                        height: 36

                        background: Rectangle {
                            color: okButton.pressed ? Qt.darker(palette.highlight, 1.3) :
                                   okButton.hovered ? Qt.darker(palette.highlight, 1.1) : palette.highlight
                            radius: 6
                            border.color: Qt.darker(palette.highlight, 1.2)
                            border.width: 1

                            Behavior on color {
                                ColorAnimation { duration: 150 }
                            }

                            // Subtle inner glow when hovered
                            Rectangle {
                                anchors.fill: parent
                                anchors.margins: 1
                                radius: 5
                                color: "transparent"
                                border.color: okButton.hovered ? "white" : "transparent"
                                opacity: 0.3

                                Behavior on border.color {
                                    ColorAnimation { duration: 150 }
                                }
                            }
                        }

                        contentItem: Text {
                            text: okButton.text
                            color: palette.highlightedText
                            font.pixelSize: 13
                            font.weight: Font.Medium
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

                        onClicked: {
                            // Smooth close animation
                            closeAnimation.start()
                        }
                    }
                }
            }
        }

        // Smooth opening animation
        onVisibleChanged: {
            if (visible) {
                openAnimation.start()
            }
        }

        // Opening animation
        NumberAnimation {
            id: openAnimation
            target: container
            property: "scale"
            from: 0.8
            to: 1.0
            duration: 200
            easing.type: Easing.OutBack
        }

        // Closing animation
        SequentialAnimation {
            id: closeAnimation

            NumberAnimation {
                target: container
                property: "scale"
                from: 1.0
                to: 0.9
                duration: 150
                easing.type: Easing.InQuart
            }

            NumberAnimation {
                target: authErrorWindow
                property: "opacity"
                from: 1.0
                to: 0.0
                duration: 100
            }

            ScriptAction {
                script: {
                    authErrorWindow.visible = false
                    authErrorWindow.opacity = 1.0
                    container.scale = 1.0

                    // Reset ke login page
                    isLoggedIn = false
                    isProfileVisible = false
                    showRegisterPage = false
                    logger.logout()
                }
            }
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
            // Tampilkan dialog error sesi berakhir
            authErrorText.text = message
            authErrorWindow.visible = true
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

        // Positioning di tengah layar
        anchors.centerIn: Overlay.overlay
        width: Math.min(320, parent.width - 40)
        height: implicitHeight

        // Compact modern design
        padding: 0
        margins: 0
        topPadding: 0
        bottomPadding: 0
        leftPadding: 0
        rightPadding: 0

        onAccepted: visible = false

        background: Rectangle {
            color: palette.window
            radius: 8
            border.color: palette.mid
            border.width: 1

            // Subtle shadow
            layer.enabled: true
        }

        contentItem: Column {
            spacing: 0

            // Compact header
            Rectangle {
                width: parent.width
                height: 48
                color: palette.highlight
                radius: 8

                // Square bottom corners
                Rectangle {
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: 8
                    color: palette.highlight
                }

                Row {
                    anchors.centerIn: parent
                    spacing: 8

                    // Modern error icon
                    Text {
                        text: "⚠"
                        font.pixelSize: 16
                        color: palette.highlightedText
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Text {
                        text: "Ada Kesalahan Koneksi"
                        color: palette.highlightedText
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }

            // Content area - compact
            Rectangle {
                width: parent.width
                height: Math.max(60, errorText.implicitHeight + 32)
                color: "transparent"

                ScrollView {
                    anchors.fill: parent
                    anchors.margins: 16
                    anchors.topMargin: 12
                    anchors.bottomMargin: 12

                    Text {
                        id: errorText
                        text: pingErrorText.text
                        wrapMode: Text.Wrap
                        width: parent.width
                        font.pixelSize: 13
                        color: palette.windowText
                        lineHeight: 1.3
                    }
                }
            }

            // Compact footer
            Rectangle {
                width: parent.width
                height: 44
                color: palette.window

                Rectangle {
                    width: parent.width
                    height: 1
                    color: palette.mid
                    anchors.top: parent.top
                }

                Button {
                    anchors.centerIn: parent
                    width: 80
                    height: 28
                    text: "OK"

                    background: Rectangle {
                        color: parent.pressed ? Qt.darker(palette.button, 1.2) :
                               parent.hovered ? Qt.lighter(palette.button, 1.1) : palette.button
                        radius: 4
                        border.color: palette.mid
                        border.width: parent.activeFocus ? 2 : 1

                        Behavior on color {
                            ColorAnimation { duration: 100 }
                        }
                    }

                    contentItem: Text {
                        text: parent.text
                        color: palette.buttonText
                        font.pixelSize: 12
                        font.weight: Font.Medium
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    onClicked: pingErrorDialog.accept()
                }
            }
        }

        // Smooth animations
        enter: Transition {
            NumberAnimation {
                property: "scale"
                from: 0.9
                to: 1.0
                duration: 150
                easing.type: Easing.OutQuart
            }
            NumberAnimation {
                property: "opacity"
                from: 0.0
                to: 1.0
                duration: 150
            }
        }

        exit: Transition {
            NumberAnimation {
                property: "scale"
                from: 1.0
                to: 0.9
                duration: 100
            }
            NumberAnimation {
                property: "opacity"
                from: 1.0
                to: 0.0
                duration: 100
            }
        }

        // Hidden text element untuk data binding
        Text {
            id: pingErrorText
            text: ""
            visible: false
        }
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
                                logger.logout(); // Call the new logout function
                                isLoggedIn = false
                                currentUsername = ""
                                sortedApps = []
                                logger.clearLogFilter()
                                profileImagePath = ":/profilImage.png"
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
                            needReviewReasonDialog.openWithTaskId(stableTaskMenu.taskId)
                        }
                    }
                }
                // Tambahkan komponen Dialog ini di dalam file Main.qml
                Dialog {
                    id: needReviewReasonDialog
                    title: "Alasan Permintaan Review"
                    modal: true
                    width: Math.min(520, parent.width * 0.9)
                    height: Math.min(420, parent.height * 0.8)
                    anchors.centerIn: parent
                    padding: 0
                    closePolicy: Popup.NoAutoClose

                    property int taskId: -1 // Untuk menyimpan ID task yang akan di-update

                    // Enhanced background with subtle shadow effect
                    background: Rectangle {
                        color: cardColor
                        radius: 16
                        border.color: Qt.rgba(dividerColor.r, dividerColor.g, dividerColor.b, 0.3)
                        border.width: 1

                        // Subtle shadow effect
                        Rectangle {
                            anchors.fill: parent
                            anchors.margins: -2
                            color: "transparent"
                            radius: 18
                            border.color: Qt.rgba(0, 0, 0, 0.1)
                            border.width: 1
                            z: -1
                        }
                    }

                    // Custom header with icon and better typography
                    header: Rectangle {
                        height: 90
                        color: "transparent"

                        Rectangle {
                            anchors.fill: parent
                            anchors.bottomMargin: 10
                            color: cardColor
                            radius: 16

                            Rectangle {
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.bottom: parent.bottom
                                height: parent.radius
                                color: parent.color
                            }
                        }

                        RowLayout {
                            anchors.centerIn: parent
                            anchors.topMargin: 5
                            spacing: 12

                            // Review icon
                            Rectangle {
                                width: 40
                                height: 40
                                radius: 20
                                color: Qt.rgba(33, 150, 243, 0.1)

                                Text {
                                    anchors.centerIn: parent
                                    text: "📋"
                                    font.pixelSize: 20
                                }
                            }

                            Column {
                                spacing: 4

                                Label {
                                    text: "Permintaan Review"
                                    font {
                                        pixelSize: 20
                                        weight: Font.Bold
                                    }
                                    color: textColor
                                }

                                Label {
                                    text: "Berikan alasan yang jelas untuk review"
                                    font.pixelSize: 12
                                    color: lightTextColor
                                    opacity: 0.8
                                }
                            }
                        }
                    }

                    contentItem: Item {
                        anchors.fill: parent
                        anchors.margins: 24
                        anchors.topMargin: 100

                        Column {
                            anchors.fill: parent
                            spacing: 24

                            // Enhanced info section with better visual hierarchy
                            Rectangle {
                                width: parent.width
                                height: 70
                                radius: 12
                                color: Qt.rgba(76, 175, 80, 0.05)
                                border.color: Qt.rgba(76, 175, 80, 0.2)
                                border.width: 1

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 16
                                    spacing: 12

                                    Text {
                                        text: "💡"
                                        font.pixelSize: 18
                                    }

                                    Label {
                                        text: "Tugas akan dipindahkan ke status 'Need Review'.\nManajer akan menerima notifikasi untuk melakukan review."
                                        wrapMode: Text.Wrap
                                        Layout.fillWidth: true
                                        color: textColor
                                        font.pixelSize: 13
                                        lineHeight: 1.3
                                    }
                                }
                            }

                            // Enhanced text input section
                            Column {
                                width: parent.width
                                spacing: 8

                                Label {
                                    text: "Alasan Permintaan Review *"
                                    font {
                                        pixelSize: 14
                                        weight: Font.Medium
                                    }
                                    color: textColor
                                }

                                Rectangle {
                                    width: parent.width
                                    height: 140
                                    radius: 12
                                    color: Qt.rgba(textColor.r, textColor.g, textColor.b, 0.03)
                                    border.color: reasonInput_.activeFocus ? secondaryColor : dividerColor
                                    border.width: reasonInput_.activeFocus ? 2 : 1

                                    Behavior on border.color {
                                        ColorAnimation { duration: 200 }
                                    }

                                    Behavior on border.width {
                                        NumberAnimation { duration: 200 }
                                    }

                                    ScrollView {
                                        anchors.fill: parent
                                        anchors.margins: 12
                                        clip: true

                                        TextArea {
                                            id: reasonInput_
                                            width: parent.width
                                            placeholderText: reasonInput_.text.length === 0 ? "Contoh: Butuh verifikasi dari manajer proyek, ada kendala teknis, dll." : ""
                                            wrapMode: Text.Wrap
                                            font.pixelSize: 14
                                            color: textColor
                                            selectByMouse: true
                                            background: Item {}

                                            // Character counter
                                            property int maxLength: 500

                                            onTextChanged: {
                                                if (text.length > maxLength) {
                                                    text = text.substring(0, maxLength)
                                                }
                                            }
                                        }
                                    }

                                    // Character counter display
                                    Label {
                                        anchors.right: parent.right
                                        anchors.bottom: parent.bottom
                                        anchors.margins: 8
                                        text: reasonInput_.text.length + "/" + reasonInput_.maxLength
                                        font.pixelSize: 10
                                        color: reasonInput_.text.length > reasonInput_.maxLength * 0.9 ?
                                                   Material.color(Material.Red) : lightTextColor
                                        opacity: 0.6
                                    }
                                }
                            }
                        }
                    }

                    // Enhanced footer with better button styling
                    footer: Rectangle {
                        height: 80
                        color: "transparent"

                        Rectangle {
                            anchors.fill: parent
                            color: Qt.rgba(0, 0, 0, 0.02)
                            radius: 16

                            Rectangle {
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.top: parent.top
                                height: parent.radius
                                color: parent.color
                            }
                        }

                        RowLayout {
                            anchors.centerIn: parent
                            spacing: 16

                            // Cancel button with better styling
                            Button {
                                text: "Batal"
                                flat: false
                                implicitWidth: 100
                                implicitHeight: 40

                                background: Rectangle {
                                    radius: 8
                                    color: parent.pressed ? Qt.rgba(0, 0, 0, 0.1) :
                                                            parent.hovered ? Qt.rgba(0, 0, 0, 0.05) : "transparent"
                                    border.color: dividerColor
                                    border.width: 1

                                    Behavior on color {
                                        ColorAnimation { duration: 150 }
                                    }
                                }

                                Material.foreground: lightTextColor
                                font.weight: Font.Medium

                                onClicked: needReviewReasonDialog.reject()
                            }

                            // Submit button with enhanced styling and loading state
                            Button {
                                id: submitButton_
                                text: enabled ? "Submit Review" : "Mengirim..."
                                enabled: reasonInput_.text.trim().length > 0
                                implicitWidth: 140
                                implicitHeight: 40

                                property bool isLoading: false

                                background: Rectangle {
                                    radius: 8
                                    color: parent.enabled ?
                                               (parent.pressed ? Qt.darker(secondaryColor, 1.1) :
                                                                 parent.hovered ? Qt.lighter(secondaryColor, 1.1) : secondaryColor) :
                                               Qt.rgba(secondaryColor.r, secondaryColor.g, secondaryColor.b, 0.5)

                                    Behavior on color {
                                        ColorAnimation { duration: 150 }
                                    }

                                    // Loading indicator
                                    Rectangle {
                                        width: 16
                                        height: 16
                                        radius: 8
                                        color: "white"
                                        anchors.right: parent.right
                                        anchors.rightMargin: 12
                                        anchors.verticalCenter: parent.verticalCenter
                                        visible: submitButton_.isLoading
                                        opacity: 0.8

                                        RotationAnimation on rotation {
                                            running: submitButton_.isLoading
                                            from: 0
                                            to: 360
                                            duration: 1000
                                            loops: Animation.Infinite
                                        }
                                    }
                                }

                                Material.foreground: "white"
                                font.weight: Font.Medium

                                onClicked: {
                                    isLoading = true
                                    enabled = false

                                    // Pindahkan logika XMLHttpRequest ke sini
                                    var payload = {
                                        "status": "need-review",
                                        "alasan": reasonInput_.text.trim()
                                    };

                                    var apiUrl = "https://deskmon.pranala-dt.co.id/api/update-status-task/" +
                                            needReviewReasonDialog.taskId + "/" + logger.currentUserId;

                                    var request = new XMLHttpRequest();
                                    request.open("POST", apiUrl);
                                    request.setRequestHeader("Content-Type", "application/json");
                                    request.setRequestHeader("Authorization", "Bearer " + logger.authToken);

                                    request.onreadystatechange = function() {
                                        if (request.readyState === XMLHttpRequest.DONE) {
                                            submitButton_.isLoading = false
                                            if (request.status === 200) {
                                                console.log("Task status updated to 'need review' with reason.");
                                                logger.fetchAndStoreTasks(); // Refresh daftar task
                                                needReviewReasonDialog.accept(); // Tutup dialog setelah submit
                                            } else {
                                                console.error("Failed to update task status:", request.status, request.responseText);
                                                submitButton_.enabled = true // Re-enable button on error
                                            }
                                        }
                                    };

                                    request.send(JSON.stringify(payload));
                                }
                            }
                        }
                    }

                    // Fungsi untuk membuka dialog sambil mengirimkan taskId
                    function openWithTaskId(id) {
                        taskId = id;
                        reasonInput_.text = ""; // Kosongkan input setiap kali dialog dibuka
                        submitButton_.isLoading = false // Reset loading state
                        submitButton_.enabled = true // Reset button state
                        open();
                    }
                }







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
                    width: 420
                    height: 320
                    padding: 0

                    property int taskId: -1

                    // Remove default background
                    background: Item {}

                    // Custom background with shadow effect
                    Rectangle {
                        id: shadowRect
                        anchors.fill: parent
                        anchors.margins: -8
                        color: "transparent"

                        // Drop shadow effect
                        Rectangle {
                            anchors.fill: parent
                            anchors.margins: 8
                            color: "#20000000"
                            radius: 16
                            opacity: 0.3
                        }

                        Rectangle {
                            anchors.fill: parent
                            anchors.margins: 4
                            color: "#10000000"
                            radius: 14
                            opacity: 0.2
                        }
                    }

                    contentItem: Rectangle {
                        color: cardColor
                        radius: 16
                        border.width: 1
                        border.color: Qt.rgba(255, 255, 255, 0.1)

                        // Gradient overlay for depth
                        Rectangle {
                            anchors.fill: parent
                            radius: 16
                            gradient: Gradient {
                                GradientStop { position: 0.0; color: Qt.rgba(255, 255, 255, 0.05) }
                                GradientStop { position: 1.0; color: "transparent" }
                            }
                        }

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 32
                            spacing: 24

                            // Icon section
                            Item {
                                Layout.alignment: Qt.AlignHCenter
                                Layout.preferredWidth: 64
                                Layout.preferredHeight: 64

                                Rectangle {
                                    anchors.centerIn: parent
                                    width: 64
                                    height: 64
                                    radius: 32
                                    color: Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 0.1)
                                    border.width: 2
                                    border.color: Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 0.3)

                                    Text {
                                        anchors.centerIn: parent
                                        text: "⚠"
                                        font.pixelSize: 28
                                        color: accentColor
                                    }
                                }
                            }

                            // Title and message section
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 12

                                Label {
                                    text: "Switch Project?"
                                    font {
                                        pixelSize: 20
                                        family: "Segoe UI"
                                        weight: Font.Medium
                                    }
                                    color: textColor
                                    Layout.alignment: Qt.AlignHCenter
                                }

                                Label {
                                    text: "You're about to switch to another project. Your current progress will be saved automatically."
                                    font {
                                        pixelSize: 14
                                        family: "Segoe UI"
                                    }
                                    color: Qt.rgba(textColor.r, textColor.g, textColor.b, 0.7)
                                    wrapMode: Text.Wrap
                                    Layout.fillWidth: true
                                    horizontalAlignment: Text.AlignHCenter
                                    lineHeight: 1.4
                                }
                            }



                            // Button section
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 16

                                Button {
                                    id: cancelButton
                                    text: "Cancel"
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 48

                                    background: Rectangle {
                                        radius: 8
                                        color: cancelButton.pressed ? Qt.rgba(textColor.r, textColor.g, textColor.b, 0.1) :
                                               cancelButton.hovered ? Qt.rgba(textColor.r, textColor.g, textColor.b, 0.05) : "transparent"
                                        border.width: 1
                                        border.color: Qt.rgba(textColor.r, textColor.g, textColor.b, 0.2)

                                        Behavior on color {
                                            ColorAnimation { duration: 150 }
                                        }
                                    }

                                    contentItem: Text {
                                        text: cancelButton.text
                                        font {
                                            pixelSize: 14
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
                                    Layout.preferredHeight: 48

                                    background: Rectangle {
                                        radius: 8
                                        color: confirmButton.pressed ? Qt.darker(accentColor, 1.2) :
                                               confirmButton.hovered ? Qt.lighter(accentColor, 1.1) : accentColor

                                        Behavior on color {
                                            ColorAnimation { duration: 150 }
                                        }

                                        // Subtle glow effect
                                        Rectangle {
                                            anchors.fill: parent
                                            anchors.margins: -2
                                            radius: 10
                                            color: "transparent"
                                            border.width: 1
                                            border.color: Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 0.3)
                                            opacity: confirmButton.hovered ? 1 : 0

                                            Behavior on opacity {
                                                NumberAnimation { duration: 150 }
                                            }
                                        }
                                    }

                                    contentItem: Text {
                                        text: confirmButton.text
                                        font {
                                            pixelSize: 14
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
                            Item {
                                Layout.fillHeight: true
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
                                duration: 250
                                easing.type: Easing.OutCubic
                            }
                            NumberAnimation {
                                property: "scale"
                                from: 0.8
                                to: 1.0
                                duration: 300
                                easing.type: Easing.OutBack
                                easing.overshoot: 1.2
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
                                duration: 200
                                easing.type: Easing.InCubic
                            }
                            NumberAnimation {
                                property: "scale"
                                from: 1.0
                                to: 0.9
                                duration: 200
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
