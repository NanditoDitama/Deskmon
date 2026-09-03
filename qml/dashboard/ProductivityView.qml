import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Effects
import "../theme"
import "../components"
import "../application_submission"

Item {
    id: productivityRoot
    anchors.fill: parent

    property string clockIn: "--:--"
    property string clockOut: "--:--"
    property int userId: (typeof logger !== "undefined") ? logger.currentUserId : -1

    // State Statistik Aplikasi & Domain
    property var appDurations: ({})
    property var sortedApps: []
    property var sortedDomains: []
    property bool showAllPercentages: false

    // State Filter Tanggal
    property date startSelectedDate: new Date(NaN)
    property date endSelectedDate: new Date(NaN)
    property bool isDateSelected: false

    Component.onCompleted: {
        var today = new Date();
        startSelectedDate = today;
        endSelectedDate = today;
        isDateSelected = true;
        updateAppDurations();
        updateDomainDurations();
    }

    Connections {
        target: (typeof logger !== "undefined") ? logger : null
        function onLogContentChanged() {
            updateAppDurations();
            updateDomainDurations();
        }
    }

    function formatDuration(seconds) {
        if (seconds < 60) {
            return seconds + "s";
        } else if (seconds < 3600) {
            var minutes = Math.floor(seconds / 60);
            var secs = seconds % 60;
            return minutes + "m " + secs + "s";
        } else {
            var hours = Math.floor(seconds / 3600);
            var mins = Math.floor((seconds % 3600) / 60);
            var secs = seconds % 60;
            return hours + "h " + mins + "m " + secs + "s";
        }
    }

    function extractDomain(urlString) {
        if (!urlString || urlString.trim() === "") return "";
        try {
            var fullUrl = urlString.startsWith("http") ? urlString : "https://" + urlString;
            var urlObj = new URL(fullUrl);
            var hostname = urlObj.hostname;
            if (hostname.startsWith("www.")) return hostname.substring(4);
            return hostname;
        } catch (e) {
            var domain = urlString.split('/')[0];
            if (domain.startsWith("www.")) return domain.substring(4);
            return domain;
        }
    }

    function getProductivityType(name, url) {
        if (typeof logger === "undefined") return "neutral";
        var typeInt = logger.getAppProductivityType(name, url);
        switch(typeInt) {
        case 1: return "productive";
        case 2: return "non-productive";
        default: return "neutral";
        }
    }

    function updateAppDurations() {
        if (typeof logger === "undefined" || !logger.logContent) return;
        var durations = {};
        var logs = logger.logContent.split('\n');
        var totalDuration = 0;

        for (var i = 0; i < logs.length; i++) {
            var parts = logs[i].split(',');
            if (parts.length >= 4 && parts[2].trim() !== '' && parts[3].trim() !== '') {
                var appName = parts[2].trim();
                if (appName === "Idle") continue;

                var start = parts[0].trim();
                var end = parts[1].trim();
                var startTime = new Date("2000-01-01 " + start);
                var endTime = new Date("2000-01-01 " + end);
                var durationSec = (endTime - startTime) / 1000;

                if (durationSec > 0) {
                    if (durations[appName] === undefined) {
                        durations[appName] = 0;
                    }
                    durations[appName] += durationSec;
                    totalDuration += durationSec;
                }
            }
        }

        var appArray = [];
        for (var app in durations) {
            var percentage = totalDuration > 0 ? (durations[app] / totalDuration) * 100 : 0;
            appArray.push({
                name: app,
                duration: durations[app],
                percentage: percentage,
                productivityType: getProductivityType(app, "")
            });
        }

        appArray.sort((a, b) => b.duration - a.duration);
        appDurations = durations;
        sortedApps = appArray;
    }

    function updateDomainDurations() {
        if (typeof logger === "undefined" || !logger.logContent) return;
        var domainDurations = {};
        var logs = logger.logContent.split('\n').filter(line => line.trim() !== '');
        var totalDuration = 0;

        for (var i = 0; i < logs.length; i++) {
            var parts = logs[i].split(',');
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
        for (var d in domainDurations) {
            var percentage = totalDuration > 0 ? (domainDurations[d] / totalDuration) * 100 : 0;
            domainArray.push({
                name: d,
                duration: domainDurations[d],
                percentage: percentage,
                productivityType: getProductivityType(d, d)
            });
        }

        domainArray.sort((a, b) => b.duration - a.duration);
        sortedDomains = domainArray;
    }

    function fetchClockData() {
        // Cek pengaman (guard condition)
        if (userId <= 0 || typeof logger.authToken === "undefined" || logger.authToken === "") {
            console.log("fetchClockData: Guard check failed. User ID or Token not ready.");
            return;
        }

        console.log("fetchClockData: Attempting to fetch from API...");
        var xhr = new XMLHttpRequest();
        xhr.onreadystatechange = function() {
            if (xhr.readyState === XMLHttpRequest.DONE) {
                if (xhr.status === 200) {
                    try {
                        var response = JSON.parse(xhr.responseText);
                        console.log("fetchClockData: Success response text:", xhr.responseText);
                        var rawIn = null;
                        var rawOut = null;
                        if (response.data) {
                            rawIn = response.data.clock_in || response.data.check_in || response.data.clockIn || response.data.checkIn;
                            rawOut = response.data.clock_out || response.data.check_out || response.data.clockOut || response.data.checkOut;
                        }
                        if (!rawIn && response.clock_in) rawIn = response.clock_in;
                        if (!rawIn && response.check_in) rawIn = response.check_in;
                        if (!rawOut && response.clock_out) rawOut = response.clock_out;
                        if (!rawOut && response.check_out) rawOut = response.check_out;

                        if (rawIn) {
                            clockIn = rawIn;
                            clockOut = rawOut || "Online";
                        } else if (response.success) {
                            clockIn = response.clock_in || "--:--";
                            clockOut = response.clock_out || "Online";
                        } else {
                            console.log("fetchClockData: API Error:", response.message || "Unknown error");
                            clockOut = "Error"; // <-- TAMBAHAN 1: API Error
                        }
                    } catch (e) {
                        console.error("fetchClockData: Error parsing JSON:", e, xhr.responseText);
                        clockOut = "Error"; // <-- TAMBAHAN 2: JSON Error
                    }
                } else {
                    // Ini adalah permintaan Anda (Koneksi gagal)
                    console.log("fetchClockData: Network error. Status:", xhr.status, xhr.statusText);
                    clockOut = "Error"; // <-- TAMBAHAN 3: Network Error
                }
            }
        }

        xhr.open("GET", "https://deskmon.pranala-dt.co.id/api/get-check-in/" + userId);

        // PENTING: Tambahkan header Authorization
        xhr.setRequestHeader("Authorization", "Bearer " + logger.authToken);

        xhr.send();
    }

    // FUNGSI 2: Untuk mengecek dan memulai timer
    // Ini adalah fungsi yang seharusnya dipanggil oleh Connections
    function checkAndFetchClockData() {
        // Cek jika KEDUA data siap dan timer belum jalan
        if (logger.currentUserId > 0 && logger.authToken !== "" && !clockRefreshTimer.running) {

            console.log("checkAndFetchClockData: User AND Token are ready. Starting timer...");
            fetchClockData(); // Panggil pertama kali
            clockRefreshTimer.start(); // Mulai timer
        } else {
            console.log("checkAndFetchClockData: Conditions not met. Waiting... (UserID: " + logger.currentUserId + ", Token Ready: " + (logger.authToken !== "") + ", Timer Running: " + clockRefreshTimer.running + ")");
        }
    }

    // TIMER: Dibuat tidak berjalan saat awal
    Timer {
        id: clockRefreshTimer
        interval: 30000  // 1 menit
        repeat: true
        running: false // PENTING: Awalnya false
        onTriggered: fetchClockData() // Timer hanya memanggil fetchClockData
    }

    // CONNECTIONS: Menunggu C++ siap
    Connections {
        target: logger

        // Sinyal 1: Saat User ID berubah
        function onCurrentUserIdChanged() {
            console.log("Connections: onCurrentUserIdChanged detected. ID:", logger.currentUserId);
            if (logger.currentUserId <= 0) { // Handle logout
                console.log("Connections: User logged out. Stopping clock timer.");
                clockRefreshTimer.stop();
                clockIn = "--:--";
                clockOut = "--:--";
            } else {
                // Coba cek, mungkin token sudah siap
                checkAndFetchClockData();
            }
        }

        // Sinyal 2: Saat Auth Token berubah
        function onAuthTokenChanged() {
            console.log("Connections: onAuthTokenChanged detected. Token is ready.");
            // Coba cek, mungkin ID sudah siap
            checkAndFetchClockData();
        }
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
                        color: Theme.primaryColor
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
                                color: parent.hovered ? Qt.lighter(Theme.cardColor, 1.5) : "transparent"
                                border.color: Theme.dividerColor
                                border.width: 1
                            }
                            contentItem: Label {
                                text: parent.text
                                font: parent.font
                                color: Theme.accentColor
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
                                color: parent.hovered ? Qt.lighter(Theme.cardColor, 1.5) : "transparent"
                                border.color: Theme.dividerColor
                                border.width: 1
                            }
                            contentItem: Label {
                                text: parent.text
                                font: parent.font
                                color: Theme.accentColor
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            onClicked: dateRangeDialog.open()
                        }
                    }
                }
                DateRangeDialog{
                    id: dateRangeDialog
                    parent: Overlay.overlay
                }

                // Divider
                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    radius: 1
                    color: Theme.dividerColor
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
                                    color: Theme.primaryColor
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
                                        color: Theme.textColor
                                    }

                                    // Percentage
                                    Label {
                                        text: currentPercentage.toFixed(1) + "%"
                                        font {
                                            family: "Segoe UI"
                                            pixelSize: 14
                                            weight: Font.DemiBold
                                        }
                                        color: Theme.primaryColor
                                    }

                                    // Duration
                                    Label {
                                        text: formatDuration(modelData.duration)
                                        font {
                                            family: "Segoe UI"
                                            pixelSize: 14
                                        }
                                        color: Theme.lightTextColor
                                    }
                                }

                                // Progress bar
                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 6
                                    radius: 3
                                    color: Qt.alpha(Theme.dividerColor, 0.3)

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
                                                    if (modelData.productivityType === "productive") baseColor = Theme.primaryColor;
                                                    else if (modelData.productivityType === "non-productive") baseColor = Theme.nonProductiveColor;
                                                    else baseColor = Theme.neutralColor;

                                                    // Membuat warna transparan (alpha = 0)
                                                    return Qt.alpha(baseColor, 0.0);
                                                }
                                            }
                                            GradientStop {
                                                position: 1.0
                                                color: {
                                                    // Warna terang penuh (alpha = 1)
                                                    if (modelData.productivityType === "productive") return Theme.primaryColor;
                                                    if (modelData.productivityType === "non-productive") return Theme.nonProductiveColor;
                                                    if (modelData.productivityType === "neutral") return Theme.neutralColor;
                                                    return Theme.neutralColor;
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
            color: Theme.dividerColor
        }

        // --- KOLOM PENGGUNAAN DOMAIN ---
        ColumnLayout {
            Layout.preferredWidth: parent.width * 0.3
            Layout.fillHeight: true
            spacing: 12

            // Header untuk Domain
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 20

                Label {
                    text: "Website Usage"
                    font {
                        family: "Segoe UI"
                        weight: Font.DemiBold
                        pixelSize: 18
                        letterSpacing: 0.5
                    }
                    color: Theme.primaryColor
                    Layout.fillWidth: true
                }

                Rectangle { // Divider di bawah header
                    Layout.fillWidth: true
                    height: 1
                    radius: 1
                    color: Theme.dividerColor
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
                            Item {
                                Layout.preferredWidth: 24
                                Layout.preferredHeight: 24

                                // Fallback Rectangle (original placeholder with the initial letter)
                                Rectangle {
                                    id: fallbackIcon
                                    anchors.fill: parent
                                    radius: 4
                                    color: Qt.rgba(
                                               Math.random() * 0.5 + 0.3,
                                               Math.random() * 0.5 + 0.3,
                                               Math.random() * 0.5 + 0.3,
                                               0.2
                                               )
                                    // Only show this placeholder if the web icon fails to load
                                    visible: webIcon.status !== Image.Ready

                                    Label {
                                        text: modelData.name ? modelData.name.charAt(0).toUpperCase() : "?"
                                        anchors.centerIn: parent
                                        font {
                                            family: "Segoe UI"
                                            weight: Font.Bold
                                            pixelSize: 12
                                        }
                                        color: Theme.primaryColor
                                    }
                                }

                                // Website Favicon Image
                                Image {
                                    id: webIcon
                                    anchors.fill: parent
                                    source: modelData.name ? "https://www.google.com/s2/favicons?sz=32&domain=" + modelData.name : ""
                                    asynchronous: true
                                    smooth: true
                                    visible: status === Image.Ready
                                    fillMode: Image.PreserveAspectFit
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
                                        color: Theme.textColor
                                    }

                                    // Percentage
                                    Label {
                                        text: currentPercentage.toFixed(1) + "%"
                                        font {
                                            family: "Segoe UI"
                                            pixelSize: 14
                                            weight: Font.DemiBold
                                        }
                                        color: Theme.primaryColor
                                    }

                                    // Duration
                                    Label {
                                        text: formatDuration(modelData.duration)
                                        font {
                                            family: "Segoe UI"
                                            pixelSize: 14
                                        }
                                        color: Theme.lightTextColor
                                    }
                                }

                                // Progress bar
                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 6
                                    radius: 3
                                    color: Qt.alpha(Theme.dividerColor, 0.3)

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
                                                    if (modelData.productivityType === "productive") baseColor = Theme.primaryColor;
                                                    else if (modelData.productivityType === "non-productive") baseColor = Theme.nonProductiveColor;
                                                    else baseColor = Theme.neutralColor;

                                                    // Membuat warna transparan (alpha = 0)
                                                    return Qt.alpha(baseColor, 0.0);
                                                }
                                            }
                                            GradientStop {
                                                position: 1.0
                                                color: {
                                                    // Warna terang penuh (alpha = 1)
                                                    if (modelData.productivityType === "productive") return Theme.primaryColor;
                                                    if (modelData.productivityType === "non-productive") return Theme.nonProductiveColor;
                                                    if (modelData.productivityType === "neutral") return Theme.neutralColor;
                                                    return Theme.neutralColor;
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
            color: Theme.dividerColor
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
                        color: Theme.primaryColor
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
                            color: Theme.accentColor
                        }
                        onClicked: {
                            var apps = logger.getProductivityApps();
                            applicationsDialog.productiveAppsModel.clear();
                            applicationsDialog.nonProductiveAppsModel.clear();
                            for (var i = 0; i < apps.length; i++) {
                                if (apps[i].type === 1) {
                                    applicationsDialog.productiveAppsModel.append({
                                        "appName": apps[i].appName,
                                        "url": apps[i].url
                                    });
                                } else if (apps[i].type === 2) {
                                    applicationsDialog.nonProductiveAppsModel.append({
                                        "appName": apps[i].appName,
                                        "url": apps[i].url
                                    });
                                }
                            }
                            applicationsDialog.open();
                        }
                    }
                    MonitoredApplicationsDialog{
                        id:applicationsDialog
                        parent:Overlay.overlay
                    }
                }
                // Divider
                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    radius: 1
                    color: Theme.dividerColor
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
                            property real idleAngle: 0
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
                                        gradient.addColorStop(0, "" + color)
                                        gradient.addColorStop(1, "" + Qt.lighter(color, 1.3))
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
                                var totalAngle = productiveAngle + nonProductiveAngle  + idleAngle + neutralAngle
                                var availableAngle = 2 * Math.PI - 0.02 // Sisakan gap total

                                // Scale down semua angle jika total melebihi batas
                                var scaleFactor = 1
                                if (totalAngle > availableAngle) {
                                    scaleFactor = availableAngle / totalAngle
                                }

                                // Productive segment - always starts first
                                if (productiveAngle > 0) {
                                    var scaledProductiveAngle = productiveAngle * scaleFactor
                                    drawRingSegment(currentStartAngle, scaledProductiveAngle, Theme.productiveColor, true)
                                    currentStartAngle += scaledProductiveAngle + segmentGap
                                }

                                // Non-productive segment - starts after productive
                                if (nonProductiveAngle > 0) {
                                    var scaledNonProductiveAngle = nonProductiveAngle * scaleFactor
                                    drawRingSegment(currentStartAngle, scaledNonProductiveAngle, Theme.nonProductiveColor)
                                    currentStartAngle += scaledNonProductiveAngle + segmentGap
                                }

                                if (idleAngle > 0) {
                                    var scaledIdleAngle = idleAngle * scaleFactor;
                                    drawRingSegment(currentStartAngle, scaledIdleAngle, Theme.warningColor);
                                    currentStartAngle += scaledIdleAngle + segmentGap;
                                }

                                // Neutral segment - starts after non-productive
                                if (neutralAngle > 0) {
                                    var scaledNeutralAngle = neutralAngle * scaleFactor
                                    drawRingSegment(currentStartAngle, scaledNeutralAngle, Theme.neutralColor)
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

                                ctx.fillStyle = Theme.primaryColor
                                ctx.font = "bold 32px 'Segoe UI', system-ui, -apple-system"
                                ctx.fillText(progressPercent + "%", 0, 0)
                                ctx.restore()

                                // Subtitle dengan fade-in effect
                                ctx.font = "600 13px 'Segoe UI', system-ui, -apple-system"
                                ctx.fillStyle = Qt.alpha(Theme.primaryColor, 0.8 * animationProgress)
                                ctx.fillText("Productive", centerX, centerY + 18)

                                // Decorative center dot
                                if (animationProgress > 0.7) {
                                    var dotOpacity = (animationProgress - 0.7) / 0.3
                                    ctx.beginPath()
                                    ctx.arc(centerX, centerY + 35, 2, 0, 2 * Math.PI)
                                    ctx.fillStyle = Qt.alpha(Theme.primaryColor, 0.4 * dotOpacity)
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
                        var idle = logger.productivityStats.idle || 0

                        // Normalize if total exceeds 100%
                        var total = productive + nonProductive + neutral
                        if (total > 100) {
                            productive = (productive / total) * 100
                            nonProductive = (nonProductive / total) * 100
                            neutral = (neutral / total) * 100
                            idle = (idle / total) * 100
                        }

                        // Stop any ongoing animation and reset angles
                        chartAnimator.stop()
                        productivityCanvas.productiveAngle = 0
                        productivityCanvas.nonProductiveAngle = 0
                        productivityCanvas.neutralAngle =
                                productivityCanvas.idleAngle = 0
                        productivityCanvas.animationProgress = 0
                        productivityCanvas.glowIntensity = 0

                        // Set new target values
                        chartAnimator.productiveTarget = productive
                        chartAnimator.nonProductiveTarget = nonProductive
                        chartAnimator.neutralTarget = neutral
                        chartAnimator.idleTarget = idle

                        // Start enhanced animation
                        chartAnimator.start()
                    }
                }

                Component.onCompleted: {
                    // Initialize with clean slate
                    productivePercent.value = 0
                    nonProductivePercent.value = 0
                    neutralPercent.value = 0
                    idlePercent.value = 0

                    productivityCanvas.productiveAngle = 0
                    productivityCanvas.nonProductiveAngle = 0
                    productivityCanvas.neutralAngle = 0
                    productivityCanvas.idleAngle = 0
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
                    property real idleTarget: 0

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
                        PauseAnimation { duration: 100 }

                        // ** NEW: Idle Animation Phase **
                        ParallelAnimation {
                            NumberAnimation {
                                target: productivityCanvas
                                property: "idleAngle"
                                from: 0
                                to: (chartAnimator.idleTarget / 100) * 2 * Math.PI
                                duration: 600
                                easing.type: Easing.OutBack
                            }
                            NumberAnimation {
                                target: idlePercent
                                property: "value"
                                from: 0
                                to: chartAnimator.idleTarget
                                duration: 600
                                easing.type: Easing.OutCubic
                            }
                        }

                        PauseAnimation { duration: 100 }

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

                    // Productive
                    RowLayout {
                        spacing: 10
                        Rectangle {
                            implicitWidth: 16
                            implicitHeight: 16
                            radius: 4
                            color: Theme.productiveColor
                            border {
                                width: 1
                                color: Qt.darker(Theme.productiveColor, 1.2)
                            }
                        }
                        Label {
                            text: "Productive"
                            font {
                                pixelSize: 13
                                weight: Font.Medium
                            }
                            color: Theme.textColor
                            Layout.fillWidth: true
                        }
                        Label {
                            text: Math.round(productivePercent.value) + "%"
                            font {
                                pixelSize: 13
                                weight: Font.DemiBold
                            }
                            color: Theme.productiveColor
                        }
                    }

                    // Non-Productive
                    RowLayout {
                        spacing: 10
                        Rectangle {
                            implicitWidth: 16
                            implicitHeight: 16
                            radius: 4
                            color: Theme.nonProductiveColor
                            border {
                                width: 1
                                color: Qt.darker(Theme.nonProductiveColor, 1.2)
                            }
                        }
                        Label {
                            text: "Non-Productive"
                            font {
                                pixelSize: 13
                                weight: Font.Medium
                            }
                            color: Theme.textColor
                            Layout.fillWidth: true
                        }
                        Label {
                            text: Math.round(nonProductivePercent.value) + "%"
                            font {
                                pixelSize: 13
                                weight: Font.DemiBold
                            }
                            color: Theme.nonProductiveColor
                        }
                    }


                    // Idle Legend Item
                    RowLayout {
                        visible: idlePercent.value > 0
                        spacing: 10
                        Rectangle {
                            implicitWidth: 16
                            implicitHeight: 16
                            radius: Theme.radiusSmall
                            color: Theme.warningColor
                            border {
                                width: 1
                                color: Qt.darker(Theme.warningColor, 1.2)
                            }
                        }
                        Label {
                            text: "Idle"
                            font {
                                pixelSize: 13
                                weight: Font.Medium
                            }
                            color: Theme.textColor
                            Layout.fillWidth: true
                        }
                        Label {
                            text: Math.round(idlePercent.value) + "%"
                            font {
                                pixelSize: 13
                                weight: Font.DemiBold
                            }
                            color: Theme.warningColor
                        }
                    }

                    // Neutral
                    RowLayout {
                        spacing: 10
                        Rectangle {
                            implicitWidth: 16
                            implicitHeight: 16
                            radius: 4
                            color: Theme.neutralColor
                            border {
                                width: 1
                                color: Qt.darker(Theme.neutralColor, 1.2)
                            }
                        }
                        Label {
                            text: "Neutral"
                            font {
                                pixelSize: 13
                                weight: Font.Medium
                            }
                            color: Theme.textColor
                            Layout.fillWidth: true
                        }
                        Label {
                            text: Math.round(neutralPercent.value) + "%"
                            font {
                                pixelSize: 13
                                weight: Font.DemiBold
                            }
                            color: Theme.neutralColor
                        }
                    }

                    // Divider
                    Rectangle {
                        Layout.topMargin: 8
                        Layout.fillWidth: true
                        implicitHeight: 1
                        color: Theme.dividerColor
                    }

                    // Modular Work Timer Card
                    WorkTimerCard {
                        clockIn: productivityRoot.clockIn
                        clockOut: productivityRoot.clockOut
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
            id: idlePercent
            visible: false
            property real value: 0
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
