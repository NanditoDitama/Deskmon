import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material
import QtQuick.Dialogs
import QtQuick.Effects
import QtQuick.Window
import "theme"
import "components"
import "dialogs"
import "layouts"
import "login"
import "dashboard"
import "application_submission"

Window {
    id: window
    title: qsTr("Deskmon - v" + appVersion)
    visibility: Window.Maximized
    minimumWidth: 900
    minimumHeight: 700
    property string appVersion: (typeof logger !== "undefined" && logger.appVersion) ? logger.appVersion : (Qt.application.version || "")
    color: Theme.cardColor

    // Sinkronisasi tema Material dengan mode singleton Theme
    Material.theme: Theme.isDarkMode ? Material.Dark : Material.Light

    onClosing: function(close) {
        console.log("Tombol 'X' ditekan. Menyembunyikan window ke tray.")
        close.accepted = false
        window.hide()
    }

    // =========================================================================
    // Application-Level State
    // =========================================================================
    property bool isLoggedIn: false
    property string currentUsername: ""
    property bool showIdleNotification: false
    property string idleNotificationText: ""

    Component.onCompleted: {
        if (typeof Application !== "undefined" && Application.styleHints) {
            Theme.isDarkMode = Application.styleHints.colorScheme === Qt.ColorScheme.Dark
        } else {
            Theme.isDarkMode = Material.theme === Material.Dark
        }

        if (typeof logger !== "undefined" && logger.currentUserId === -1) {
            isLoggedIn = false
            console.log("Ke halaman login untuk login ulang")
        }
        window.visibility = Window.Maximized
    }

    function showEarlyLeaveDialog() {
        earlyLeaveReasonDialog.show()
    }

    function showTaskDetailsDialog(taskId, action, nextTaskId) {
        taskDetailsDialog.show(taskId, action, nextTaskId)
    }

    function showReviewNotification(message) {
        if (window.visibility === Window.Windowed || window.visibility === Window.Maximized) {
            reviewNotificationPopup.showNotification(message)
        }
    }

    function goToLoginPage() {
        isLoggedIn = false
        console.log("Navigasi ke halaman login")
    }

    function showSystemTrayNotification(title, message) {
        if (typeof SystemTrayIcon !== 'undefined' && SystemTrayIcon.supportsMessages) {
            SystemTrayIcon.showMessage(title, message)
        }
    }

    // =========================================================================
    // Global Toast & System Tray Notifications
    // =========================================================================
    ToastNotification {
        id: notification
        parent: Overlay.overlay
    }

    Connections {
        target: (typeof logger !== "undefined") ? logger : null

        function onShowNotification(type, message) {
            notification.show(type, message)
        }

        function onRequestLoginPage() {
            isLoggedIn = false
            showIdleNotification = false
            console.log("Sesi berakhir, meminta login ulang")
        }

        function onTaskReviewNotification(message) {
            console.log("Review notification:", message)
            showSystemTrayNotification("Task Review", message)
            notification.show("warning", message)
        }

        function onRequestTaskDetails(taskId, action, nextTaskId) {
            taskDetailsDialog.show(taskId, action, nextTaskId)
        }

        function onTaskDetailsSubmissionSuccess() {
            taskDetailsDialog.isLoading = false
            taskDetailsDialog.close()
        }

        function onTaskDetailsSubmissionFailed(errorMessage) {
            taskDetailsDialog.isLoading = false
        }

        function onTaskDetailsSubmissionFailedWithRetry(errorMessage, taskId, details, action, nextTaskId) {
            taskDetailsDialog.isLoading = false
            taskDetailsDialog.close()
            failureDialog.show(errorMessage, taskId, details, action, nextTaskId)
        }

        function onShowAuthTokenErrorWindow(message) {
            showIdleNotification = false
            if (idleNotificationWindow.visible) {
                idleNotificationWindow.close()
            }
            if (isLoggedIn) {
                console.log("Token expired, showing auth error window.")
                authErrorWindow.errorMessage = Lang.t(message)
                authErrorWindow.visible = true
                authErrorWindow.raise()
            }
        }

        function onReadyToProceedWithLogout() {
            console.log("Processing logout cleanup...")
            taskDetailsDialog.close()
            taskDetailsDialog.visible = false
            authErrorWindow.visible = false
            authErrorWindow.close()
            logger.logout()
            isLoggedIn = false
            currentUsername = ""
            logger.clearLogFilter()
        }

        function onShowPingErrorDialog(message) {
            var translated = Lang.t(message)
            if (translated === message && typeof message === "string" && message.indexOf("Koneksi gagal: ") === 0) {
                translated = Lang.t("Connection failed: ") + message.substring("Koneksi gagal: ".length)
            }
            pingErrorDialog.errorMessage = translated
            pingErrorDialog.open()
        }

        function onHidePingErrorDialog() {
            pingErrorDialog.close()
        }
    }

    Connections {
        target: (typeof idleChecker !== "undefined") ? idleChecker : null

        function onShowIdleNotification(message) {
            idleNotificationText = message
            showIdleNotification = true
            if (window.visibility === Window.Minimized) {
                showSystemTrayNotification("Idle Detected", message)
            }
        }

        function onHideIdleNotification() {
            showIdleNotification = false
        }
    }

    // =========================================================================
    // Global Application Dialogs
    // =========================================================================
    TaskDetailsDialog {
        id: taskDetailsDialog
    }

    FailureDialog {
        id: failureDialog
    }

    EarlyLeaveDialog {
        id: earlyLeaveReasonDialog
    }

    NeedReviewDialog {
        id: needReviewReasonDialog
    }

    IdleNotificationWindow {
        id: idleNotificationWindow
        visible: showIdleNotification
        onResumeRequested: {
            showIdleNotification = false
            if (typeof logger !== "undefined" && logger.isTaskPaused)
                logger.toggleTaskPause()
        }
        onDismissRequested: {
            showIdleNotification = false
        }
    }

    AuthErrorWindow {
        id: authErrorWindow
        onLoginRedirectRequested: {
            isLoggedIn = false
            if (typeof logger !== "undefined")
                logger.logout()
        }
    }

    PingErrorDialog {
        id: pingErrorDialog
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

        background: Rectangle {
            color: Theme.cardColor
            radius: 12
            border.color: Qt.alpha(Theme.warningColor, 0.25)
            border.width: 1
        }

        contentItem: RowLayout {
            spacing: 12
            anchors.fill: parent
            anchors.margins: 16

            Rectangle {
                width: 36
                height: 36
                radius: 18
                color: Qt.alpha(Theme.warningColor, 0.12)
                Layout.alignment: Qt.AlignVCenter

                Image {
                    anchors.centerIn: parent
                    width: 20
                    height: 20
                    source: "qrc:/icon.ico"
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 4

                Label {
                    text: Lang.t("Task Review Reminder")
                    font.pixelSize: 15
                    font.weight: Font.DemiBold
                    color: Theme.textColor
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }

                Label {
                    id: reviewNotificationText
                    text: ""
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                    color: Qt.alpha(Theme.textColor, 0.75)
                    font.pixelSize: 12
                    maximumLineCount: 2
                    elide: Text.ElideRight
                }
            }

            Button {
                Layout.alignment: Qt.AlignTop
                Layout.preferredWidth: 28
                Layout.preferredHeight: 28
                padding: 0
                flat: true
                onClicked: reviewNotificationPopup.close()
                contentItem: Text {
                    text: "×"
                    font.pixelSize: 18
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: Qt.alpha(Theme.textColor, 0.6)
                }
            }
        }

        function showNotification(message) {
            reviewNotificationText.text = message
            open()
            reviewNotificationTimer.restart()
        }

        Timer {
            id: reviewNotificationTimer
            interval: 10000
            onTriggered: reviewNotificationPopup.close()
        }
    }

    // =========================================================================
    // Page Views (Login, Profile, Dashboard)
    // =========================================================================

    // 1. Halaman Login
    Rectangle {
        anchors.fill: parent
        color: Theme.backgroundColor
        visible: !isLoggedIn

        LoginPage {
            anchors.fill: parent
        }
    }

    // 2. Halaman Dashboard Utama
    Rectangle {
        anchors.fill: parent
        color: Theme.backgroundColor
        visible: isLoggedIn

        DashboardPage {
            username: currentUsername
            onLogoutRequested: {
                if (typeof logger !== "undefined" && logger.activeTaskId && logger.activeTaskId > 0) {
                    console.log("Task aktif terdeteksi (ID: " + logger.activeTaskId + "). Membuka dialog detail.")
                    taskDetailsDialog.show(logger.activeTaskId, "logout", -1)
                } else {
                    console.log("Tidak ada task aktif. Langsung logout.")
                    if (typeof logger !== "undefined")
                        logger.taskDetailsDialogClosed("logout")
                }
            }
            onNeedReviewRequested: function(taskId) {
                needReviewReasonDialog.openWithTaskId(taskId, logger)
            }
        }
    }
}
