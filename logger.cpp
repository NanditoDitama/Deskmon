#include "logger.h"
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

#else
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "uiautomationcore.lib")

Logger::Logger(QObject *parent) : QObject(parent)
{
    initializeDatabase();
    initializeProductivityDatabase();
    checkTaskStatusBeforeStart();

    m_productiveAppsModel = new QSqlQueryModel(this);
    m_nonProductiveAppsModel = new QSqlQueryModel(this);
    m_productiveAppsModel->setQuery("SELECT aplikasi AS appName, window_title AS windowTitle, jenis AS type FROM aplikasi WHERE jenis = 1", m_productivityDb);
    m_nonProductiveAppsModel->setQuery("SELECT aplikasi AS appName, window_title AS windowTitle, jenis AS type FROM aplikasi WHERE jenis = 2", m_productivityDb);

    //m_taskTimer.setInterval(1000);
    //connect(&m_taskTimer, &QTimer::timeout, this, &Logger::updateTaskTime);
    //m_taskTimer.start();
    m_isTrackingActive = true;
    m_networkManager = new QNetworkAccessManager(this);

    m_pingTimer.setInterval(30000); // 1 menit
    connect(&m_pingTimer, &QTimer::timeout, this, [this]() {
        if (m_activeTaskId != -1 && !m_isTaskPaused) {
            sendPing(m_activeTaskId);
        }
    });

    // Inisialisasi dan mulai timer untuk "Time at Work"
    connect(&m_workTimer, &QTimer::timeout, this, &Logger::updateWorkTimeAndSave);
    m_workTimer.start(1000); // 1 detik
    m_workTimer.start();
        // Update setiap detik

    m_productivePingTimer.setInterval(180000); // 3 menit = 180000 ms
    connect(&m_productivePingTimer, &QTimer::timeout, this, &Logger::sendProductiveTimeToAPI);
    m_productivePingTimer.start();

    m_usageReportTimer.setInterval(300000); // 5 menit = 300,000 ms
    connect(&m_usageReportTimer, &QTimer::timeout, this, &Logger::sendDailyUsageReport);

    m_taskRefreshTimer.setInterval(180000);
    connect(&m_taskRefreshTimer, &QTimer::timeout, this, &Logger::refreshTasks);
    m_taskRefreshTimer.start();


}
Logger::~Logger()
{
    // Pastikan data terakhir disimpan sebelum aplikasi ditutup
    saveWorkTimeData();
    sendWorkTimeToAPI();
    if (m_db.isOpen()) {
        m_db.close();
    }
    if (m_productivityDb.isOpen()) {
        m_productivityDb.close();
    }
    delete m_productiveAppsModel;
    delete m_nonProductiveAppsModel;
}

// Implementasi getter untuk properti baru
int Logger::workTimeElapsedSeconds() const
{
    return m_workTimeElapsedSeconds;
}


void Logger::startGlobalTimer()
{
    m_globalTimeUsage = 0;
    emit globalTimeUsageChanged();
}

bool Logger::ensureDatabaseOpen() const
{
    if (!m_db.isOpen()) {
        if (!m_db.open()) {
            qWarning() << "Failed to reopen activity database:" << m_db.lastError().text();
            return false;
        }
        qDebug() << "Activity database reopened successfully";
    }
    return true;
}

bool Logger::ensureProductivityDatabaseOpen() const
{
    if (!m_productivityDb.isOpen()) {
        if (!m_productivityDb.open()) {
            qWarning() << "Failed to reopen productivity database:" << m_productivityDb.lastError().text();
            return false;
        }
        qDebug() << "Productivity database reopened successfully";
    }
    return true;
}


void Logger::showAuthTokenErrorMessage()
{
    // Cek agar pesan tidak muncul bertumpuk jika beberapa API call gagal bersamaan
    if (m_isTokenErrorVisible) {
        return;
    }
    m_isTokenErrorVisible = true; // Set flag bahwa pesan sedang ditampilkan

    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setWindowTitle("Sesi Berakhir");
    msgBox.setText("Sesi Anda telah berakhir atau tidak valid.\nSilakan login ulang untuk melanjutkan.");
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec(); // Menampilkan pesan dan menunggu pengguna menekan OK

    // Setelah pengguna menekan OK, panggil logout
    logout();
}

void Logger::refreshAll()
{

    bool wasPaused = m_isTaskPaused;
    int activeTaskBeforeRefresh = m_activeTaskId;

    if (!ensureDatabaseOpen() || !ensureProductivityDatabaseOpen()) {
        qWarning() << "Cannot refresh: One or both databases are not open";
        return;
    }

    if (m_currentUserId == -1) {
        qWarning() << "Cannot refresh: No user logged in";
        return;
    }

    qDebug() << "Starting system refresh for user_id:" << m_currentUserId;

    // 1. Sinkronkan data dari server (hanya sekali untuk setiap sumber data)
    qDebug() << "Fetching tasks and productivity apps from server...";
    fetchAndStoreTasks();
    fetchAndStoreProductivityApps();

    {
        QSqlQuery query(m_productivityDb);
        query.prepare("SELECT id FROM task WHERE user_id = :user_id AND status = 'on-progress' LIMIT 1");
        query.bindValue(":user_id", m_currentUserId);
        if (query.exec() && query.next()) {
            int taskId = query.value(0).toInt();
            setActiveTask(taskId);
            m_taskStartTime = QDateTime::currentSecsSinceEpoch();

            QSqlQuery timeQuery(m_productivityDb);
            timeQuery.prepare("SELECT time_usage FROM task WHERE id = :id");
            timeQuery.bindValue(":id", taskId);
            if (timeQuery.exec() && timeQuery.next()) {
                m_taskTimeOffset = timeQuery.value(0).toInt();
            }

            qDebug() << "Task with status 'on-progress' activated. Task ID:" << taskId;
        }
    }

    // 2. Sinkronkan ulang status internal aplikasi dari database LOKAL yang sudah diperbarui.
    // Ini menggantikan logika pemulihan status yang keliru dan panggilan syncActiveTask() yang berlebihan.
    QSqlQuery query(m_productivityDb);
    query.prepare("SELECT id, paused, time_usage FROM task WHERE user_id = :user_id AND active = 1");
    query.bindValue(":user_id", m_currentUserId);

    if (!query.exec()) {
        qWarning() << "Failed to query for active task during refresh:" << query.lastError().text();
    } else {
        if (query.next()) {
            // Tugas aktif ditemukan di database, perbarui status internal.
            int newActiveTaskId = query.value(0).toInt();
            bool newPausedState = query.value(1).toBool();

            if (m_activeTaskId != newActiveTaskId) {
                qDebug() << "Active task has changed via sync. Old:" << m_activeTaskId << "New:" << newActiveTaskId;
                m_activeTaskId = newActiveTaskId;
            }

            m_isTaskPaused = newPausedState;
            m_taskTimeOffset = query.value(2).toInt();
            if (!m_isTaskPaused) {
                m_taskStartTime = QDateTime::currentSecsSinceEpoch();
            }
            qDebug() << "Internal state synchronized. Active Task ID:" << m_activeTaskId << "Paused:" << m_isTaskPaused;

        } else {
            // Tidak ada tugas aktif yang ditemukan, reset status internal.
            if (m_activeTaskId != -1) {
                qDebug() << "Previously active task" << m_activeTaskId << "is no longer active after sync.";
                m_activeTaskId = -1;
                m_isTaskPaused = false;
                m_taskTimeOffset = 0;
                m_taskStartTime = 0;
            }
        }
    }
    m_isTaskPaused = wasPaused;
    m_activeTaskId = activeTaskBeforeRefresh;

    // 3. Perbarui info jendela yang sedang aktif.
    logActiveWindow();

    // 4. Pancarkan sinyal untuk memberitahu UI agar memperbarui tampilannya.
    emit taskListChanged();
    emit logContentChanged();
    emit logCountChanged();
    emit productivityStatsChanged();
    emit currentAppNameChanged();
    emit currentWindowTitleChanged();
    emit globalTimeUsageChanged();
    emit trackingActiveChanged();
    emit taskPausedChanged();
    emit activeTaskChanged();

    qDebug() << "Refresh all completed.";
}


void Logger::refreshTasks()
{
    if (m_currentUserId == -1 || !ensureProductivityDatabaseOpen()) {
        qDebug() << "Skipping task refresh - no user logged in or database not open";
        return;
    }

    qDebug() << "Refreshing tasks...";

    // 1. Sync tasks from server
    fetchAndStoreTasks();

    // 2. Update status of all tasks
    QSqlQuery query(m_productivityDb);
    query.prepare("SELECT id FROM task WHERE user_id = :user_id");
    query.bindValue(":user_id", m_currentUserId);

    if (query.exec()) {
        while (query.next()) {
            int taskId = query.value(0).toInt();
            updateTaskStatus(taskId);
        }
    } else {
        qWarning() << "Failed to fetch tasks for refresh:" << query.lastError().text();
    }

    // 3. Sync active task
    syncActiveTask();

    emit taskListChanged();
    qDebug() << "Task refresh completed";
}

void Logger::sendLogoutToAPI()
{
    if (m_currentUserId == -1 || m_authToken.isEmpty()) {
        qWarning() << "Cannot send logout to API: No user logged in or no auth token";
        return;
    }

    QJsonObject payload;

    QEventLoop loop;
    QNetworkRequest request(QUrl("https://deskmon.pranala-dt.co.id/api/logout"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_authToken.toUtf8());

    qDebug() << "Sending logout to API:" << QJsonDocument(payload).toJson();

    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(QJsonObject()).toJson());
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QTimer::singleShot(5000, &loop, &QEventLoop::quit); // timeout jaga-jaga
    loop.exec();

    if (reply->error() == QNetworkReply::NoError) {
        qDebug() << "Logout success:" << reply->readAll();
    } else {
        qWarning() << "Logout failed:" << reply->errorString();
    }
    reply->deleteLater();
}

void Logger::logout()
{
    saveWorkTimeData();
    sendWorkTimeToAPI();
    sendLogoutToAPI();
    m_workTimer.stop();
    m_taskRefreshTimer.stop();

    // Stop all active tracking
    if (m_activeTaskId != -1) {
        setActiveTask(-1); // This will stop the current task
    }

    if (m_activeTaskId != -1 && !m_isTaskPaused) {
        QString currentTime = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
        sendPausePlayDataToAPI(
            m_activeTaskId,
            m_lastPlayStartTime.toString(Qt::ISODateWithMs),
            currentTime,
            "pause"
            );
        qDebug() << "Sent pause status to server before logout. From" << m_lastPlayStartTime << "to" << currentTime;
    }




    // Stop all timers
    m_taskTimer.stop();
    m_pingTimer.stop();

    // Reset tracking state
    m_isTrackingActive = false;
    m_isTaskPaused = true;
    m_activeTaskId = -1;
    m_taskTimeOffset = 0;
    m_taskStartTime = 0;
    m_pauseStartTime = 0;

    // Clear user data
    m_currentUserId = -1;
    m_currentUsername.clear();
    m_currentUserEmail.clear();
    m_userEmail.clear();
    m_authToken.clear();
    QSqlQuery clearTokenQuery(m_db);
    clearTokenQuery.prepare("UPDATE users SET token = '' WHERE id = :id");
    clearTokenQuery.bindValue(":id", m_currentUserId);
    clearTokenQuery.exec();

    m_usageReportTimer.stop();
    qDebug() << "Daily usage report timer stopped.";
    sendDailyUsageReport();


    // Emit signals to update UI
    emit activeTaskChanged();
    emit taskPausedChanged();
    emit trackingActiveChanged();
    emit currentUserIdChanged();
    emit currentUsernameChanged();
    emit currentUserEmailChanged();
    emit userEmailChanged();
    emit taskListChanged();



    qDebug() << "User logged out, all tracking stopped";
}

bool Logger::validateFilePath(const QString &filePath)
{
    QString localPath = filePath;
    if (localPath.startsWith("file:///")) {
        localPath = QUrl(filePath).toLocalFile();
    } else if (localPath.startsWith("file://")) {
        localPath = localPath.mid(7);
    }

    QFileInfo fileInfo(localPath);
    if (!fileInfo.exists()) {
        qWarning() << "File does not exist:" << localPath;
        return false;
    }
    if (!fileInfo.isFile() || !fileInfo.isReadable()) {
        qWarning() << "File is not a valid file or is not readable:" << localPath;
        return false;
    }

    QImage image(localPath);
    if (image.isNull()) {
        qWarning() << "File is not a valid image:" << localPath;
        return false;
    }

    qDebug() << "File validated successfully:" << localPath;
    return true;
}

void Logger::initializeDatabase()
{
    m_db = QSqlDatabase::addDatabase("QSQLITE", "activity_db");
    m_db.setDatabaseName("activity_logs.db");

    if (!m_db.open()) {
        qWarning() << "Failed to open activity database:" << m_db.lastError().text();
        return;
    }

    QSqlQuery query(m_db);
    // Dalam fungsi initializeDatabase()
    if (!query.exec("CREATE TABLE IF NOT EXISTS log ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "id_user INTEGER NOT NULL, "
                    "start_time INTEGER NOT NULL, "
                    "end_time INTEGER NOT NULL, "
                    "app_name TEXT, "
                    "title TEXT, "
                    "url TEXT, "  // Tambahkan kolom baru untuk URL
                    "FOREIGN KEY(id_user) REFERENCES users(id) ON DELETE CASCADE)")) {
        qWarning() << "Failed to create log table:" << query.lastError().text();
    }

    // Dalam fungsi initializeDatabase()
    if (!query.exec("CREATE TABLE IF NOT EXISTS users ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "username TEXT UNIQUE NOT NULL, "
                    "password TEXT NOT NULL, "
                    "department TEXT, "
                    "profile_image TEXT, "
                    "email TEXT, "
                    "role TEXT, "
                    "token TEXT)")) {  // <-- Tambah kolom token
        qWarning() << "Failed to create users table:" << query.lastError().text();
    }
    emit logCountChanged();
}

void Logger::initializeProductivityDatabase()
{
    m_productivityDb = QSqlDatabase::addDatabase("QSQLITE", "productivity_db");
    m_productivityDb.setDatabaseName("produktif_app_db.db");

    if (!m_productivityDb.open()) {
        qWarning() << "Failed to open productivity database:" << m_productivityDb.lastError().text();
        return;
    }

    migrateProductivityDatabase();

    QSqlQuery query(m_productivityDb);
    if (!query.exec("CREATE TABLE IF NOT EXISTS aplikasi ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "aplikasi TEXT NOT NULL, "
                    "window_title TEXT, "
                    "url TEXT, "  // Kolom baru untuk URL/domain
                    "jenis INTEGER NOT NULL, "
                    "productivity INTEGER NOT NULL DEFAULT 0, "
                    "for_user TEXT NOT NULL DEFAULT '0')")) {
        qWarning() << "Failed to create aplikasi table:" << query.lastError().text();
    }

    if (!query.exec("CREATE TABLE IF NOT EXISTS task ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "project_name TEXT NOT NULL, "
                    "task TEXT, "
                    "max_time INTEGER NOT NULL, "
                    "time_usage INTEGER NOT NULL, "
                    "active BOOLEAN NOT NULL, "
                    "status TEXT NOT NULL, "
                    "paused BOOLEAN NOT NULL DEFAULT 0,"
                    "user_id INTEGER NOT NULL)")) {
        qWarning() << "Failed to create task table:" << query.lastError().text();
    }

    if (!query.exec("CREATE TABLE IF NOT EXISTS completed_tasks ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "project_name TEXT NOT NULL, "
                    "task TEXT NOT NULL, "
                    "max_time INTEGER NOT NULL, "
                    "time_usage INTEGER NOT NULL, "
                    "completed_time INTEGER NOT NULL)"
                    "user_id INTEGER NOT NULL)")) {
        qWarning() << "Failed to create completed_tasks table:" << query.lastError().text();
    }
    // Di logger.cpp - fungsi initializeProductivityDatabase()
    if (!query.exec("CREATE TABLE IF NOT EXISTS idle_settings ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "threshold_seconds INTEGER)")) {
        qWarning() << "Failed to create idle_settings table:" << query.lastError().text();
    }
    if (!query.exec("CREATE TABLE IF NOT EXISTS log_paused ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "task_id INTEGER NOT NULL, "
                    "start_reality TEXT NOT NULL, "  // ISO 8601 format: 'YYYY-MM-DDTHH:MM:SS.SSSSSSZ'
                    "end_reality TEXT, "
                    "current_status TEXT NOT NULL, "  // 'pause' or 'play'
                    "FOREIGN KEY(task_id) REFERENCES task(id))")) {
        qWarning() << "Failed to create log_paused table:" << query.lastError().text();
    }
    // Buat tabel baru untuk "Time at Work"
    if (!query.exec("CREATE TABLE IF NOT EXISTS work_time ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "user_id INTEGER NOT NULL, "
                    "date TEXT NOT NULL, "
                    "elapsed_seconds INTEGER NOT NULL DEFAULT 0, "
                    "UNIQUE(user_id, date))")) {
        qWarning() << "Failed to create work_time table:" << query.lastError().text();
    }
}


void Logger::checkAndCreateNewDayRecord()
{
    if (m_currentUserId == -1 || !ensureProductivityDatabaseOpen()) return;

    QString today = QDate::currentDate().toString("yyyy-MM-dd");

    QSqlQuery query(m_productivityDb);
    // Cek apakah ada record untuk hari ini
    query.prepare("SELECT elapsed_seconds FROM work_time WHERE user_id = :user_id AND date = :date");
    query.bindValue(":user_id", m_currentUserId);
    query.bindValue(":date", today);

    if (query.exec() && query.next()) {
        // Record untuk hari ini sudah ada, tidak perlu melakukan apa-apa
        return;
    } else {
        // Tidak ada record untuk hari ini, buat record baru dengan waktu 0
        m_workTimeElapsedSeconds = 0;
        emit workTimeElapsedSecondsChanged();
        saveWorkTimeData(); // Simpan nilai awal 0
        qDebug() << "New day detected. Work time reset for user:" << m_currentUserId;
    }
}

void Logger::loadWorkTimeData()
{
    if (m_currentUserId == -1 || !ensureProductivityDatabaseOpen()) {
        m_workTimeElapsedSeconds = 0;
        emit workTimeElapsedSecondsChanged();
        return;
    }

    QString today = QDate::currentDate().toString("yyyy-MM-dd");
    QSqlQuery query(m_productivityDb);
    query.prepare("SELECT elapsed_seconds FROM work_time WHERE user_id = :user_id AND date = :date");
    query.bindValue(":user_id", m_currentUserId);
    query.bindValue(":date", today);

    if (query.exec() && query.next()) {
        m_workTimeElapsedSeconds = query.value(0).toInt();
    } else {
        // Tidak ada record untuk hari ini, berarti waktu kerja adalah 0
        m_workTimeElapsedSeconds = 0;
        // Buat record untuk hari baru
        saveWorkTimeData();
    }
    qDebug() << "Loaded work time for" << today << ":" << m_workTimeElapsedSeconds << "seconds";
    emit workTimeElapsedSecondsChanged();
}

void Logger::saveWorkTimeData()
{
    if (m_currentUserId == -1 || !ensureProductivityDatabaseOpen()) return;

    QString today = QDate::currentDate().toString("yyyy-MM-dd");
    QSqlQuery query(m_productivityDb);
    // Gunakan INSERT OR REPLACE untuk menyederhanakan (membuat baru atau memperbarui yang sudah ada)
    query.prepare("INSERT OR REPLACE INTO work_time (user_id, date, elapsed_seconds) "
                  "VALUES (:user_id, :date, :seconds)");
    query.bindValue(":user_id", m_currentUserId);
    query.bindValue(":date", today);
    query.bindValue(":seconds", m_workTimeElapsedSeconds);

    if (!query.exec()) {
        qWarning() << "Failed to save work time:" << query.lastError().text();
    }
}

void Logger::sendWorkTimeToAPI()
{
    if (m_currentUserId == -1 || m_authToken.isEmpty()) {
        qWarning() << "Cannot send work time: No user logged in or no auth token";
        return;
    }

    // Prepare payload
    QJsonObject payload;
    payload["user_id"] = m_currentUserId;
    payload["time_at_work"] = m_workTimeElapsedSeconds;

    // Configure request
    QNetworkRequest request(QUrl("https://deskmon.pranala-dt.co.id/api/send-time-at-work"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_authToken.toUtf8());

    qDebug() << "Sending work time to API:" << QJsonDocument(payload).toJson();

    // Send POST request
    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(payload).toJson());

    // Handle timeout (10 seconds)
    QTimer::singleShot(10000, reply, &QNetworkReply::abort);

    // Handle response
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            qDebug() << "Work time successfully sent to API. Response:" << response;
        } else {
            qWarning() << "Failed to send work time to API:" << reply->errorString();
            qDebug() << "HTTP Status:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            qDebug() << "Response Body:" << reply->readAll();
        }
        reply->deleteLater();
    });
}

void Logger::updateWorkTimeAndSave() {
    if (m_activeTaskId != -1 && !m_isTaskPaused) {
        m_workTimeElapsedSeconds++;
        emit workTimeElapsedSecondsChanged();

        if (m_workTimeElapsedSeconds % 10 == 0) {
            saveWorkTimeData();
        }
    }
}

// Updated getAppProductivityType function
int Logger::getAppProductivityType(const QString &appName, const QString &url) const
{
    if (!ensureProductivityDatabaseOpen() || m_currentUserId == -1) {
        return 0;
    }

    // Helper function untuk normalisasi string
    auto normalizeString = [](const QString &str) {
        return str.toLower()
        .remove(' ')
            .remove('-')
            .remove('_')
            .remove('.');
    };

    // Helper function untuk cek user permission
    auto checkUserPermission = [this](const QString &forUsers) -> bool {
        if (forUsers == "0") {
            return true; // Global untuk semua user
        }

        QString currentUserStr = QString::number(m_currentUserId);
        QStringList userList = forUsers.split(',', Qt::SkipEmptyParts);

        for (const QString &userId : userList) {
            if (userId.trimmed() == currentUserStr) {
                return true;
            }
        }

        return false;
    };
    auto extractDomain = [](const QString &url) -> QString {
        if (url.isEmpty()) return "";

        QUrl qurl(url);
        QString domain = qurl.host();

        // Jika QUrl gagal parse, coba ekstrak manual
        if (domain.isEmpty()) {
            QRegularExpression domainRegex(R"((?:https?://)?(?:www\.)?([^/]+))");
            QRegularExpressionMatch match = domainRegex.match(url);
            if (match.hasMatch()) {
                domain = match.captured(1);
            }
        }

        // Hapus www. prefix
        if (domain.startsWith("www.")) {
            domain = domain.mid(4);
        }

        return domain.toLower();
    };

    // Cek apakah ini aplikasi browser atau non-browser
    bool isBrowserApp = !url.isEmpty();

    if (isBrowserApp) {
        // BROWSER APPLICATION: Gunakan domain matching
        QString domain = extractDomain(url);
        if (domain.isEmpty()) {
            return 0; // Tidak bisa ekstrak domain
        }

        QSqlQuery query(m_productivityDb);
        query.prepare(R"(
            SELECT url, jenis, for_user
            FROM aplikasi
            WHERE url IS NOT NULL
            AND url != ''
        )");
        query.bindValue(":userPattern", "%," + QString::number(m_currentUserId) + ",%");

        if (query.exec()) {
            while (query.next()) {
                QString dbUrl = query.value(0).toString();
                int jenis = query.value(1).toInt();
                QString forUsers = query.value(2).toString();

                // Check user permission
                if (!checkUserPermission(forUsers)) {
                    continue;
                }

                QString dbDomain = extractDomain(dbUrl);
                if (dbDomain.isEmpty()) continue;

                // Exact domain match
                if (domain == dbDomain) {
                    return jenis;
                }

                // Check if domain contains subdomain match
                if (domain.endsWith("." + dbDomain) || dbDomain.endsWith("." + domain)) {
                    return jenis;
                }
            }
        }
    } else {
        // NON-BROWSER APPLICATION: Gunakan app name matching
        QString normApp = normalizeString(appName);

        QSqlQuery query(m_productivityDb);
        query.prepare(R"(
            SELECT aplikasi, jenis, for_user
            FROM aplikasi
            WHERE (url IS NULL OR url = '')
            AND aplikasi IS NOT NULL
            AND aplikasi != ''
        )");
        query.bindValue(":userPattern", "%," + QString::number(m_currentUserId) + ",%");

        if (query.exec()) {
            while (query.next()) {
                QString dbApp = query.value(0).toString();
                int jenis = query.value(1).toInt();
                QString forUsers = query.value(2).toString();

                // Check user permission
                if (!checkUserPermission(forUsers)) {
                    continue;
                }

                QString normDbApp = normalizeString(dbApp);

                // Exact match
                if (normApp == normDbApp) {
                    return jenis;
                }

                // Contains match (kedua arah)
                if (normApp.contains(normDbApp) || normDbApp.contains(normApp)) {
                    return jenis;
                }
            }
        }
    }

    return 0; // Default neutral
}

// Updated calculateTodayProductiveSeconds function
int Logger::calculateTodayProductiveSeconds() const
{
    if (!ensureDatabaseOpen() || m_currentUserId == -1) {
        return 0;
    }

    QString today = QDate::currentDate().toString("yyyy-MM-dd");
    int totalProductiveSeconds = 0;

    // Helper function untuk cek user permission
    auto checkUserPermission = [this](const QString &forUsers) -> bool {
        if (forUsers == "0") {
            return true; // Global untuk semua user
        }

        QString currentUserStr = QString::number(m_currentUserId);
        QStringList userList = forUsers.split(',', Qt::SkipEmptyParts);

        for (const QString &userId : userList) {
            if (userId.trimmed() == currentUserStr) {
                return true;
            }
        }

        return false;
    };
    auto extractDomain = [](const QString &url) -> QString {
        if (url.isEmpty()) return QString();

        QUrl qurl(url);
        QString host = qurl.host();

        if (host.isEmpty()) {
            QRegularExpression domainRegex(R"((?:https?://)?(?:www\.)?([^/]+))");
            QRegularExpressionMatch match = domainRegex.match(url);
            if (match.hasMatch()) {
                host = match.captured(1);
            }
        }

        if (host.startsWith("www.")) {
            host = host.mid(4);
        }

        return host.toLower();
    };

    // Helper function untuk normalisasi string
    auto normalizeString = [](const QString &str) {
        return str.toLower()
        .remove(' ')
            .remove('-')
            .remove('_')
            .remove('.');
    };

    // 1. Load productivity rules from database
    QHash<QString, int> productiveApps;     // Non-browser apps
    QHash<QString, int> productiveDomains;  // Browser domains

    if (ensureProductivityDatabaseOpen()) {
        QSqlQuery query(m_productivityDb);
        query.prepare(R"(
            SELECT aplikasi, url, jenis, for_user
            FROM aplikasi
            WHERE jenis IN (1, 2)
        )");
        query.bindValue(":userPattern", "%," + QString::number(m_currentUserId) + ",%");

        if (query.exec()) {
            while (query.next()) {
                QString appName = query.value(0).toString();
                QString url = query.value(1).toString();
                int type = query.value(2).toInt();
                QString forUsers = query.value(3).toString();

                // Check user permission
                if (!checkUserPermission(forUsers)) {
                    continue;
                }

                if (!url.isEmpty()) {
                    // Browser app rule - store by domain
                    QString domain = extractDomain(url);
                    if (!domain.isEmpty()) {
                        productiveDomains[domain] = type;
                    }
                } else if (!appName.isEmpty()) {
                    // Non-browser app rule
                    productiveApps[appName] = type;
                }
            }
        }
    }

    // 2. Process today's activity logs
    QSqlQuery logQuery(m_db);
    logQuery.prepare(R"(
        SELECT start_time, end_time, app_name, title, url
        FROM log
        WHERE id_user = :user_id
        AND date(start_time, 'unixepoch', 'localtime') = :today
        ORDER BY start_time ASC
    )");
    logQuery.bindValue(":user_id", m_currentUserId);
    logQuery.bindValue(":today", today);

    if (logQuery.exec()) {
        QHash<QString, int> appProductivityTime;
        QHash<QString, int> domainProductivityTime;
        QHash<QString, int> matchStats;

        while (logQuery.next()) {
            qint64 start = logQuery.value(0).toLongLong();
            qint64 end = logQuery.value(1).toLongLong();
            QString appName = logQuery.value(2).toString();
            QString title = logQuery.value(3).toString();
            QString url = logQuery.value(4).toString();

            int duration = end - start;
            if (duration <= 0) continue;

            int productivityType = 0;
            QString matchMethod = "none";
            QString matchedItem = "";

            bool isBrowserApp = !url.isEmpty();

            if (isBrowserApp) {
                // Browser application - check domain
                QString domain = extractDomain(url);
                if (!domain.isEmpty()) {
                    // Direct domain match
                    if (productiveDomains.contains(domain)) {
                        productivityType = productiveDomains[domain];
                        matchMethod = "domain_exact";
                        matchedItem = domain;
                    } else {
                        // Check for subdomain matches
                        for (auto it = productiveDomains.begin(); it != productiveDomains.end(); ++it) {
                            const QString &ruleDomain = it.key();
                            if (domain.endsWith("." + ruleDomain) || ruleDomain.endsWith("." + domain)) {
                                productivityType = it.value();
                                matchMethod = "domain_subdomain";
                                matchedItem = ruleDomain;
                                break;
                            }
                        }
                    }
                }
            } else {
                // Non-browser application - check app name
                QString normApp = normalizeString(appName);

                // Direct app match
                for (auto it = productiveApps.begin(); it != productiveApps.end(); ++it) {
                    const QString &ruleApp = it.key();
                    QString normRuleApp = normalizeString(ruleApp);

                    if (normApp == normRuleApp) {
                        productivityType = it.value();
                        matchMethod = "app_exact";
                        matchedItem = ruleApp;
                        break;
                    } else if (normApp.contains(normRuleApp) || normRuleApp.contains(normApp)) {
                        productivityType = it.value();
                        matchMethod = "app_contains";
                        matchedItem = ruleApp;
                        break;
                    }
                }
            }

            bool isProductive = (productivityType == 1);

            if (isProductive) {
                totalProductiveSeconds += duration;

                if (isBrowserApp) {
                    QString domain = extractDomain(url);
                    if (!domain.isEmpty()) {
                        domainProductivityTime[domain] += duration;
                    }
                } else {
                    appProductivityTime[appName] += duration;
                }

                matchStats[matchMethod] += duration;
            }
        }

        // 3. Debug output
        qDebug() << "==== Updated Productivity Breakdown ====";
        qDebug() << "Total Productive Time:" << formatDuration(totalProductiveSeconds);

        qDebug() << "\nMatching Method Statistics:";
        for (auto it = matchStats.begin(); it != matchStats.end(); ++it) {
            if (totalProductiveSeconds > 0) {
                double percentage = (double)it.value() / totalProductiveSeconds * 100;
                qDebug() << QString("%1: %2 (%3%)")
                                .arg(it.key(), -20)
                                .arg(formatDuration(it.value()))
                                .arg(percentage, 0, 'f', 1);
            }
        }

        qDebug() << "\nTop Productive Domains (Browser Apps):";
        QList<QPair<int, QString>> sortedDomains;
        for (auto it = domainProductivityTime.begin(); it != domainProductivityTime.end(); ++it) {
            if (it.value() > 0) {
                sortedDomains.append(qMakePair(it.value(), it.key()));
            }
        }
        std::sort(sortedDomains.begin(), sortedDomains.end(), std::greater<QPair<int, QString>>());
        for (const auto &pair : sortedDomains.mid(0, 10)) {
            qDebug() << QString("%1: %2").arg(pair.second, -30).arg(formatDuration(pair.first));
        }

        qDebug() << "\nTop Productive Apps (Non-Browser):";
        QList<QPair<int, QString>> sortedApps;
        for (auto it = appProductivityTime.begin(); it != appProductivityTime.end(); ++it) {
            if (it.value() > 0) {
                sortedApps.append(qMakePair(it.value(), it.key()));
            }
        }
        std::sort(sortedApps.begin(), sortedApps.end(), std::greater<QPair<int, QString>>());
        for (const auto &pair : sortedApps.mid(0, 10)) {
            qDebug() << QString("%1: %2").arg(pair.second, -30).arg(formatDuration(pair.first));
        }

    } else {
        qWarning() << "Failed to fetch today's logs:" << logQuery.lastError().text();
    }

    return totalProductiveSeconds;
}

// Updated productivityStats function
QVariantMap Logger::productivityStats() const
{
    if (!ensureDatabaseOpen() || !ensureProductivityDatabaseOpen()) {
        qWarning() << "Cannot compute productivity stats: Database is not open";
        return QVariantMap();
    }
    if (m_currentUserId == -1) {
        qWarning() << "Cannot compute productivity stats: No user logged in";
        return QVariantMap();
    }

    QVariantMap stats;
    double productiveTime = 0;
    double nonProductiveTime = 0;
    double neutralTime = 0;
    double totalTime = 0;

    QString queryStr = "SELECT start_time, end_time, app_name, title, url FROM log "
                       "WHERE app_name IS NOT NULL AND id_user = :id_user ";

    if (!m_startDateFilter.isEmpty()) {
        queryStr += QString("AND date(start_time, 'unixepoch', 'localtime') >= date('%1') ")
        .arg(m_startDateFilter);
    }
    if (!m_endDateFilter.isEmpty()) {
        queryStr += QString("AND date(start_time, 'unixepoch', 'localtime') <= date('%1') ")
        .arg(m_endDateFilter);
    }

    QSqlQuery query(m_db);
    query.prepare(queryStr);
    query.bindValue(":id_user", m_currentUserId);

    if (!query.exec()) {
        qWarning() << "Failed to fetch logs for productivity stats:" << query.lastError().text();
        return stats;
    }

    while (query.next()) {
        qint64 start = query.value(0).toLongLong();
        qint64 end = query.value(1).toLongLong();
        QString appName = query.value(2).toString();
        QString url = query.value(4).toString();

        double duration = end - start;
        if (duration <= 0) continue;

        // Use updated function that focuses on app vs domain
        int type = getAppProductivityType(appName, url);

        switch (type) {
        case 1:
            productiveTime += duration;
            break;
        case 2:
            nonProductiveTime += duration;
            break;
        default:
            neutralTime += duration;
            break;
        }
        totalTime += duration;
    }

    double total = totalTime > 0 ? totalTime : 1;
    stats["productive"] = (productiveTime / total) * 100;
    stats["nonProductive"] = (nonProductiveTime / total) * 100;
    stats["neutral"] = (neutralTime / total) * 100;
    return stats;
}


void Logger::sendProductiveTimeToAPI()
{
    if (m_currentUserId == -1 || m_authToken.isEmpty()) {
        qWarning() << "Cannot send productive time: Not logged in or missing token";
        return;
    }

    int productiveSeconds = calculateTodayProductiveSeconds();
    qDebug() << "Sending productive time:" << productiveSeconds << "seconds";

    QJsonObject payload;
    payload["user_id"] = m_currentUserId;
    payload["productive_time"] = productiveSeconds;

    QNetworkRequest request(QUrl("https://deskmon.pranala-dt.co.id/api/send-productive-time"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_authToken.toUtf8());

    QNetworkReply *reply = m_networkManager->post(request, QJsonDocument(payload).toJson());

    QTimer::singleShot(10000, reply, &QNetworkReply::abort); // Timeout 10 detik

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            qDebug() << "Productive time sent successfully. Response:" << reply->readAll();
        } else {
            qWarning() << "Failed to send productive time:" << reply->errorString();
        }
        reply->deleteLater();
    });
}


void Logger::checkTaskStatusBeforeStart()
{
    if (!ensureProductivityDatabaseOpen()) {
        qWarning() << "Cannot check task status: Productivity database is not open";
        return;
    }

    if (m_currentUserId == -1) {
        qWarning() << "Cannot check task status: No user logged in";
        return;
    }

    QSqlQuery query(m_productivityDb);
    // Prioritaskan task dengan status on-progress
    query.prepare("SELECT id, paused, time_usage, status FROM task WHERE user_id = :user_id AND (active = 1 OR status = 'on-progress') ORDER BY status = 'on-progress' DESC LIMIT 1");
    query.bindValue(":user_id", m_currentUserId);

    if (!query.exec()) {
        qWarning() << "Failed to check active task status:" << query.lastError().text();
        return;
    }

    if (query.next()) {
        m_activeTaskId = query.value(0).toInt();
        m_isTaskPaused = query.value(1).toBool();
        QString status = query.value(3).toString();

        // Jika status on-progress, pastikan task tidak paused
        if (status == "on-progress") {
            m_isTaskPaused = false;
            // Update database untuk memastikan konsistensi
            QSqlQuery updateQuery(m_productivityDb);
            updateQuery.prepare("UPDATE task SET paused = 0 WHERE id = :id");
            updateQuery.bindValue(":id", m_activeTaskId);
            updateQuery.exec();
        }

        m_isTrackingActive = !m_isTaskPaused;

        if (!m_isTaskPaused) {
            m_taskStartTime = QDateTime::currentSecsSinceEpoch();
            QSqlQuery timeQuery(m_productivityDb);
            timeQuery.prepare("SELECT time_usage FROM task WHERE id = :id");
            timeQuery.addBindValue(m_activeTaskId);
            if (timeQuery.exec() && timeQuery.next()) {
                m_taskTimeOffset = timeQuery.value(0).toInt();
            }
        }

        qDebug() << "Active task found. ID:" << m_activeTaskId
                 << "| Paused:" << m_isTaskPaused
                 << "| Status:" << status
                 << "| Tracking:" << m_isTrackingActive;
    } else {
        m_activeTaskId = -1;
        m_isTaskPaused = false;
        m_isTrackingActive = true;
        qDebug() << "No active task found for user_id:" << m_currentUserId;
    }
    // Cari user yang memiliki token login
    QSqlQuery autoLoginQuery(m_db);
    if (autoLoginQuery.exec("SELECT id, username, email, token FROM users WHERE token IS NOT NULL AND token != '' LIMIT 1")) {
        if (autoLoginQuery.next()) {
            m_currentUserId = autoLoginQuery.value(0).toInt();
            m_currentUsername = autoLoginQuery.value(1).toString();
            m_currentUserEmail = autoLoginQuery.value(2).toString();
            m_authToken = autoLoginQuery.value(3).toString();
            qDebug() << "Auto login as user ID:" << m_currentUserId << "Username:" << m_currentUsername;
            emit currentUserIdChanged();
            emit currentUsernameChanged();
            emit currentUserEmailChanged();
            emit userEmailChanged();
            checkAndCreateNewDayRecord(); // Pastikan record hari ini ada
            loadWorkTimeData();           // Muat waktu kerja yang sudah tersimpan
        }
    }


    emit activeTaskChanged();
    emit taskPausedChanged();
    emit trackingActiveChanged();
}




int Logger::getIdleThreshold() const
{
    if (!ensureProductivityDatabaseOpen()) {
        qWarning() << "Cannot access productivity database, returning default idle threshold: 120 seconds";
        return 180;
    }

    QSqlQuery query(m_productivityDb);
    query.prepare("SELECT threshold_seconds FROM idle_settings LIMIT 1");
    if (query.exec() && query.next()) {
        int threshold = query.value(0).toInt();
        if (threshold > 0) {
            qDebug() << "Retrieved idle threshold from database:" << threshold << "seconds";
            return threshold;
        }
        qWarning() << "Invalid threshold in database (null or <= 0), returning default: 180 seconds";
        return 180;
    }
    qWarning() << "No threshold found in database, returning default: 120 seconds";
    return 180;
}


QVariantList Logger::getProductivityApps() const {
    if (!ensureProductivityDatabaseOpen() || m_currentUserId == -1) {
        return QVariantList();
    }

    QVariantList apps;
    QSqlQuery query(m_productivityDb);
    query.prepare(R"(
        SELECT aplikasi, jenis, url, for_user
        FROM aplikasi
        WHERE for_user = '0'
           OR ',' || for_user || ',' LIKE :userPattern
    )");
    query.bindValue(":userPattern", "%," + QString::number(m_currentUserId) + ",%");

    if (query.exec()) {
        while (query.next()) {
            QVariantMap app;
            app["appName"] = query.value(0).toString();
            app["type"] = query.value(1).toInt();
            app["url"] = query.value(2).toString();
            apps.append(app);
        }
    } else {
        qWarning() << "Failed to fetch productivity apps:" << query.lastError().text();
    }

    return apps;
}


void Logger::setIdleThreshold(int seconds)
{
    if (!ensureProductivityDatabaseOpen()) {
        qWarning() << "Cannot set idle threshold: Database is not open";
        return;
    }

    if (seconds <= 0) {
        qWarning() << "Invalid idle threshold value:" << seconds;
        return;
    }

    QSqlQuery query(m_productivityDb);
    query.prepare("INSERT OR REPLACE INTO idle_settings (id, threshold_seconds) VALUES (1, :threshold)");
    query.bindValue(":threshold", seconds);
    if (!query.exec()) {
        qWarning() << "Failed to set idle threshold:" << query.lastError().text();
    } else {
        qDebug() << "Idle threshold updated to:" << seconds << "seconds";
        emit idleThresholdChanged();
    }
}

QString Logger::currentAppName() const
{
    return m_currentAppName;
}

QString Logger::currentWindowTitle() const
{
    return m_currentWindowTitle;
}

int Logger::logCount() const
{
    if (!ensureDatabaseOpen()) {
        qWarning() << "Cannot count logs: Database is not open";
        return 0;
    }
    if (m_currentUserId == -1) {
        qWarning() << "Cannot count logs: No user logged in";
        return 0;
    }

    QString queryStr = "SELECT COUNT(*) FROM log WHERE app_name IS NOT NULL AND title IS NOT NULL AND id_user = :id_user";
    if (!m_startDateFilter.isEmpty()) {
        queryStr += QString(" AND date(start_time, 'unixepoch', 'localtime') >= date('%1')")
        .arg(m_startDateFilter);
    }
    if (!m_endDateFilter.isEmpty()) {
        queryStr += QString(" AND date(start_time, 'unixepoch', 'localtime') <= date('%1')")
        .arg(m_endDateFilter);
    }

    QSqlQuery query(m_db);
    query.prepare(queryStr);
    query.bindValue(":id_user", m_currentUserId);
    if (!query.exec()) {
        qWarning() << "Failed to count logs:" << query.lastError().text();
        return 0;
    }
    if (query.next()) {
        return query.value(0).toInt();
    }

    return 0;
}

QString Logger::logContent() const
{
    if (!ensureDatabaseOpen()) {
        qWarning() << "Cannot fetch log content: Database is not open";
        return "";
    }

    if (m_currentUserId == -1) {
        qWarning() << "Cannot fetch log content: No user logged in";
        return "";
    }

    QString content;
    // MODIFIKASI 1: Tambahkan 'url' ke dalam query SELECT
    QString queryStr = "SELECT start_time, end_time, app_name, title, url FROM log "
                       "WHERE app_name IS NOT NULL AND title IS NOT NULL AND id_user = :id_user ";

    if (!m_startDateFilter.isEmpty()) {
        queryStr += QString("AND date(start_time, 'unixepoch', 'localtime') >= date('%1') ")
        .arg(m_startDateFilter);
    }
    if (!m_endDateFilter.isEmpty()) {
        queryStr += QString("AND date(start_time, 'unixepoch', 'localtime') <= date('%1') ")
        .arg(m_endDateFilter);
    }

    queryStr += "ORDER BY start_time DESC";

    QSqlQuery query(m_db);
    query.prepare(queryStr);
    query.bindValue(":id_user", m_currentUserId);
    if (!query.exec()) {
        qWarning() << "Failed to fetch log content:" << query.lastError().text();
        return content;
    }
    qDebug() << "logContent query executed, userId:" << m_currentUserId << ", rows:" << query.size();
    while (query.next()) {
        qint64 start = query.value(0).toLongLong();
        qint64 end = query.value(1).toLongLong();
        QString app = query.value(2).toString();
        QString title = query.value(3).toString();
        QString url = query.value(4).toString(); // Ambil data URL

        // MODIFIKASI 2: Tambahkan URL ke dalam format string, menjadi 5 bagian
        content += QString("%1,%2,%3,%4,%5\n")
                       .arg(QDateTime::fromSecsSinceEpoch(start).toString("hh:mm:ss"))
                       .arg(QDateTime::fromSecsSinceEpoch(end).toString("hh:mm:ss"))
                       .arg(app)
                       .arg(title)
                       .arg(url); // Tambahkan URL di sini
    }
    qDebug() << "logContent returned" << content.count('\n') << "lines";
    return content;
}




// HAPUS fungsi productivityStats yang lama di logger.cpp, lalu GANTI dengan yang ini.


QVariantList Logger::getAvailableApps() const
{
    QVariantList apps = {
        "Other", "Chrome", "Firefox", "Edge", "Safari", "Opera",
        "Visual Studio Code", "Qt Creator", "Android Studio", "IntelliJ IDEA", "PyCharm", "Xcode",
        "Microsoft Word", "Excel", "PowerPoint", "WPS Office", "LibreOffice", "OneNote", "Obsidian",
        "Photoshop", "GIMP", "Figma", "Canva", "Blender", "Premiere Pro", "After Effects",
        "Slack", "Microsoft Teams", "Zoom", "Google Meet", "Discord", "Skype",
        "Notion", "Trello", "Jira", "Asana", "ClickUp",
        "Postman", "FileZilla", "Docker", "GitHub Desktop", "GitKraken", "Terminal",
        "Deskmon", "Desklog-Client", "Explorer", "Outlook", "Thunderbird"
    };

    return apps;
}

// Tambahkan fungsi ini di logger.cpp
QString extractDomain(const QString &urlString) {
    if (urlString.isEmpty()) {
        return QString();
    }

    QUrl url(urlString);
    QString host = url.host();

    // Menghilangkan subdomain "www." agar lebih konsisten
    if (host.startsWith("www.")) {
        host = host.mid(4);
    }

    return host;
}
void Logger::addProductivityApp(const QString &appName, const QString &windowTitle, const QString &url, int productivityType)
{
    if (!ensureProductivityDatabaseOpen()) {
        qWarning() << "Database tidak terbuka";
        return;
    }

    // 1. Simpan ke database lokal terlebih dahulu
    QSqlQuery query(m_productivityDb);
    query.prepare("INSERT INTO aplikasi (aplikasi, window_title, url, jenis, productivity) "
                  "VALUES (:app, :window, :url, :type, :prod)");
    query.bindValue(":app", appName);
    query.bindValue(":window", windowTitle.isEmpty() ? QVariant() : windowTitle);
    query.bindValue(":url", url.isEmpty() ? QVariant() : url);
    query.bindValue(":type", 0); // 0 = menunggu approval
    query.bindValue(":prod", productivityType);

    if (query.exec()) {
        qDebug() << "Aplikasi ditambahkan. Menunggu approval admin.";

        // 2. Kirim data ke API
        sendProductivityAppToAPI(appName, windowTitle, url, productivityType);

        // Refresh model
        QString productiveQuery = QString("SELECT aplikasi AS appName, window_title AS windowTitle, jenis AS type FROM aplikasi WHERE jenis = 1 AND (for_user = '0' OR for_user LIKE '%%1%')").arg(m_currentUserId);
        QString nonProductiveQuery = QString("SELECT aplikasi AS appName, window_title AS windowTitle, jenis AS type FROM aplikasi WHERE jenis = 2 AND (for_user = '0' OR for_user LIKE '%%1%')").arg(m_currentUserId);
        m_productiveAppsModel->setQuery(productiveQuery, m_productivityDb);
        m_nonProductiveAppsModel->setQuery(nonProductiveQuery, m_productivityDb);
        refreshProductivityModels();
        emit productivityAppsChanged();
    } else {
        qWarning() << "Gagal menambahkan aplikasi:" << query.lastError();
    }
}

void Logger::sendProductivityAppToAPI(const QString &appName, const QString &windowTitle, const QString &url, int productivityType)
{
    if (m_authToken.isEmpty()) {
        qWarning() << "Cannot send productivity app: No authentication token available";
        return;
    }

    if (m_currentUserId == -1) {
        qWarning() << "Cannot send productivity app: No user logged in";
        return;
    }

    QString status;
    switch(productivityType) {
    case 1: status = "productive"; break;
    case 2: status = "non-productive"; break;
    default: status = "neutral"; break;
    }

    QJsonObject payload;
    payload["application_name"] = appName;
    payload["productivity_status"] = status;
    payload["user_id"] = m_currentUserId;

    if (!windowTitle.isEmpty()) {
        payload["process_name"] = windowTitle;
    }

    if (!url.isEmpty()) {
        payload["url"] = url;
    }

    QNetworkRequest request(QUrl("https://deskmon.pranala-dt.co.id/api/app-request/store"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_authToken.toUtf8());

    qDebug() << "Sending productivity app to API:" << QJsonDocument(payload).toJson();

    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(payload).toJson());
    QTimer::singleShot(30000, reply, &QNetworkReply::abort);

    connect(reply, &QNetworkReply::finished, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            qDebug() << "Productivity app successfully sent to API. Response:" << response;
        } else {
            qWarning() << "Failed to send productivity app to API:" << reply->errorString();
            if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 401) {
                emit authTokenError("Authentication token expired");
            }
        }
        reply->deleteLater();
    });
}
void Logger::fetchAndStoreProductivityApps()
{
    // 1. Pastikan database terbuka dan user login
    if (!ensureProductivityDatabaseOpen()) {
        qWarning() << "Cannot fetch productivity apps: Database is not open";
        return;
    }

    if (m_currentUserId == -1) {
        qWarning() << "Cannot fetch productivity apps: No user logged in";
        return;
    }

    if (m_authToken.isEmpty()) {
        qWarning() << "Cannot fetch productivity apps: No auth token";
        return;
    }

    // 2. Buat request ke API
    QNetworkRequest request(QUrl("https://deskmon.pranala-dt.co.id/api/app-request/all"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_authToken.toUtf8());

    qDebug() << "Fetching productivity apps from API for user:" << m_currentUserId;

    // 3. Kirim GET request dengan timeout
    QNetworkReply *reply = m_networkManager->get(request);
    QTimer::singleShot(30000, reply, &QNetworkReply::abort); // Timeout 30 detik

    // Debug raw request
    qDebug() << "Request URL:" << request.url().toString();
    qDebug() << "Request Headers:" << request.rawHeaderList();

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        handleProductivityAppsResponse(reply);
    });
}

void Logger::handleProductivityAppsResponse(QNetworkReply *reply)
{
    QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> replyPtr(reply);

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Failed to fetch productivity apps:" << reply->errorString();
        return;
    }

    QByteArray responseData = reply->readAll();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);

    if (jsonDoc.isNull() || !jsonDoc.isObject()) {
        qWarning() << "Invalid JSON response";
        return;
    }

    QJsonObject jsonObj = jsonDoc.object();
    if (!jsonObj["success"].toBool()) {
        qWarning() << "API returned error:" << jsonObj["message"].toString();
        return;
    }

    QJsonArray appsArray = jsonObj["data"].isArray() ? jsonObj["data"].toArray() : QJsonArray();

    qDebug() << "==============================================";
    qDebug() << "Received" << appsArray.size() << "productivity apps from server:";
    qDebug() << "==============================================";

    for (const QJsonValue &appValue : appsArray) {
        if (!appValue.isObject()) continue;

        QJsonObject appObj = appValue.toObject();
        QString appName = appObj["application_name"].toString();
        QString status = appObj["productivity_status"].toString().toLower();
        QString processName = appObj["process_name"].toString();
        QString url = appObj["url"].toString();
        int userId = appObj["user_id"].toInt();

        qDebug() << "App:" << appName
                 << "| Process:" << (processName.isEmpty() ? "N/A" : processName)
                 << "| URL:" << (url.isEmpty() ? "N/A" : url)
                 << "| Status:" << status
                 << "| User ID:" << userId;
    }
    qDebug() << "==============================================";

    if (!m_productivityDb.transaction()) {
        qWarning() << "Failed to start transaction";
        return;
    }

    QSqlQuery query(m_productivityDb);
    bool success = true;
    int insertCount = 0;
    int updateCount = 0;
    int unchangedCount = 0;

    // Struktur untuk menyimpan data server
    struct ServerApp {
        QString appName;
        QString processName;
        QString url;
        int jenis;
        int userId;
        QString forUsers;
    };

    // Parse semua data dari server
    QList<ServerApp> serverApps;
    for (const QJsonValue &appValue : appsArray) {
        if (!appValue.isObject()) continue;

        QJsonObject appObj = appValue.toObject();
        ServerApp serverApp;
        serverApp.appName = appObj["application_name"].toString();
        serverApp.processName = appObj["process_name"].toString();
        serverApp.url = appObj["url"].toString();
        serverApp.userId = appObj["user_id"].toInt();
        serverApp.forUsers = QString::number(serverApp.userId);

        QString status = appObj["productivity_status"].toString().toLower();
        if (status == "productive") serverApp.jenis = 1;
        else if (status == "non-productive") serverApp.jenis = 2;
        else serverApp.jenis = 0;

        serverApps.append(serverApp);
    }

    // Proses setiap aplikasi dari server
    for (const ServerApp &serverApp : serverApps) {
        // Cek apakah aplikasi dengan nama dan URL yang sama sudah ada di database
        QString checkQuery;

        // Query untuk mencari aplikasi berdasarkan aplikasi dan URL saja (tanpa for_user)
        if (serverApp.url.isEmpty() || serverApp.url == "N/A") {
            checkQuery = "SELECT id, jenis, window_title, for_user FROM aplikasi "
                         "WHERE aplikasi = :appName AND (url IS NULL OR url = '' OR url = 'N/A')";
        } else {
            checkQuery = "SELECT id, jenis, window_title, for_user FROM aplikasi "
                         "WHERE aplikasi = :appName AND url = :url";
        }

        query.prepare(checkQuery);
        query.bindValue(":appName", serverApp.appName);

        if (!serverApp.url.isEmpty() && serverApp.url != "N/A") {
            query.bindValue(":url", serverApp.url);
        }

        if (!query.exec()) {
            qWarning() << "Failed to check existing app:" << query.lastError();
            success = false;
            break;
        }

        bool foundExactMatch = false;
        bool foundZeroTypeRecord = false;
        int zeroTypeRecordId = -1;

        // Cek semua record yang cocok dengan aplikasi dan URL
        while (query.next()) {
            int existingId = query.value(0).toInt();
            int existingJenis = query.value(1).toInt();
            QString existingWindowTitle = query.value(2).toString();
            QString existingForUser = query.value(3).toString();

            // Cek apakah ada record dengan for_user yang sama (exact match)
            if (existingForUser == serverApp.forUsers) {
                foundExactMatch = true;

                // Cek apakah perlu diupdate
                bool needsUpdate = false;

                if (existingJenis != serverApp.jenis) {
                    needsUpdate = true;
                    qDebug() << "Status mismatch for" << serverApp.appName << ": DB=" << existingJenis << "Server=" << serverApp.jenis;
                }

                QString expectedWindowTitle = (serverApp.processName.isEmpty() || serverApp.processName == "N/A") ? QString() : serverApp.processName;
                if (existingWindowTitle != expectedWindowTitle) {
                    needsUpdate = true;
                    qDebug() << "Process mismatch for" << serverApp.appName << ": DB=" << existingWindowTitle << "Server=" << expectedWindowTitle;
                }

                if (needsUpdate) {
                    // Update record yang sama persis
                    QString updateQuery = "UPDATE aplikasi SET jenis = :jenis, window_title = :windowTitle "
                                          "WHERE id = :id";
                    QSqlQuery updateQ(m_productivityDb);
                    updateQ.prepare(updateQuery);
                    updateQ.bindValue(":jenis", serverApp.jenis);
                    updateQ.bindValue(":windowTitle", expectedWindowTitle.isEmpty() ? QVariant() : expectedWindowTitle);
                    updateQ.bindValue(":id", existingId);

                    if (!updateQ.exec()) {
                        qWarning() << "Failed to update exact match app:" << serverApp.appName << updateQ.lastError();
                        success = false;
                        break;
                    } else {
                        updateCount++;
                        qDebug() << "Updated exact match app:" << serverApp.appName << "for user" << serverApp.userId;
                    }
                } else {
                    unchangedCount++;
                    qDebug() << "Exact match app unchanged:" << serverApp.appName << "for user" << serverApp.userId;
                }
                break; // Keluar dari loop karena sudah menemukan exact match
            }
            // Cek apakah ada record dengan jenis = 0 (default/unset)
            else if (existingJenis == 0) {
                foundZeroTypeRecord = true;
                zeroTypeRecordId = existingId;
            }
        }

        if (!foundExactMatch) {
            if (foundZeroTypeRecord) {
                // Update record dengan jenis = 0 karena tidak ada exact match
                QString expectedWindowTitle = (serverApp.processName.isEmpty() || serverApp.processName == "N/A") ? QString() : serverApp.processName;

                QString updateQuery = "UPDATE aplikasi SET jenis = :jenis, window_title = :windowTitle, for_user = :forUsers "
                                      "WHERE id = :id";
                QSqlQuery updateQ(m_productivityDb);
                updateQ.prepare(updateQuery);
                updateQ.bindValue(":jenis", serverApp.jenis);
                updateQ.bindValue(":windowTitle", expectedWindowTitle.isEmpty() ? QVariant() : expectedWindowTitle);
                updateQ.bindValue(":forUsers", serverApp.forUsers);
                updateQ.bindValue(":id", zeroTypeRecordId);

                if (!updateQ.exec()) {
                    qWarning() << "Failed to update zero-type record:" << serverApp.appName << updateQ.lastError();
                    success = false;
                    break;
                } else {
                    updateCount++;
                    qDebug() << "Updated zero-type record:" << serverApp.appName << "from jenis=0 to jenis=" << serverApp.jenis << "for user" << serverApp.userId;
                }
            } else {
                // Tidak ada record yang cocok sama sekali, insert baru
                QString insertQuery = "INSERT INTO aplikasi (aplikasi, window_title, url, jenis, for_user) "
                                      "VALUES (:app, :window, :url, :type, :forUsers)";
                QSqlQuery insertQ(m_productivityDb);
                insertQ.prepare(insertQuery);
                insertQ.bindValue(":app", serverApp.appName);
                insertQ.bindValue(":window", (serverApp.processName.isEmpty() || serverApp.processName == "N/A") ? QVariant() : serverApp.processName);
                insertQ.bindValue(":url", (serverApp.url.isEmpty() || serverApp.url == "N/A") ? QVariant() : serverApp.url);
                insertQ.bindValue(":type", serverApp.jenis);
                insertQ.bindValue(":forUsers", serverApp.forUsers);

                if (!insertQ.exec()) {
                    qWarning() << "Failed to insert new app:" << serverApp.appName << insertQ.lastError();
                    success = false;
                    break;
                } else {
                    insertCount++;
                    qDebug() << "Inserted new app:" << serverApp.appName << "for user" << serverApp.userId;
                }
            }
        }
    }

    if (success) {
        if (!m_productivityDb.commit()) {
            qWarning() << "Failed to commit transaction";
            m_productivityDb.rollback();
        } else {
            qDebug() << "==============================================";
            qDebug() << "Database sync completed successfully:";
            qDebug() << "- Inserted:" << insertCount << "new apps";
            qDebug() << "- Updated:" << updateCount << "existing apps";
            qDebug() << "- Unchanged:" << unchangedCount << "apps";
            qDebug() << "- Total processed:" << serverApps.size() << "apps";
            qDebug() << "==============================================";

            refreshProductivityModels();
            emit productivityAppsChanged();
        }
    } else {
        qWarning() << "Failed to sync productivity apps with database";
        m_productivityDb.rollback();
    }
}

void Logger::refreshProductivityModels()
{
    if (!ensureProductivityDatabaseOpen()) return;

    // Gabungkan aturan global (for_user=0) dan spesifik user
    QString productiveQuery = QString(
                                  "SELECT a.id, a.aplikasi AS appName, a.window_title AS windowTitle, a.url, a.jenis AS type "
                                  "FROM aplikasi a "
                                  "WHERE a.jenis = 1 AND (a.for_user = '0' OR a.for_user LIKE '%%1%' OR "
                                  "EXISTS (SELECT 1 FROM aplikasi WHERE aplikasi = a.aplikasi AND "
                                  "COALESCE(url,'') = COALESCE(a.url,'') AND for_user = '0')) "
                                  "GROUP BY a.aplikasi, COALESCE(a.url, '') "  // Hindari duplikat
                                  "ORDER BY a.for_user = '0' DESC"  // Prioritaskan aturan spesifik user
                                  ).arg(m_currentUserId);

    QString nonProductiveQuery = QString(
                                     "SELECT a.id, a.aplikasi AS appName, a.window_title AS windowTitle, a.url, a.jenis AS type "
                                     "FROM aplikasi a "
                                     "WHERE a.jenis = 2 AND (a.for_user = '0' OR a.for_user LIKE '%%1%' OR "
                                     "EXISTS (SELECT 1 FROM aplikasi WHERE aplikasi = a.aplikasi AND "
                                     "COALESCE(url,'') = COALESCE(a.url,'') AND for_user = '0')) "
                                     "GROUP BY a.aplikasi, COALESCE(a.url, '') "
                                     "ORDER BY a.for_user = '0' DESC"
                                     ).arg(m_currentUserId);

    m_productiveAppsModel->setQuery(productiveQuery, m_productivityDb);
    m_nonProductiveAppsModel->setQuery(nonProductiveQuery, m_productivityDb);

    // Debug output
    qDebug() << "Productive apps count:" << m_productiveAppsModel->rowCount();
    qDebug() << "Non-productive apps count:" << m_nonProductiveAppsModel->rowCount();
}


// logger.cpp

// GANTI FUNGSI YANG LAMA DENGAN VERSI BARU INI
// logger.cpp
void Logger::sendDailyUsageReport()
{
    if (m_currentUserId == -1 || m_authToken.isEmpty()) {
        qWarning() << "Cannot send usage report: No user logged in or no auth token.";
        return;
    }

    if (!ensureDatabaseOpen() || !ensureProductivityDatabaseOpen()) {
        qWarning() << "Cannot send usage report: Database not accessible.";
        return;
    }

    qDebug() << "Preparing aggregated daily usage report...";
    QString today = QDate::currentDate().toString("yyyy-MM-dd");

    // Struktur data untuk menyimpan agregasi
    QHash<QString, qint64> appDurations; // Untuk aplikasi non-browser
    QHash<QString, int> appProductivity;
    QHash<QPair<QString, QString>, qint64> browserUsage; // <appName, url> -> duration
    QHash<QString, int> domainProductivity;

    QSqlQuery logQuery(m_db);
    logQuery.prepare(R"(
        SELECT app_name, title, url, start_time, end_time
        FROM log
        WHERE id_user = :user_id
        AND date(start_time, 'unixepoch', 'localtime') = :today
        AND app_name != 'Idle'
    )");
    logQuery.bindValue(":user_id", m_currentUserId);
    logQuery.bindValue(":today", today);

    if (!logQuery.exec()) {
        qWarning() << "Failed to fetch logs for aggregation:" << logQuery.lastError().text();
        return;
    }

    // Helper function untuk ekstrak domain dari URL

    // Helper function untuk ekstrak domain dari URL - VERSI YANG DIPERBAIKI
    auto extractDomain = [](const QString& url) -> QString {
        if (url.isEmpty()) return "";

        QString cleanUrl = url;

        // Tambahkan scheme jika belum ada
        if (!cleanUrl.startsWith("http://") && !cleanUrl.startsWith("https://")) {
            cleanUrl = "https://" + cleanUrl;
        }

        QUrl qurl(cleanUrl);
        QString domain = qurl.host();

        // Jika masih kosong, coba parsing manual
        if (domain.isEmpty()) {
            // Hapus scheme jika ada
            QString temp = url;
            temp.remove(QRegularExpression("^https?://"));

            // Ambil bagian sebelum slash pertama
            int slashPos = temp.indexOf('/');
            if (slashPos != -1) {
                temp = temp.left(slashPos);
            }

            // Ambil bagian sebelum query parameter atau fragment
            int queryPos = temp.indexOf('?');
            if (queryPos != -1) {
                temp = temp.left(queryPos);
            }

            int fragmentPos = temp.indexOf('#');
            if (fragmentPos != -1) {
                temp = temp.left(fragmentPos);
            }

            domain = temp;
        }

        // Hapus www. prefix
        if (domain.startsWith("www.")) {
            domain = domain.mid(4);
        }

        return domain;
    };

    qDebug() << "==== RAW LOG DATA ====";
    while (logQuery.next()) {
        QString appName = logQuery.value(0).toString();
        QString title = logQuery.value(1).toString();
        QString urlString = logQuery.value(2).toString();
        qint64 startTime = logQuery.value(3).toLongLong();
        qint64 endTime = logQuery.value(4).toLongLong();
        qint64 duration = endTime - startTime;

        if (duration <= 0) continue;

        // Cek apakah ini aplikasi browser (punya URL)
        bool isBrowserApp = !urlString.isEmpty();

        if (isBrowserApp) {
            // Untuk browser, simpan per URL
            QString domain = extractDomain(urlString);
            QPair<QString, QString> key(appName, domain);
            browserUsage[key] += duration;

            if (!domainProductivity.contains(domain)) {
                domainProductivity[domain] = getAppProductivityType(appName, urlString);
            }
        } else {
            // Untuk non-browser, simpan per aplikasi
            appDurations[appName] += duration;
            if (!appProductivity.contains(appName)) {
                appProductivity[appName] = getAppProductivityType(appName, "");
            }
        }
    }

    // --- Membuat Payload ---
    QJsonArray dataArray;

    qDebug() << "==== NON-BROWSER APPLICATION USAGE ====";
    // 1. Tambahkan aplikasi non-browser
    for (auto it = appDurations.constBegin(); it != appDurations.constEnd(); ++it) {
        QString appName = it.key();
        int prodType = appProductivity.value(appName, 0);
        QString statusString = "neutral";
        if (prodType == 1) statusString = "productive";
        else if (prodType == 2) statusString = "non-productive";

        qDebug() << "App:" << appName
                 << "| Total Duration:" << it.value() << "seconds ("
                 << formatDuration(it.value()) << ")"
                 << "| Status:" << statusString;

        QJsonObject appObject;
        appObject["user_id"] = m_currentUserId;
        appObject["app_name"] = appName;
        appObject["duration"] = it.value();
        appObject["url"] = QJsonValue(); // Null untuk non-browser
        appObject["status"] = statusString;
        dataArray.append(appObject);
    }

    qDebug() << "==== BROWSER USAGE BY DOMAIN ====";
    // 2. Tambahkan penggunaan browser per domain
    for (auto it = browserUsage.constBegin(); it != browserUsage.constEnd(); ++it) {
        QString appName = it.key().first;  // Nama browser (chrome, firefox, etc)
        QString domain = it.key().second;  // Domain
        int prodType = domainProductivity.value(domain, 0);
        QString statusString = "neutral";
        if (prodType == 1) statusString = "productive";
        else if (prodType == 2) statusString = "non-productive";

        qDebug() << "Browser:" << appName
                 << "| Domain:" << domain
                 << "| Duration:" << it.value() << "seconds ("
                 << formatDuration(it.value()) << ")"
                 << "| Status:" << statusString;

        QJsonObject browserObject;
        browserObject["user_id"] = m_currentUserId;
        browserObject["app_name"] = appName;
        browserObject["duration"] = it.value();
        browserObject["url"] = domain;  // Simpan domain sebagai URL
        browserObject["status"] = statusString;
        dataArray.append(browserObject);
    }

    if (dataArray.isEmpty()) {
        qDebug() << "No aggregated data to report. Skipping API call.";
        return;
    }

    // Buat payload akhir
    QJsonObject payload;
    payload["data"] = dataArray;

    // Log payload yang akan dikirim
    qDebug() << "==== FINAL PAYLOAD TO SEND ====";
    qDebug() << "User ID:" << m_currentUserId;
    qDebug() << "Date:" << today;
    qDebug() << "Total Entries:" << dataArray.count();
    qDebug() << "Payload JSON:" << QJsonDocument(payload).toJson(QJsonDocument::Indented);

    // Kirim request
    QNetworkRequest request(QUrl("https://deskmon.pranala-dt.co.id/api/productivity-app"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_authToken.toUtf8());

    qDebug() << "Sending aggregated usage report to API. Entries:" << dataArray.count();
    QNetworkReply *reply = m_networkManager->post(request, QJsonDocument(payload).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        handleDailyUsageReportResponse(reply);
    });
}

void Logger::handleDailyUsageReportResponse(QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        qDebug() << "Daily usage report sent successfully. Response:" << response;
    } else {
        qWarning() << "Failed to send daily usage report:" << reply->errorString();
        qDebug() << "HTTP Status:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        qDebug() << "Response Body:" << reply->readAll();
    }
    reply->deleteLater();
}


QVariantList Logger::getPendingApplicationRequests() {
    QVariantList requests;

    if (!ensureProductivityDatabaseOpen()) {
        qWarning() << "Cannot get pending requests: Database is not open";
        return requests;
    }

    QSqlQuery query(m_productivityDb);
    // Perbarui query untuk menyertakan kolom url dan productivity
    query.prepare("SELECT id, aplikasi, window_title, url, productivity, for_user FROM aplikasi WHERE jenis = 0");

    if (query.exec()) {
        while (query.next()) {
            QVariantMap request;
            request["id"] = query.value(0);
            request["app_name"] = query.value(1); // aplikasi
            request["window_title"] = query.value(2);
            request["url"] = query.value(3); // URL/domain
            request["productivity"] = query.value(4); // Nilai produktivitas

            // Konversi jenis produktivitas ke string yang lebih deskriptif
            int productivity = query.value(4).toInt();
            switch(productivity) {
            case 1: request["productivity_text"] = "Productive"; break;
            case 2: request["productivity_text"] = "Non-Productive"; break;
            default: request["productivity_text"] = "Neutral"; break;
            }

            // Format for_user untuk tampilan yang lebih baik
            QString forUsers = query.value(5).toString();
            if (forUsers == "0") {
                request["for_users"] = "All Users";
            } else {
                QStringList userIds = forUsers.split(',', Qt::SkipEmptyParts);
                request["for_users"] = userIds.join(", ");
            }

            requests.append(request);
        }
    } else {
        qWarning() << "Error getting pending requests:" << query.lastError().text();
    }

    return requests;
}


void Logger::fetchAndStoreTasks()
{
    // 1. Pastikan database produktivitas terbuka sebelum memulai proses
    if (!ensureProductivityDatabaseOpen()) {
        qWarning() << "Cannot fetch and store tasks: Productivity database is not open";
        return;
    }

    // 2. Pastikan ada pengguna yang login dan memiliki ID yang valid
    if (m_currentUserId == -1) {
        qWarning() << "Cannot fetch tasks: No user logged in";
        return;
    }

    // 3. Periksa dan ambil token autentikasi jika belum ada
    if (m_authToken.isEmpty()) {
        QSqlQuery tokenQuery(m_db);
        tokenQuery.prepare("SELECT token FROM users WHERE id = ?");
        tokenQuery.addBindValue(m_currentUserId);
        if (tokenQuery.exec() && tokenQuery.next()) {
            m_authToken = tokenQuery.value(0).toString();
            qDebug() << "Token retrieved from database:" << m_authToken;
        }

        // Jika token masih kosong, hentikan proses dan beri sinyal kesalahan
        if (m_authToken.isEmpty()) {
            qWarning() << "Skipping task fetch: No auth token available";
            emit authTokenError("No authentication token");
            return;
        }
    }

    // 4. Siapkan permintaan HTTP GET ke endpoint server dengan user_id
    QString apiUrl = QString("https://deskmon.pranala-dt.co.id/api/task-by-user/%1").arg(m_currentUserId);
    QNetworkRequest request;
    request.setUrl(QUrl(apiUrl));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_authToken.toUtf8());

    qDebug() << "Sending request to:" << apiUrl << "with token:" << m_authToken;

    // 5. Kirim permintaan ke server dan hubungkan respons ke slot penanganan
    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [=]() {
        handleTaskFetchReply(reply);
    });
}

void Logger::handleTaskFetchReply(QNetworkReply *reply)
{
    // 5. Periksa kode status HTTP dari respons
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    qDebug() << "HTTP Status Code:" << statusCode;

    // 6. Tangani kesalahan jaringan jika ada
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Failed to fetch tasks: Network error:" << reply->errorString();
        reply->deleteLater();
        return;
    }

    // 7. Baca data respons dari server
    QByteArray responseData = reply->readAll();
    qDebug() << "Response data:" << responseData;

    // 8. Tangani kasus autentikasi gagal (token tidak valid atau kedaluwarsa)
    if (statusCode == 401) {
        qWarning() << "Unauthorized access. Token may be invalid or expired.";
        showAuthTokenErrorMessage(); // <-- UBAH KE BARIS INI
        reply->deleteLater();
        return;
    }

    // 9. Parse data JSON dari respons
    QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
    if (jsonDoc.isNull() || !jsonDoc.isObject()) {
        qWarning() << "Invalid JSON response from tasks endpoint. Response:" << responseData;
        reply->deleteLater();
        return;
    }

    QJsonObject jsonObj = jsonDoc.object();

    // 10. Periksa apakah permintaan berhasil berdasarkan field 'success'
    if (!jsonObj["success"].toBool()) {
        qWarning() << "Tasks API returned failure:" << jsonObj["message"].toString();
        reply->deleteLater();
        return;
    }

    // 11. Ambil array tugas dari respons
    QJsonArray tasksArray = jsonObj["data"].toArray();
    if (tasksArray.isEmpty()) {
        qDebug() << "No tasks found in response";
        reply->deleteLater();
        return;
    }

    // 12. Mulai transaksi database untuk memastikan integritas data
    QSqlQuery query(m_productivityDb);
    m_productivityDb.transaction();

    // 13. Ambil tugas yang sudah ada di database lokal untuk perbandingan
    QMap<int, QPair<int, int>> existingTasks; // taskId -> (max_time, time_usage)
    query.exec("SELECT id, max_time, time_usage FROM task");
    while (query.next()) {
        int taskId = query.value(0).toInt();
        int maxTime = query.value(1).toInt();
        int timeUsage = query.value(2).toInt();
        existingTasks.insert(taskId, QPair<int, int>(maxTime, timeUsage));
    }

    // 14. Proses setiap tugas dari respons server
    for (const QJsonValue &taskValue : tasksArray) {
        QJsonObject taskObj = taskValue.toObject();

        // Lewati tugas yang sudah selesai
        QString status = taskObj["status"].toString();
        if (status == "completed") {
            qDebug() << "Skipping completed task ID" << taskObj["id"].toInt();
            continue;
        }

        // Validasi bahwa semua field wajib ada
        if (!taskObj.contains("id") || !taskObj.contains("title") ||
            !taskObj.contains("description") || !taskObj.contains("user_id")) {
            qWarning() << "Skipping task with missing required fields";
            continue;
        }

        int taskId = taskObj["id"].toInt();
        QString projectName = taskObj["title"].toString();
        QString taskDesc = taskObj["description"].toString();
        int userId = taskObj["user_id"].toInt();

        // Lewati tugas yang bukan milik pengguna saat ini
        if (userId != m_currentUserId) {
            qDebug() << "Skipping task ID" << taskId << "for user ID" << userId << "(not current user)";
            continue;
        }

        // Ambil data duration dan total_duration dari server
        QJsonValue durationValue = taskObj["duration"];
        QJsonValue totalDurationValue = taskObj["total_duration"];

        // Konversi duration dari server (dalam jam) ke detik untuk max_time
        int serverMaxTime = 0;
        if (!durationValue.isNull()) {
            if (durationValue.isString()) {
                serverMaxTime = static_cast<int>(durationValue.toString().toDouble() * 3600); // jam -> detik
            } else {
                serverMaxTime = durationValue.toInt() * 3600; // jam -> detik
            }
        }

        // Konversi total_duration dari server (dalam jam) ke detik untuk time_usage
        int serverTimeUsage = 0;
        if (!totalDurationValue.isNull()) {
            if (totalDurationValue.isString()) {
                serverTimeUsage = static_cast<int>(totalDurationValue.toString().toDouble() * 3600); // jam -> detik
            } else {
                serverTimeUsage = totalDurationValue.toInt() * 3600; // jam -> detik
            }
        }

        bool taskExists = existingTasks.contains(taskId);

        if (taskExists) {
            // 15. Untuk tugas yang sudah ada, update berdasarkan data server
            QPair<int, int> currentValues = existingTasks[taskId];
            int currentMaxTime = currentValues.first;
            int currentTimeUsage = currentValues.second;

            // Tentukan max_time yang akan digunakan
            int finalMaxTime = currentMaxTime;
            if (serverMaxTime > 0) {
                // Jika server memberikan duration, gunakan yang lebih besar
                finalMaxTime = qMax(serverMaxTime, currentMaxTime);
            } else if (currentMaxTime == 0) {
                // Jika tidak ada di server dan lokal juga 0, set default 8 jam
                finalMaxTime = 8 * 3600;
            }

            // Tentukan time_usage yang akan digunakan
            int finalTimeUsage = currentTimeUsage;
            if (serverTimeUsage > 0) {
                // Jika server memberikan total_duration, gunakan nilai server
                finalTimeUsage = serverTimeUsage;
            }

            query.prepare("UPDATE task SET project_name = :projectName, task = :taskDesc, "
                          "max_time = :maxTime, time_usage = :timeUsage WHERE id = :id");
            query.bindValue(":id", taskId);
            query.bindValue(":projectName", projectName);
            query.bindValue(":taskDesc", taskDesc);
            query.bindValue(":maxTime", finalMaxTime);
            query.bindValue(":timeUsage", finalTimeUsage);

            qDebug() << "Updating task ID" << taskId
                     << "- max_time:" << finalMaxTime << "(current:" << currentMaxTime << ", server:" << serverMaxTime << ")"
                     << "- time_usage:" << finalTimeUsage << "(current:" << currentTimeUsage << ", server:" << serverTimeUsage << ")";
        } else {
            // 16. Untuk tugas baru, masukkan dengan nilai dari server atau default
            int finalMaxTime = (serverMaxTime > 0) ? serverMaxTime : 8 * 3600; // default 8 jam
            int finalTimeUsage = (serverTimeUsage > 0) ? serverTimeUsage : 0;

            query.prepare("INSERT INTO task (id, project_name, task, max_time, time_usage, active, status, paused, user_id) "
                          "VALUES (:id, :projectName, :taskDesc, :maxTime, :timeUsage, 0, 'Pending', 0, :userId)");
            query.bindValue(":id", taskId);
            query.bindValue(":projectName", projectName);
            query.bindValue(":taskDesc", taskDesc);
            query.bindValue(":maxTime", finalMaxTime);
            query.bindValue(":timeUsage", finalTimeUsage);
            query.bindValue(":userId", userId);

            qDebug() << "Inserting new task ID" << taskId
                     << "with max_time:" << finalMaxTime
                     << "and time_usage:" << finalTimeUsage;
        }

        // 17. Eksekusi query dan tangani kesalahan
        if (!query.exec()) {
            qWarning() << "Failed to save task ID" << taskId << ":" << query.lastError().text();
            continue;
        }

        qDebug() << "Task" << (taskExists ? "updated" : "inserted") << ": ID =" << taskId
                 << ", Project =" << projectName << ", User ID =" << userId;
    }

    // 18. Commit transaksi atau rollback jika gagal
    if (!m_productivityDb.commit()) {
        qWarning() << "Failed to commit transaction:" << m_productivityDb.lastError().text();
        m_productivityDb.rollback();
    } else {
        qDebug() << "Successfully processed" << tasksArray.size() << "tasks";
        emit taskListChanged(); // Beri tahu UI bahwa daftar tugas telah diperbarui
    }

    // 19. Hapus objek reply untuk mencegah kebocoran memori
    reply->deleteLater();
}




QVariantList Logger::taskList() const
{
    if (!ensureProductivityDatabaseOpen()) {
        qWarning() << "Cannot fetch task list: Database is not open";
        return QVariantList();
    }
    if (m_currentUserId == -1) {
        qWarning() << "Cannot fetch task list: No user logged in";
        return QVariantList();
    }
    QVariantList tasks;
    QSqlQuery query(m_productivityDb);
    query.prepare("SELECT id, project_name, task, max_time, time_usage, active, status FROM task WHERE user_id = :user_id");
    query.bindValue(":user_id", m_currentUserId);
    if (!query.exec()) {
        qWarning() << "Failed to fetch tasks:" << query.lastError().text();
        return tasks;
    }
    while (query.next()) {
        QVariantMap task;
        task["id"] = query.value(0).toInt();
        task["project_name"] = query.value(1).toString();
        task["task"] = query.value(2).toString();
        task["max_time"] = query.value(3).toInt();
        task["time_usage"] = query.value(4).toInt();
        task["active"] = query.value(5).toBool();
        QString status = query.value(6).toString();

        // Handle status logic
        if (status.toLower() == "review") {
            // Task is under review - keep original status
            task["status"] = "Review";
        } else if (task["active"].toBool()) {
            // Active task - show current state
            status = m_isTaskPaused ? "Paused" : "Is Running";
            task["status"] = status;
        } else {
            // Inactive task
            task["status"] = "Pending";
        }

        tasks.append(task);
    }
    return tasks;
}


void Logger::migrateProductivityDatabase()
{
    if (!ensureProductivityDatabaseOpen()) {
        qWarning() << "Cannot migrate productivity database: Not open";
        return;
    }

    QSqlQuery query(m_productivityDb);

    // Tambah kolom user_id ke tabel task jika belum ada
    query.exec("PRAGMA table_info(task)");
    bool hasUserId = false;
    while (query.next()) {
        if (query.value("name").toString() == "user_id") {
            hasUserId = true;
            break;
        }
    }
    if (!hasUserId) {
        if (!query.exec("ALTER TABLE task ADD COLUMN user_id INTEGER NOT NULL DEFAULT 0")) {
            qWarning() << "Failed to add user_id to task table:" << query.lastError().text();
        }
    }

    // Tambah kolom user_id ke tabel completed_tasks jika belum ada
    query.exec("PRAGMA table_info(completed_tasks)");
    hasUserId = false;
    while (query.next()) {
        if (query.value("name").toString() == "user_id") {
            hasUserId = true;
            break;
        }
    }
    if (!hasUserId) {
        if (!query.exec("ALTER TABLE completed_tasks ADD COLUMN user_id INTEGER NOT NULL DEFAULT 0")) {
            qWarning() << "Failed to add user_id to completed_tasks table:" << query.lastError().text();
        }
    }
}


void Logger::setActiveTask(int taskId)
{
    if (!ensureProductivityDatabaseOpen()) {
        qWarning() << "Cannot set active task: Database is not open";
        return;
    }

    QSqlQuery query(m_productivityDb);

    // Simpan time_usage untuk tugas aktif sebelumnya (jika ada) dan kirim status stop
    if (m_activeTaskId != -1) {
        // Cek status tugas sebelumnya
        query.prepare("SELECT status, task FROM task WHERE id = :id");
        query.bindValue(":id", m_activeTaskId);
        if (!query.exec() || !query.next()) {
            qWarning() << "Failed to fetch status for previous task:" << query.lastError().text();
            return;
        }

        QString prevStatus = query.value(0).toString().toLower();
        QString taskName = query.value(1).toString();

        // Hanya ubah status ke 'Pending' jika bukan 'review' atau 'completed'
        QString newStatus = (prevStatus == "review" || prevStatus == "completed") ? prevStatus : "Pending";

        // Hitung time_usage
        qint64 currentEpoch = QDateTime::currentSecsSinceEpoch();
        qint64 timeUsed = m_taskTimeOffset + (m_isTaskPaused ? 0 : (currentEpoch - m_taskStartTime));

        // Update tugas sebelumnya
        query.prepare("UPDATE task SET active = 0, status = :status, time_usage = :timeUsage, paused = 0 WHERE id = :id");
        query.bindValue(":status", newStatus);
        query.bindValue(":timeUsage", timeUsed);
        query.bindValue(":id", m_activeTaskId);
        if (!query.exec()) {
            qWarning() << "Failed to update previous task:" << query.lastError().text();
        }

        // Kirim status stop untuk tugas sebelumnya
        if (!m_isTaskPaused) {
            QString currentTime = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
            QString startTime = QDateTime::fromSecsSinceEpoch(m_taskStartTime).toString(Qt::ISODateWithMs);
            sendPausePlayDataToAPI(m_activeTaskId, startTime, currentTime, "pause");
        }

        //stopPingTimer();
        //saveWorkTimeData();
    }

    // Jika taskId valid, set tugas baru
    if (taskId != -1) {
        // Ambil time_usage dari tugas baru
        query.prepare("SELECT time_usage FROM task WHERE id = :id");
        query.bindValue(":id", taskId);
        if (!query.exec() || !query.next()) {
            qWarning() << "Failed to fetch time_usage for task:" << query.lastError().text();
            return;
        }
        m_taskTimeOffset = query.value(0).toInt();
        m_taskStartTime = QDateTime::currentSecsSinceEpoch();

        // Aktifkan tugas baru dengan status paused
        query.prepare("UPDATE task SET active = 1, status = 'Paused', paused = 1 WHERE id = :id");
        query.bindValue(":id", taskId);
        if (!query.exec()) {
            qWarning() << "Failed to activate task:" << query.lastError().text();
            return;
        }

        // Kirim status pause untuk tugas baru
        QString currentTime = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
        //sendPausePlayDataToAPI(taskId, currentTime, currentTime, "pause");

        // Set max time untuk tugas
        setMaxTimeForTask(taskId);

        // Gunakan QTimer untuk memulai task setelah delay 3 detik
        QTimer::singleShot(2000, this, [this, taskId, currentTime]() {
            if (m_activeTaskId == taskId) { // Pastikan task masih aktif
                // Resume task setelah delay
                QSqlQuery resumeQuery(m_productivityDb);
                resumeQuery.prepare("UPDATE task SET paused = 0, status = 'on-progress' WHERE id = :id");
                resumeQuery.bindValue(":id", taskId);
                if (!resumeQuery.exec()) {
                    qWarning() << "Failed to resume task:" << resumeQuery.lastError().text();
                    return;
                }

                // Update local state
                m_isTaskPaused = false;
                m_isTrackingActive = true;
                m_taskStartTime = QDateTime::currentSecsSinceEpoch();
                m_pauseStartTime = 0;

                // Mulai periode play baru
                QString newTime = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
                QSqlQuery logQuery(m_productivityDb);
                logQuery.prepare(
                    "UPDATE log_paused "
                    "SET end_reality = ? "
                    "WHERE task_id = ? AND current_status = 'pause' AND end_reality IS NULL");
                logQuery.addBindValue(newTime);
                logQuery.addBindValue(taskId);
                logQuery.exec();

                logQuery.prepare(
                    "INSERT INTO log_paused (task_id, start_reality, end_reality, current_status) "
                    "VALUES (?, ?, NULL, 'play')");
                logQuery.addBindValue(taskId);
                logQuery.addBindValue(newTime);
                logQuery.exec();

                startPingTimer(taskId);

                // Emit signals
                emit taskPausedChanged();
                emit trackingActiveChanged();
                emit taskListChanged();

                qDebug() << "Task automatically resumed after delay";
            }
        });
    }

    // Sinkronkan tugas
    //fetchAndStoreTasks();
    //syncActiveTask();

    // Update state
    m_activeTaskId = taskId;
    m_isTaskPaused = true; // Awalnya di-pause, akan di-resume setelah delay
    m_isTrackingActive = false;
    m_pauseStartTime = QDateTime::currentSecsSinceEpoch();

    // Emit sinyal
    emit taskPausedChanged();
    emit trackingActiveChanged();
    emit activeTaskChanged();
    emit taskListChanged();
}

void Logger::sendPing(int taskId)
{
    if (!ensureProductivityDatabaseOpen()) {
        qWarning() << "Cannot send ping: Database is not open";
        return;
    }

    if (m_authToken.isEmpty()) {
        qWarning() << "Cannot send ping: No authentication token available";
        return;
    }

    if (m_currentUserId == -1) {
        qWarning() << "Cannot send ping: No user logged in";
        return;
    }

    // Buat payload JSON sesuai format yang diminta
    QJsonObject payload;
    payload["task_id"] = QString::number(taskId); // Task ID sebagai string

    // Konfigurasi request
    QNetworkRequest request;
    request.setUrl(QUrl("https://deskmon.pranala-dt.co.id/api/ping"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_authToken.toUtf8());

    qDebug() << "Sending ping with payload:" << QJsonDocument(payload).toJson();

    // Kirim request POST
    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(payload).toJson());

    // Handle timeout (30 detik)
    QTimer::singleShot(30000, reply, &QNetworkReply::abort);

    // Handle response
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        QByteArray responseData = reply->readAll();
        QString responseText = QString::fromUtf8(responseData);

        QJsonParseError parseError;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData, &parseError);

        bool showPopup = false;
        QString popupMessage;
        bool refreshRequired = false;

        if (parseError.error == QJsonParseError::NoError && jsonDoc.isObject()) {
            QJsonObject jsonObj = jsonDoc.object();

            // Cek jika response meminta refresh
            if (jsonObj.contains("refresh_required") && jsonObj["refresh_required"].toBool()) {
                refreshRequired = true;
                qDebug() << "Server requested application refresh";
            }

            if (jsonObj.contains("success") && jsonObj["success"].isBool()) {
                bool success = jsonObj["success"].toBool();
                if (!success) {
                    showPopup = true;
                    popupMessage = "API returned error:\n\n" + responseText;
                } else {
                    qDebug() << "Success response from server:" << responseText;
                }
            } else {
                showPopup = true;
                popupMessage = "Invalid response format (no 'success' field):\n\n" + responseText;
            }
        } else {
            showPopup = true;
            popupMessage = "Failed to parse JSON response:\n\n" + responseText;
        }

        if (reply->error() != QNetworkReply::NoError) {
            showPopup = true;
            popupMessage = "Network error:\n" + reply->errorString();
        }

        if (showPopup) {
            QMessageBox::warning(nullptr, "API Response", popupMessage);
        }

        // Jika server meminta refresh, panggil refreshAll()
        if (refreshRequired) {
            qDebug() << "Performing application refresh as requested by server";
            this->refreshAll();
        }

        reply->deleteLater();
    });
}



void Logger::startPingTimer(int taskId)
{
    if (taskId <= 0 || m_currentUserId == -1) return;

    // Kirim ping pertama segera
    sendPing(taskId);

    // Mulai timer untuk ping berikutnya
    m_pingTimer.start();
    qDebug() << "Started ping timer for client_id:" << m_currentUserId << "and task_id:" << taskId;
}

void Logger::stopPingTimer()
{
    m_pingTimer.stop();
    qDebug() << "Stopped ping timer";
}


QString Logger::getUserPassword(const QString &username) {
    // Implementasi query database untuk mendapatkan password
    QSqlQuery query;
    query.prepare("SELECT password FROM users WHERE username = ?");
    query.addBindValue(username);

    if (query.exec() && query.next()) {
        return query.value(0).toString();
    }
    return "";
}

void Logger::updateTaskStatus(int taskId)
{
    if (!ensureProductivityDatabaseOpen()) {
        qWarning() << "Cannot update task status: Database is not open";
        return;
    }

    if (m_currentUserId == -1) {
        qWarning() << "Cannot update task status: No user logged in";
        return;
    }

    // Validasi taskId
    if (taskId <= 0) {
        qWarning() << "Invalid taskId:" << taskId;
        return;
    }

    // Validasi task ID dan kepemilikan user
    QSqlQuery query(m_productivityDb);
    query.prepare("SELECT user_id FROM task WHERE id = :id");
    query.bindValue(":id", taskId);
    if (!query.exec() || !query.next()) {
        qWarning() << "Task not found or invalid task ID:" << taskId;
        return;
    }
    if (query.value(0).toInt() != m_currentUserId) {
        qWarning() << "Task ID" << taskId << "does not belong to current user:" << m_currentUserId;
        return;
    }

    // Buat URL dan request
    QString url = QString("https://deskmon.pranala-dt.co.id/api/get-current-task-status/%1").arg(taskId);
    if (url.isEmpty()) {
        qWarning() << "URL is empty for taskId:" << taskId;
        return;
    }

    QNetworkRequest request;
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (!m_authToken.isEmpty()) {
        request.setRawHeader("Authorization", "Bearer " + m_authToken.toUtf8());
    } else {
        qWarning() << "No authentication token available for taskId:" << taskId;
        return;
    }

    // Kirim request
    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, taskId]() {
        handleTaskStatusReply(reply, taskId);
    });
}

void Logger::handleTaskStatusReply(QNetworkReply *reply, int taskId)
{
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Failed to get task status for taskId" << taskId << ": Network error:" << reply->errorString();
        reply->deleteLater();
        return;
    }

    QByteArray responseData = reply->readAll();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
    if (jsonDoc.isNull() || !jsonDoc.isObject()) {
        qWarning() << "Invalid JSON response for taskId" << taskId << ":" << responseData;
        reply->deleteLater();
        return;
    }

    QJsonObject jsonObj = jsonDoc.object();
    if (!jsonObj["success"].toBool()) {
        qWarning() << "Task status API failed for taskId" << taskId << ":" << jsonObj["message"].toString();
        reply->deleteLater();
        return;
    }

    QString apiStatus = jsonObj["data"].toString();
    if (apiStatus.isEmpty()) {
        qWarning() << "No status data in response for taskId" << taskId;
        reply->deleteLater();
        return;
    }

    // Dapatkan nama task untuk notifikasi
    QString taskName = getTaskName(taskId);


    // Petakan status API ke status database
    QString dbStatus;
    bool isReviewStatus = false;
    bool isNeedReview = false;
    bool isNeedRevise = false;

    if (apiStatus == "created" || apiStatus == "pending") {
        dbStatus = "Pending";
    }
    else if (apiStatus == "on-progress") {
        // aktifkan task ini langsung
        setActiveTask(taskId);
        m_isTaskPaused = false;
        m_isTrackingActive = true;
        m_taskStartTime = QDateTime::currentSecsSinceEpoch();
        dbStatus = "On Progress";

        QSqlQuery timeQuery(m_productivityDb);
        timeQuery.prepare("SELECT time_usage FROM task WHERE id = :id");
        timeQuery.bindValue(":id", taskId);
        if (timeQuery.exec() && timeQuery.next()) {
            m_taskTimeOffset = timeQuery.value(0).toInt();
        }

        qDebug() << "Task with status 'on-progress' set as active from handleTaskStatusReply. Task ID:" << taskId;
    }
    else if (apiStatus == "on-review") {
        dbStatus = "Review";
        isReviewStatus = true;
        QString message = QString("Task '%1' is under system review").arg(taskName);
        emit taskReviewNotification(message);
    }
    else if (apiStatus == "need-review") {
        dbStatus = "Need Review";
        isNeedReview = true;
    }
    else if (apiStatus == "need-revise") {
        dbStatus = "Need Revise";
        isNeedRevise = true;
    }
    else if (apiStatus == "completed") {
        dbStatus = "completed";
        finishTask(taskId);
        reply->deleteLater();
        return;
    }
    else {
        qWarning() << "Unknown status for taskId" << taskId << ":" << apiStatus;
        reply->deleteLater();
        return;
    }

    // Hanya pause task jika status on-review (bukan Need Review)
    if (isReviewStatus && m_activeTaskId == taskId) {
        QSqlQuery pauseQuery(m_productivityDb);
        pauseQuery.prepare("UPDATE task SET active = 0, paused = 1 WHERE id = :id");
        pauseQuery.bindValue(":id", taskId);
        if (pauseQuery.exec()) {
            m_activeTaskId = -1;
            m_isTaskPaused = false;
            emit activeTaskChanged();
            emit taskPausedChanged();
        }
    }

    emit taskStatusChanged(taskId, dbStatus);

    // Update database
    QSqlQuery query(m_productivityDb);
    query.prepare("UPDATE task SET status = :status WHERE id = :id AND user_id = :user_id");
    query.bindValue(":status", dbStatus);
    query.bindValue(":id", taskId);
    query.bindValue(":user_id", m_currentUserId);
    if (!query.exec()) {
        qWarning() << "Failed to update task status for taskId" << taskId << ":" << query.lastError().text();
    } else {
        qDebug() << "Task status updated to" << dbStatus << "for taskId" << taskId;
        // If task is active and status changed to Review, deselect it
        if (dbStatus == "review" && m_activeTaskId == taskId) {
            query.prepare("UPDATE task SET active = 0, paused = 0 WHERE id = :id");
            query.bindValue(":id", taskId);
            if (!query.exec()) {
                qWarning() << "Failed to deselect task ID" << taskId << ":" << query.lastError().text();
            } else {
                m_activeTaskId = -1;
                m_isTaskPaused = false;
                qDebug() << "Deselected task ID" << taskId << "due to Review status";
                emit activeTaskChanged();
                emit taskPausedChanged();

                QString pauseMessage = QString("Task '%1' has been paused automatically because it's under review.").arg(taskName);
                emit taskReviewNotification(pauseMessage);
            }
        }
        emit taskListChanged();
    }

    reply->deleteLater();
}

QString Logger::getTaskName(int taskId)
{
    if (!ensureProductivityDatabaseOpen()) {
        qWarning() << "Cannot get task name: Database is not open";
        return "Unknown Task";
    }

    QSqlQuery query(m_productivityDb);
    query.prepare("SELECT task FROM task WHERE id = :id AND user_id = :user_id");
    query.bindValue(":id", taskId);
    query.bindValue(":user_id", m_currentUserId);

    if (!query.exec() || !query.next()) {
        qWarning() << "Failed to get task name for ID" << taskId << ":" << query.lastError().text();
        return "Unknown Task";
    }

    return query.value(0).toString();
}


void Logger::finishTask(int taskId)
{
    if (!ensureProductivityDatabaseOpen()) {
        qWarning() << "Cannot finish task: Database is not open";
        return;
    }

    if (m_currentUserId == -1) {
        qWarning() << "Cannot finish task: No user logged in";
        return;
    }

    QSqlQuery query(m_productivityDb);

    // Verifikasi bahwa taskId milik user saat ini
    query.prepare("SELECT user_id, project_name, task, max_time, time_usage FROM task WHERE id = :id");
    query.bindValue(":id", taskId);
    if (!query.exec() || !query.next()) {
        qWarning() << "Failed to fetch task details:" << query.lastError().text();
        return;
    }
    if (query.value(0).toInt() != m_currentUserId) {
        qWarning() << "Task ID" << taskId << "does not belong to current user:" << m_currentUserId;
        return;
    }

    QString projectName = query.value(1).toString();
    QString taskDesc = query.value(2).toString();
    int maxTime = query.value(3).toInt();
    int timeUsage = m_activeTaskId == taskId && !m_isTaskPaused
                        ? m_taskTimeOffset + (QDateTime::currentSecsSinceEpoch() - m_taskStartTime)
                        : query.value(4).toInt();
    qint64 completedTime = QDateTime::currentSecsSinceEpoch();

    query.prepare("INSERT INTO completed_tasks (project_name, task, max_time, time_usage, completed_time, user_id) "
                  "VALUES (:projectName, :task, :maxTime, :timeUsage, :completedTime, :user_id)");
    query.bindValue(":projectName", projectName);
    query.bindValue(":task", taskDesc);
    query.bindValue(":maxTime", maxTime);
    query.bindValue(":timeUsage", timeUsage);
    query.bindValue(":completedTime", completedTime);
    query.bindValue(":user_id", m_currentUserId);
    if (!query.exec()) {
        qWarning() << "Failed to insert into completed_tasks:" << query.lastError().text();
        return;
    }

    query.prepare("DELETE FROM task WHERE id = :id");
    query.bindValue(":id", taskId);
    if (!query.exec()) {
        qWarning() << "Failed to delete task:" << query.lastError().text();
        return;
    }

    if (m_activeTaskId == taskId) {
        m_activeTaskId = -1;
        m_isTaskPaused = false;
        m_pauseStartTime = 0;
        m_taskTimeOffset = 0;
        m_taskStartTime = 0;
        emit activeTaskChanged();
        emit taskPausedChanged();
    }

    emit taskListChanged();
}


void Logger::setMaxTimeForTask(int taskId)
{
    if (!ensureProductivityDatabaseOpen()) {
        qWarning() << "Cannot set max time for task: Database is not open";
        return;
    }

    QSqlQuery query(m_productivityDb);
    query.prepare("SELECT max_time FROM task WHERE id = :id");
    query.bindValue(":id", taskId);
    if (!query.exec() || !query.next()) {
        qWarning() << "Failed to fetch max_time for task:" << query.lastError().text();
        return;
    }

    int maxTime = query.value(0).toInt();
    if (maxTime == 0) {
        int newMaxTime = 8 * 3600; // 8 hours in seconds
        query.prepare("UPDATE task SET max_time = :maxTime WHERE id = :id");
        query.bindValue(":maxTime", newMaxTime);
        query.bindValue(":id", taskId);
        if (!query.exec()) {
            qWarning() << "Failed to set max_time:" << query.lastError().text();
        }
    }
}

void Logger::toggleTaskPause()
{
    if (!ensureProductivityDatabaseOpen()) {
        qWarning() << "Cannot toggle pause: Productivity database is not open";
        return;
    }

    if (m_activeTaskId == -1) {
        qWarning() << "No active task to pause/resume";
        return;
    }


    // Get current timestamp in ISO format
    QString currentTime = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);

    QSqlQuery query(m_productivityDb);
    m_productivityDb.transaction();  // Start transaction for atomic operations

    try {
        if (!m_isTaskPaused) {
            // CASE 1: Pausing an active task (Play -> Pause)

            // 1. Update time_usage in task table
            qint64 currentEpoch = QDateTime::currentSecsSinceEpoch();
            qint64 timeUsed = m_taskTimeOffset + (currentEpoch - m_taskStartTime);

            query.prepare("UPDATE task SET time_usage = ?, paused = 1, status = 'Paused' WHERE id = ?");
            query.addBindValue(timeUsed);
            query.addBindValue(m_activeTaskId);
            if (!query.exec()) {
                throw std::runtime_error("Failed to update task status");
            }

            // 2. Close the open 'play' period in log_paused
            query.prepare(
                "UPDATE log_paused "
                "SET end_reality = ? "
                "WHERE task_id = ? AND current_status = 'play' AND end_reality IS NULL"
                );
            query.addBindValue(currentTime);
            query.addBindValue(m_activeTaskId);
            if (!query.exec()) {
                throw std::runtime_error("Failed to close play period");
            }

            // 3. Start new 'pause' period
            query.prepare(
                "INSERT INTO log_paused (task_id, start_reality, end_reality, current_status) "
                "VALUES (?, ?, NULL, 'pause')"
                );
            query.addBindValue(m_activeTaskId);
            query.addBindValue(currentTime);
            if (!query.exec()) {
                throw std::runtime_error("Failed to log pause start");
            }
            sendPausePlayDataToAPI(m_activeTaskId,
                                   m_lastPlayStartTime.toString(Qt::ISODateWithMs),
                                   currentTime,
                                   "pause");

            stopPingTimer();
            saveWorkTimeData();

            // Update local state
            m_isTaskPaused = true;
            m_isTrackingActive = false;
            m_taskTimeOffset = timeUsed;
            m_pauseStartTime = currentEpoch;

            qDebug() << "Task paused at" << currentTime;
        } else {
            // CASE 2: Resuming a paused task (Pause -> Play)

            // 1. Close the open 'pause' period in log_paused
            query.prepare(
                "UPDATE log_paused "
                "SET end_reality = ? "
                "WHERE task_id = ? AND current_status = 'pause' AND end_reality IS NULL"
                );
            query.addBindValue(currentTime);
            query.addBindValue(m_activeTaskId);
            if (!query.exec()) {
                throw std::runtime_error("Failed to close pause period");
            }

            // 2. Start new 'play' period
            query.prepare(
                "INSERT INTO log_paused (task_id, start_reality, end_reality, current_status) "
                "VALUES (?, ?, NULL, 'play')"
                );
            query.addBindValue(m_activeTaskId);
            query.addBindValue(currentTime);
            if (!query.exec()) {
                throw std::runtime_error("Failed to log play start");
            }
            startPingTimer(m_activeTaskId);
            // Update local state
            m_isTaskPaused = false;
            m_isTrackingActive = true;
            m_taskStartTime = QDateTime::currentSecsSinceEpoch();
            m_pauseStartTime = 0;

            qDebug() << "Task resumed at" << currentTime;
        }


        if (!m_productivityDb.commit()) {
            throw std::runtime_error("Failed to commit transaction");
        }

        // Emit signals after successful commit
        emit taskPausedChanged();
        emit trackingActiveChanged();
        emit taskListChanged();

    } catch (const std::exception& e) {
        m_productivityDb.rollback();
        qCritical() << "Error in toggleTaskPause:" << e.what();
    }
}

void Logger::sendPausePlayDataToAPI(int taskId, const QString& startTime,
                                    const QString& endTime, const QString& status)
{

    // 1. Validasi token
    if (m_authToken.isEmpty()) {
        qWarning() << "No authentication token available";
        return;
    }

    // 2. Hanya kirim payload jika status pause (stop)
    if (status != "pause") {
        qDebug() << "Skip sending play status to API";
        return;
    }

    // 3. Siapkan payload JSON hanya untuk status stop
    QJsonObject payload;
    payload["status"] = "stop"; // Hanya kirim status stop

    // 4. Konfigurasi request PUT
    QNetworkRequest request;
    QString url = QString("https://deskmon.pranala-dt.co.id/api/end-implementation/%1").arg(taskId);
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_authToken.toUtf8());

    // 5. Debug output
    qDebug() << "Sending PUT to:" << url;
    qDebug() << "Payload:" << QJsonDocument(payload).toJson(QJsonDocument::Indented);

    // 6. Kirim request PUT
    QBuffer *buffer = new QBuffer();
    buffer->setData(QJsonDocument(payload).toJson());
    buffer->open(QIODevice::ReadOnly);

    QNetworkReply* reply = m_networkManager->put(request, buffer);
    buffer->setParent(reply);  // Auto-delete buffer ketika reply dihapus

    // 7. Handle timeout (30 detik)
    QTimer::singleShot(30000, reply, &QNetworkReply::abort);

    // 8. Handle response
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        QByteArray responseData = reply->readAll();
        QString responseText = QString::fromUtf8(responseData);

        QJsonParseError parseError;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData, &parseError);

        bool showPopup = false;
        QString popupMessage;

        if (parseError.error == QJsonParseError::NoError && jsonDoc.isObject()) {
            QJsonObject jsonObj = jsonDoc.object();
            if (jsonObj.contains("success") && jsonObj["success"].isBool()) {
                bool success = jsonObj["success"].toBool();
                if (!success) {
                    showPopup = true;
                    popupMessage = "API returned error:\n\n" + responseText;
                } else {
                    qDebug() << "Success response from server:" << responseText;
                }
            } else {
                showPopup = true;
                popupMessage = "Invalid response format (no 'success' field):\n\n" + responseText;
            }
        } else {
            showPopup = true;
            popupMessage = "Failed to parse JSON response:\n\n" + responseText;
        }

        if (reply->error() != QNetworkReply::NoError) {
            showPopup = true;
            popupMessage = "Network error:\n" + reply->errorString();
        }

        if (showPopup) {
            QMessageBox::warning(nullptr, "API Response", popupMessage);
        }

        reply->deleteLater();
    });
}

void Logger::revertTaskChange()
{
    if (!ensureProductivityDatabaseOpen()) {
        qWarning() << "Cannot revert task change: Database is not open";
        return;
    }

    QSqlQuery query(m_productivityDb);

    // Revert the active task to previous state
    if (m_activeTaskId != -1) {
        // Reactivate the previous task if it was paused
        query.prepare("UPDATE task SET active = 1, paused = 1 WHERE id = :id");
        query.bindValue(":id", m_activeTaskId);
        if (!query.exec()) {
            qWarning() << "Failed to reactivate previous task:" << query.lastError().text();
        }

        // Update local state
        m_isTaskPaused = true;
        m_isTrackingActive = false;
    }

    emit taskListChanged();
    emit taskPausedChanged();
    emit trackingActiveChanged();
    emit activeTaskChanged();
}


//void Logger::updateTaskTime()
//{
//    m_globalTimeUsage += 1;
//    emit globalTimeUsageChanged();
//
//    if (m_isTaskPaused || m_activeTaskId == -1 || !ensureProductivityDatabaseOpen()) {
//        return;
//    }
//
//    QSqlQuery query(m_productivityDb);
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



QString Logger::formatDuration(int seconds) const
{
    if (seconds < 60) {
        return QString("%1s").arg(seconds);
    } else if (seconds < 3600) {
        int minutes = seconds / 60;
        int secs = seconds % 60;
        return QString("%1m %2s").arg(minutes).arg(secs);
    } else {
        int hours = seconds / 3600;
        int mins = (seconds % 3600) / 60;
        int secs = seconds % 60;
        return QString("%1h %2m %3s").arg(hours).arg(mins).arg(secs);
    }
}

QString Logger::debugShowRawData() const
{
    if (!ensureDatabaseOpen()) {
        qWarning() << "Cannot fetch raw data: Database is not open";
        return "";
    }

    QString result;
    QSqlQuery query(m_db);
    query.prepare("SELECT start_time, datetime(start_time, 'unixepoch', 'localtime') as start_date, app_name, title FROM log ORDER BY start_time DESC LIMIT 10");
    if (!query.exec()) {
        qWarning() << "Failed to fetch raw data:" << query.lastError().text();
        return result;
    }
    while (query.next()) {
        result += QString("%1 | %2 | %3 | %4\n")
        .arg(query.value(0).toString())
            .arg(query.value(1).toString())
            .arg(query.value(2).toString())
            .arg(query.value(3).toString());
    }
    return result;
}

void Logger::showLogs()
{
    emit logContentChanged();
}

bool Logger::authenticate(const QString &loginInput, const QString &password)
{
    if (!ensureDatabaseOpen()) {
        qWarning() << "Cannot authenticate: Database is not open";
        return false;
    }

    // Deteksi apakah input adalah email atau username
    bool isEmail = loginInput.contains("@");
    QString loginType = isEmail ? "email" : "username";
    qDebug() << "Attempting login with" << loginType << ":" << loginInput;

    // 1. Buat HTTP request ke API
    QNetworkRequest request(QUrl("https://deskmon.pranala-dt.co.id/api/login"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject jsonPayload;
    if (isEmail) {
        jsonPayload["email"] = loginInput;
    } else {
        // Logika untuk mencari email dari username (jika ada)
        QSqlQuery emailQuery(m_db);
        emailQuery.prepare("SELECT email FROM users WHERE username = :username");
        emailQuery.bindValue(":username", loginInput);
        if (emailQuery.exec() && emailQuery.next()) {
            jsonPayload["email"] = emailQuery.value(0).toString();
        } else {
            jsonPayload["email"] = loginInput;
        }
    }
    jsonPayload["password"] = password;
    QJsonDocument doc(jsonPayload);
    QByteArray data = doc.toJson();

    qDebug() << "Sending login request with payload:" << doc.toJson(QJsonDocument::Compact);

    // 2. Kirim request dan tunggu response
    QNetworkReply *reply = m_networkManager->post(request, data);
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    // 3. Handle response dari API
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        QJsonDocument jsonResponse = QJsonDocument::fromJson(response);
        QJsonObject jsonObj = jsonResponse.object();

        qDebug() << "API Response:" << jsonResponse.toJson(QJsonDocument::Compact);

        if (jsonObj["success"].toBool()) {
            // Jika API login berhasil
            qDebug() << "API login successful. Storing user data...";

            // 5. Parse data user dari response
            QJsonObject userData = jsonObj["user"].toObject();
            int userId = userData["id"].toInt();
            QString username = userData["name"].toString();
            QString userEmail = userData["email"].toString();
            QString role = userData["role"].toObject()["rolename"].toString();
            QString department = userData["department"].toObject()["rolename"].toString();
            QString token = jsonObj["token"].toString();
            // 6. Siapkan dan eksekusi query untuk menyimpan data ke database
            QSqlQuery query(m_db);
            query.prepare(
                "INSERT OR REPLACE INTO users "
                "(id, username, password, email, department, role, token) "
                "VALUES (:id, :username, :password, :email, :department, :role, :token)"
                );
            query.bindValue(":id", userId);
            query.bindValue(":username", username);
            query.bindValue(":password", hashPassword(password)); // Hash password
            query.bindValue(":email", userEmail);
            query.bindValue(":role", role);
            query.bindValue(":department", department);
            query.bindValue(":token", token);

            if (!query.exec()) {
                qWarning() << "Gagal menyimpan user ke database lokal:" << query.lastError();
            } else {
                qDebug() << "Data user tersimpan di database lokal. ID:" << userId;
            }



            m_isTokenErrorVisible = false;

            m_workTimer.start(1000);
            m_workTimer.start();
            // Lanjutkan sisa proses
            setCurrentUserInfo(userId, username, userEmail);
            checkAndCreateNewDayRecord();
            loadWorkTimeData();
            startGlobalTimer();
            syncActiveTask();
            fetchAndStoreTasks();
            m_usageReportTimer.start();
            m_isTrackingActive = true;
            m_isTaskPaused = false;
            m_pauseStartTime = 0;


            reply->deleteLater();
            return true;
        } else {
            qWarning() << "API Login failed:" << jsonObj["message"].toString();
        }
    } else {
        qWarning() << "Network error during API login:" << reply->errorString();
    }

    // Jika kode sampai di sini, artinya API login gagal. Lakukan fallback ke login lokal.
    qDebug() << "Attempting local login fallback...";
    QSqlQuery localQuery(m_db);
    localQuery.prepare("SELECT id, username, email, password, token FROM users WHERE email = :loginInput OR username = :loginInput");
    localQuery.bindValue(":loginInput", loginInput);

    if (localQuery.exec() && localQuery.next()) {
        if (localQuery.value(3).toString() == hashPassword(password)) {
            // Login lokal berhasil
            int userId = localQuery.value(0).toInt();
            QString username = localQuery.value(1).toString();
            QString userEmail = localQuery.value(2).toString();
            m_authToken = localQuery.value(4).toString();

            qDebug() << "Local login successful for user:" << username;
            qDebug() << "Using stored token:" << (m_authToken.isEmpty() ? "No token" : "Token available");

            setCurrentUserInfo(userId, username, userEmail);
            checkAndCreateNewDayRecord();

            m_workTimer.start(1000);
            m_workTimer.start();
            loadWorkTimeData();
            startGlobalTimer();
            syncActiveTask();
            if (!m_authToken.isEmpty()) { fetchAndStoreTasks(); }

            reply->deleteLater();
            return true;
        }
    }

    qDebug() << "Login failed completely (API and Local).";
    reply->deleteLater();
    return false;
}

// Helper function untuk set current user info dan emit signals
void Logger::setCurrentUserInfo(int userId, const QString &username, const QString &email)
{
    m_currentUserId = userId;
    m_currentUsername = username;
    m_currentUserEmail = email;
    m_userEmail = email;

    emit currentUserIdChanged();
    emit currentUsernameChanged();
    emit currentUserEmailChanged();
    emit userEmailChanged();

    qDebug() << "Current user set - ID:" << userId << ", Username:" << username << ", Email:" << email;
}

QString Logger::getCurrentToken() const {
    return m_authToken;
}

void Logger::clearToken() {
    m_authToken.clear();

    // Also clear token from database
    if (m_currentUserId > 0) {
        QSqlQuery query(m_db);
        query.prepare("UPDATE users SET token = NULL WHERE id = ?");
        query.addBindValue(m_currentUserId);
        if (!query.exec()) {
            qWarning() << "Failed to clear token from database:" << query.lastError().text();
        }
    }

    qDebug() << "Token cleared from memory and database";
}

QString Logger::getUserEmail(const QString &username)
{
    if (!ensureDatabaseOpen()) {
        qWarning() << "Cannot get user email: Database is not open";
        return "";
    }

    QSqlQuery query(m_db);
    query.prepare("SELECT email FROM users WHERE username = :username");
    query.bindValue(":username", username);

    if (query.exec() && query.next()) {
        QString email = query.value(0).toString();
        qDebug() << "Found email for user" << username << ":" << email;
        return email;
    }

    qWarning() << "Failed to get email for user:" << username;
    return "";
}

QString Logger::getUserDepartment(const QString &username)
{
    if (!ensureDatabaseOpen()) {
        qWarning() << "Cannot get user department: Database is not open";
        return "";
    }

    QSqlQuery query(m_db);
    query.prepare("SELECT role FROM users WHERE username = :username");
    query.bindValue(":username", username);

    if (query.exec() && query.next()) {
        QString dept = query.value(0).toString();
        qDebug() << "Found department/role for user" << username << ":" << dept;
        return dept;
    }

    qWarning() << "Failed to get department for user:" << username;
    return "";
}



QString Logger::hashPassword(const QString &password)
{
    return QString(QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex());
}

bool Logger::isUsernameTaken(const QString &username)
{
    if (!ensureDatabaseOpen()) {
        qWarning() << "Cannot check username: Database is not open";
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare("SELECT COUNT(*) FROM users WHERE username = :username");
    query.bindValue(":username", username);
    if (!query.exec()) {
        qWarning() << "Failed to check username:" << query.lastError().text();
        return false;
    }

    if (query.next()) {
        return query.value(0).toInt() > 0;
    }
    return false;
}



QString Logger::updateUserProfile(const QString &currentUsername, const QString &newUsername, const QString &newPassword)
{
    if (!ensureDatabaseOpen()) {
        return "Database is not accessible";
    }

    if (currentUsername.isEmpty() || newUsername.isEmpty()) {
        return "Username cannot be empty";
    }

    QSqlQuery query(m_db);
    if (newUsername != currentUsername && isUsernameTaken(newUsername)) {
        return "Username already taken";
    }

    // Prepare the query based on whether password is being changed
    if (newPassword.isEmpty()) {
        query.prepare("UPDATE users SET username = :newUsername WHERE username = :currentUsername");
    } else {
        query.prepare("UPDATE users SET username = :newUsername, password = :newPassword WHERE username = :currentUsername");
        query.bindValue(":newPassword", hashPassword(newPassword));
    }

    query.bindValue(":newUsername", newUsername);
    query.bindValue(":currentUsername", currentUsername);

    if (!query.exec()) {
        qWarning() << "Failed to update user profile:" << query.lastError().text();
        return "Database error: " + query.lastError().text();
    }

    if (query.numRowsAffected() == 0) {
        qWarning() << "No user found with username:" << currentUsername;
        return "User not found";
    }

    qDebug() << "User profile updated successfully for:" << newUsername;
    return "";
}



QString Logger::cropProfileImage(const QString &imagePath, qreal x, qreal y, qreal imageWidth, qreal imageHeight, qreal cropWidth, qreal cropHeight)
{
    QString localPath = imagePath;
    if (localPath.startsWith("file:///")) {
        localPath = QUrl(imagePath).toLocalFile();
    } else if (localPath.startsWith("file://")) {
        localPath = localPath.mid(7);
    }

    qDebug() << "Cropping image from path:" << localPath;

    QFileInfo fileInfo(localPath);
    if (!fileInfo.exists()) {
        qWarning() << "Image file does not exist:" << localPath;
        return "";
    }
    if (!fileInfo.isFile() || !fileInfo.isReadable()) {
        qWarning() << "Image file is not a valid file or is not readable:" << localPath;
        return "";
    }

    QImage image(localPath);
    if (image.isNull()) {
        qWarning() << "Failed to load image:" << localPath << "- Possibly corrupted or unsupported format";
        return "";
    }

    qreal scaleX = image.width() / imageWidth;
    qreal scaleY = image.height() / imageHeight;

    int cropSize = qMin(cropWidth, cropHeight) * scaleX;

    int cropX = (-x) * scaleX;
    int cropY = (-y) * scaleY;

    cropX = qMax(0, qMin(cropX, image.width() - cropSize));
    cropY = qMax(0, qMin(cropY, image.height() - cropSize));

    qDebug() << "Crop parameters: x=" << cropX << ", y=" << cropY << ", size=" << cropSize;

    QImage cropped = image.copy(cropX, cropY, cropSize, cropSize);
    if (cropped.isNull()) {
        qWarning() << "Failed to crop image: Invalid crop parameters";
        return "";
    }

    QImage circularImage(cropped.size(), QImage::Format_ARGB32);
    circularImage.fill(Qt::transparent);

    QPainter painter(&circularImage);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(QBrush(cropped));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(0, 0, cropSize, cropSize);
    painter.end();

    // Simpan ke direktori permanen dengan subfolder per user
    QDir appDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    QString userSubDir = QString("profiles/%1").arg(getUsernameById(m_currentUserId));
    if (!appDir.exists(userSubDir)) {
        appDir.mkpath(userSubDir);
    }

    // Gunakan username untuk nama file unik
    QString username = getUsernameById(m_currentUserId);
    if (username.isEmpty()) {
        qWarning() << "Cannot save cropped image: No valid user logged in";
        return "";
    }

    QString outputPath = appDir.filePath(QString("%1/profile_%2_%3.png")
                                             .arg(userSubDir)
                                             .arg(m_currentUserId)
                                             .arg(QDateTime::currentMSecsSinceEpoch()));
    if (!circularImage.save(outputPath, "PNG")) {
        qWarning() << "Failed to save cropped image to:" << outputPath;
        return "";
    }

    qDebug() << "Cropped image saved for user" << username << "to:" << outputPath;

    return QUrl::fromLocalFile(outputPath).toString() + "?t=" + QString::number(QDateTime::currentMSecsSinceEpoch());
}



QString Logger::getUsernameById(int userId) const
{
    if (!ensureDatabaseOpen()) {
        qWarning() << "Cannot retrieve username: Database is not open";
        return "";
    }

    QSqlQuery query(m_db);
    query.prepare("SELECT username FROM users WHERE id = :id");
    query.bindValue(":id", userId);

    if (!query.exec() || !query.next()) {
        qWarning() << "Failed to retrieve username:" << query.lastError().text();
        return "";
    }

    return query.value(0).toString();
}

void Logger::clearLogFilter()
{
    m_startDateFilter = "";
    m_endDateFilter = "";
    emit logContentChanged();
    emit logCountChanged();
    emit productivityStatsChanged();
}

void Logger::logActiveWindow()
{
    if (!m_isTrackingActive || m_isTaskPaused) {
        return; // Jangan catat aktivitas jika tracking nonaktif atau task paused
    }

    if (!ensureDatabaseOpen()) {
        qWarning() << "Cannot log active window: Database is not open";
        return;
    }

    qint64 currentTime = QDateTime::currentSecsSinceEpoch();
    WindowInfo currentInfo = getActiveWindowInfo();

    if (m_isFirstCheck) {
        m_lastWindowInfo = currentInfo;
        m_lastActivityTime = currentTime;
        m_isFirstCheck = false;
        return;
    }

    if (currentInfo.appName != m_lastWindowInfo.appName ||
        currentInfo.title != m_lastWindowInfo.title) {
        logWindowChange(m_lastWindowInfo, m_lastActivityTime, currentTime - 1);
        m_lastActivityTime = currentTime;
        m_lastWindowInfo = currentInfo;
    }

    m_currentAppName = currentInfo.appName;
    m_currentWindowTitle = currentInfo.title;
    emit currentAppNameChanged();
    emit currentWindowTitleChanged();
}

void Logger::syncActiveTask()
{
    // 1. Pastikan database produktivitas terbuka
    if (!ensureProductivityDatabaseOpen()) {
        qWarning() << "Cannot sync tasks: Database is not open";
        return;
    }

    // 2. Pastikan ada pengguna yang sedang login
    if (m_currentUserId == -1) {
        qWarning() << "Cannot sync tasks: No user logged in";
        return;
    }

    // 3. Debug: Tampilkan semua tugas untuk pengguna saat ini
    QSqlQuery debugQuery(m_productivityDb);
    debugQuery.prepare("SELECT id, active, user_id, status FROM task WHERE user_id = :user_id");
    debugQuery.bindValue(":user_id", m_currentUserId);
    if (debugQuery.exec()) {
        qDebug() << "Daftar tugas untuk user_id" << m_currentUserId << ":";
        while (debugQuery.next()) {
            qDebug() << "ID:" << debugQuery.value(0).toInt()
            << "Active:" << debugQuery.value(1).toBool()
            << "User ID:" << debugQuery.value(2).toInt()
            << "Status:" << debugQuery.value(3).toString();
        }
    } else {
        qWarning() << "Gagal menampilkan daftar tugas:" << debugQuery.lastError().text();
    }

    // 4. Ambil semua tugas dari database lokal untuk pengguna saat ini
    QSqlQuery query(m_productivityDb);
    query.prepare("SELECT id, paused, time_usage, active FROM task WHERE user_id = :user_id");
    query.bindValue(":user_id", m_currentUserId);
    if (!query.exec()) {
        qWarning() << "Failed to query tasks:" << query.lastError().text();
        return;
    }

    // 5. Simpan ID tugas untuk pembaruan status
    QList<int> taskIds;
    bool hasActiveTask = false;
    while (query.next()) {
        int taskId = query.value(0).toInt();
        if (taskId <= 0) {
            qWarning() << "Invalid task ID found:" << taskId;
            continue;
        }
        taskIds.append(taskId);

        // 6. Perbarui informasi tugas aktif jika ada
        if (query.value(3).toBool()) { // Kolom active
            if (hasActiveTask) {
                qWarning() << "Multiple active tasks detected, resetting previous active task";
                // Nonaktifkan tugas sebelumnya jika ada lebih dari satu tugas aktif
                QSqlQuery resetQuery(m_productivityDb);
                resetQuery.prepare("UPDATE task SET active = 0, paused = 0 WHERE id = :id");
                resetQuery.bindValue(":id", m_activeTaskId);
                if (!resetQuery.exec()) {
                    qWarning() << "Failed to reset previous active task:" << resetQuery.lastError().text();
                }
            }
            m_activeTaskId = taskId;
            m_isTaskPaused = query.value(1).toBool();
            m_taskTimeOffset = query.value(2).toInt();
            m_taskStartTime = QDateTime::currentSecsSinceEpoch();
            hasActiveTask = true;
            qDebug() << "Active task synchronized: ID =" << m_activeTaskId << ", Paused =" << m_isTaskPaused;
        }
    }

    // 7. Jika tidak ada tugas aktif, reset variabel terkait
    if (!hasActiveTask) {
        m_activeTaskId = -1;
        m_isTaskPaused = false;
        m_taskTimeOffset = 0;
        m_taskStartTime = 0;
        qDebug() << "No active task found for user_id:" << m_currentUserId;
    }

    // 8. Sinkronkan semua tugas dengan server
    fetchAndStoreTasks();

    // 9. Perbarui status setiap tugas dari server
    for (int taskId : taskIds) {
        updateTaskStatus(taskId);
    }

    // 10. Emit sinyal untuk memberitahu perubahan ke UI
    emit activeTaskChanged();
    emit taskPausedChanged();
    emit taskListChanged();
}



void Logger::logIdle(qint64 startTime, qint64 endTime)
{
    if (!ensureDatabaseOpen()) {
        qWarning() << "Cannot log idle time: Database is not open";
        return;
    }

    if (endTime <= startTime) {
        qWarning() << "Invalid idle period: endTime (" << endTime << ") <= startTime (" << startTime << ")";
        return;
    }

    if (m_currentUserId == -1) {
        qWarning() << "Cannot log idle time: No user logged in";
        return;
    }

    QSqlQuery query(m_db);
    query.prepare("INSERT INTO log (id_user, start_time, end_time, app_name, title) "
                  "VALUES (:id_user, :start, :end, :app, :title)");
    query.bindValue(":id_user", m_currentUserId);
    query.bindValue(":start", startTime);
    query.bindValue(":end", endTime);
    query.bindValue(":app", QString("Idle"));
    query.bindValue(":title", QString("No active window"));

    if (!query.exec()) {
        qWarning() << "Failed to log idle time:" << query.lastError().text();
    } else {
        emit logCountChanged();
        emit logContentChanged();
        emit productivityStatsChanged();
    }
}

void Logger::logWindowChange(const Logger::WindowInfo &info, qint64 startTime, qint64 endTime)
{
    if (!ensureDatabaseOpen()) {
        qWarning() << "Cannot log window change: Database is not open";
        return;
    }

    if (endTime <= startTime) {
        qWarning() << "Invalid window change period: endTime (" << endTime << ") <= startTime (" << startTime << ")";
        return;
    }
    if (m_currentUserId == -1) {
        qWarning() << "Cannot log window change: No user logged in";
        return;
    }

    QSqlQuery query(m_db);
    query.prepare("INSERT INTO log (id_user, start_time, end_time, app_name, title, url) "
                  "VALUES (:id_user, :start, :end, :app, :title, :url)");
    query.bindValue(":id_user", m_currentUserId);
    query.bindValue(":start", startTime);
    query.bindValue(":end", endTime);
    query.bindValue(":app", info.appName);
    query.bindValue(":title", info.title);
    query.bindValue(":url", info.url.isEmpty() ? QVariant() : info.url);

    if (!query.exec()) {
        qWarning() << "Failed to log window change:" << query.lastError().text();
    } else {
        emit logCountChanged();
        emit logContentChanged();
        emit productivityStatsChanged();
    }
}

void Logger::setLogFilter(const QString &startDate, const QString &endDate)
{
    qDebug() << "Setting log filter - Start Date:" << startDate << "End Date:" << endDate;
    m_startDateFilter = startDate;
    m_endDateFilter = endDate;
    emit logContentChanged();
    emit logCountChanged();
    emit productivityStatsChanged();
}

bool Logger::updateProfileImage(const QString &username, const QString &imagePath)
{
    if (!ensureDatabaseOpen()) {
        qWarning() << "Cannot update profile image: Database is not open";
        return false;
    }

    if (username.isEmpty()) {
        qWarning() << "Cannot update profile image: Username is empty";
        return false;
    }

    // Validate that username exists
    QSqlQuery checkQuery(m_db);
    checkQuery.prepare("SELECT id FROM users WHERE username = :username");
    checkQuery.bindValue(":username", username);
    if (!checkQuery.exec() || !checkQuery.next()) {
        qWarning() << "No user found with username:" << username;
        return false;
    }
    int userId = checkQuery.value(0).toInt();

    // Delete old image
    QString oldImagePath = getProfileImagePath(username);
    if (!oldImagePath.isEmpty()) {
        QString localOldPath = oldImagePath;
        if (localOldPath.startsWith("file:///")) {
            localOldPath = QUrl(localOldPath).toLocalFile();
        } else if (localOldPath.startsWith("file://")) {
            localOldPath = localOldPath.mid(7);
        }
        QFile oldFile(localOldPath);
        if (oldFile.exists()) {
            if (!oldFile.remove()) {
                qWarning() << "Failed to delete old profile image:" << localOldPath;
            } else {
                qDebug() << "Deleted old profile image for" << username << ":" << localOldPath;
            }
        }
    }

    // Save new image path
    QSqlQuery query(m_db);
    query.prepare("UPDATE users SET profile_image = :imagePath WHERE username = :username");
    query.bindValue(":imagePath", imagePath);
    query.bindValue(":username", username);

    if (!query.exec()) {
        qWarning() << "Failed to update profile image for" << username << ":" << query.lastError().text();
        return false;
    }

    if (query.numRowsAffected() == 0) {
        qWarning() << "No rows affected for username:" << username;
        return false;
    }

    qDebug() << "Profile image updated successfully for" << username << "to" << imagePath;

    // Emit signal with a unique path (already includes timestamp from cropProfileImage)
    emit profileImageChanged(username, imagePath);
    return true;
}

QString Logger::getProfileImagePath(const QString &username)
{
    if (!ensureDatabaseOpen()) {
        qWarning() << "Cannot retrieve profile image path: Database is not open";
        return "";
    }

    if (username.isEmpty()) {
        qWarning() << "Cannot retrieve profile image path: Username is empty";
        return "";
    }

    QSqlQuery query(m_db);
    query.prepare("SELECT profile_image FROM users WHERE username = :username");
    query.bindValue(":username", username);

    if (!query.exec()) {
        qWarning() << "Failed to retrieve profile image path:" << query.lastError().text();
        return "";
    }

    if (query.next()) {
        QString imagePath = query.value(0).toString();
        if (imagePath.isEmpty()) {
            return "";
        }
        return imagePath;
    }

    qWarning() << "No user found with username:" << username;
    return "";
}

Logger::WindowInfo Logger::getActiveWindowInfo()
{
#ifdef Q_OS_WIN
    return getActiveWindowInfoWindows();
#elif defined(Q_OS_MACOS)
    return getActiveWindowInfoMacOS();
#elif defined(Q_OS_LINUX)
    return getActiveWindowInfoLinux();
#else
    WindowInfo info;
    info.appName = "Unsupported OS";
    info.title = "Unsupported OS";
    return info;
#endif
}

#ifdef Q_OS_WIN
Logger::WindowInfo Logger::getActiveWindowInfoWindows() {
    WindowInfo info;
    HWND hwnd = GetForegroundWindow();

    if (hwnd == NULL) {
        info.appName = "Unknown";
        info.title = "No active window";
        return info;
    }

    // Dapatkan judul window
    wchar_t buffer[256];
    GetWindowTextW(hwnd, buffer, 256);
    info.title = QString::fromWCharArray(buffer);

    // Dapatkan nama aplikasi
    info.appName = getAppNameFromHwnd(hwnd);

    // Dapatkan URL jika browser (menggunakan UI Automation)
    info.url = getBrowserUrlWindows(hwnd);

    if (info.appName.isEmpty()) info.appName = "Unknown";
    if (info.title.isEmpty()) info.title = "No active window";

    return info;
}
#elif defined(Q_OS_MACOS)
Logger::WindowInfo Logger::getActiveWindowInfoMacOS() {
    WindowInfo info;

    // Get app name
    {
        QProcess appProcess;
        appProcess.start("osascript", {
                                          "-e",
                                          "tell application \"System Events\" to get name of first application process whose frontmost is true"
                                      });

        if (appProcess.waitForFinished(5000)) {
            info.appName = QString(appProcess.readAllStandardOutput()).trimmed();
            qDebug() << "App name:" << info.appName;
        } else {
            appProcess.kill();
            qDebug() << "App name script timed out";
            qDebug() << "Error:" << appProcess.readAllStandardError();
        }
    }

    // Get window title
    {
        QProcess titleProcess;
        titleProcess.start("osascript", {
                                            "-e",
                                            "tell application \"System Events\" to get name of first window of (first application process whose frontmost is true)"
                                        });

        if (titleProcess.waitForFinished(5000)) {
            info.title = QString(titleProcess.readAllStandardOutput()).trimmed();
            qDebug() << "Window title:" << info.title;
        } else {
            titleProcess.kill();
            qDebug() << "Window title script timed out";
            qDebug() << "Error:" << titleProcess.readAllStandardError();
        }
    }

    if (info.appName.isEmpty()) info.appName = "Unknown";
    if (info.title.isEmpty()) info.title = "No active window";

    return info;
}
#elif defined(Q_OS_LINUX)
Logger::WindowInfo Logger::getActiveWindowInfoLinux() {
    WindowInfo info;

    // Coba dengan xdotool (lebih umum di Linux)
    QProcess process;
    process.start("xdotool", {"getwindowfocus", "getwindowname"});
    if (process.waitForFinished(100)) {
        info.title = QString(process.readAllStandardOutput()).trimmed();
    }

    process.start("xdotool", {"getwindowfocus", "getwindowpid"});
    if (process.waitForFinished(100)) {
        QString pid = QString(process.readAllStandardOutput()).trimmed();
        if (!pid.isEmpty()) {
            process.start("ps", {"-p", pid, "-o", "comm="});
            if (process.waitForFinished(100)) {
                info.appName = QString(process.readAllStandardOutput()).trimmed();
            }
        }
    }

    if (info.title.isEmpty()) {
        process.start("wmctrl", {"-l"});
        if (process.waitForFinished(100)) {
            QStringList windows = QString(process.readAllStandardOutput()).split('\n', Qt::SkipEmptyParts);
            for (const QString &window : windows) {
                QStringList parts = window.split(' ', Qt::SkipEmptyParts);
                if (parts.size() >= 4 && parts[0].contains("0x")) {
                    info.title = parts.mid(3).join(' ');
                    break;
                }
            }
        }
    }

    // Setelah mendapatkan info dasar, coba dapatkan URL
    info.url = getBrowserUrlLinux();

    if (info.appName.isEmpty()) info.appName = "Unknown";
    if (info.title.isEmpty()) info.title = "No active window";

    return info;
}
#elif defined(Q_OS_MACOS)
Logger::WindowInfo Logger::getActiveWindowInfoMacOS() {
    WindowInfo info;

    // Ambil nama aplikasi yang aktif
    QProcess process;
    process.start("osascript", {"-e", "tell application \"System Events\" to get name of first application process whose frontmost is true"});
    process.waitForFinished(500);
    info.appName = QString::fromUtf8(process.readAllStandardOutput()).trimmed();

    // Ambil nama window (judul)
    process.start("osascript", {
                                   "-e",
                                   "tell application \"System Events\" to tell (first application process whose frontmost is true) to get value of attribute \"AXTitle\" of front window"
                               });
    process.waitForFinished(1000);
    info.title = QString::fromUtf8(process.readAllStandardOutput()).trimmed();

    // Coba ambil URL jika browser dikenal
    QString browser = info.appName.toLower();
    QString url;

    if (browser == "safari") {
        process.start("osascript", {"-e", "tell application \"Safari\" to return URL of front document"});
    } else if (browser == "google chrome") {
        process.start("osascript", {"-e", "tell application \"Google Chrome\" to return URL of active tab of front window"});
    } else if (browser == "microsoft edge") {
        process.start("osascript", {"-e", "tell application \"Microsoft Edge\" to return URL of active tab of front window"});
    } else if (browser == "brave browser") {
        process.start("osascript", {"-e", "tell application \"Brave Browser\" to return URL of active tab of front window"});
    } else if (browser == "arc") {
        process.start("osascript", {"-e", "tell application \"Arc\" to return URL of active tab of front window"});
    }

    if (process.state() != QProcess::NotRunning) {
        if (process.waitForFinished(1000)) {
            url = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
        }
    }

    // Fallback jika URL kosong: coba ekstrak dari window title
    if (url.isEmpty()) {
        QRegularExpression urlRegex(R"(https?://[^\s/$.?#].[^\s]*)");
        QRegularExpressionMatch match = urlRegex.match(info.title);
        if (match.hasMatch()) {
            url = match.captured(0);
        }
    }

    info.url = url;

    if (info.appName.isEmpty()) info.appName = "Unknown";
    if (info.title.isEmpty()) info.title = "No active window";

    return info;
}
#endif


#ifdef Q_OS_WIN
QString Logger::getAppNameFromHwnd(HWND hwnd) {
    DWORD processId;
    GetWindowThreadProcessId(hwnd, &processId);

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (hProcess != NULL) {
        wchar_t exePath[MAX_PATH];
        if (GetModuleFileNameExW(hProcess, NULL, exePath, MAX_PATH)) {
            QFileInfo fileInfo(QString::fromWCharArray(exePath));
            CloseHandle(hProcess);
            return fileInfo.baseName();
        }
        CloseHandle(hProcess);
    }
    return "Unknown";
}

QString Logger::getBrowserUrlWindows(HWND hwnd) {
    QString appName = getAppNameFromHwnd(hwnd).toLower();

    // Hanya proses jika ini adalah peramban yang dikenal
    if (!appName.contains("chrome") && !appName.contains("firefox") &&
        !appName.contains("edge") && !appName.contains("opera")) {
        return QString();
    }

    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr)) {
        qWarning() << "Failed to initialize COM for UI Automation";
        return QString();
    }

    IUIAutomation *pAutomation = NULL;
    hr = CoCreateInstance(__uuidof(CUIAutomation), NULL, CLSCTX_INPROC_SERVER, __uuidof(IUIAutomation), (void**)&pAutomation);
    if (FAILED(hr) || !pAutomation) {
        qWarning() << "Failed to create UI Automation instance.";
        CoUninitialize();
        return QString();
    }

    IUIAutomationElement *pRootElement = NULL;
    hr = pAutomation->ElementFromHandle(hwnd, &pRootElement);
    if (FAILED(hr) || !pRootElement) {
        qWarning() << "Failed to get UI Automation element from handle.";
        pAutomation->Release();
        CoUninitialize();
        return QString();
    }

    // === PERBAIKAN DIMULAI DI SINI ===
    // Kondisi untuk menemukan address bar.
    // Kita harus membuat VARIANT secara manual untuk COM API.
    IUIAutomationCondition *pCondition = NULL;
    VARIANT varControlType;
    varControlType.vt = VT_I4; // VT_I4 for a 4-byte integer. Control IDs are integers.
    varControlType.lVal = UIA_EditControlTypeId; // The actual control type ID.

    hr = pAutomation->CreatePropertyCondition(UIA_ControlTypePropertyId, varControlType, &pCondition);
    // === PERBAIKAN SELESAI ===

    IUIAutomationElement *pAddressBar = NULL;
    if (SUCCEEDED(hr) && pCondition) {
        // Cari elemen address bar di dalam turunan elemen window
        pRootElement->FindFirst(TreeScope_Descendants, pCondition, &pAddressBar);
        pCondition->Release();
    }

    QString url;
    if (pAddressBar) {
        VARIANT vtValue;
        // Inisialisasi vtValue untuk keamanan
        VariantInit(&vtValue);
        hr = pAddressBar->GetCurrentPropertyValue(UIA_ValueValuePropertyId, &vtValue);
        if (SUCCEEDED(hr) && vtValue.vt == VT_BSTR && vtValue.bstrVal != NULL) {
            url = QString::fromWCharArray(vtValue.bstrVal);
            VariantClear(&vtValue); // Gunakan VariantClear untuk membersihkan VARIANT
        }
        pAddressBar->Release();
    }

    pRootElement->Release();
    pAutomation->Release();
    CoUninitialize();

    return url;
}
#elif defined(Q_OS_LINUX)
QString Logger::getBrowserUrlLinux() {
    // Metode ini tidak andal, hanya sebagai fallback.
    QProcess process;
    process.start("xdotool", {"getactivewindow", "getwindowname"});
    if (process.waitForFinished(100)) {
        QString title = QString(process.readAllStandardOutput()).trimmed();
        QRegularExpression urlRegex(R"(https?://[^\s/$.?#].[^\s]*)");
        QRegularExpressionMatch match = urlRegex.match(title);
        if (match.hasMatch()) {
            return match.captured(0);
        }
    }
    return "";
}
#elif defined(Q_OS_MACOS)
QString Logger::getBrowserUrlMac() {
    // 1. Ambil nama aplikasi aktif
    QProcess frontAppProc;
    frontAppProc.start("osascript", {"-e", "tell application \"System Events\" to get name of first application process whose frontmost is true"});
    frontAppProc.waitForFinished(1000);
    QString frontApp = frontAppProc.readAllStandardOutput().trimmed();

    QString url;

    // 2. Coba ambil URL langsung jika aplikasi adalah browser populer
    QStringList script;
    if (frontApp == "Safari") {
        script << "tell application \"Safari\" to try" << "return URL of front document" << "on error" << "return \"\"" << "end try";
    } else if (frontApp == "Google Chrome" || frontApp == "Brave Browser" || frontApp == "Microsoft Edge") {
        script << QString("tell application \"%1\" to try").arg(frontApp)
        << "return URL of active tab of front window"
        << "on error" << "return \"\"" << "end try";
    } else {
        // 3. Fallback: ambil title dari window dan cari URL di dalamnya
        QProcess titleProc;
        titleProc.start("osascript", {
                                         "-e",
                                         "tell application \"System Events\" to tell (first application process whose frontmost is true) to get name of front window"
                                     });
        titleProc.waitForFinished(1000);
        QString windowTitle = titleProc.readAllStandardOutput().trimmed();

        QRegularExpression urlRegex(R"(https?://[^\s/$.?#].[^\s]*)");
        QRegularExpressionMatch match = urlRegex.match(windowTitle);
        if (match.hasMatch()) {
            return match.captured(0);
        } else {
            return ""; // Tidak ada URL dalam title
        }
    }

    // 4. Eksekusi skrip AppleScript jika tersedia
    if (!script.isEmpty()) {
        QProcess scriptProc;
        scriptProc.start("osascript", QStringList() << "-e" << script.join("\n"));
        scriptProc.waitForFinished(1000);
        url = scriptProc.readAllStandardOutput().trimmed();
    }

    return url;
}

#endif
