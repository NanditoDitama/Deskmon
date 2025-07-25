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
                DateRange{
                    id: dateRangeDialog
                    parent: Overlay.overlay
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
                    Monitored_Applications{
                        id:applicationsDialog
                        parent:Overlay.overlay
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
