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
import "profile"
import "dashboard"
import "application_submission"

Window {
    id: window
    title: qsTr("Deskmon - v" + appVersion)
    visibility: Window.Maximized
    minimumWidth: 900
    minimumHeight: 700
    property string appVersion: "1.0.3.4"
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
    property bool isProfileVisible: false
    property string currentUsername: ""
    property string profileImagePath: ":/profilImage.png"
    property bool showIdleNotification: false
    property string idleNotificationText: ""

    Component.onCompleted: {
        if (typeof Qt.styleHints !== "undefined") {
            Theme.isDarkMode = Qt.styleHints.colorScheme === Qt.Dark
        } else {
            Theme.isDarkMode = Material.theme === Material.Dark
        }

        if (typeof logger !== "undefined" && logger.currentUserId === -1) {
            isLoggedIn = false
            console.log("Ke halaman login untuk login ulang")
        }
        window.visibility = Window.Maximized
    }

    function goToLoginPage() {
        isLoggedIn = false
        isProfileVisible = false
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
            isProfileVisible = false
            showIdleNotification = false
            console.log("Sesi berakhir, meminta login ulang")
        }

        function onProfileImageChanged(username, newPath) {
            if (username === currentUsername) {
                console.log("Profile image changed for", username, "to", newPath)
                profileImagePath = newPath
            }
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
            taskDetailsDialog.loadingIndicator.running = false
            taskDetailsDialog.close()
        }

        function onTaskDetailsSubmissionFailedWithRetry(errorMessage, taskId, details, action, nextTaskId) {
            taskDetailsDialog.loadingIndicator.running = false
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
                authErrorWindow.errorMessage = message
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
            profileImagePath = ":/profilImage.png"
            logger.clearLogFilter()
        }

        function onShowPingErrorDialog(message) {
            pingErrorDialog.errorMessage = message
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
            isProfileVisible = false
            if (typeof logger !== "undefined")
                logger.logout()
        }
    }

    PingErrorDialog {
        id: pingErrorDialog
    }

    // =========================================================================
    // Page Views (Login, Profile, Dashboard)
    // =========================================================================

    // 1. Halaman Login
    Rectangle {
        anchors.fill: parent
        color: Theme.backgroundColor
        visible: !isLoggedIn && !isProfileVisible

        LoginPage {
            anchors.fill: parent
        }
    }

    // 2. Halaman Profil
    Rectangle {
        anchors.fill: parent
        color: Theme.backgroundColor
        visible: isProfileVisible
        opacity: isProfileVisible ? 1 : 0
        Behavior on opacity {
            NumberAnimation { duration: 300 }
        }

        ProfilePage {
            anchors.fill: parent
        }

        Label {
            id: profileErrorLabel
            text: ""
            color: profileErrorLabel.text.includes("success") ? Theme.successColor : Theme.dangerColor
            font.pixelSize: Theme.fontSizeSmall
            Layout.alignment: Qt.AlignHCenter
            visible: text !== ""
        }
    }

    // 3. Halaman Dashboard Utama
    Rectangle {
        anchors.fill: parent
        color: Theme.backgroundColor
        visible: isLoggedIn && !isProfileVisible

        DashboardPage {
            username: currentUsername
            onProfileRequested: {
                profileErrorLabel.text = ""
                isProfileVisible = true
            }
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
