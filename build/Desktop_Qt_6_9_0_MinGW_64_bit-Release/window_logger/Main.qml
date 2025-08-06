import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material
import QtQuick.Dialogs
import QtQuick.Effects
import QtQuick.Window

ApplicationWindow {
    id: window
    title: qsTr("Deskmon - v1.0.1.5")
    visibility: Window.Maximized
    minimumWidth: 900
    minimumHeight: 900

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
            window.visibility = Window.Maximized

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
            console.log("Gagal mem-parsing URL, mencoba fallback:", urlString);
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
        width: 300
        height: 220
        x: (Screen.width - width) / 2
        y: (Screen.height - height) / 2
        flags: Qt.Dialog | Qt.WindowStaysOnTopHint
        modality: Qt.ApplicationModal
        title: "Idle Detected"

        // Handle window closing
        onVisibleChanged: {
            if (!visible) {
                showIdleNotification = false
            }
        }

        // Simple binding approach
        visible: showIdleNotification

        background: Rectangle {
            color: cardColor
            border.color: dividerColor
            border.width: 1
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 12

            // SVG Image - Option 2: Using Rectangle container
            Rectangle {
                width: 24
                height: 24
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 4
                color: "transparent"

                Image {
                    anchors.fill: parent
                    source: "qrc:/icons/danger.svg" // Ganti dengan path file SVG Anda
                    fillMode: Image.PreserveAspectFit
                }
            }

            Label {
                text: "Idle Detected"
                font { bold: true; pixelSize: 16 }
                color: nonProductiveColor
                Layout.alignment: Qt.AlignHCenter
            }

            Label {
                text: idleNotificationText
                font.pixelSize: 14
                wrapMode: Text.Wrap
                Layout.fillWidth: true
                Layout.fillHeight: true
            }

            Button {
                text: "OK"
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 100
                Material.background: secondaryColor
                Material.foreground: "white"
                onClicked: {
                    showIdleNotification = false
                    idleNotificationWindow.close()
                    if (logger.isTaskPaused) {
                        logger.toggleTaskPause() // Ini akan melanjutkan task yang dijeda
                    }
                }
            }
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
    }

    // Fungsi untuk system tray notification
    function showSystemTrayNotification(title, message) {
        if (typeof SystemTrayIcon !== 'undefined' && SystemTrayIcon.supportsMessages) {
            SystemTrayIcon.showMessage(title, message)
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




    // Dashboard
    Rectangle {
        anchors.fill: parent
        color: backgroundColor
        visible: isLoggedIn && !isProfileVisible

        Component.onCompleted: {
            console.log("Dashboard component completed. Setting date range to today.");
            // Logika ini akan berjalan setiap kali dasbor ditampilkan setelah login
            var today = new Date();
            startSelectedDate = today;     // [cite: 150]
            endSelectedDate = today;       // [cite: 150]
            isDateSelected = true;         // [cite: 151]
            applyDateRange();              // [cite: 151]
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
                        spacing: 12
                        layoutDirection: Qt.RightToLeft

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
                        Layout.minimumWidth: 500
                        Layout.maximumWidth: 800
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
                    title: "Confirm Task Switch"
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
                                text: "Are you sure you want to switch to another project?"
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
                                    onClicked: confirmSwitchDialog.reject()
                                }

                                Button {
                                    text: "OK"
                                    Layout.preferredWidth: 120
                                    Layout.preferredHeight: 40
                                    Material.background: secondaryColor
                                    Material.foreground: "white"
                                    font.pixelSize: 14
                                    onClicked: {
                                        logger.setActiveTask(confirmSwitchDialog.taskId)
                                        confirmSwitchDialog.accept()
                                        isPop_up_waktuhabis_open = false
                                        isPop_up_waktuhabis_kurangdari_open = false
                                        console.log ("ok 1", isPop_up_waktuhabis_open )
                                        console.log ("ok 2", isPop_up_waktuhabis_kurangdari_open )
                                    }
                                }
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
