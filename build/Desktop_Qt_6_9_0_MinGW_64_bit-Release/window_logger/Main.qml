import QtQuick
import QtQuick.Controls 2.15
import QtQuick.Layouts
import QtQuick.Controls.Material
import QtQuick.Dialogs
import QtQuick.Effects

ApplicationWindow {
    id: window
    width: Screen.width
    height: Screen.Height
    title: qsTr("Deskmon")
    visible: true
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
        // Pisahkan jam dan menit
        var parts = timeStr.split(":")
        if(parts.length !== 2) return 0
        return parseInt(parts[0]) * 60 + parseInt(parts[1])
    }

    onPublic_curent_timeChanged: {
        console.log("Max:", public_max_time, "Current:", public_curent_time)

        // Konversi ke menit
        var maxMinutes = timeStringToMinutes(public_max_time)
        var currentMinutes = timeStringToMinutes(public_curent_time)
        var diffMinutes = maxMinutes - currentMinutes

        console.log("Selisih menit:", diffMinutes)

        if(diffMinutes <= 0) {

            console.log ("chek 1", isPop_up_waktuhabis_open)
            if(isPop_up_waktuhabis_open == false){
                warningWindowComponent.newText = "Waktu anda Sudah Habis"
                isPop_up_waktuhabis_open = true
                warningWindowComponent.show()
                console.log("Waktu sudah habis!")
            }
            // Tampilkan popup waktu habis

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




    // Login Page
    Rectangle {
        anchors.fill: parent
        color: backgroundColor
        visible: !isLoggedIn && !isProfileVisible && !showRegisterPage

        Component.onCompleted: {
            // Cek apakah user sebelumnya sudah login
            if (logger.currentUserId !== -1) {
                usernameField.text = logger.currentUsername
                passwordField.text = logger.getUserPassword(logger.currentUsername) // ambil password

                console.log("Pre-filled login with:", usernameField.text)
            }
        }


        Rectangle {
            anchors.centerIn: parent
            width: 360
            height: 500
            color: cardColor
            radius: 12
            border.color: dividerColor
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 24
                spacing: 16

                Label {
                    text: "Deskmon"
                    font { bold: true; pixelSize: 24; family: "Segoe UI" }
                    color: primaryColor
                    Layout.alignment: Qt.AlignHCenter
                }

                Label {
                    text: "Sign in to track your activity"
                    font { pixelSize: 16; family: "Segoe UI" }
                    color: lightTextColor
                    Layout.alignment: Qt.AlignHCenter
                }

                TextField {
                    id: usernameField
                    placeholderText: "Username"
                    Layout.fillWidth: true
                    font.pixelSize: 16
                    padding: 12
                    background: Rectangle {
                        color: window.Material.theme === Material.Dark ? "#282828" : "#F9FAFB"
                        radius: 8
                    }
                    onAccepted: loginButton.clicked()
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    TextField {
                        id: passwordField
                        placeholderText: "Password"
                        echoMode: showPassword ? TextInput.Normal : TextInput.Password
                        Layout.fillWidth: true
                        font.pixelSize: 16
                        padding: 12
                        background: Rectangle {
                            color: window.Material.theme === Material.Dark ? "#282828" : "#F9FAFB"
                            radius: 8
                        }
                        onAccepted: loginButton.clicked()
                    }

                    Button {
                        id: showPasswordButton
                        icon.source: showPassword ? visibilityIcon : visibilityOffIcon
                        icon.color: primaryColor
                        icon.width: 24
                        icon.height: 24
                        flat: true
                        Layout.preferredWidth: 48
                        Layout.preferredHeight: 48
                        onClicked: showPassword = !showPassword
                        background: Rectangle {
                            color: "transparent"
                        }
                    }
                }

                Button {
                    id: loginButton
                    text: "Login"
                    Layout.fillWidth: true
                    Layout.preferredHeight: 48
                    Material.background: secondaryColor
                    Material.foreground: "white"
                    font.pixelSize: 16
                    onClicked: {
                        if (logger.authenticate(usernameField.text, passwordField.text)) {
                            console.log("Login successful")

                            // Set login state
                            isLoggedIn = true

                            // Ambil data user dari Logger properties
                            currentUsername = logger.currentUsername
                            console.log("Current username from logger:", currentUsername)
                            console.log("Current email from logger:", logger.currentUserEmail)

                            // Update temp variables
                            tempUsername = currentUsername
                            tempPassword = ""
                            tempDepartment = logger.getUserDepartment(currentUsername)

                            // Debug log
                            console.log("Username:", currentUsername)
                            console.log("Email:", logger.getUserEmail(currentUsername))
                            console.log("Department:", tempDepartment)

                            // Ambil profile image path
                            var savedImagePath = logger.getProfileImagePath(currentUsername)
                            profileImagePath = savedImagePath !== "" ? savedImagePath + "?t=" + new Date().getTime() : ":/profilImage.png"
                            refreshProfileImage()

                            // Set date range
                            var today = new Date()
                            startSelectedDate = today
                            endSelectedDate = today
                            isDateSelected = true
                            applyDateRange()

                            // Clear form fields
                            usernameField.text = ""
                            passwordField.text = ""
                            error_Label.text = ""

                        } else {
                            console.log("Login failed")
                            error_Label.text = "Invalid username or password"
                        }
                    }



                    Behavior on Material.background {
                        ColorAnimation { duration: 200 }
                    }
                }
                function refreshProfileImage() {
                    console.log("Refreshing profile image for user:", currentUsername, "path:", profileImagePath)
                    profileImage.source = ""
                    profileImage.source = profileImagePath
                }

                Label {
                    id: error_Label
                    text: ""
                    color: "red"
                    font.pixelSize: 14
                    Layout.alignment: Qt.AlignHCenter
                }
            }
        }
    }




    // Profile Page
    Rectangle {
        anchors.fill: parent
        color: window.backgroundColor
        visible: isProfileVisible
        opacity: isProfileVisible ? 1 : 0
        Behavior on opacity {
            NumberAnimation { duration: 300 }
        }

        Rectangle {
            width: parent.width
            height: 280
            gradient: Gradient {
                GradientStop {
                    position: 0.0
                    color: Qt.rgba(primaryColor.r,
                                   primaryColor.g,
                                   primaryColor.b,
                                   1.0)
                }
                GradientStop {
                    position: 1.0
                    color: Qt.rgba(primaryColor.r,
                                   primaryColor.g,
                                   primaryColor.b,
                                   0.0)
                }
            }
            anchors.top: parent.top
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // Header with back button and title
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 80

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 16
                    anchors.rightMargin: 16
                    spacing: 16

                    RoundButton {
                        id: backButton
                        radius: 20
                        Layout.preferredWidth: 40
                        Layout.preferredHeight: 40
                        Material.background: Qt.rgba(1,1,1,0.2)
                        Material.foreground: "white"
                        icon.source: "qrc:/icons/arrow_back.svg"
                        icon.width: 24
                        icon.height: 24
                        onClicked: isProfileVisible = false

                        HoverHandler {
                            cursorShape: Qt.PointingHandCursor
                        }
                    }

                    Label {
                        text: "My Profile"
                        font {
                            bold: true;
                            pixelSize: 22;
                            family: "Segoe UI Semibold"
                        }
                        color: "white"
                        Layout.leftMargin: 8
                    }

                    Item { Layout.fillWidth: true }
                }
            }

            // Profile Card with modern design
            Rectangle {
                Layout.preferredWidth: Math.min(500, parent.width - 32)
                Layout.fillHeight: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 8
                Layout.bottomMargin: 16
                color: window.cardColor
                radius: 24
                Layout.alignment: Qt.AlignHCenter
                layer.enabled: true

                Flickable {
                    anchors.fill: parent
                    anchors.margins: 20
                    contentHeight: profileContent.height
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds

                    ColumnLayout {
                        id: profileContent
                        width: parent.width
                        spacing: 24

                        // Profile Picture Section
                        ColumnLayout {
                            Layout.alignment: Qt.AlignHCenter
                            spacing: 16

                            // Profile Picture Container
                            Item {
                                Layout.alignment: Qt.AlignHCenter
                                width: 140
                                height: 140

                                Rectangle {
                                    id: profileFrame
                                    anchors.fill: parent
                                    radius: width/2
                                    color: "transparent"
                                    border.color: window.dividerColor
                                    border.width: 2
                                    layer.enabled: true

                                    Image {
                                        id: profileImage
                                        anchors.fill: parent
                                        anchors.margins: 4
                                        source: profileImagePath
                                        fillMode: Image.PreserveAspectCrop
                                        layer.enabled: true
                                        cache: false
                                        onStatusChanged: {
                                            if (status === Image.Ready) {
                                                console.log("Profile image loaded successfully:", source)
                                            } else if (status === Image.Error) {
                                                console.log("Failed to load profile image:", source, "falling back to default")
                                                source = ":/profilImage.png"
                                            }
                                        }

                                        Rectangle {
                                            visible: profileImage.status !== Image.Ready
                                            anchors.fill: parent
                                            color: primaryColor
                                            radius: width/2

                                            Image {
                                                anchors.centerIn: parent
                                                source: "qrc:/icons/camera.svg"
                                                width: 60
                                                height: 60
                                                opacity: 0.7
                                            }
                                        }
                                    }
                                }

                                RoundButton {
                                    anchors.bottom: profileFrame.bottom
                                    anchors.right: profileFrame.right
                                    radius: 16
                                    width: 46
                                    height: 46
                                    Material.background: cardColor
                                    opacity: 0.7
                                    Material.foreground: primaryColor
                                    icon.source: "qrc:/icons/edit.svg"
                                    icon.width: 28
                                    icon.height: 28
                                    onClicked: fileDialog.open()

                                    background: Rectangle {
                                        radius: parent.radius
                                        color: cardColor
                                        border.color: primaryColor
                                        border.width: 2
                                    }

                                    HoverHandler {
                                        cursorShape: Qt.PointingHandCursor
                                    }
                                }
                            }

                            // User Info Display
                            ColumnLayout {
                                Layout.alignment: Qt.AlignHCenter
                                spacing: 4

                                Label {
                                    text: logger.currentUsername || "Username not set"
                                    font {
                                        pixelSize: 20;
                                        bold: true;
                                        family: "Segoe UI Semibold"
                                    }
                                    color: window.textColor
                                    Layout.alignment: Qt.AlignHCenter

                                    // Debug connection
                                    Component.onCompleted: {
                                        console.log("Profile page - Username label:", text)
                                        console.log("Logger currentUsername:", logger.currentUsername)
                                    }

                                    // Update when logger properties change
                                    Connections {
                                        target: logger
                                        function onCurrentUsernameChanged() {
                                            console.log("Username changed to:", logger.currentUsername)
                                        }
                                    }
                                }

                                Label {
                                    text: logger.currentUserEmail || "Email not set"
                                    font {
                                        pixelSize: 14;
                                        family: "Segoe UI"
                                    }
                                    color: window.lightTextColor
                                    Layout.alignment: Qt.AlignHCenter

                                    // Debug connection
                                    Component.onCompleted: {
                                        console.log("Profile page - Email label:", text)
                                        console.log("Logger currentUserEmail:", logger.currentUserEmail)
                                    }

                                    // Update when logger properties change
                                    Connections {
                                        target: logger
                                        function onCurrentUserEmailChanged() {
                                            console.log("Email changed to:", logger.currentUserEmail)
                                        }
                                    }
                                }
                            }
                        }

                        // Profile Details Section
                        ColumnLayout {
                            spacing: 16
                            Layout.fillWidth: true
                            Layout.leftMargin: 8
                            Layout.rightMargin: 8

                            // Username Field
                            ColumnLayout {
                                spacing: 6
                                Layout.fillWidth: true

                                Label {
                                    text: "Username"
                                    font {
                                        pixelSize: 13;
                                        bold: true;
                                        family: "Segoe UI"
                                    }
                                    color: window.lightTextColor
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    height: 48
                                    radius: 12
                                    color: window.Material.theme === Material.Dark ? "#282828" : "#F9FAFB"
                                    border.color: window.dividerColor
                                    border.width: 1

                                    Label {
                                        anchors.fill: parent
                                        anchors.leftMargin: 16
                                        anchors.rightMargin: 16
                                        verticalAlignment: Text.AlignVCenter
                                        text: logger.currentUsername || "Username not set"
                                        font.pixelSize: 15
                                        color: window.textColor
                                        elide: Text.ElideRight
                                    }
                                }
                            }

                            // Email Field
                            ColumnLayout {
                                spacing: 6
                                Layout.fillWidth: true

                                Label {
                                    text: "Email"
                                    font {
                                        pixelSize: 13;
                                        bold: true;
                                        family: "Segoe UI"
                                    }
                                    color: window.lightTextColor
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    height: 48
                                    radius: 12
                                    color: window.Material.theme === Material.Dark ? "#282828" : "#F9FAFB"
                                    border.color: window.dividerColor
                                    border.width: 1

                                    Label {
                                        anchors.fill: parent
                                        anchors.leftMargin: 16
                                        anchors.rightMargin: 16
                                        verticalAlignment: Text.AlignVCenter
                                        text: logger.currentUserEmail || "Email not set"
                                        font.pixelSize: 15
                                        color: window.textColor
                                        elide: Text.ElideRight
                                    }
                                }
                            }

                            // Role Field
                            ColumnLayout {
                                spacing: 6
                                Layout.fillWidth: true

                                Label {
                                    text: "Role"
                                    font {
                                        pixelSize: 13;
                                        bold: true;
                                        family: "Segoe UI"
                                    }
                                    color: window.lightTextColor
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    height: 48
                                    radius: 12
                                    color: window.Material.theme === Material.Dark ? "#282828" : "#F9FAFB"
                                    border.color: window.dividerColor
                                    border.width: 1

                                    Label {
                                        anchors.fill: parent
                                        anchors.leftMargin: 16
                                        anchors.rightMargin: 16
                                        verticalAlignment: Text.AlignVCenter
                                        text: logger.getUserDepartment(logger.currentUsername) || "Role not set"
                                        font.pixelSize: 15
                                        color: window.textColor
                                        elide: Text.ElideRight
                                    }
                                }
                            }

                            // Password Field
                            ColumnLayout {
                                spacing: 6
                                Layout.fillWidth: true

                                Label {
                                    text: "Password"
                                    font { pixelSize: 13; bold: true; family: "Segoe UI" }
                                    color: window.lightTextColor
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    height: 48
                                    radius: 12
                                    color: window.Material.theme === Material.Dark ? "#282828" : "#F9FAFB"
                                    border.color: window.dividerColor
                                    border.width: 1

                                    Label {
                                        anchors.fill: parent
                                        anchors.leftMargin: 16
                                        anchors.rightMargin: 16
                                        verticalAlignment: Text.AlignVCenter
                                        text: "••••••••"
                                        font.pixelSize: 15
                                        color: window.textColor
                                    }
                                }
                            }
                        }

                        // Status Message
                        Label {
                            id: profileErrorLabel
                            text: ""
                            color: profileErrorLabel.text.includes("success") ? "#10B981" : "#EF4444"
                            font.pixelSize: 13
                            Layout.alignment: Qt.AlignHCenter
                            visible: text !== ""
                        }
                    }
                }
            }
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


                    RowLayout {
                        anchors.fill: parent
                        spacing: 24

                        // Application Usage Section (Left)
                        ColumnLayout {
                            Layout.preferredWidth: parent.width * 0.3
                            Layout.fillHeight: true
                            spacing: 12

                            // Header
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 8

                                    Label {
                                        text: "Application Usage"
                                        font {
                                            family: "Segoe UI"
                                            weight: Font.DemiBold
                                            pixelSize: 18
                                            letterSpacing: 0.5
                                        }
                                        color: primaryColor
                                        Layout.fillWidth: true
                                    }

                                    // Button group
                                    Row {
                                        spacing: 8
                                        Layout.alignment: Qt.AlignRight

                                        Button {
                                            text: showAllPercentages ? "Top 4" : "All"
                                            height: 38
                                            padding: 0
                                            leftPadding: 12
                                            rightPadding: 12
                                            font {
                                                pixelSize: 12
                                                family: "Segoe UI"
                                                weight: Font.Medium
                                            }
                                            background: Rectangle {
                                                radius: 14
                                                color: parent.hovered ? Qt.lighter(cardColor, 1.5) : "transparent"
                                                border.color: dividerColor
                                                border.width: 1
                                            }
                                            contentItem: Label {
                                                text: parent.text
                                                font: parent.font
                                                color: accentColor
                                                horizontalAlignment: Text.AlignHCenter
                                                verticalAlignment: Text.AlignVCenter
                                            }
                                            onClicked: showAllPercentages = !showAllPercentages
                                        }

                                        Button {
                                            id: dateRangeButton
                                            text: !isNaN(startSelectedDate.getTime()) ?
                                                      (!isNaN(endSelectedDate.getTime()) ?
                                                           Qt.formatDate(startSelectedDate, "MMM d") + "-" + Qt.formatDate(endSelectedDate, "MMM d") :
                                                           Qt.formatDate(startSelectedDate, "MMM d")) :
                                                      "Date Range"
                                            height: 38
                                            padding: 0
                                            leftPadding: 12
                                            rightPadding: 12
                                            font {
                                                pixelSize: 12
                                                family: "Segoe UI"
                                                weight: Font.Medium
                                            }
                                            background: Rectangle {
                                                radius: 14
                                                color: parent.hovered ? Qt.lighter(cardColor, 1.5) : "transparent"
                                                border.color: dividerColor
                                                border.width: 1
                                            }
                                            contentItem: Label {
                                                text: parent.text
                                                font: parent.font
                                                color: accentColor
                                                horizontalAlignment: Text.AlignHCenter
                                                verticalAlignment: Text.AlignVCenter
                                            }
                                            onClicked: dateRangeDialog.open()
                                        }
                                    }
                                }

                                // Divider
                                Rectangle {
                                    Layout.fillWidth: true
                                    height: 1
                                    radius: 1
                                    color: dividerColor
                                }
                            }

                            // Content
                            ScrollView {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                clip: true

                                ListView {
                                    id: percentageListView
                                    model: showAllPercentages ? sortedApps : sortedApps.slice(0, 4)
                                    spacing: 12
                                    width: parent.width

                                    delegate: Item {
                                        width: percentageListView.width
                                        height: 48

                                        property real targetPercentage: model.modelData.percentage
                                        property real currentPercentage: 0


                                        NumberAnimation on currentPercentage {
                                            id: percentageAnim
                                            from: 0
                                            to: targetPercentage
                                            duration: 1000
                                            easing.type: Easing.OutBack
                                            running: true
                                        }

                                        RowLayout {
                                            anchors.fill: parent
                                            spacing: 12



                                            // App icon placeholder
                                            Rectangle {
                                                Layout.preferredWidth: 24
                                                Layout.preferredHeight: 24
                                                radius: 4
                                                color: Qt.rgba(
                                                           Math.random() * 0.5 + 0.3,
                                                           Math.random() * 0.5 + 0.3,
                                                           Math.random() * 0.5 + 0.3,
                                                           0.2
                                                           )

                                                Label {
                                                    text: modelData.name.charAt(0).toUpperCase()
                                                    anchors.centerIn: parent
                                                    font {
                                                        family: "Segoe UI"
                                                        weight: Font.Bold
                                                        pixelSize: 12
                                                    }
                                                    color: primaryColor
                                                }
                                            }

                                            ColumnLayout {
                                                Layout.fillWidth: true
                                                Layout.fillHeight: true
                                                spacing: 4

                                                RowLayout {
                                                    Layout.fillWidth: true
                                                    spacing: 8

                                                    // App name
                                                    Label {
                                                        text: modelData.name
                                                        Layout.fillWidth: true
                                                        elide: Text.ElideRight
                                                        font {
                                                            family: "Segoe UI"
                                                            pixelSize: 14
                                                            weight: Font.Medium
                                                        }
                                                        color: textColor
                                                    }

                                                    // Percentage
                                                    Label {
                                                        text: currentPercentage.toFixed(1) + "%"
                                                        font {
                                                            family: "Segoe UI"
                                                            pixelSize: 14
                                                            weight: Font.DemiBold
                                                        }
                                                        color: primaryColor
                                                    }

                                                    // Duration
                                                    Label {
                                                        text: formatDuration(modelData.duration)
                                                        font {
                                                            family: "Segoe UI"
                                                            pixelSize: 14
                                                        }
                                                        color: lightTextColor
                                                    }
                                                }

                                                // Progress bar
                                                Rectangle {
                                                    Layout.fillWidth: true
                                                    Layout.preferredHeight: 6
                                                    radius: 3
                                                    color: Qt.rgba(dividerColor.r, dividerColor.g, dividerColor.b, 0.3)

                                                    Rectangle {
                                                        width: parent.width * (currentPercentage / 100)
                                                        height: parent.height
                                                        radius: 3

                                                        // Menggunakan gradient dari transparan (kiri) ke terang (kanan)
                                                        gradient: Gradient {
                                                            orientation: Gradient.Horizontal
                                                            GradientStop {
                                                                position: 0.0
                                                                color: {
                                                                    var baseColor;
                                                                    if (modelData.productivityType === "productive") baseColor = primaryColor;
                                                                    else if (modelData.productivityType === "non-productive") baseColor = nonProductiveColor;
                                                                    else baseColor = neutralColor;

                                                                    // Membuat warna transparan (alpha = 0)
                                                                    return Qt.rgba(baseColor.r, baseColor.g, baseColor.b, 0.0);
                                                                }
                                                            }
                                                            GradientStop {
                                                                position: 1.0
                                                                color: {
                                                                    // Warna terang penuh (alpha = 1)
                                                                    if (modelData.productivityType === "productive") return primaryColor;
                                                                    if (modelData.productivityType === "non-productive") return nonProductiveColor;
                                                                    if (modelData.productivityType === "neutral") return neutralColor;
                                                                    return neutralColor;
                                                                }
                                                            }
                                                        }

                                                        Behavior on width {
                                                            NumberAnimation {
                                                                duration: 1000
                                                                easing.type: Easing.OutBack
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

                        Rectangle {
                            Layout.fillHeight: true
                            width: 1
                            color: dividerColor
                        }

                        // --- KOLOM PENGGUNAAN DOMAIN ---
                        ColumnLayout {
                            Layout.preferredWidth: parent.width * 0.3
                            Layout.fillHeight: true
                            spacing: 12

                            // Header untuk Domain
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                Label {
                                    text: "Website Usage"
                                    font {
                                        family: "Segoe UI"
                                        weight: Font.DemiBold
                                        pixelSize: 18
                                        letterSpacing: 0.5
                                    }
                                    color: primaryColor
                                    Layout.fillWidth: true
                                }

                                Rectangle { // Divider di bawah header
                                    Layout.fillWidth: true
                                    height: 1
                                    radius: 1
                                    color: dividerColor
                                }
                            }

                            // Konten ListView untuk Domain
                            ScrollView {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                clip: true

                                ListView {
                                    id: domainsListView
                                    model: showAllPercentages ? sortedDomains : sortedDomains.slice(0, 4)
                                    spacing: 12
                                    width: parent.width

                                    // Delegate untuk domainsListView (gunakan kode yang sudah dibuat dari jawaban sebelumnya
                                    // yang sudah memiliki warna progress bar dinamis)
                                    delegate: Item {
                                        width: percentageListView.width
                                        height: 48

                                        property real targetPercentage: model.modelData.percentage
                                        property real currentPercentage: 0

                                        NumberAnimation on currentPercentage {
                                            id: percentageAnim_
                                            from: 0
                                            to: targetPercentage
                                            duration: 1000
                                            easing.type: Easing.OutBack
                                            running: true
                                        }

                                        RowLayout {
                                            anchors.fill: parent
                                            spacing: 12



                                            // App icon placeholder
                                            Rectangle {
                                                Layout.preferredWidth: 24
                                                Layout.preferredHeight: 24
                                                radius: 4
                                                color: Qt.rgba(
                                                           Math.random() * 0.5 + 0.3,
                                                           Math.random() * 0.5 + 0.3,
                                                           Math.random() * 0.5 + 0.3,
                                                           0.2
                                                           )

                                                Label {
                                                    text: modelData.name.charAt(0).toUpperCase()
                                                    anchors.centerIn: parent
                                                    font {
                                                        family: "Segoe UI"
                                                        weight: Font.Bold
                                                        pixelSize: 12
                                                    }
                                                    color: primaryColor
                                                }
                                            }

                                            ColumnLayout {
                                                Layout.fillWidth: true
                                                Layout.fillHeight: true
                                                spacing: 4

                                                RowLayout {
                                                    Layout.fillWidth: true
                                                    spacing: 8

                                                    // App name
                                                    Label {
                                                        text: modelData.name
                                                        Layout.fillWidth: true
                                                        elide: Text.ElideRight
                                                        font {
                                                            family: "Segoe UI"
                                                            pixelSize: 14
                                                            weight: Font.Medium
                                                        }
                                                        color: textColor
                                                    }

                                                    // Percentage
                                                    Label {
                                                        text: currentPercentage.toFixed(1) + "%"
                                                        font {
                                                            family: "Segoe UI"
                                                            pixelSize: 14
                                                            weight: Font.DemiBold
                                                        }
                                                        color: primaryColor
                                                    }

                                                    // Duration
                                                    Label {
                                                        text: formatDuration(modelData.duration)
                                                        font {
                                                            family: "Segoe UI"
                                                            pixelSize: 14
                                                        }
                                                        color: lightTextColor
                                                    }
                                                }

                                                // Progress bar
                                                Rectangle {
                                                    Layout.fillWidth: true
                                                    Layout.preferredHeight: 6
                                                    radius: 3
                                                    color: Qt.rgba(dividerColor.r, dividerColor.g, dividerColor.b, 0.3)

                                                    Rectangle {
                                                        width: parent.width * (currentPercentage / 100)
                                                        height: parent.height
                                                        radius: 3

                                                        // Menggunakan gradient dari transparan (kiri) ke terang (kanan)
                                                        gradient: Gradient {
                                                            orientation: Gradient.Horizontal
                                                            GradientStop {
                                                                position: 0.0
                                                                color: {
                                                                    var baseColor;
                                                                    if (modelData.productivityType === "productive") baseColor = primaryColor;
                                                                    else if (modelData.productivityType === "non-productive") baseColor = nonProductiveColor;
                                                                    else baseColor = neutralColor;

                                                                    // Membuat warna transparan (alpha = 0)
                                                                    return Qt.rgba(baseColor.r, baseColor.g, baseColor.b, 0.0);
                                                                }
                                                            }
                                                            GradientStop {
                                                                position: 1.0
                                                                color: {
                                                                    // Warna terang penuh (alpha = 1)
                                                                    if (modelData.productivityType === "productive") return primaryColor;
                                                                    if (modelData.productivityType === "non-productive") return nonProductiveColor;
                                                                    if (modelData.productivityType === "neutral") return neutralColor;
                                                                    return neutralColor;
                                                                }
                                                            }
                                                        }

                                                        Behavior on width {
                                                            NumberAnimation {
                                                                duration: 1000
                                                                easing.type: Easing.OutBack
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

                        // Vertical Divider
                        Rectangle {
                            Layout.fillHeight: true
                            width: 1
                            color: dividerColor
                        }

                        // Productivity Section (Right)
                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            spacing: 12

                            // Header
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 8

                                    Label {
                                        text: "Productivity"
                                        font {
                                            family: "Segoe UI"
                                            weight: Font.DemiBold
                                            pixelSize: 18
                                            letterSpacing: 0.5
                                        }
                                        color: primaryColor
                                    }

                                    Item { Layout.fillWidth: true }

                                    Button {
                                        id: app
                                        text: "Show Applications"
                                        font {
                                            pixelSize: 10
                                        }
                                        background: Rectangle {
                                            color: "transparent"
                                        }

                                        contentItem: Text {
                                            text: app.text
                                            font: app.font
                                            color: accentColor
                                        }
                                        onClicked: {
                                            var apps = logger.getProductivityApps();
                                            productiveAppsModel.clear();
                                            nonProductiveAppsModel.clear();
                                            for (var i = 0; i < apps.length; i++) {
                                                if (apps[i].type === 1) {
                                                    productiveAppsModel.append({
                                                                                   "appName": apps[i].appName,
                                                                                   "url": apps[i].url
                                                                               });
                                                } else if (apps[i].type === 2) {
                                                    nonProductiveAppsModel.append({
                                                                                      "appName": apps[i].appName,
                                                                                      "url": apps[i].url
                                                                                  });
                                                }
                                            }
                                            applicationsDialog.open();
                                        }
                                    }
                                }
                                // Divider
                                Rectangle {
                                    Layout.fillWidth: true
                                    height: 1
                                    radius: 1
                                    color: dividerColor
                                }
                            }





                            RowLayout {
                                spacing: 30
                                Layout.fillWidth: true
                                Layout.preferredHeight: 250
                                // Combined Productivity Circle
                                Item {
                                    Layout.alignment: Qt.AlignHCenter
                                    Layout.preferredWidth: 210
                                    Layout.preferredHeight: 210

                                    Rectangle {
                                        id: circleContainer
                                        anchors.fill: parent
                                        color: "transparent"

                                        Canvas {
                                            id: productivityCanvas
                                            anchors.fill: parent
                                            anchors.margins: 0

                                            property real productiveAngle: 0
                                            property real nonProductiveAngle: 0
                                            property real neutralAngle: 0
                                            property real animationProgress: 0
                                            property real glowIntensity: 0
                                            property real rotationOffset: 0

                                            onPaint: {
                                                var ctx = getContext("2d")
                                                ctx.clearRect(0, 0, width, height)

                                                var centerX = width / 2
                                                var centerY = height / 2
                                                var outerRadius = Math.min(width, height) / 2 - 20
                                                var ringWidth = 16
                                                var innerRadius = outerRadius - ringWidth
                                                var startAngle = -Math.PI / 2  // Fixed start position at top

                                                // Background ring dengan efek subtle
                                                ctx.beginPath()
                                                ctx.arc(centerX, centerY, outerRadius, 0, 2 * Math.PI)
                                                ctx.arc(centerX, centerY, innerRadius, 0, 2 * Math.PI, true)
                                                ctx.fillStyle = Qt.rgba(0.95, 0.95, 0.95, 0)
                                                ctx.fill()

                                                // Glow effect untuk segmen aktif


                                                // Fungsi untuk menggambar segmen donat yang selalu berbentuk ring
                                                function drawRingSegment(startAngle, angleSpan, color, gradient = false) {
                                                    if (angleSpan <= 0) return

                                                    var animatedAngleSpan = angleSpan * animationProgress

                                                    // PERBAIKAN: Pastikan angleSpan tidak pernah mencapai atau melebihi 2*PI
                                                    // Sisakan sedikit gap agar ring tetap berlubang
                                                    var maxAngleSpan = 2 * Math.PI - 0.01 // Sisakan gap kecil (sekitar 0.6 derajat)
                                                    if (animatedAngleSpan >= maxAngleSpan) {
                                                        animatedAngleSpan = maxAngleSpan
                                                    }

                                                    var endAngle = startAngle + animatedAngleSpan

                                                    ctx.beginPath()

                                                    // Gambar outer arc
                                                    ctx.arc(centerX, centerY, outerRadius, startAngle, endAngle, false)

                                                    // Connect to inner arc
                                                    ctx.lineTo(
                                                                centerX + innerRadius * Math.cos(endAngle),
                                                                centerY + innerRadius * Math.sin(endAngle)
                                                                )

                                                    // Gambar inner arc (reverse direction)
                                                    ctx.arc(centerX, centerY, innerRadius, endAngle, startAngle, true)

                                                    // Close path
                                                    ctx.closePath()

                                                    // Apply gradient if requested
                                                    if (gradient && animatedAngleSpan > 0) {
                                                        var gradientStartX = centerX + (outerRadius * 0.7) * Math.cos(startAngle)
                                                        var gradientStartY = centerY + (outerRadius * 0.7) * Math.sin(startAngle)
                                                        var gradientEndX = centerX + (outerRadius * 0.7) * Math.cos(endAngle)
                                                        var gradientEndY = centerY + (outerRadius * 0.7) * Math.sin(endAngle)

                                                        var gradient = ctx.createLinearGradient(gradientStartX, gradientStartY, gradientEndX, gradientEndY)
                                                        gradient.addColorStop(0, color)
                                                        gradient.addColorStop(1, Qt.lighter(color, 1.3))
                                                        ctx.fillStyle = gradient
                                                    } else {
                                                        ctx.fillStyle = color
                                                    }

                                                    ctx.fill()
                                                }

                                                // Draw segments secara berurutan dengan animasi yang tepat
                                                var segmentGap = 0.015 // Smaller gap between segments
                                                var currentStartAngle = startAngle

                                                // PERBAIKAN: Hitung total angle untuk memastikan tidak melebihi batas
                                                var totalAngle = productiveAngle + nonProductiveAngle + neutralAngle
                                                var availableAngle = 2 * Math.PI - 0.02 // Sisakan gap total

                                                // Scale down semua angle jika total melebihi batas
                                                var scaleFactor = 1
                                                if (totalAngle > availableAngle) {
                                                    scaleFactor = availableAngle / totalAngle
                                                }

                                                // Productive segment - always starts first
                                                if (productiveAngle > 0) {
                                                    var scaledProductiveAngle = productiveAngle * scaleFactor
                                                    drawRingSegment(currentStartAngle, scaledProductiveAngle, productiveColor, true)
                                                    currentStartAngle += scaledProductiveAngle + segmentGap
                                                }

                                                // Non-productive segment - starts after productive
                                                if (nonProductiveAngle > 0) {
                                                    var scaledNonProductiveAngle = nonProductiveAngle * scaleFactor
                                                    drawRingSegment(currentStartAngle, scaledNonProductiveAngle, nonProductiveColor)
                                                    currentStartAngle += scaledNonProductiveAngle + segmentGap
                                                }

                                                // Neutral segment - starts after non-productive
                                                if (neutralAngle > 0) {
                                                    var scaledNeutralAngle = neutralAngle * scaleFactor
                                                    drawRingSegment(currentStartAngle, scaledNeutralAngle, neutralColor)
                                                }

                                                // Clear shadow for text
                                                ctx.shadowColor = "transparent"
                                                ctx.shadowBlur = 0

                                                // Center content dengan animasi yang lebih smooth
                                                ctx.textAlign = "center"
                                                ctx.textBaseline = "middle"

                                                // Main percentage dengan scale animation
                                                var progressPercent = Math.round((productiveAngle / (2 * Math.PI) * 100) * animationProgress)
                                                var textScale = 0.8 + (0.2 * animationProgress) // Scale from 80% to 100%

                                                ctx.save()
                                                ctx.translate(centerX, centerY - 8)
                                                ctx.scale(textScale, textScale)

                                                ctx.fillStyle = primaryColor
                                                ctx.font = "bold 32px 'Segoe UI', system-ui, -apple-system"
                                                ctx.fillText(progressPercent + "%", 0, 0)
                                                ctx.restore()

                                                // Subtitle dengan fade-in effect
                                                ctx.font = "600 13px 'Segoe UI', system-ui, -apple-system"
                                                ctx.fillStyle = Qt.rgba(primaryColor.r, primaryColor.g, primaryColor.b, 0.8 * animationProgress)
                                                ctx.fillText("Productive", centerX, centerY + 18)

                                                // Decorative center dot
                                                if (animationProgress > 0.7) {
                                                    var dotOpacity = (animationProgress - 0.7) / 0.3
                                                    ctx.beginPath()
                                                    ctx.arc(centerX, centerY + 35, 2, 0, 2 * Math.PI)
                                                    ctx.fillStyle = Qt.rgba(primaryColor.r, primaryColor.g, primaryColor.b, 0.4 * dotOpacity)
                                                    ctx.fill()
                                                }
                                            }
                                        }
                                    }
                                }

                                // Updated animation connections and logic
                                Connections {
                                    target: logger
                                    function onProductivityStatsChanged() {
                                        // Calculate angles based on percentages
                                        var productive = logger.productivityStats.productive || 0
                                        var nonProductive = logger.productivityStats.nonProductive || 0
                                        var neutral = logger.productivityStats.neutral || 0

                                        // Normalize if total exceeds 100%
                                        var total = productive + nonProductive + neutral
                                        if (total > 100) {
                                            productive = (productive / total) * 100
                                            nonProductive = (nonProductive / total) * 100
                                            neutral = (neutral / total) * 100
                                        }

                                        // Stop any ongoing animation and reset angles
                                        chartAnimator.stop()
                                        productivityCanvas.productiveAngle = 0
                                        productivityCanvas.nonProductiveAngle = 0
                                        productivityCanvas.neutralAngle = 0
                                        productivityCanvas.animationProgress = 0
                                        productivityCanvas.glowIntensity = 0

                                        // Set new target values
                                        chartAnimator.productiveTarget = productive
                                        chartAnimator.nonProductiveTarget = nonProductive
                                        chartAnimator.neutralTarget = neutral

                                        // Start enhanced animation
                                        chartAnimator.start()
                                    }
                                }

                                Component.onCompleted: {
                                    // Initialize with clean slate
                                    productivePercent.value = 0
                                    nonProductivePercent.value = 0
                                    neutralPercent.value = 0

                                    productivityCanvas.productiveAngle = 0
                                    productivityCanvas.nonProductiveAngle = 0
                                    productivityCanvas.neutralAngle = 0
                                    productivityCanvas.animationProgress = 0
                                    productivityCanvas.glowIntensity = 0
                                    productivityCanvas.rotationOffset = 0
                                    productivityCanvas.requestPaint()
                                }

                                // Enhanced animation with multiple effects
                                ParallelAnimation {
                                    id: chartAnimator

                                    property real productiveTarget: 0
                                    property real nonProductiveTarget: 0
                                    property real neutralTarget: 0

                                    // Main progress animation
                                    NumberAnimation {
                                        target: productivityCanvas
                                        property: "animationProgress"
                                        from: 0
                                        to: 1
                                        duration: 2000
                                        easing.type: Easing.OutCubic
                                    }

                                    // Subtle glow pulse effect
                                    SequentialAnimation {
                                        PauseAnimation { duration: 500 }
                                        NumberAnimation {
                                            target: productivityCanvas
                                            property: "glowIntensity"
                                            from: 0
                                            to: 1
                                            duration: 800
                                            easing.type: Easing.InOutSine
                                        }
                                        NumberAnimation {
                                            target: productivityCanvas
                                            property: "glowIntensity"
                                            from: 1
                                            to: 0.3
                                            duration: 700
                                            easing.type: Easing.InOutSine
                                        }
                                    }

                                    // Micro rotation effect removed for precise positioning
                                    NumberAnimation {
                                        target: productivityCanvas
                                        property: "rotationOffset"
                                        from: 0
                                        to: 0
                                        duration: 1
                                    }

                                    // Staggered segment animations - sequential growth
                                    SequentialAnimation {
                                        PauseAnimation { duration: 300 }

                                        // Phase 1: Productive segment grows completely
                                        ParallelAnimation {
                                            NumberAnimation {
                                                target: productivityCanvas
                                                property: "productiveAngle"
                                                from: 0
                                                to: (chartAnimator.productiveTarget / 100) * 2 * Math.PI
                                                duration: 1000
                                                easing.type: Easing.OutBack
                                                easing.overshoot: 0.2
                                            }
                                            NumberAnimation {
                                                target: productivePercent
                                                property: "value"
                                                from: 0
                                                to: chartAnimator.productiveTarget
                                                duration: 1000
                                                easing.type: Easing.OutCubic
                                            }
                                        }

                                        PauseAnimation { duration: 150 }

                                        // Phase 2: Non-productive segment grows after productive is complete
                                        ParallelAnimation {
                                            NumberAnimation {
                                                target: productivityCanvas
                                                property: "nonProductiveAngle"
                                                from: 0
                                                to: (chartAnimator.nonProductiveTarget / 100) * 2 * Math.PI
                                                duration: 800
                                                easing.type: Easing.OutBack
                                                easing.overshoot: 0.15
                                            }
                                            NumberAnimation {
                                                target: nonProductivePercent
                                                property: "value"
                                                from: 0
                                                to: chartAnimator.nonProductiveTarget
                                                duration: 800
                                                easing.type: Easing.OutCubic
                                            }
                                        }

                                        PauseAnimation { duration: 150 }

                                        // Phase 3: Neutral segment grows after non-productive is complete
                                        ParallelAnimation {
                                            NumberAnimation {
                                                target: productivityCanvas
                                                property: "neutralAngle"
                                                from: 0
                                                to: (chartAnimator.neutralTarget / 100) * 2 * Math.PI
                                                duration: 700
                                                easing.type: Easing.OutBack
                                                easing.overshoot: 0.1
                                            }
                                            NumberAnimation {
                                                target: neutralPercent
                                                property: "value"
                                                from: 0
                                                to: chartAnimator.neutralTarget
                                                duration: 700
                                                easing.type: Easing.OutCubic
                                            }
                                        }
                                    }
                                }

                                // High-performance animation timer
                                Timer {
                                    id: animationTimer
                                    interval: 16 // 60fps
                                    repeat: true
                                    running: chartAnimator.running
                                    onTriggered: productivityCanvas.requestPaint()
                                }
                                // Vertical Legend (right side)
                                ColumnLayout {
                                    spacing: 12
                                    Layout.alignment: Qt.AlignVCenter
                                    Layout.fillHeight: true
                                    Layout.preferredWidth: 180

                                    // Legend Title
                                    Label {
                                        text: "Time Distribution"
                                        font {
                                            pixelSize: 14
                                            weight: Font.DemiBold
                                            capitalization: Font.AllUppercase
                                        }
                                        color: Qt.darker(textColor, 1.3)
                                        Layout.bottomMargin: 8
                                    }

                                    // Productive
                                    RowLayout {
                                        spacing: 10
                                        Rectangle {
                                            implicitWidth: 16
                                            implicitHeight: 16
                                            radius: 4
                                            color: productiveColor
                                            border {
                                                width: 1
                                                color: Qt.darker(productiveColor, 1.2)
                                            }
                                        }
                                        Label {
                                            text: "Productive"
                                            font {
                                                pixelSize: 13
                                                weight: Font.Medium
                                            }
                                            color: textColor
                                            Layout.fillWidth: true
                                        }
                                        Label {
                                            text: Math.round(productivePercent.value) + "%"
                                            font {
                                                pixelSize: 13
                                                weight: Font.DemiBold
                                            }
                                            color: productiveColor
                                        }
                                    }

                                    // Non-Productive
                                    RowLayout {
                                        spacing: 10
                                        Rectangle {
                                            implicitWidth: 16
                                            implicitHeight: 16
                                            radius: 4
                                            color: nonProductiveColor
                                            border {
                                                width: 1
                                                color: Qt.darker(nonProductiveColor, 1.2)
                                            }
                                        }
                                        Label {
                                            text: "Non-Productive"
                                            font {
                                                pixelSize: 13
                                                weight: Font.Medium
                                            }
                                            color: textColor
                                            Layout.fillWidth: true
                                        }
                                        Label {
                                            text: Math.round(nonProductivePercent.value) + "%"
                                            font {
                                                pixelSize: 13
                                                weight: Font.DemiBold
                                            }
                                            color: nonProductiveColor
                                        }
                                    }

                                    // Neutral
                                    RowLayout {
                                        spacing: 10
                                        Rectangle {
                                            implicitWidth: 16
                                            implicitHeight: 16
                                            radius: 4
                                            color: neutralColor
                                            border {
                                                width: 1
                                                color: Qt.darker(neutralColor, 1.2)
                                            }
                                        }
                                        Label {
                                            text: "Neutral"
                                            font {
                                                pixelSize: 13
                                                weight: Font.Medium
                                            }
                                            color: textColor
                                            Layout.fillWidth: true
                                        }
                                        Label {
                                            text: Math.round(neutralPercent.value) + "%"
                                            font {
                                                pixelSize: 13
                                                weight: Font.DemiBold
                                            }
                                            color: neutralColor
                                        }
                                    }

                                    // Optional: Add subtle divider
                                    Rectangle {
                                        Layout.topMargin: 8
                                        Layout.fillWidth: true
                                        implicitHeight: 1
                                        color: dividerColor
                                    }

                                    // Timer Display
                                    ColumnLayout {
                                        spacing: 8
                                        Layout.fillWidth: true

                                        RowLayout {
                                            Layout.fillWidth: true
                                            spacing: 20

                                            Label {
                                                text: "Time at Work"
                                                font.pixelSize: 14
                                                font.weight: Font.Medium
                                                color: primaryColor
                                            }

                                            Item { Layout.fillWidth: true }

                                            Label {
                                                text: Math.round(workTimer.getProgress() * 100) + "%"
                                                font.pixelSize: 14
                                                font.weight: Font.Bold
                                                color: workTimer.elapsedSeconds >= 28800 ? "#27ae60" : "#e74c3c"
                                            }
                                        }

                                        RowLayout {
                                            Layout.fillWidth: true
                                            spacing : 10

                                            Label {
                                                text: workTimer.getFormattedElapsed()
                                                font.pixelSize: 10
                                                font.weight: Font.Medium
                                                color: workTimer.elapsedSeconds >= 28800 ? "#27ae60" : lightTextColor
                                            }

                                            Rectangle {
                                                Layout.fillWidth: true
                                                height: 6
                                                radius: 3
                                                color: Qt.rgba(0, 0, 0, 0.1)

                                                Rectangle {
                                                    width: parent.width * workTimer.getProgress()
                                                    height: parent.height
                                                    radius: 3
                                                    color: primaryColor
                                                    Behavior on width { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
                                                }
                                            }

                                            Label {
                                                text: "8h"
                                                font.pixelSize: 10
                                                font.weight: Font.Medium
                                                color: lightTextColor
                                            }
                                        }
                                    }


                                    // Work Timer Object (Sekarang hanya sebagai penyedia data, logika ada di C++)
                                    QtObject {
                                        id: workTimer

                                        property int elapsedSeconds: logger.workTimeElapsedSeconds
                                        property int totalWorkSeconds: 28800 // 8 jam

                                        function getFormattedElapsed() {
                                            var hours = Math.floor(elapsedSeconds / 3600)
                                            var minutes = Math.floor((elapsedSeconds % 3600) / 60)
                                            var seconds = elapsedSeconds % 60

                                            return String(hours).padStart(2, '0') + ":" +
                                                    String(minutes).padStart(2, '0') + ":" +
                                                    String(seconds).padStart(2, '0')
                                        }

                                        function getProgress() {
                                            return Math.min(1.0, elapsedSeconds / totalWorkSeconds)
                                        }
                                    }

                                    Connections {
                                        target: logger
                                        function onWorkTimeElapsedSecondsChanged() {
                                            workTimer.elapsedSeconds = logger.workTimeElapsedSeconds
                                        }
                                    }


                                }
                            }






                            // Keep these for the legend display
                            Label {
                                id: productivePercent
                                visible: false
                                property real value: 0

                                NumberAnimation on value {
                                    id: productivePercentAnim
                                    duration: 1000
                                    easing.type: Easing.OutCubic
                                }
                            }

                            Label {
                                id: nonProductivePercent
                                visible: false
                                property real value: 0

                                NumberAnimation on value {
                                    id: nonProductivePercentAnim
                                    duration: 1000
                                    easing.type: Easing.OutCubic
                                }
                            }

                            Label {
                                id: neutralPercent
                                visible: false
                                property real value: 0

                                NumberAnimation on value {
                                    id: neutralPercentAnim
                                    duration: 1000
                                    easing.type: Easing.OutCubic
                                }
                            }
                        }
                    }
                }



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

                Dialog {
                    id: applicationsDialog
                    title: "<b>Monitored Applications</b>"
                    modal: true

                    // Search function
                    function filterApps() {
                        var searchText = search_Field.text.toLowerCase().trim()

                        filteredProductiveAppsModel.clear()
                        filteredNonProductiveAppsModel.clear()

                        // Filter productive apps
                        for (var i = 0; i < productiveAppsModel.count; i++) {
                            var item = productiveAppsModel.get(i)
                            var appName = item.appName ? item.appName.toLowerCase() : ""
                            var url = item.url ? item.url.toLowerCase() : ""

                            if (searchText === "" ||
                                    appName.indexOf(searchText) !== -1 ||
                                    url.indexOf(searchText) !== -1) {
                                filteredProductiveAppsModel.append(item)
                            }
                        }

                        // Filter non-productive apps
                        for (var j = 0; j < nonProductiveAppsModel.count; j++) {
                            var item2 = nonProductiveAppsModel.get(j)
                            var appName2 = item2.appName ? item2.appName.toLowerCase() : ""
                            var url2 = item2.url ? item2.url.toLowerCase() : ""

                            if (searchText === "" ||
                                    appName2.indexOf(searchText) !== -1 ||
                                    url2.indexOf(searchText) !== -1) {
                                filteredNonProductiveAppsModel.append(item2)
                            }
                        }
                    }

                    function extractDomain(url) {
                        if (!url) return ""
                        // Remove protocol and path
                        var domain = url.replace(/^https?:\/\//, '').split('/')[0]
                        // Remove www. if present
                        return domain.replace(/^www\./, '')
                    }
                    // Initialize filtered models when dialog opens
                    onOpened: {
                        filterApps()
                    }

                    // Re-filter when original models change
                    Connections {
                        target: productiveAppsModel
                        function onCountChanged() {
                            applicationsDialog.filterApps()
                        }
                    }

                    Connections {
                        target: nonProductiveAppsModel
                        function onCountChanged() {
                            applicationsDialog.filterApps()
                        }
                    }

                    footer: DialogButtonBox {
                        alignment: Qt.AlignRight
                        background: Rectangle {
                            color: cardColor
                            radius: 8
                        }
                        Button {
                            text: "Tambah Aplikasi"
                            flat: true
                            onClicked: {
                                applicationsDialog.close()
                                addAppDialog.open()
                            }
                            background: Rectangle {
                                radius: 14
                                color: parent.hovered ? Qt.lighter(cardColor, 1.5) : "transparent"
                                border.color: dividerColor
                                border.width: 1
                            }
                            contentItem: Text {
                                text: parent.text
                                color: textColor
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        Button {
                            text: "OK"
                            flat: true
                            onClicked: applicationsDialog.accept()
                            background: Rectangle {
                                radius: 14
                                color: parent.hovered ? Qt.lighter(cardColor, 1.5) : "transparent"
                                border.color: dividerColor
                                border.width: 1
                            }
                            contentItem: Text {
                                text: parent.text
                                color: textColor
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }

                    width: 900
                    height: 700
                    anchors.centerIn: parent
                    background: Rectangle {
                        color: cardColor
                        radius: 8
                    }

                    Column {
                        spacing: 15
                        anchors.fill: parent
                        anchors.margins: 20

                        // Search and Filter Row
                        Row {
                            width: parent.width
                            spacing: 15
                            height: 50

                            // Search Field
                            TextField {
                                id: search_Field
                                width: parent.width * 0.6
                                height: 40
                                placeholderText: "Search applications..."
                                leftPadding: 40

                                // Trigger search on text change
                                onTextChanged: {
                                    searchTimer.restart()
                                }

                                // Add timer to prevent excessive filtering while typing
                                Timer {
                                    id: searchTimer
                                    interval: 500 // 500ms delay
                                    onTriggered: applicationsDialog.filterApps()
                                }

                                background: Rectangle {
                                    color: dividerColor
                                    radius: 8
                                    border.color: search_Field.activeFocus ? "#1976d2" : "#e0e0e0"
                                    border.width: 1

                                    Image {
                                        source: "qrc:/icons/search.svg"
                                        width: 20
                                        height: 20
                                        anchors.left: parent.left
                                        anchors.leftMargin: 12
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                }
                            }

                            // Clear search button
                            Button {
                                width: 40
                                height: 40
                                visible: search_Field.text.length > 0
                                flat: true
                                onClicked: {
                                    search_Field.text = ""
                                    search_Field.focus = true
                                }

                                background: Rectangle {
                                    radius: 8
                                    color: parent.hovered ? Qt.lighter(cardColor, 1.2) : "transparent"
                                    border.color: dividerColor
                                    border.width: 1
                                }

                                contentItem: Text {
                                    text: "✕"
                                    color: lightTextColor
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                    font.pixelSize: 16
                                }
                            }
                            Rectangle {
                                Layout.fillWidth: true
                                height: 0
                                color: dividerColor
                                Layout.topMargin: 4
                            }

                            // Request Button
                            Button {
                                id: requestButton
                                text: "Request"
                                height: 40
                                leftPadding: 16
                                rightPadding: 16
                                font.pixelSize: 14
                                onClicked: requestDialog.open()

                                background: Rectangle {
                                    radius: 8
                                    color: parent.hovered ? Qt.lighter(cardColor, 1.5) : "transparent"
                                }

                                contentItem: Text {
                                    text: parent.text
                                    font: parent.font
                                    color: textColor
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                            }
                        }

                        // Main Content Row
                        Row {
                            width: parent.width
                            height: parent.height - 70
                            spacing: 15

                            // Productive Apps Column
                            Column {
                                width: parent.width * 0.48
                                height: parent.height
                                spacing: 8

                                Rectangle {
                                    width: parent.width
                                    height: 40
                                    color: "transparent"
                                    radius: 6

                                    Text {
                                        text: "Productive Apps"
                                        font.bold: true
                                        font.pixelSize: 16
                                        color: "#1976d2"
                                        anchors.verticalCenter: parent.verticalCenter
                                        anchors.left: parent.left
                                        anchors.leftMargin: 15
                                    }
                                }

                                ListView {
                                    id: productiveListView
                                    width: parent.width
                                    height: parent.height - 50
                                    clip: true
                                    spacing: 6
                                    model: filteredProductiveAppsModel // Use filtered model
                                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                                    delegate: Rectangle {
                                        width: parent.width
                                        height: 50
                                        gradient: Gradient {
                                            orientation: Gradient.Horizontal
                                            GradientStop { position: 0.1; color: hover ? "#1976d2" : cardColor }
                                            GradientStop { position: 1.0; color: cardColor }
                                        }

                                        property bool hover: false

                                        MouseArea {
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            onEntered: parent.hover = true
                                            onExited: parent.hover = false
                                        }

                                        Row {
                                            spacing: 12
                                            anchors.fill: parent
                                            anchors.verticalCenter: parent.verticalCenter

                                            Rectangle {
                                                width: 2
                                                height: parent.height
                                                color: "#1976d2"
                                            }

                                            Rectangle {
                                                width: 32
                                                height: 32
                                                radius: 6
                                                color: "#e3f2fd"
                                                anchors.verticalCenter: parent.verticalCenter

                                                Text {
                                                    text: model.appName.charAt(0).toUpperCase()
                                                    font.bold: true
                                                    font.pixelSize: 14
                                                    color: "#1976d2"
                                                    anchors.centerIn: parent
                                                }
                                            }

                                            Column {
                                                width: parent.width - 50
                                                spacing: 2
                                                anchors.verticalCenter: parent.verticalCenter

                                                Text {
                                                    text: model.appName
                                                    font.bold: true
                                                    font.pixelSize: 12
                                                    color: hover ? "white" : textColor
                                                    width: parent.width
                                                    elide: Text.ElideRight
                                                }

                                                Text {
                                                    text: extractDomain(model.url) || "Aplikasi"
                                                    font.pixelSize: 10
                                                    color: hover ? "white" : "#757575"
                                                    width: parent.width
                                                    elide: Text.ElideRight
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            // Vertical Separator
                            Rectangle {
                                width: 1
                                height: parent.height
                                color: "#e0e0e0"
                            }

                            // Non-Productive Apps Column
                            Column {
                                width: parent.width * 0.48
                                height: parent.height
                                spacing: 8

                                Rectangle {
                                    width: parent.width
                                    height: 40
                                    color: "transparent"
                                    radius: 6

                                    Text {
                                        text: "Non-Productive Apps"
                                        font.bold: true
                                        font.pixelSize: 15
                                        color: "#d32f2f"
                                        anchors.verticalCenter: parent.verticalCenter
                                        anchors.left: parent.left
                                        anchors.leftMargin: 15
                                    }
                                }

                                ListView {
                                    id: nonProductiveListView
                                    width: parent.width
                                    height: parent.height - 50
                                    clip: true
                                    spacing: 6
                                    model: filteredNonProductiveAppsModel // Use filtered model
                                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                                    delegate: Rectangle {
                                        width: parent.width
                                        height: 50
                                        gradient: Gradient {
                                            orientation: Gradient.Horizontal
                                            GradientStop { position: 0.1; color: hover ? "#d32f2f" : cardColor }
                                            GradientStop { position: 1.0; color: cardColor }
                                        }

                                        property bool hover: false

                                        MouseArea {
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            onEntered: parent.hover = true
                                            onExited: parent.hover = false
                                        }

                                        Row {
                                            spacing: 12
                                            anchors.fill: parent
                                            anchors.verticalCenter: parent.verticalCenter

                                            Rectangle {
                                                width: 2
                                                height: parent.height
                                                color: "#d32f2f"
                                            }

                                            Rectangle {
                                                width: 32
                                                height: 32
                                                radius: 6
                                                color: "#ffebee"
                                                anchors.verticalCenter: parent.verticalCenter

                                                Text {
                                                    text: model.appName.charAt(0).toUpperCase()
                                                    font.bold: true
                                                    font.pixelSize: 14
                                                    color: "#d32f2f"
                                                    anchors.centerIn: parent
                                                }
                                            }

                                            Column {
                                                width: parent.width - 50
                                                spacing: 2
                                                anchors.verticalCenter: parent.verticalCenter

                                                Text {
                                                    text: model.appName
                                                    font.bold: true
                                                    font.pixelSize: 12
                                                    color: hover ? "white" : textColor
                                                    width: parent.width
                                                    elide: Text.ElideRight
                                                }

                                                Text {
                                                    text: model.url || "Aplikasi"
                                                    font.pixelSize: 10
                                                    color: hover ? "white" : "#757575"
                                                    width: parent.width
                                                    elide: Text.ElideRight
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                Dialog {
                    id: requestDialog
                    title: "Application Requests"
                    modal: true
                    width: Math.min(parent.width * 0.8, 800)
                    height: Math.min(parent.height * 0.8, 600)
                    x: (parent.width - width) / 2
                    y: (parent.height - height) / 2
                    padding: 16
                    dim: true

                    property var pendingRequests: logger.getPendingApplicationRequests()

                    background: Rectangle {
                        color: cardColor
                        radius: 12
                        border.color: dividerColor
                        border.width: 1

                        Rectangle {
                            anchors.fill: parent
                            radius: parent.radius
                            gradient: Gradient {
                                GradientStop { position: 0.0; color: Qt.rgba(0,0,0,0.05) }
                                GradientStop { position: 1.0; color: "transparent" }
                            }
                        }
                    }

                    contentItem: ColumnLayout {
                        spacing: 16

                        Label {
                            text: "Pending Application Requests"
                            font {
                                pixelSize: 18
                                bold: true
                                family: "Segoe UI"
                            }
                            color: primaryColor
                            Layout.alignment: Qt.AlignHCenter
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: dividerColor
                        }

                        ScrollView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true

                            ListView {
                                id: requestListView
                                model: requestDialog.pendingRequests
                                spacing: 8
                                boundsBehavior: Flickable.StopAtBounds

                                delegate: Rectangle {
                                    width: requestListView.width
                                    height: 100  // Increased height to accommodate more info
                                    radius: 8
                                    color: index % 2 === 0 ? Qt.lighter(cardColor, 1.1) : cardColor
                                    border.color: dividerColor
                                    border.width: 1

                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.margins: 12
                                        spacing: 16

                                        // App icon placeholder
                                        Rectangle {
                                            Layout.preferredWidth: 48
                                            Layout.preferredHeight: 48
                                            radius: 8
                                            color: Qt.rgba(
                                                       Math.random() * 0.5 + 0.3,
                                                       Math.random() * 0.5 + 0.3,
                                                       Math.random() * 0.5 + 0.3,
                                                       0.2
                                                       )

                                            Label {
                                                text: modelData.app_name.charAt(0).toUpperCase()
                                                anchors.centerIn: parent
                                                font {
                                                    family: "Segoe UI"
                                                    weight: Font.Bold
                                                    pixelSize: 18
                                                }
                                                color: primaryColor
                                            }
                                        }

                                        ColumnLayout {
                                            Layout.fillWidth: true
                                            spacing: 4

                                            // Application name row
                                            RowLayout {
                                                Layout.fillWidth: true
                                                spacing: 8

                                                Label {
                                                    text: modelData.app_name
                                                    font {
                                                        family: "Segoe UI"
                                                        pixelSize: 16
                                                        weight: Font.Medium
                                                    }
                                                    color: textColor
                                                    elide: Text.ElideRight
                                                }

                                                // Productivity type badge
                                                Rectangle {
                                                    visible: modelData.productivity_text
                                                    radius: 4
                                                    color: {
                                                        if (modelData.productivity === 1) return "#4CAF50"; // Green for productive
                                                        if (modelData.productivity === 2) return "#F44336"; // Red for non-productive
                                                        return "#9E9E9E"; // Gray for neutral
                                                    }
                                                    Layout.preferredHeight: 20
                                                    Layout.preferredWidth: productivityText.width + 12
                                                    opacity: 0.8

                                                    Label {
                                                        id: productivityText
                                                        text: modelData.productivity_text || ""
                                                        anchors.centerIn: parent
                                                        font {
                                                            family: "Segoe UI"
                                                            pixelSize: 10
                                                            weight: Font.DemiBold
                                                        }
                                                        color: "white"
                                                    }
                                                }
                                            }

                                            // URL display
                                            Label {
                                                text: "URL: " + (modelData.url || "Not specified")
                                                font {
                                                    family: "Segoe UI"
                                                    pixelSize: 12
                                                }
                                                color: lightTextColor
                                                elide: Text.ElideMiddle
                                                Layout.fillWidth: true
                                            }

                                            // For users display
                                            Label {
                                                text: "For: " + modelData.for_users
                                                font {
                                                    family: "Segoe UI"
                                                    pixelSize: 11
                                                }
                                                color: Qt.lighter(lightTextColor, 1.2)
                                                elide: Text.ElideRight
                                                Layout.fillWidth: true
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        Button {
                            text: "Close"
                            Layout.alignment: Qt.AlignRight
                            Layout.preferredWidth: 120
                            Layout.preferredHeight: 40
                            onClicked: requestDialog.close()

                            background: Rectangle {
                                radius: 8
                                color: parent.hovered ? Qt.lighter(secondaryColor, 1.1) : secondaryColor
                            }

                            contentItem: Text {
                                text: parent.text
                                font.pixelSize: 14
                                color: "white"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }

                    onOpened: {
                        pendingRequests = logger.getPendingApplicationRequests()
                    }
                }




                Dialog {
                    id: addAppDialog
                    title: "Tambah Aplikasi Produktivitas"
                    property int selectedProductivityType: 1
                    modal: true
                    anchors.centerIn: parent
                    width: Math.min(parent.width * 0.8, 1000)
                    height: Math.min(parent.height * 0.8, 600)
                    padding: 0
                    background: Rectangle {
                        color: cardColor
                        radius: 8
                    }

                    footer: DialogButtonBox {
                        alignment: Qt.AlignRight
                        padding: 10
                        background: Rectangle {
                            color: cardColor
                            radius: 8
                        }
                        Button {
                            text: "Batal"
                            flat: true
                            onClicked: {
                                addAppDialog.close()
                                applicationsDialog.open()
                            }
                            background: Rectangle {
                                radius: 14
                                color: parent.hovered ? Qt.lighter(cardColor, 1.5) : "transparent"
                                border.color: dividerColor
                                border.width: 1
                            }
                            contentItem: Text {
                                text: parent.text
                                color: textColor
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        Button {
                            id: addButton
                            text: "Tambahkan"
                            Layout.alignment: Qt.AlignRight
                            font.pixelSize: 14
                            padding: 8
                            enabled: {
                                if (appList.currentItem && appList.model[appList.currentIndex] === "Other") {
                                    return txtAppName.text.trim().length > 0
                                }
                                if (txtWebsite.visible) {
                                    return txtWebsite.text.trim().length > 0
                                }
                                return appList.currentItem !== null
                            }
                            background: Rectangle {
                                color: addButton.enabled ? "#0078d4" : "#cccccc"
                                radius: 4
                                Rectangle {
                                    anchors.fill: parent
                                    color: "#005ea2"
                                    radius: 4
                                    visible: parent.parent.hovered && addButton.enabled
                                }
                            }
                            contentItem: Text {
                                text: parent.text
                                color: "#ffffff"
                                font: parent.font
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            onClicked: {
                                const prodType = addAppDialog.selectedProductivityType
                                const appName = appList.model[appList.currentIndex] === "Other" ?
                                                  txtAppName.text.trim() : appList.model[appList.currentIndex]
                                const url = txtWebsite.visible ? txtWebsite.text.trim() : ""

                                // Panggil fungsi dengan parameter URL baru
                                logger.addProductivityApp(appName, "", url, prodType)
                                addAppDialog.close()
                                notification.show()
                            }
                        }
                    }

                    RowLayout {
                        anchors.fill: parent
                        spacing: 15
                        anchors.margins: 15

                        // Left: App Selection List
                        Rectangle {
                            color: cardColor
                            clip: true
                            Layout.preferredWidth: parent.width * 0.45
                            Layout.fillHeight: true
                            radius: 4
                            border.color: dividerColor
                            border.width: 1

                            ColumnLayout {
                                anchors.fill: parent
                                spacing: 0

                                Label {
                                    text: "Daftar Aplikasi:"
                                    font.pixelSize: 14
                                    font.bold: true
                                    color: textColor
                                    padding: 10
                                    Layout.fillWidth: true
                                    background: Rectangle {
                                        color: Qt.lighter(cardColor, 1.1)
                                    }
                                }

                                TextField {
                                    id: searchField
                                    Layout.fillWidth: true
                                    Layout.margins: 10
                                    placeholderText: "Cari aplikasi..."
                                    font.pixelSize: 14
                                    background: Rectangle {
                                        radius: 4
                                        border.color: dividerColor
                                        border.width: 1
                                        color: cardColor
                                        implicitHeight: 36
                                    }
                                    onTextChanged: {
                                        appList.currentIndex = 0 // Reset indeks saat pencarian berubah
                                    }
                                }

                                ListView {
                                    id: appList
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    clip: true
                                    spacing: 4
                                    model: {
                                        let apps = logger.getAvailableApps()
                                        return apps.filter(app => app.toLowerCase().includes(searchField.text.toLowerCase()))
                                    }
                                    currentIndex: 0
                                    ScrollBar.vertical: ScrollBar {
                                        policy: ScrollBar.AsNeeded
                                        width: 8
                                    }

                                    delegate: Rectangle {
                                        width: parent.width
                                        height: 50
                                        color: cardColor

                                        property bool hover: false
                                        property bool isSelected: ListView.isCurrentItem

                                        Rectangle {
                                            width: 2
                                            height: parent.height
                                            color: hover ? Qt.lighter(primaryColor, 1.5) : "transparent"
                                            visible: hover && !isSelected
                                        }

                                        Rectangle {
                                            width: 2
                                            height: parent.height
                                            visible: isSelected
                                            gradient: Gradient {
                                                orientation: Gradient.Vertical
                                                GradientStop { position: 0.0; color: primaryColor }
                                                GradientStop { position: 1.0; color: Qt.darker(primaryColor, 1.5) }
                                            }
                                        }

                                        MouseArea {
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            onEntered: parent.hover = true
                                            onExited: parent.hover = false
                                            onClicked: {
                                                appList.currentIndex = index
                                            }
                                        }

                                        Row {
                                            spacing: 12
                                            anchors.fill: parent
                                            anchors.verticalCenter: parent.verticalCenter
                                            leftPadding: 10

                                            Rectangle {
                                                width: 32
                                                height: 32
                                                radius: 6
                                                color: Qt.lighter(primaryColor, 1.8)
                                                anchors.verticalCenter: parent.verticalCenter

                                                Text {
                                                    text: modelData.charAt(0).toUpperCase()
                                                    font.bold: true
                                                    font.pixelSize: 14
                                                    color: primaryColor
                                                    anchors.centerIn: parent
                                                }
                                            }

                                            Column {
                                                width: parent.width - 60
                                                spacing: 2
                                                anchors.verticalCenter: parent.verticalCenter

                                                Text {
                                                    text: modelData
                                                    font.bold: true
                                                    font.pixelSize: 12
                                                    color: hover || isSelected ? (hover ? primaryColor : textColor) : textColor
                                                    width: parent.width
                                                    elide: Text.ElideRight
                                                }

                                                Text {
                                                    text: {
                                                        if (modelData === "Other") return "Aplikasi Lain"
                                                        if (["Chrome", "Firefox", "Edge", "Safari", "Opera"].includes(modelData))
                                                            return "Browser Web"
                                                        return "Aplikasi"
                                                    }
                                                    font.pixelSize: 10
                                                    color: hover || isSelected ? (hover ? primaryColor : "#757575") : "#757575"
                                                    width: parent.width
                                                    elide: Text.ElideRight
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        // Divider
                        Rectangle {
                            width: 1
                            Layout.fillHeight: true
                            color: dividerColor
                        }

                        // Right: Inputs and Controls
                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            spacing: 15
                            Layout.leftMargin: 10
                            Layout.rightMargin: 5

                            // Nama Aplikasi Custom
                            Label {
                                text: "Nama Aplikasi:"
                                font.pixelSize: 14
                                color: textColor
                                Layout.fillWidth: true
                                visible: txtAppName.visible
                            }

                            TextField {
                                id: txtAppName
                                Layout.fillWidth: true
                                placeholderText: "Masukkan nama aplikasi, pastikan nama aplikasi nya benar"
                                font.pixelSize: 14
                                visible: false
                                background: Rectangle {
                                    radius: 4
                                    border.color: dividerColor
                                    border.width: 1
                                    color: cardColor
                                    implicitHeight: 36
                                }
                            }



                            // URL/Domain
                            Label {
                                text: "Domain Website:"
                                font.pixelSize: 14
                                color: textColor
                                Layout.fillWidth: true
                                visible: txtWebsite.visible
                            }

                            TextField {
                                id: txtWebsite
                                Layout.fillWidth: true
                                placeholderText: "Masukkan Domain Website (contoh: youtube.com, google.com, dll)"
                                font.pixelSize: 14
                                visible: false
                                background: Rectangle {
                                    radius: 4
                                    border.color: dividerColor
                                    border.width: 1
                                    color: cardColor
                                    implicitHeight: 36
                                }
                                validator: RegularExpressionValidator {
                                    // Validator sederhana untuk URL/domain
                                    regularExpression: /^(https?:\/\/)?([\da-z\.-]+)\.([a-z\.]{2,6})([\/\w \.-]*)*\/?$/
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                color: dividerColor
                                Layout.topMargin: 4
                            }

                            // Tipe Produktivitas
                            ColumnLayout {
                                spacing: 16
                                Layout.fillWidth: true

                                // Assuming these theme properties exist in parent context
                                property color textColor: isDarkMode ? "#f9fafb" : "#1f2937"
                                property color backgroundColor: isDarkMode ? "#374151" : "#f8fafc"
                                property color borderColor: isDarkMode ? "#4b5563" : "#e5e7eb"
                                property color accentColor: "#3b82f6"
                                property bool isDarkMode: false

                                // Header with improved typography
                                Label {
                                    id: headerLabel
                                    text: "Tipe Produktivitas"
                                    font.family: "Segoe UI, Arial, sans-serif"
                                    font.pixelSize: 16
                                    font.weight: Font.SemiBold
                                    color: textColor
                                    leftPadding: 4

                                    Behavior on color {
                                        ColorAnimation { duration: 300 }
                                    }
                                }

                                // Radio button group with modern styling
                                ColumnLayout {
                                    spacing: 12
                                    Layout.leftMargin: 8
                                    Layout.fillWidth: true

                                    // Produktif option
                                    RadioButton {
                                        id: produktifRadio
                                        text: "Produktif"
                                        checked: true
                                        font.family: "Segoe UI, Arial, sans-serif"
                                        font.pixelSize: 14
                                        Layout.fillWidth: true

                                        onCheckedChanged: if (checked) addAppDialog.selectedProductivityType = 1

                                        contentItem: Text {
                                            text: produktifRadio.text
                                            font: produktifRadio.font
                                            color: textColor
                                            verticalAlignment: Text.AlignVCenter
                                            leftPadding: produktifRadio.indicator.width + produktifRadio.spacing

                                            Behavior on color {
                                                ColorAnimation { duration: 300 }
                                            }
                                        }

                                        indicator: Rectangle {
                                            implicitWidth: 22
                                            implicitHeight: 22
                                            x: 0
                                            y: (produktifRadio.height - height) / 2
                                            radius: 11
                                            color: "transparent"
                                            border.color: produktifRadio.checked ? accentColor : borderColor
                                            border.width: produktifRadio.checked ? 2 : 1

                                            Behavior on border.color {
                                                ColorAnimation { duration: 200 }
                                            }

                                            Behavior on border.width {
                                                NumberAnimation { duration: 200 }
                                            }

                                            Rectangle {
                                                anchors.centerIn: parent
                                                width: 10
                                                height: 10
                                                radius: 5
                                                color: accentColor
                                                visible: produktifRadio.checked

                                                // Scale animation for dot
                                                scale: produktifRadio.checked ? 1.0 : 0.0
                                                Behavior on scale {
                                                    NumberAnimation {
                                                        duration: 200
                                                        easing.type: Easing.OutBack
                                                        easing.overshoot: 0.3
                                                    }
                                                }
                                            }

                                            // Ripple effect on click
                                            Rectangle {
                                                id: produktifRipple
                                                anchors.centerIn: parent
                                                width: 0
                                                height: 0
                                                radius: width / 2
                                                color: Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 0.3)
                                                visible: false

                                                PropertyAnimation {
                                                    id: produktifRippleAnim
                                                    target: produktifRipple
                                                    properties: "width,height"
                                                    to: 40
                                                    duration: 300
                                                    onStarted: produktifRipple.visible = true
                                                    onFinished: {
                                                        produktifRipple.width = 0
                                                        produktifRipple.height = 0
                                                        produktifRipple.visible = false
                                                    }
                                                }
                                            }
                                        }

                                        // Add icon for visual enhancement
                                        Rectangle {
                                            width: 16
                                            height: 16
                                            radius: 3
                                            color: "#10b981"
                                            anchors.right: parent.right
                                            anchors.verticalCenter: parent.indicator.verticalCenter
                                            anchors.rightMargin: 8
                                            visible: produktifRadio.checked

                                            Text {
                                                anchors.centerIn: parent
                                                text: "✓"
                                                color: "white"
                                                font.pixelSize: 10
                                                font.bold: true
                                            }

                                            scale: produktifRadio.checked ? 1.0 : 0.0
                                            Behavior on scale {
                                                NumberAnimation {
                                                    duration: 300
                                                    easing.type: Easing.OutBack
                                                }
                                            }
                                        }

                                        MouseArea {
                                            anchors.fill: parent
                                            onPressed: produktifRippleAnim.start()
                                            onClicked: produktifRadio.checked = true
                                        }
                                    }

                                    // Non-Produktif option
                                    RadioButton {
                                        id: nonProduktifRadio
                                        text: "Non-Produktif"
                                        font.family: "Segoe UI, Arial, sans-serif"
                                        font.pixelSize: 14
                                        Layout.fillWidth: true

                                        onCheckedChanged: if (checked) addAppDialog.selectedProductivityType = 2

                                        contentItem: Text {
                                            text: nonProduktifRadio.text
                                            font: nonProduktifRadio.font
                                            color: textColor
                                            verticalAlignment: Text.AlignVCenter
                                            leftPadding: nonProduktifRadio.indicator.width + nonProduktifRadio.spacing

                                            Behavior on color {
                                                ColorAnimation { duration: 300 }
                                            }
                                        }

                                        indicator: Rectangle {
                                            implicitWidth: 22
                                            implicitHeight: 22
                                            x: 0
                                            y: (nonProduktifRadio.height - height) / 2
                                            radius: 11
                                            color: "transparent"
                                            border.color: nonProduktifRadio.checked ? accentColor : borderColor
                                            border.width: nonProduktifRadio.checked ? 2 : 1

                                            Behavior on border.color {
                                                ColorAnimation { duration: 200 }
                                            }

                                            Rectangle {
                                                anchors.centerIn: parent
                                                width: 10
                                                height: 10
                                                radius: 5
                                                color: accentColor
                                                visible: nonProduktifRadio.checked

                                                scale: nonProduktifRadio.checked ? 1.0 : 0.0
                                                Behavior on scale {
                                                    NumberAnimation {
                                                        duration: 200
                                                        easing.type: Easing.OutBack
                                                        easing.overshoot: 0.3
                                                    }
                                                }
                                            }
                                        }

                                        Rectangle {
                                            width: 16
                                            height: 16
                                            radius: 3
                                            color: "#ef4444"
                                            anchors.right: parent.right
                                            anchors.verticalCenter: parent.indicator.verticalCenter
                                            anchors.rightMargin: 8
                                            visible: nonProduktifRadio.checked

                                            Text {
                                                anchors.centerIn: parent
                                                text: "×"
                                                color: "white"
                                                font.pixelSize: 12
                                                font.bold: true
                                            }

                                            scale: nonProduktifRadio.checked ? 1.0 : 0.0
                                            Behavior on scale {
                                                NumberAnimation {
                                                    duration: 300
                                                    easing.type: Easing.OutBack
                                                }
                                            }
                                        }
                                    }

                                    // Netral option
                                    RadioButton {
                                        id: netralRadio
                                        text: "Netral"
                                        font.family: "Segoe UI, Arial, sans-serif"
                                        font.pixelSize: 14
                                        Layout.fillWidth: true

                                        onCheckedChanged: if (checked) addAppDialog.selectedProductivityType = 0

                                        contentItem: Text {
                                            text: netralRadio.text
                                            font: netralRadio.font
                                            color: textColor
                                            verticalAlignment: Text.AlignVCenter
                                            leftPadding: netralRadio.indicator.width + netralRadio.spacing

                                            Behavior on color {
                                                ColorAnimation { duration: 300 }
                                            }
                                        }

                                        indicator: Rectangle {
                                            implicitWidth: 22
                                            implicitHeight: 22
                                            x: 0
                                            y: (netralRadio.height - height) / 2
                                            radius: 11
                                            color: "transparent"
                                            border.color: netralRadio.checked ? accentColor : borderColor
                                            border.width: netralRadio.checked ? 2 : 1

                                            Behavior on border.color {
                                                ColorAnimation { duration: 200 }
                                            }

                                            Rectangle {
                                                anchors.centerIn: parent
                                                width: 10
                                                height: 10
                                                radius: 5
                                                color: accentColor
                                                visible: netralRadio.checked

                                                scale: netralRadio.checked ? 1.0 : 0.0
                                                Behavior on scale {
                                                    NumberAnimation {
                                                        duration: 200
                                                        easing.type: Easing.OutBack
                                                        easing.overshoot: 0.3
                                                    }
                                                }
                                            }
                                        }

                                        Rectangle {
                                            width: 16
                                            height: 16
                                            radius: 3
                                            color: "#6b7280"
                                            anchors.right: parent.right
                                            anchors.verticalCenter: parent.indicator.verticalCenter
                                            anchors.rightMargin: 8
                                            visible: netralRadio.checked

                                            Text {
                                                anchors.centerIn: parent
                                                text: "–"
                                                color: "white"
                                                font.pixelSize: 12
                                                font.bold: true
                                            }

                                            scale: netralRadio.checked ? 1.0 : 0.0
                                            Behavior on scale {
                                                NumberAnimation {
                                                    duration: 300
                                                    easing.type: Easing.OutBack
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            Item {
                                Layout.fillHeight: true
                            }
                        }

                        Connections {
                            target: appList
                            function onCurrentIndexChanged() {
                                if (appList.currentItem) {
                                    const appName = appList.model[appList.currentIndex]
                                    txtWebsite.visible = ["Chrome", "Firefox", "Edge", "Safari", "Opera"].includes(appName)
                                    txtAppName.visible = appName === "Other"
                                }
                            }
                        }
                    }
                }










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
                                                text: ""
                                                font.pixelSize: 14
                                                color: lightTextColor
                                            }

                                            Label {
                                                text: {
                                                    var activeTask = logger.taskList.find(task => task.id === logger.activeTaskId)
                                                    if (activeTask) {
                                                        // Format waktu ke HH:MM
                                                        var hours = Math.floor(activeTask.time_usage / 3600)
                                                        var minutes = Math.floor((activeTask.time_usage % 3600) / 60)
                                                        var here_text = (hours < 10 ? "0" + hours : hours) + ":" + (minutes < 10 ? "0" + minutes : minutes)
                                                        public_curent_time = here_text
                                                        return here_text
                                                    }
                                                    return "00:00"
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
                                                            return parent.width * Math.min(1, activeTask.time_usage / activeTask.max_time)
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
                                            }
                                            Label {
                                                text: {
                                                    var activeTask = logger.taskList.find(task => task.id === logger.activeTaskId)
                                                    if (activeTask) {
                                                        // Format waktu maksimal ke HH:MM
                                                        var hours = Math.floor(activeTask.max_time / 3600)
                                                        var minutes = Math.floor((activeTask.max_time % 3600) / 60)
                                                        var time_max  = (hours < 10 ? "0" + hours : hours) + ":" + (minutes < 10 ? "0" + minutes : minutes)
                                                        public_max_time = time_max
                                                        return time_max
                                                    }
                                                    return "00:00"
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
                                            return isActive ? Qt.lighter(cardColor, 1.6) : cardColor
                                        }
                                        border.color: {
                                            if (isReview) {
                                                return Qt.rgba(255/255, 152/255, 0/255, 0.3) // Soft orange border
                                            }
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
                                            enabled: !delegateRoot.isReview
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
                                                    visible: !delegateRoot.isReview

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

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 12

                            Label {
                                text: "Activity Monitor"
                                font { bold: true; pixelSize: 16; family: "Segoe UI" }
                                color: primaryColor
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                height: 1
                                color: dividerColor
                            }

                            // Current Activity Section
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                Label {
                                    text: "Current Window"
                                    font { bold: true; pixelSize: 14 }
                                    color: textColor
                                }

                                GridLayout {
                                    columns: 2
                                    columnSpacing: 12
                                    rowSpacing: 8
                                    Layout.fillWidth: true

                                    Label {
                                        text: "Application:"
                                        font.pixelSize: 12
                                        color: lightTextColor
                                    }
                                    Label {
                                        text: logger.currentAppName || "Unknown"
                                        font.pixelSize: 12
                                        color: textColor
                                        elide: Text.ElideRight
                                    }

                                    Label {
                                        text: "Window Title:"
                                        font.pixelSize: 12
                                        color: lightTextColor
                                    }
                                    Label {
                                        text: logger.currentWindowTitle || "Unknown"
                                        font.pixelSize: 12
                                        color: textColor
                                        elide: Text.ElideRight
                                    }
                                    Label {
                                        text: "Total Logs:"
                                        font.pixelSize: 12
                                        color: lightTextColor
                                    }
                                    Label {
                                        text: logger.logCount
                                        color: lightTextColor
                                        font.pixelSize: 12
                                    }
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                height: 1
                                color: dividerColor
                            }

                            // Recent Activity Section
                            ColumnLayout {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                spacing: 8

                                Label {
                                    text: "Recent Activity"
                                    font {
                                        family: "Segoe UI"
                                        weight: Font.Medium
                                        pixelSize: 14
                                    }
                                    color: textColor
                                }

                                ScrollView {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    clip: true

                                    ListView {
                                        id: activityListView
                                        model: logger.logContent.split('\n').filter(line => line.trim() !== '').slice(0, 15)
                                        spacing: 4
                                        width: parent.width

                                        header: RowLayout {
                                            width: activityListView.width
                                            spacing: 8

                                            Label {
                                                text: "Time"
                                                font {
                                                    family: "Segoe UI"
                                                    weight: Font.DemiBold
                                                    pixelSize: 12
                                                }
                                                color: textColor
                                                Layout.preferredWidth: 100
                                            }

                                            Label {
                                                text: "Duration"
                                                font {
                                                    family: "Segoe UI"
                                                    weight: Font.DemiBold
                                                    pixelSize: 12
                                                }
                                                color: textColor
                                                Layout.preferredWidth: 80
                                            }

                                            Label {
                                                text: "Application"
                                                font {
                                                    family: "Segoe UI"
                                                    weight: Font.DemiBold
                                                    pixelSize: 12
                                                }
                                                color: textColor
                                                Layout.preferredWidth: 150
                                            }

                                            Label {
                                                text: "Title"
                                                font {
                                                    family: "Segoe UI"
                                                    weight: Font.DemiBold
                                                    pixelSize: 12
                                                }
                                                color: textColor
                                                Layout.fillWidth: true
                                            }
                                        }

                                        delegate: Rectangle {
                                            width: activityListView.width
                                            height: 36
                                            color: index % 2 === 0 ? cardColor : Qt.lighter(cardColor, 1.1)
                                            radius: 4

                                            RowLayout {
                                                anchors.fill: parent
                                                anchors.leftMargin: 8
                                                anchors.rightMargin: 8
                                                spacing: 8

                                                Label {
                                                    text: {
                                                        const parts = modelData.split(',')
                                                        if (parts.length >= 4) {
                                                            return parts[0].trim()
                                                        }
                                                        return "Unknown"
                                                    }
                                                    Layout.preferredWidth: 100
                                                    font {
                                                        family: "Segoe UI"
                                                        pixelSize: 11
                                                    }
                                                    color: lightTextColor
                                                    elide: Text.ElideRight
                                                }

                                                Label {
                                                    text: {
                                                        const parts = modelData.split(',')
                                                        if (parts.length >= 4) {
                                                            const start = parts[0].trim()
                                                            const end = parts[1].trim()
                                                            const startTime = new Date("2000-01-01 " + start)
                                                            const endTime = new Date("2000-01-01 " + end)
                                                            const durationSec = (endTime - startTime) / 1000
                                                            return Math.floor(durationSec/60) + "m " + (durationSec%60) + "s"
                                                        }
                                                        return ""
                                                    }
                                                    Layout.preferredWidth: 80
                                                    font {
                                                        family: "Segoe UI"
                                                        pixelSize: 11
                                                    }
                                                    color: lightTextColor
                                                }

                                                Label {
                                                    text: {
                                                        const parts = modelData.split(',')
                                                        if (parts.length >= 4) {
                                                            return parts[2].trim()
                                                        }
                                                        return "Unknown"
                                                    }
                                                    Layout.preferredWidth: 150
                                                    font {
                                                        family: "Segoe UI"
                                                        pixelSize: 11
                                                    }
                                                    color: lightTextColor
                                                    elide: Text.ElideRight
                                                }

                                                Label {
                                                    text: {
                                                        const parts = modelData.split(',')
                                                        if (parts.length >= 4) {
                                                            return parts[3].trim()
                                                        }
                                                        return ""
                                                    }
                                                    font {
                                                        family: "Segoe UI"
                                                        pixelSize: 11
                                                    }
                                                    color: lightTextColor
                                                    elide: Text.ElideRight
                                                    Layout.fillWidth: true
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                }

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

                        onTriggered: {
                            var payload = {
                                "status": "need-review"
                            }

                            var apiUrl = "https://deskmon.pranala-dt.co.id/api/update-status-task/" +
                                    stableTaskMenu.taskId + "/" + stableTaskMenu.userId;

                            var request = new XMLHttpRequest()
                            request.open("POST", apiUrl)
                            request.setRequestHeader("Content-Type", "application/json")
                            request.setRequestHeader("Authorization", "Bearer " + stableTaskMenu.authToken)

                            request.onreadystatechange = function() {
                                if (request.readyState === XMLHttpRequest.DONE) {
                                    if (request.status === 200) {
                                        console.log("Task status updated to 'need review'")
                                        logger.fetchAndStoreTasks()
                                    } else {
                                        console.error("Failed to update task status:", request.status, request.responseText)
                                    }
                                }
                            }

                            request.send(JSON.stringify(payload))
                        }
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



    // FileDialog for selecting image
    FileDialog {
        id: fileDialog
        title: "Choose a profile picture"
        nameFilters: ["Image files (*.png *.jpg *.jpeg)"]
        onAccepted: {
            var fileUrl = fileDialog.selectedFile.toString()
            console.log("FileDialog accepted - selectedFile:", fileUrl)

            if (!logger.validateFilePath(fileUrl)) {
                console.log("File validation failed: url=", fileUrl)
                profileErrorLabel.text = "Selected file is invalid or cannot be accessed"
                return
            }

            tempImagePath = fileUrl
            console.log("Opening crop dialog with tempImagePath:", tempImagePath)
            cropDialog.open()
        }
        onRejected: {
            profileErrorLabel.text = ""
            console.log("FileDialog rejected")
        }
    }

    Dialog {
        id: cropDialog
        title: "Crop Profile Picture"
        modal: true
        anchors.centerIn: parent
        width: Math.min(parent.width * 0.9, 700)
        height: Math.min(parent.height * 0.9, 800)
        padding: 0

        property real imageScale: 1.0
        property real imageX: 0
        property real imageY: 0

        background: Rectangle {
            color: cardColor
            radius: 12
            border.color: dividerColor
            border.width: 1
            layer.enabled: true

        }

        contentItem: Rectangle {
            color: cardColor
            radius: 12
            anchors.fill: parent

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 24
                spacing: 24

                Label {
                    text: "Adjust your profile picture"
                    font {
                        pixelSize: 20
                        family: "Segoe UI"
                        weight: Font.Medium
                    }
                    color: textColor
                    Layout.alignment: Qt.AlignHCenter
                }

                Label {
                    text: "Move and zoom the image to fit the circular frame"
                    font.pixelSize: 14
                    color: Qt.darker(textColor, 1.4)
                    Layout.alignment: Qt.AlignHCenter
                }

                // Main cropping area
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.maximumHeight: 400

                    Rectangle {
                        id: cropArea
                        anchors.centerIn: parent
                        width: Math.min(parent.width, parent.height)
                        height: width
                        radius: width/2
                        color: "transparent"
                        border.color: Qt.lighter(dividerColor, 1.2)
                        border.width: 2
                        clip: true

                        // Semi-transparent overlay for outside area
                        Rectangle {
                            anchors.fill: parent
                            radius: parent.radius
                            color: "#99000000"
                            visible: cropMask.visible
                        }

                        Image {
                            id: cropImage
                            source: tempImagePath
                            fillMode: Image.PreserveAspectFit
                            width: cropArea.width * cropDialog.imageScale
                            height: cropArea.height * cropDialog.imageScale
                            x: cropDialog.imageX
                            y: cropDialog.imageY
                            smooth: true
                            mipmap: true
                            asynchronous: true
                            cache: false

                            onStatusChanged: {
                                if (status === Image.Ready) {
                                    console.log("Image loaded successfully:", tempImagePath)
                                    cropDialog.imageScale = Math.min(1.0, cropArea.width / implicitWidth)
                                    cropDialog.imageX = (cropArea.width - width) / 2
                                    cropDialog.imageY = (cropArea.height - height) / 2
                                    zoomSlider.value = cropDialog.imageScale
                                } else if (status === Image.Error) {
                                    console.log("Failed to load image:", tempImagePath)
                                    profileErrorLabel.text = "Failed to load image for cropping"
                                    cropDialog.close()
                                }
                            }
                        }

                        // Circular mask (visible only during editing)
                        Rectangle {
                            id: cropMask
                            anchors.fill: parent
                            radius: parent.radius
                            color: "transparent"
                            border.color: "white"
                            border.width: 2
                            visible: true
                        }
                    }

                    MouseArea {
                        anchors.fill: cropArea
                        acceptedButtons: Qt.LeftButton
                        cursorShape: Qt.OpenHandCursor

                        property real lastX: 0
                        property real lastY: 0

                        onPressed: {
                            lastX = mouse.x
                            lastY = mouse.y
                            cursorShape = Qt.ClosedHandCursor
                        }

                        onReleased: {
                            cursorShape = Qt.OpenHandCursor
                        }

                        onPositionChanged: {
                            if (pressed) {
                                var deltaX = mouse.x - lastX
                                var deltaY = mouse.y - lastY

                                // Calculate new position
                                var newX = cropDialog.imageX + deltaX
                                var newY = cropDialog.imageY + deltaY

                                // Get image dimensions after scaling
                                var scaledWidth = cropImage.width
                                var scaledHeight = cropImage.height

                                // Calculate maximum allowed movement
                                var maxX = (scaledWidth - cropArea.width) / 2
                                var maxY = (scaledHeight - cropArea.height) / 2

                                // Limit movement to keep image within crop area
                                if (scaledWidth > cropArea.width) {
                                    newX = Math.min(maxX, Math.max(-maxX, newX))
                                } else {
                                    newX = (cropArea.width - scaledWidth) / 2
                                }

                                if (scaledHeight > cropArea.height) {
                                    newY = Math.min(maxY, Math.max(-maxY, newY))
                                } else {
                                    newY = (cropArea.height - scaledHeight) / 2
                                }

                                cropDialog.imageX = newX
                                cropDialog.imageY = newY

                                lastX = mouse.x
                                lastY = mouse.y
                            }

                        }



                        onWheel: {
                            var delta = wheel.angleDelta.y / 1200
                            var newScale = cropDialog.imageScale + delta
                            zoomSlider.value = Math.min(Math.max(newScale, zoomSlider.from), zoomSlider.to)
                        }
                    }
                }

                // Zoom controls
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Label {
                        text: "Zoom: " + Math.round(zoomSlider.value * 100) + "%"
                        font.pixelSize: 12
                        color: Qt.darker(textColor, 1.4)
                        Layout.alignment: Qt.AlignHCenter
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        Button {
                            text: "−"
                            Layout.preferredWidth: 48
                            Layout.preferredHeight: 48
                            Material.background: secondaryColor
                            Material.foreground: "white"
                            font {
                                pixelSize: 24
                                family: "Segoe UI Symbol"
                                weight: Font.Bold
                            }
                            onClicked: zoomSlider.value = Math.max(zoomSlider.from, zoomSlider.value - 0.1)
                        }

                        Slider {
                            id: zoomSlider
                            from: 0.2
                            to: 3.0
                            value: 1.0
                            stepSize: 0.05
                            snapMode: Slider.SnapAlways
                            Layout.fillWidth: true
                            onValueChanged: {
                                var oldScale = cropDialog.imageScale
                                cropDialog.imageScale = value

                                // Adjust position to keep the center in view
                                var centerX = cropDialog.imageX + (cropImage.width * 0.5)
                                var centerY = cropDialog.imageY + (cropImage.height * 0.5)

                                cropDialog.imageX = centerX - (cropImage.width * 0.5)
                                cropDialog.imageY = centerY - (cropImage.height * 0.5)

                                // Ensure image stays within bounds
                                var scaledWidth = cropImage.width
                                var scaledHeight = cropImage.height
                                var maxX = (scaledWidth - cropArea.width) / 2
                                var maxY = (scaledHeight - cropArea.height) / 2

                                if (scaledWidth > cropArea.width) {
                                    cropDialog.imageX = Math.min(maxX, Math.max(-maxX, cropDialog.imageX))
                                } else {
                                    cropDialog.imageX = (cropArea.width - scaledWidth) / 2
                                }

                                if (scaledHeight > cropArea.height) {
                                    cropDialog.imageY = Math.min(maxY, Math.max(-maxY, cropDialog.imageY))
                                } else {
                                    cropDialog.imageY = (cropArea.height - scaledHeight) / 2
                                }
                            }
                        }

                        Button {
                            text: "+"
                            Layout.preferredWidth: 48
                            Layout.preferredHeight: 48
                            Material.background: secondaryColor
                            Material.foreground: "white"
                            font {
                                pixelSize: 24
                                family: "Segoe UI Symbol"
                                weight: Font.Bold
                            }
                            onClicked: zoomSlider.value = Math.min(zoomSlider.to, zoomSlider.value + 0.1)
                        }
                    }
                }

                // Action buttons
                RowLayout {
                    Layout.alignment: Qt.AlignRight
                    spacing: 16

                    Button {
                        text: "Cancel"
                        Layout.preferredWidth: 120
                        Layout.preferredHeight: 48
                        Material.background: "transparent"
                        Material.foreground: textColor
                        font {
                            pixelSize: 14
                            weight: Font.Medium
                        }
                        onClicked: {
                            cropDialog.reject()
                            profileErrorLabel.text = ""
                        }
                    }

                    Button {
                        text: "Save"
                        Layout.preferredWidth: 120
                        Layout.preferredHeight: 48
                        Material.background: accentColor
                        Material.foreground: "white"
                        font {
                            pixelSize: 14
                            weight: Font.Medium
                        }
                        onClicked: {
                            var croppedPath = logger.cropProfileImage(
                                        tempImagePath,
                                        cropDialog.imageX,
                                        cropDialog.imageY,
                                        cropImage.width,
                                        cropImage.height,
                                        cropArea.width,
                                        cropArea.height
                                        )
                            if (croppedPath !== "") {
                                console.log("Cropped image path:", croppedPath)
                                if (logger.updateProfileImage(currentUsername, croppedPath)) {
                                    profileImagePath = croppedPath
                                    profileImage.source = ""
                                    profileImage.source = profileImagePath
                                    cropDialog.accept()
                                    profileErrorLabel.text = "Profile picture updated successfully"
                                    profileErrorLabel.color = "#4CAF50" // Green
                                } else {
                                    console.log("Failed to update profile image in database")
                                    profileErrorLabel.text = "Failed to update profile image"
                                    profileErrorLabel.color = "#F44336" // Red
                                }
                            } else {
                                console.log("Failed to crop image")
                                profileErrorLabel.text = "Failed to crop image"
                                profileErrorLabel.color = "#F44336" // Red
                            }
                        }
                    }
                }
            }
        }
    }
}
