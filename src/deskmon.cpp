 #include "deskmon.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QDebug>
#include <QProcess>
#include <QFileInfo>
#include <QCryptographicHash>
#include <QImage>
#include <QDir>
#include <QUrl>
#include <QPainter>
#include <QFileIconProvider>
#include <QStandardPaths>
#include <QJsonArray>
#include <QVariant>
#include <QBuffer>
#include <QRegularExpression>
#include <QMessageBox>
#include <QApplication>

#include <QTemporaryFile>

#ifdef Q_OS_WIN
#include <windows.h>
#include <psapi.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <UIAutomation.h>

#elif defined(Q_OS_MAC)
#include <AppKit/AppKit.h>
#include <Foundation/Foundation.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#include <Carbon/Carbon.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOKitKeys.h>
#include <IOKit/IOKitLib.h>
#elif defined(Q_OS_LINUX)
// X11 and desktop environment
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/shape.h>

// System and process management
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <sys/statvfs.h>
#include <signal.h>
#include <fcntl.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>

// File and directory operations
#include <filesystem>
#include <fstream>
#include <iostream>

// GTK/GLib (if using GTK integration)
#ifdef HAVE_GTK
#include <gtk/gtk.h>
#include <gio/gio.h>
#include <glib.h>
#endif

// D-Bus (for desktop notifications and system integration)
#ifdef HAVE_DBUS
#include <dbus/dbus.h>
#endif

// For desktop file handling and MIME types
#include <magic.h>
#include <mntent.h>

// Network and system info
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Memory and CPU info
#include <sys/resource.h>
#include <sys/times.h>

// For file system monitoring (inotify)
#include <sys/inotify.h>

// Audio system (if needed)
#ifdef HAVE_PULSEAUDIO
#include <pulse/pulseaudio.h>
#endif

#ifdef HAVE_ALSA
#include <alsa/asoundlib.h>
#endif

// Threading
#include <pthread.h>

// Standard C libraries commonly used on Linux
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <climits>
#else
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "uiautomationcore.lib")

Deskmon::Deskmon(QObject *parent) : QObject(parent)
{
    m_dataManager = new DataManager(this);
    m_windowManager = new WindowManager(this);
    m_apiManager = new ApiManager(this);
    m_sessionManager = new SessionManager(this);
    m_sessionManager->setDataManager(m_dataManager);
    m_taskController = new TaskController(this);

    checkTaskStatusBeforeStart();

    m_productiveAppsModel = new QSqlQueryModel(this);
    m_nonProductiveAppsModel = new QSqlQueryModel(this);
    refreshProductivityModels();

    m_isTrackingActive = true;

    m_pingTimer.setInterval(30000); // 30 detik
    connect(&m_pingTimer, &QTimer::timeout, this, [this]() {
        if (m_currentUserId != -1) {
            if (m_activeTaskId != -1 && !m_isTaskPaused) {
                sendPing(m_activeTaskId);
            } else {
                sendPing(-1);
            }
        }
    });

    m_apiWorkTimeTimer.setInterval(60000); // 1 menit
    connect(&m_apiWorkTimeTimer, &QTimer::timeout, this, &Deskmon::fetchWorkTimeFromAPI);

    fetchWorkTimeFromAPI(); 
    m_apiWorkTimeTimer.start();

    m_productivePingTimer.setInterval(180000); // 3 menit
    connect(&m_productivePingTimer, &QTimer::timeout, this, &Deskmon::sendProductiveTimeToAPI);
    m_productivePingTimer.start();

    m_usageReportTimer.setInterval(300000); // 5 menit
    connect(&m_usageReportTimer, &QTimer::timeout, this, &Deskmon::sendDailyUsageReport);

    m_taskRefreshTimer.setInterval(180000);
    connect(&m_taskRefreshTimer, &QTimer::timeout, this, &Deskmon::refreshTasks);
    m_taskRefreshTimer.start();

    m_lastShownPingError.clear();
    m_pingRetryCount = 0;

    m_logModel = new QSqlQueryModel(this);

    // Manager signal connections
    connect(m_sessionManager, &SessionManager::loginResult, this, [this](bool success, const QString &msg, int id, const QString &user, const QString &email, const QString &token) {
        if (success) {
            m_authToken = token;
            setCurrentUserInfo(id, user, email);
            refreshAll();
        }
        emit loginCompleted(success, msg);
    });

    connect(m_apiManager, &ApiManager::tasksFetched, this, [this](const QJsonArray &tasks) {
        // Logic to store tasks in DB
        emit taskListChanged();
    });

    connect(m_apiManager, &ApiManager::productivityAppsFetched, this, [this](const QJsonArray &apps) {
        m_dataManager->syncProductivityApps(apps, m_currentUserId);
        refreshProductivityModels();
        updateProductivityCache();
        emit productivityAppsChanged();
    });

    connect(m_taskController, &TaskController::activeTaskChanged, this, [this](int id) {
        m_activeTaskId = id;
        emit activeTaskChanged();
    });
}

Deskmon::~Deskmon()
{
    saveWorkTimeData();
    sendWorkTimeToAPI();
}

void Deskmon::notify(const QString &type, const QString &message) {
    emit showNotification(type, message);
}

void Deskmon::startGlobalTimer()
{
    m_globalTimeUsage = 0;
    emit globalTimeUsageChanged();
}

bool Deskmon::ensureDatabaseOpen() const
{
    return m_dataManager->ensureActivityDbOpen();
}

bool Deskmon::ensureProductivityDatabaseOpen() const
{
    return m_dataManager->ensureProductivityDbOpen();
}




void Deskmon::showAuthTokenErrorMessage()
{
    if (m_isTokenErrorVisible) {
        return;
    }
    m_isTokenErrorVisible = true;

    // emit sinyal ke QML
    emit showAuthTokenErrorWindow("Sesi Anda telah berakhir atau tidak valid.\nSilakan login ulang untuk melanjutkan.");

    // Setelah user menutup jendela di QML, QML bisa panggil slot logout()
    // supaya keluar otomatis.
}

void Deskmon::refreshAll()
{
    if (m_currentUserId == -1) return;

    fetchAndStoreTasks();
    fetchAndStoreProductivityApps();
    m_taskController->syncActiveTask(m_currentUserId);
    
    emit taskListChanged();
    emit logContentChanged();
    emit productivityStatsChanged();
}


void Deskmon::refreshTasks()
{
    m_taskController->refreshTasks(m_currentUserId);
    emit taskListChanged();
}





// Tambahkan di dalam file logger.cpp


// Updated calculateTodayProductiveSeconds function


// Updated productivityStats function
// Updated productivityStats function




// HAPUS fungsi productivityStats yang lama di logger.cpp, lalu GANTI dengan yang ini.

// Tambahkan fungsi ini di logger.cpp


// logger.cpp

// logger.cpp


// Update/Ganti fungsi isTaskFromPreviousMonth menjadi isTaskExpired

// logger.cpp

// Temukan fungsi ini dan ubah isinya

// logger.cpp dalam fungsi Deskmon::sendPing
// logger.cpp dalam fungsi Deskmon::sendPing


// void Deskmon::stopPingTimer()
// {
//     m_pingTimer.stop();
//     qDebug() << "Stopped ping timer";
// }


//void Deskmon::updateTaskTime()
//{
//    m_globalTimeUsage += 1;
//    emit globalTimeUsageChanged();
//
//    if (m_isTaskPaused || m_activeTaskId == -1 || !ensureProductivityDatabaseOpen()) {
//        return;
//    }
//
//    QSqlQuery query(m_dataManager->productivityDb());
//    query.prepare("SELECT user_id, status FROM task WHERE id = :id");
//    query.bindValue(":id", m_activeTaskId);
//    if (!query.exec() || !query.next()) {
//        qWarning() << "Task not found or invalid task ID:" << m_activeTaskId;
//        return;
//    }
//    if (query.value(0).toInt() != m_currentUserId) {
//        qWarning() << "Task ID" << m_activeTaskId << "does not belong to current user:" << m_currentUserId;
//        return;
//    }
//
//    QString status = query.value(1).toString().toLower();
//    if (status == "review") {
//        qDebug() << "Skipping time update for task ID" << m_activeTaskId << "in Review status";
//        return;
//    }
//
//    qint64 currentTime = QDateTime::currentSecsSinceEpoch();
//    qint64 timeUsage = m_taskTimeOffset + (currentTime - m_taskStartTime);
//
//    query.prepare("UPDATE task SET time_usage = :timeUsage WHERE id = :id");
//    query.bindValue(":timeUsage", timeUsage);
//    query.bindValue(":id", m_activeTaskId);
//    if (!query.exec()) {
//        qWarning() << "Failed to update task time_usage:" << query.lastError().text();
//        return;
//    }
//
//    emit taskListChanged();
//}

// Helper function untuk set current user info dan emit signals
QString Deskmon::statusMessage() const
{
    return m_statusMessage;
}

// 1. Fungsi yang dipanggil QML saat tombol "OK" di dialog ditekan

// 2. Fungsi untuk mengirim data ke server (endpoint API perlu disesuaikan)
// Di logger.cpp


// 2. Tambahkan implementasi fungsi utama






