import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Effects
import QtQuick.Window
import "../theme"
import "../components"
import "../application_submission"

Item {
    id: productivityRoot
    anchors.fill: parent

    readonly property bool isCompact: Window.window ? Window.window.width < 1060 : false
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
        applyDateRange();
        if (typeof logger !== "undefined") {
            checkAndFetchClockData();
        }
    }

    function applyDateRange() {
        if (typeof logger === "undefined") return;
        if (!isDateSelected || isNaN(startSelectedDate.getTime())) {
            logger.clearLogFilter();
        } else {
            var startStr = Qt.formatDate(startSelectedDate, "yyyy-MM-dd");
            var endObj = isNaN(endSelectedDate.getTime()) ? startSelectedDate : endSelectedDate;
            var endStr = Qt.formatDate(endObj, "yyyy-MM-dd");
            logger.setLogFilter(startStr, endStr);
        }
        usageDebounceTimer.stop();
        updateUsageData();
    }

    Timer {
        id: usageDebounceTimer
        interval: 80
        repeat: false
        onTriggered: updateUsageData()
    }

    Connections {
        target: (typeof logger !== "undefined") ? logger : null
        function onLogContentChanged() {
            usageDebounceTimer.restart();
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

    function parseTimeToSeconds(t) {
        if (!t) return -1;
        var p = t.split(':');
        if (p.length < 2) return -1;
        var h = parseInt(p[0], 10);
        var m = parseInt(p[1], 10);
        var s = p.length >= 3 ? parseInt(p[2], 10) : 0;
        if (isNaN(h) || isNaN(m) || isNaN(s)) return -1;
        return h * 3600 + m * 60 + s;
    }

    function extractDomain(urlString) {
        if (!urlString) return "";
        var s = urlString.trim();
        var lower = s.toLowerCase();
        if (s === "" || lower === "n/a" || s.indexOf(" ") !== -1) return "";
        if (lower.startsWith("chrome://") || lower.startsWith("chrome-extension://") ||
            lower.startsWith("edge://") || lower.startsWith("edge-extension://") ||
            lower.startsWith("moz-extension://") || lower.startsWith("about:") ||
            lower.startsWith("devtools://") || lower.startsWith("file://") ||
            lower.startsWith("view-source:") || lower.startsWith("brave://") ||
            lower.startsWith("opera://") || lower.startsWith("vivaldi://") ||
            lower.startsWith("data:") || lower.startsWith("blob:") ||
            lower.startsWith("javascript:")) {
            return "";
        }
        var schemeIdx = s.indexOf("://");
        if (schemeIdx !== -1) {
            var scheme = s.substring(0, schemeIdx).toLowerCase();
            if (scheme !== "http" && scheme !== "https") return "";
            s = s.substring(schemeIdx + 3);
        }
        var slashIdx = s.indexOf("/");
        if (slashIdx !== -1) s = s.substring(0, slashIdx);
        var queryIdx = s.indexOf("?");
        if (queryIdx !== -1) s = s.substring(0, queryIdx);
        var hashIdx = s.indexOf("#");
        if (hashIdx !== -1) s = s.substring(0, hashIdx);
        var atIdx = s.indexOf("@");
        if (atIdx !== -1) s = s.substring(atIdx + 1);
        var colonIdx = s.indexOf(":");
        if (colonIdx !== -1) s = s.substring(0, colonIdx);
        s = s.toLowerCase();
        if (s.startsWith("www.")) s = s.substring(4);
        if (s === "localhost") return s;
        if (/^(\d{1,3}\.){3}\d{1,3}$/.test(s)) return s;
        if (s.indexOf(".") === -1 || s.startsWith(".") || s.endsWith(".")) return "";
        var parts = s.split(".");
        if (parts.length < 2) return "";
        for (var i = 0; i < parts.length; i++) {
            if (!/^[a-z0-9-]+$/.test(parts[i]) || parts[i].startsWith("-") || parts[i].endsWith("-"))
                return "";
        }
        var tld = parts[parts.length - 1];
        if (!/[a-z]/.test(tld) || tld.length < 2) return "";
        var blockedExts = {
            png:1, jpg:1, jpeg:1, gif:1, svg:1, webp:1, bmp:1, ico:1, tif:1, tiff:1,
            pdf:1, txt:1, md:1, csv:1, tsv:1, json:1, xml:1, yaml:1, yml:1, toml:1,
            ini:1, cfg:1, conf:1, log:1, sql:1, db:1, sqlite:1, zip:1, rar:1, "7z":1,
            tar:1, gz:1, bz2:1, xz:1, exe:1, msi:1, dll:1, so:1, dylib:1, app:1, dmg:1,
            deb:1, rpm:1, apk:1, ipa:1, cpp:1, hpp:1, c:1, h:1, cc:1, cxx:1, cs:1,
            java:1, kt:1, swift:1, py:1, rb:1, go:1, rs:1, php:1, js:1, jsx:1, ts:1,
            tsx:1, mjs:1, cjs:1, qml:1, css:1, scss:1, less:1, vue:1, svelte:1,
            doc:1, docx:1, xls:1, xlsx:1, ppt:1, pptx:1, odt:1, ods:1, odp:1,
            mp3:1, wav:1, flac:1, aac:1, ogg:1, mp4:1, mkv:1, avi:1, mov:1, wmv:1,
            webm:1, bin:1, iso:1, img:1, tmp:1, bak:1, lock:1, env:1
        };
        if (blockedExts[tld]) return "";
        return s;
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

    function updateUsageData() {
        if (typeof logger === "undefined") return;
        var content = logger.logContent || "";
        if (content.length === 0) {
            appDurations = {};
            sortedApps = [];
            sortedDomains = [];
            return;
        }

        var durations = {};
        var domainDurations = {};
        var totalAppDuration = 0;
        var totalDomainDuration = 0;

        var pos = 0;
        var len = content.length;
        while (pos < len) {
            var nl = content.indexOf('\n', pos);
            var line = (nl === -1) ? content.substring(pos) : content.substring(pos, nl);
            pos = (nl === -1) ? len : nl + 1;
            if (line.length === 0) continue;

            var parts = line.split(',');
            if (parts.length < 4) continue;
            var appName = parts[2].trim();
            var windowTitle = parts[3].trim();
            if (appName === '' || windowTitle === '') continue;

            var startSec = parseTimeToSeconds(parts[0].trim());
            var endSec = parseTimeToSeconds(parts[1].trim());
            if (startSec < 0 || endSec < 0) continue;
            var durationSec = endSec - startSec;
            if (durationSec <= 0) continue;

            if (appName !== "Idle") {
                durations[appName] = (durations[appName] || 0) + durationSec;
                totalAppDuration += durationSec;
            }

            if (parts.length >= 5) {
                var domain = extractDomain(parts[4]);
                if (domain) {
                    domainDurations[domain] = (domainDurations[domain] || 0) + durationSec;
                    totalDomainDuration += durationSec;
                }
            }
        }

        var appArray = [];
        for (var app in durations) {
            var pct = totalAppDuration > 0 ? (durations[app] / totalAppDuration) * 100 : 0;
            appArray.push({
                name: app,
                duration: durations[app],
                percentage: pct,
                productivityType: getProductivityType(app, "")
            });
        }
        appArray.sort(function(a, b) { return b.duration - a.duration; });
        appDurations = durations;
        sortedApps = appArray;

        var domainArray = [];
        for (var d in domainDurations) {
            var dpct = totalDomainDuration > 0 ? (domainDurations[d] / totalDomainDuration) * 100 : 0;
            domainArray.push({
                name: d,
                duration: domainDurations[d],
                percentage: dpct,
                productivityType: getProductivityType(d, d)
            });
        }
        domainArray.sort(function(a, b) { return b.duration - a.duration; });
        sortedDomains = domainArray;
    }

    function updateAppDurations() {
        updateUsageData();
    }

    function updateDomainDurations() {
        updateUsageData();
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

        var baseUrl = (typeof logger !== "undefined" && logger.apiBaseUrl) ? logger.apiBaseUrl() : "https://deskmon.pranala-dt.co.id/api";
        xhr.open("GET", baseUrl + "/get-check-in/" + userId);

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
                var today = new Date();
                startSelectedDate = today;
                endSelectedDate = today;
                isDateSelected = true;
                applyDateRange();
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
        spacing: productivityRoot.isCompact ? 12 : 20

        // Application Usage Section (Left)
        ColumnLayout {
            Layout.preferredWidth: productivityRoot.isCompact ? parent.width * 0.27 : parent.width * 0.28
            Layout.minimumWidth: 160
            Layout.fillHeight: true
            spacing: productivityRoot.isCompact ? 8 : 12

            HoverHandler {
                id: appUsageHover
            }

            // Header
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 30
                    spacing: 6

                    Label {
                        text: Lang.t("Application Usage")
                        font {
                            family: "Segoe UI"
                            weight: Font.DemiBold
                            pixelSize: productivityRoot.isCompact ? 14 : Theme.fontSizeTitle
                            letterSpacing: 0.2
                        }
                        color: Theme.textColor
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                    }

                    // Button group
                    Row {
                        spacing: 6
                        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter

                        CustomButton {
                            id: dateRangeButton
                            text: {
                                if (isNaN(startSelectedDate.getTime()))
                                    return Lang.t("Date Range")
                                if (!isNaN(endSelectedDate.getTime()) &&
                                    Qt.formatDate(startSelectedDate, "yyyy-MM-dd") !== Qt.formatDate(endSelectedDate, "yyyy-MM-dd")) {
                                    return Qt.formatDate(startSelectedDate, "MMM d") + "-" + Qt.formatDate(endSelectedDate, "MMM d")
                                }
                                return Qt.formatDate(startSelectedDate, "MMM d")
                            }
                            implicitHeight: 26
                            height: 26
                            leftPadding: productivityRoot.isCompact ? 7 : 10
                            rightPadding: productivityRoot.isCompact ? 7 : 10
                            btnRadius: Theme.radiusSmall
                            btnColor: "transparent"
                            btnHoverColor: Qt.lighter(Theme.cardColor, 1.5)
                            btnTextColor: Theme.accentColor
                            btnBorderColor: Theme.dividerColor
                            btnBorderWidth: 1
                            font {
                                pixelSize: 11
                                family: "Segoe UI"
                                weight: Font.Medium
                            }
                            onClicked: dateRangeDialog.open()
                        }
                    }
                }
                DateRangeDialog{
                    id: dateRangeDialog
                    parent: Overlay.overlay
                    targetView: productivityRoot
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
                id: appScrollView
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                rightPadding: appScrollBar.isScrollable ? 14 : 0
                ScrollBar.vertical: CustomScrollBar {
                    id: appScrollBar
                    containerHovered: appUsageHover.hovered
                }
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                ListView {
                    id: percentageListView
                    model: sortedApps
                    spacing: 12
                    width: appScrollView.availableWidth

                    delegate: Item {
                        width: percentageListView.width
                        height: 48

                        property real targetPercentage: model.modelData.percentage
                        property real currentPercentage: 0



                        NumberAnimation on currentPercentage {
                            id: percentageAnim
                            from: 0
                            to: targetPercentage
                            duration: 700
                            easing.type: Easing.OutCubic
                            running: true
                        }

                        RowLayout {
                            anchors.fill: parent
                            spacing: 12


                            AppIconBadge {
                                Layout.preferredWidth: 28
                                Layout.preferredHeight: 28
                                badgeSize: 28
                                iconSize: 18
                                appName: modelData.name || ""
                                accentColor: modelData.productivityType === "non-productive"
                                             ? Theme.nonProductiveColor
                                             : (modelData.productivityType === "neutral" ? Theme.neutralColor : Theme.primaryColor)
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
                                                duration: 700
                                                easing.type: Easing.OutCubic
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
            Layout.preferredWidth: productivityRoot.isCompact ? parent.width * 0.21 : parent.width * 0.25
            Layout.minimumWidth: 130
            Layout.fillHeight: true
            spacing: productivityRoot.isCompact ? 8 : 12

            HoverHandler {
                id: webUsageHover
            }

            // Header untuk Domain
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 30

                    Label {
                        text: Lang.t("Website Usage")
                        font {
                            family: "Segoe UI"
                            weight: Font.DemiBold
                            pixelSize: productivityRoot.isCompact ? 14 : Theme.fontSizeTitle
                            letterSpacing: 0.2
                        }
                        color: Theme.textColor
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                    }
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
                id: domainScrollView
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                rightPadding: domainScrollBar.isScrollable ? 14 : 0
                ScrollBar.vertical: CustomScrollBar {
                    id: domainScrollBar
                    containerHovered: webUsageHover.hovered
                }
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                ListView {
                    id: domainsListView
                    model: sortedDomains
                    spacing: 12
                    width: domainScrollView.availableWidth

                    // Delegate untuk domainsListView (gunakan kode yang sudah dibuat dari jawaban sebelumnya
                    // yang sudah memiliki warna progress bar dinamis)
                    delegate: Item {
                        width: domainsListView.width
                        height: 48

                        property real targetPercentage: model.modelData.percentage
                        property real currentPercentage: 0

                        NumberAnimation on currentPercentage {
                            id: percentageAnim_
                            from: 0
                            to: targetPercentage
                            duration: 700
                            easing.type: Easing.OutCubic
                            running: true
                        }

                        RowLayout {
                            anchors.fill: parent
                            spacing: 12



                            // App icon placeholder
                            AppIconBadge {
                                Layout.preferredWidth: 28
                                Layout.preferredHeight: 28
                                badgeSize: 28
                                iconSize: 18
                                appName: modelData.name || ""
                                url: modelData.name || ""
                                accentColor: modelData.productivityType === "non-productive"
                                             ? Theme.nonProductiveColor
                                             : (modelData.productivityType === "neutral" ? Theme.neutralColor : Theme.primaryColor)
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
                                                duration: 700
                                                easing.type: Easing.OutCubic
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
                    Layout.preferredHeight: 30
                    spacing: 6

                    Label {
                        text: Lang.t("Productivity")
                        font {
                            family: "Segoe UI"
                            weight: Font.DemiBold
                            pixelSize: productivityRoot.isCompact ? 14 : Theme.fontSizeTitle
                            letterSpacing: 0.2
                        }
                        color: Theme.textColor
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                    }

                    CustomButton {
                        id: app
                        text: Lang.t("Show Applications")
                        implicitHeight: 26
                        leftPadding: productivityRoot.isCompact ? 7 : 10
                        rightPadding: productivityRoot.isCompact ? 7 : 10
                        btnRadius: Theme.radiusSmall
                        btnColor: "transparent"
                        btnHoverColor: Qt.alpha(Theme.accentColor, 0.1)
                        btnTextColor: Theme.accentColor
                        font.pixelSize: productivityRoot.isCompact ? 11 : 12
                        font.weight: Font.Medium
                        onClicked: {
                            applicationsDialog.setApps(logger.getProductivityApps());
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
                spacing: productivityRoot.isCompact ? 12 : 24
                Layout.fillWidth: true
                Layout.fillHeight: true
                // Combined Productivity Circle
                Item {
                    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                    Layout.preferredWidth: productivityRoot.isCompact ? 164 : 204
                    Layout.preferredHeight: productivityRoot.isCompact ? 164 : 204

                    Rectangle {
                        id: circleContainer
                        anchors.fill: parent
                        color: "transparent"

                        Canvas {
                            id: productivityCanvas
                            anchors.fill: parent
                            anchors.margins: 0
                            onWidthChanged: requestPaint()
                            onHeightChanged: requestPaint()

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
                                var outerRadius = Math.min(width, height) / 2 - (productivityRoot.isCompact ? 12 : 18)
                                var ringWidth = productivityRoot.isCompact ? 13 : 16
                                var innerRadius = outerRadius - ringWidth
                                var startAngle = -Math.PI / 2  // Fixed start position at top

                                // Background ring dengan efek subtle
                                ctx.beginPath()
                                ctx.arc(centerX, centerY, outerRadius, 0, 2 * Math.PI)
                                ctx.arc(centerX, centerY, innerRadius, 0, 2 * Math.PI, true)
                                ctx.fillStyle = Qt.rgba(0.95, 0.95, 0.95, 0)
                                ctx.fill()

                                // Glow effect untuk segmen aktif


                                // Fungsi untuk menggambar segmen donat dengan ujung membulat (rounded cap)
                                function drawRingSegment(segStart, angleSpan, color, useGradient = false) {
                                    if (angleSpan <= 0) return

                                    var animatedAngleSpan = angleSpan * animationProgress
                                    if (animatedAngleSpan <= 0.001) return

                                    var ringWidth = outerRadius - innerRadius
                                    var midRadius = (outerRadius + innerRadius) / 2
                                    var capInset = (ringWidth / 2) / midRadius

                                    // Clamp cap center inside [segStart, segStart + angleSpan] so tiny round dots keep uniform gaps
                                    var drawStart
                                    var drawEnd
                                    if (animatedAngleSpan <= 2 * capInset) {
                                        drawStart = segStart + Math.min(capInset, angleSpan * 0.5)
                                        drawEnd = drawStart + 0.001
                                    } else {
                                        drawStart = segStart + capInset
                                        drawEnd = segStart + animatedAngleSpan - capInset
                                    }

                                    ctx.beginPath()
                                    ctx.arc(centerX, centerY, midRadius, drawStart, drawEnd, false)
                                    ctx.lineWidth = ringWidth
                                    ctx.lineCap = "round"

                                    if (useGradient && animatedAngleSpan > 0) {
                                        var gradientStartX = centerX + midRadius * Math.cos(drawStart)
                                        var gradientStartY = centerY + midRadius * Math.sin(drawStart)
                                        var gradientEndX = centerX + midRadius * Math.cos(drawEnd)
                                        var gradientEndY = centerY + midRadius * Math.sin(drawEnd)

                                        var grad = ctx.createLinearGradient(gradientStartX, gradientStartY, gradientEndX, gradientEndY)
                                        grad.addColorStop(0, "" + color)
                                        grad.addColorStop(1, "" + Qt.lighter(color, 1.25))
                                        ctx.strokeStyle = grad
                                    } else {
                                        ctx.strokeStyle = color
                                    }

                                    ctx.stroke()
                                }

                                // Draw segments secara berurutan dengan gap konsisten termasuk untuk segmen kecil
                                var segments = []
                                if (productiveAngle > 0) segments.push({ angle: productiveAngle, color: Theme.productiveColor, useGradient: true })
                                if (nonProductiveAngle > 0) segments.push({ angle: nonProductiveAngle, color: Theme.nonProductiveColor, useGradient: false })
                                if (idleAngle > 0) segments.push({ angle: idleAngle, color: Theme.warningColor, useGradient: false })
                                if (neutralAngle > 0) segments.push({ angle: neutralAngle, color: Theme.neutralColor, useGradient: false })

                                var activeSegments = segments.length
                                if (activeSegments > 0) {
                                    var ringWidth = outerRadius - innerRadius
                                    var midRadius = (outerRadius + innerRadius) / 2
                                    var minVisualSpan = ringWidth / midRadius
                                    var segmentGap = activeSegments > 1 ? 0.06 : 0.0
                                    var availableAngle = 2 * Math.PI - (activeSegments > 1 ? activeSegments * segmentGap : 0.02)

                                    var totalAngle = 0
                                    for (var i = 0; i < activeSegments; i++) {
                                        totalAngle += segments[i].angle
                                    }

                                    if (totalAngle > 0) {
                                        var targetTotal = Math.min(totalAngle, availableAngle)
                                        var effMinSpan = Math.min(minVisualSpan, availableAngle / activeSegments)
                                        var isFixed = []
                                        for (var j = 0; j < activeSegments; j++) {
                                            isFixed.push(false)
                                        }

                                        var changed = true
                                        while (changed) {
                                            changed = false
                                            var fixedSum = 0
                                            var flexRawSum = 0
                                            for (var k = 0; k < activeSegments; k++) {
                                                if (isFixed[k]) fixedSum += effMinSpan
                                                else flexRawSum += segments[k].angle
                                            }
                                            var remainingBudget = Math.max(0, targetTotal - fixedSum)
                                            if (flexRawSum > 0) {
                                                for (var m = 0; m < activeSegments; m++) {
                                                    if (!isFixed[m] && (segments[m].angle / flexRawSum) * remainingBudget < effMinSpan) {
                                                        isFixed[m] = true
                                                        changed = true
                                                    }
                                                }
                                            }
                                        }

                                        var finalFixedSum = 0
                                        var finalFlexRawSum = 0
                                        for (var p = 0; p < activeSegments; p++) {
                                            if (isFixed[p]) finalFixedSum += effMinSpan
                                            else finalFlexRawSum += segments[p].angle
                                        }
                                        var flexBudget = Math.max(0, targetTotal - finalFixedSum)

                                        var currentStartAngle = startAngle
                                        for (var q = 0; q < activeSegments; q++) {
                                            var segSpan = isFixed[q]
                                                ? Math.min(effMinSpan, targetTotal / activeSegments)
                                                : (finalFlexRawSum > 0 ? (segments[q].angle / finalFlexRawSum) * flexBudget : 0)
                                            drawRingSegment(currentStartAngle, segSpan, segments[q].color, segments[q].useGradient)
                                            currentStartAngle += segSpan + segmentGap
                                        }
                                    }
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
                                ctx.font = "bold " + (productivityRoot.isCompact ? "26px" : "32px") + " 'Segoe UI', system-ui, -apple-system"
                                ctx.fillText(progressPercent + "%", 0, 0)
                                ctx.restore()

                                // Subtitle dengan fade-in effect
                                ctx.font = "600 " + (productivityRoot.isCompact ? "11px" : "13px") + " 'Segoe UI', system-ui, -apple-system"
                                ctx.fillStyle = Qt.alpha(Theme.primaryColor, 0.8 * animationProgress)
                                ctx.fillText(Lang.t("Productive"), centerX, centerY + 18)

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
                        var stats = logger.productivityStats
                        // Calculate angles based on percentages
                        var productive = stats.productive || 0
                        var nonProductive = stats.nonProductive || 0
                        var neutral = stats.neutral || 0
                        var idle = stats.idle || 0

                        // Normalize if total exceeds 100%
                        var total = productive + nonProductive + neutral + idle
                        if (total > 100) {
                            productive = (productive / total) * 100
                            nonProductive = (nonProductive / total) * 100
                            neutral = (neutral / total) * 100
                            idle = (idle / total) * 100
                        }

                        if (Math.abs(chartAnimator.productiveTarget - productive) < 0.1 &&
                            Math.abs(chartAnimator.nonProductiveTarget - nonProductive) < 0.1 &&
                            Math.abs(chartAnimator.neutralTarget - neutral) < 0.1 &&
                            Math.abs(chartAnimator.idleTarget - idle) < 0.1) {
                            return
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

                Connections {
                    target: Theme
                    function onIsDarkModeChanged() {
                        productivityCanvas.requestPaint()
                    }
                }

                Connections {
                    target: Lang
                    function onCurrentLanguageChanged() {
                        productivityCanvas.requestPaint()
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
                    spacing: productivityRoot.isCompact ? 8 : 10
                    Layout.alignment: Qt.AlignVCenter
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumWidth: 165

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
                            text: Lang.t("Productive")
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
                            text: Lang.t("Non-Productive")
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
                            text: Lang.t("Idle")
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
                            text: Lang.t("Neutral")
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
