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



#ifdef Q_OS_WIN
#include <windows.h>
#include <psapi.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#else
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shlwapi.lib")

Logger::Logger(QObject *parent) : QObject(parent)
{
    initializeDatabase();
    initializeProductivityDatabase();
    checkTaskStatusBeforeStart();

    m_productiveAppsModel = new QSqlQueryModel(this);
    m_nonProductiveAppsModel = new QSqlQueryModel(this);
    m_productiveAppsModel->setQuery("SELECT aplikasi AS appName, window_title AS windowTitle, jenis AS type FROM aplikasi WHERE jenis = 1", m_productivityDb);
    m_nonProductiveAppsModel->setQuery("SELECT aplikasi AS appName, window_title AS windowTitle, jenis AS type FROM aplikasi WHERE jenis = 2", m_productivityDb);

    m_taskTimer.setInterval(1000);
    connect(&m_taskTimer, &QTimer::timeout, this, &Logger::updateTaskTime);
    m_taskTimer.start();
    m_isTrackingActive = true;
    m_networkManager = new QNetworkAccessManager(this);

    m_pingTimer.setInterval(60000); // 1 menit
    connect(&m_pingTimer, &QTimer::timeout, this, [this]() {
        if (m_activeTaskId != -1 && !m_isTaskPaused) {
            sendPing(m_activeTaskId);
        }
    });

    // Inisialisasi dan mulai timer untuk "Time at Work"
    connect(&m_workTimer, &QTimer::timeout, this, &Logger::updateWorkTimeAndSave);
    m_workTimer.start(1000); // Update setiap detik
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

void Logger::refreshAll()
{
    // 1. Simpan status pause/play saat ini
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

    // 2. Lakukan refresh data tanpa mempengaruhi status pause/play
    qDebug() << "Memanggil fetchAndStoreTasks untuk menyinkronkan task dari server";
    fetchAndStoreTasks();

    qDebug() << "Memanggil fetchAndStoreProductivityApps untuk menyinkronkan aplikasi produktivitas";
    fetchAndStoreProductivityApps();

    // Perbarui status task aktif jika ada
    if (m_activeTaskId != -1) {
        qDebug() << "Menguji task aktif dengan ID:" << m_activeTaskId;
        updateTaskStatus(m_activeTaskId);
    }

    // Sinkronkan tugas aktif
    syncActiveTask();

    // Perbarui status jendela aktif
    logActiveWindow();

    // 3. Kembalikan status pause/play ke keadaan semula
    m_isTaskPaused = wasPaused;
    m_activeTaskId = activeTaskBeforeRefresh;


    // Memicu pembaruan semua data yang relevan dengan UI
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

    qDebug() << "Refresh all completed for user_id:" << m_currentUserId;
}




void Logger::logout()
{
    saveWorkTimeData();
    sendWorkTimeToAPI();

    // Stop all active tracking
    if (m_activeTaskId != -1) {
        setActiveTask(-1); // This will stop the current task
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
    m_workTimeElapsedSeconds = 0; // Reset waktu kerja di memori
    QSqlQuery clearTokenQuery(m_db);
    clearTokenQuery.prepare("UPDATE users SET token = '' WHERE id = :id");
    clearTokenQuery.bindValue(":id", m_currentUserId);
    clearTokenQuery.exec();



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
    if (!query.exec("CREATE TABLE IF NOT EXISTS log ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "id_user INTEGER NOT NULL, "
                    "start_time INTEGER NOT NULL, "
                    "end_time INTEGER NOT NULL, "
                    "app_name TEXT, "
                    "title TEXT, "
                    "FOREIGN KEY(id_user) REFERENCES users(id))")) {
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
                    "jenis INTEGER NOT NULL, "
                    "productivity INTEGER NOT NULL DEFAULT 0, "
                    "for_user TEXT NOT NULL DEFAULT '0')")) { // '0'=all users, '1,2,3'=specific users
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
                    "end_reality TEXT NOT NULL, "
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

void Logger::updateWorkTimeAndSave()
{
    // Waktu kerja hanya bertambah jika ada task yang berjalan (tidak di-pause)
    if (m_activeTaskId != -1 && !m_isTaskPaused) {
        m_workTimeElapsedSeconds++;
        emit workTimeElapsedSecondsChanged();

        // Simpan ke DB setiap 10 detik untuk mengurangi operasi I/O
        if (m_workTimeElapsedSeconds % 10 == 0) {
            saveWorkTimeData();
        }
    }
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
    query.prepare("SELECT id, paused FROM task WHERE active = 1 AND user_id = :user_id LIMIT 1");
    query.bindValue(":user_id", m_currentUserId);
    if (!query.exec()) {
        qWarning() << "Failed to check active task status:" << query.lastError().text();
        return;
    }

    if (query.next()) {
        m_activeTaskId = query.value(0).toInt();
        m_isTaskPaused = query.value(1).toBool();

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
    query.prepare("SELECT aplikasi, jenis, window_title, for_user FROM aplikasi WHERE (for_user = '0' OR for_user LIKE :userId)");
    query.bindValue(":userId", QString::number(m_currentUserId));
    if (query.exec()) {
        while (query.next()) {
            QVariantMap app;
            app["appName"] = query.value(0).toString(); // Nama aplikasi
            app["type"] = query.value(1).toInt(); // Jenis aplikasi
            app["window_title"] = query.value(2).toString(); // Judul jendela
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
    QString queryStr = "SELECT start_time, end_time, app_name, title FROM log "
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

        content += QString("%1,%2,%3,%4\n")
                       .arg(QDateTime::fromSecsSinceEpoch(start).toString("hh:mm:ss"))
                       .arg(QDateTime::fromSecsSinceEpoch(end).toString("hh:mm:ss"))
                       .arg(app)
                       .arg(title);
    }
    qDebug() << "logContent returned" << content.count('\n') << "lines";
    return content;
}


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

    QString queryStr = "SELECT start_time, end_time, app_name, title FROM log "
                       "WHERE app_name IS NOT NULL AND title IS NOT NULL AND id_user = :id_user ";
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
        QString title = query.value(3).toString();

        double duration = end - start;
        if (duration <= 0) continue;

        int type = getAppProductivityType(appName, title);
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

    double total = totalTime > 0 ? totalTime : 1; // Avoid division by zero
    stats["productive"] = (productiveTime / total) * 100;
    stats["nonProductive"] = (nonProductiveTime / total) * 100;
    stats["neutral"] = (neutralTime / total) * 100;

    return stats;
}
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
void Logger::addProductivityApp(const QString &appName, const QString &windowTitle, int productivityType)
{
    if (!ensureProductivityDatabaseOpen()) {
        qWarning() << "Database tidak terbuka";
        return;
    }

    // 1. Simpan ke database lokal terlebih dahulu
    QSqlQuery query(m_productivityDb);
    query.prepare("INSERT INTO aplikasi (aplikasi, window_title, jenis, productivity) "
                  "VALUES (:app, :window, :type, :prod)");
    query.bindValue(":app", appName);
    query.bindValue(":window", windowTitle.isEmpty() ? QVariant() : windowTitle);
    query.bindValue(":type", 0); // 0 = menunggu approval
    query.bindValue(":prod", productivityType);

    if (query.exec()) {
        qDebug() << "Aplikasi ditambahkan. Menunggu approval admin.";

        // 2. Kirim data ke API
        sendProductivityAppToAPI(appName, windowTitle, productivityType);

        // Refresh model
        QString productiveQuery = QString("SELECT aplikasi AS appName, window_title AS windowTitle, jenis AS type FROM aplikasi WHERE jenis = 1 AND (for_user = '0' OR for_user LIKE '%%1%')").arg(m_currentUserId);
        QString nonProductiveQuery = QString("SELECT aplikasi AS appName, window_title AS windowTitle, jenis AS type FROM aplikasi WHERE jenis = 2 AND (for_user = '0' OR for_user LIKE '%%1%')").arg(m_currentUserId);
        m_productiveAppsModel->setQuery(productiveQuery, m_productivityDb);
        m_nonProductiveAppsModel->setQuery(nonProductiveQuery, m_productivityDb);
        emit productivityAppsChanged();
    } else {
        qWarning() << "Gagal menambahkan aplikasi:" << query.lastError();
    }
}

void Logger::sendProductivityAppToAPI(const QString &appName, const QString &windowTitle, int productivityType)
{
    if (m_authToken.isEmpty()) {
        qWarning() << "Cannot send productivity app: No authentication token available";
        return;
    }

    if (m_currentUserId == -1) {
        qWarning() << "Cannot send productivity app: No user logged in";
        return;
    }

    // Konversi productivityType ke string status
    QString status;
    switch(productivityType) {
    case 1: status = "productive"; break;
    case 2: status = "non-productive"; break;
    default: status = "neutral"; break;
    }

    // Siapkan payload
    QJsonObject payload;
    payload["application_name"] = appName;
    payload["productivity_status"] = status;
    payload["user_id"] = m_currentUserId;
    if (!windowTitle.isEmpty()) {
        payload["process_name"] = windowTitle;
    }

    // Konfigurasi request
    QNetworkRequest request(QUrl("https://deskmon.pranala-dt.co.id/api/app-request/store"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_authToken.toUtf8());

    qDebug() << "Sending productivity app to API:" << QJsonDocument(payload).toJson();

    // Kirim request POST
    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(payload).toJson());

    // Handle timeout (30 detik)
    QTimer::singleShot(30000, reply, &QNetworkReply::abort);

    // Handle response
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            qDebug() << "Productivity app successfully sent to API. Response:" << response;
        } else {
            qWarning() << "Failed to send productivity app to API:" << reply->errorString();
            qDebug() << "HTTP Status:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            qDebug() << "Response Body:" << reply->readAll();

            // Jika token expired, beri sinyal untuk login ulang
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
    // Pastikan reply dihapus setelah fungsi selesai
    QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> replyPtr(reply);

    // 1. Handle error response
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Failed to fetch productivity apps:" << reply->errorString();

        // Cek jika error karena token expired (HTTP 401)
        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (statusCode == 401) {
            qWarning() << "Authentication token expired, emitting signal";
            emit authTokenError("Authentication token expired");
        }
        return;
    }

    // 2. Baca dan parse response
    QByteArray responseData = reply->readAll();
    qDebug() << "Raw API response:" << responseData;

    // Debug HTTP status code
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    qDebug() << "HTTP status code:" << statusCode;

    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "JSON parse error:" << parseError.errorString()
        << "at offset" << parseError.offset;
        return;
    }

    // Cek jika response adalah object dengan format {success: bool, data: array}
    if (jsonDoc.isObject()) {
        QJsonObject jsonObj = jsonDoc.object();

        // Cek jika response mengandung error
        if (jsonObj.contains("error") || !jsonObj["success"].toBool()) {
            QString errorMsg = jsonObj["message"].toString();
            qWarning() << "API returned error:" << errorMsg;
            return;
        }

        // Ambil data array dari field "data" jika ada
        if (jsonObj.contains("data") && jsonObj["data"].isArray()) {
            jsonDoc = QJsonDocument(jsonObj["data"].toArray());
        }
    }

    if (!jsonDoc.isArray()) {
        qWarning() << "Invalid JSON format - expected array, got:" << jsonDoc.toJson();
        return;
    }

    QJsonArray appsArray = jsonDoc.array();
    if (appsArray.isEmpty()) {
        qDebug() << "No productivity apps received from API";
        return;
    }

    // 3. Mulai transaksi database
    if (!m_productivityDb.transaction()) {
        qWarning() << "Failed to start database transaction:" << m_productivityDb.lastError().text();
        return;
    }

    QSqlQuery query(m_productivityDb);
    bool success = true;

    // 4. Proses setiap aplikasi dari API
    for (const QJsonValue &appValue : appsArray) {
        if (!appValue.isObject()) {
            qWarning() << "Skipping invalid app entry (not an object)";
            continue;
        }

        QJsonObject appObj = appValue.toObject();

        // Validasi field yang diperlukan dengan cara yang lebih fleksibel
        QString appName = appObj.value("application_name").toString();
        QString status = appObj.value("productivity_status").toString("").toLower();
        QString processName = appObj.value("process_name").toString();

        if (appName.isEmpty()) {
            qWarning() << "Skipping invalid productivity app entry (missing application_name):"
                       << appObj;
            continue;
        }

        // Konversi status string ke jenis numerik
        int jenis = 0; // Default = menunggu approval
        if (status == "productive") {
            jenis = 1;
        } else if (status == "non-productive") {
            jenis = 2;
        }

        // 5. Cek apakah data sudah ada di database
        query.prepare(
            "SELECT id FROM aplikasi "
            "WHERE aplikasi = :app AND "
            "(window_title = :process OR (window_title IS NULL AND :process IS NULL))"
            );
        query.bindValue(":app", appName);
        query.bindValue(":process", processName.isEmpty() ? QVariant() : processName);

        if (!query.exec()) {
            qWarning() << "Failed to check existing app:" << query.lastError().text();
            success = false;
            break;
        }

        if (query.next()) {
            // Data sudah ada, lakukan UPDATE
            int existingId = query.value(0).toInt();
            query.prepare(
                "UPDATE aplikasi SET "
                "jenis = :type, "
                "for_user = '0' "  // Selalu set for_user ke '0'
                "WHERE id = :id"
                );
            query.bindValue(":type", jenis);
            query.bindValue(":id", existingId);
        } else {
            // Data belum ada, lakukan INSERT
            query.prepare(
                "INSERT INTO aplikasi "
                "(aplikasi, window_title, jenis, for_user) "
                "VALUES (:app, :process, :type, '0')"  // Selalu set for_user ke '0'
                );
            query.bindValue(":app", appName);
            query.bindValue(":process", processName.isEmpty() ? QVariant() : processName);
            query.bindValue(":type", jenis);
        }

        if (!query.exec()) {
            qWarning() << "Failed to insert/update productivity app:"
                       << appName << "-" << query.lastError().text();
            success = false;
            break;
        }
    }

    // 6. Commit atau rollback transaksi
    if (success) {
        if (!m_productivityDb.commit()) {
            qWarning() << "Failed to commit transaction:" << m_productivityDb.lastError().text();
        } else {
            qDebug() << "Successfully processed" << appsArray.size() << "productivity apps from API";
            // 7. Refresh model dan emit signal
            refreshProductivityModels();
            emit productivityAppsChanged();
        }
    } else {
        m_productivityDb.rollback();
        qCritical() << "Error processing productivity apps, transaction rolled back";
    }
}

void Logger::refreshProductivityModels()
{
    if (!ensureProductivityDatabaseOpen()) return;

    QString productiveQuery = QString(
                                  "SELECT aplikasi AS appName, window_title AS windowTitle, jenis AS type "
                                  "FROM aplikasi WHERE jenis = 1 AND (for_user = '0' OR for_user LIKE '%%1%')"
                                  ).arg(m_currentUserId);

    QString nonProductiveQuery = QString(
                                     "SELECT aplikasi AS appName, window_title AS windowTitle, jenis AS type "
                                     "FROM aplikasi WHERE jenis = 2 AND (for_user = '0' OR for_user LIKE '%%1%')"
                                     ).arg(m_currentUserId);

    m_productiveAppsModel->setQuery(productiveQuery, m_productivityDb);
    m_nonProductiveAppsModel->setQuery(nonProductiveQuery, m_productivityDb);
}

// Di fungsi getAppProductivityType (gunakan kolom jenis untuk pengecekan)
int Logger::getAppProductivityType(const QString &appName, const QString &windowTitle) const
{
    if (!ensureProductivityDatabaseOpen() || m_currentUserId == -1) {
        return 0;
    }

    // Fungsi normalisasi yang lebih ringan
    auto normalizeString = [](const QString &str) {
        return str.toLower().simplified().remove(' ');
    };

    // Fungsi untuk membuat pattern matching yang efisien
    auto createSearchPatterns = [](const QString &input) -> QStringList {
        QStringList patterns;
        QString normalized = input.toLower().simplified();

        // Pattern 1: Original normalized
        patterns.append(normalized.remove(' '));

        // Pattern 2: Kata-kata individual (hanya jika ada spasi)
        if (input.contains(' ')) {
            QStringList words = normalized.split(' ', Qt::SkipEmptyParts);
            for (const QString &word : words) {
                if (word.length() >= 3) { // Minimal 3 karakter
                    patterns.append(word);
                }
            }
        }

        // Pattern 3: Hapus karakter khusus umum
        QString cleaned = normalized;
        cleaned.remove(QRegularExpression("[\\-_\\.\\(\\)\\[\\]]"));
        if (!cleaned.isEmpty() && cleaned != patterns.first()) {
            patterns.append(cleaned);
        }

        return patterns;
    };

    // Fungsi matching yang cepat
    auto fastMatch = [](const QString &target, const QStringList &patterns) -> double {
        QString normTarget = target.toLower().simplified().remove(' ');

        // Exact match - score tertinggi
        for (const QString &pattern : patterns) {
            if (normTarget == pattern) {
                return 1.0;
            }
        }

        // Contains match - bi-directional
        for (const QString &pattern : patterns) {
            if (normTarget.contains(pattern)) {
                // Score berdasarkan rasio panjang
                double ratio = (double)pattern.length() / normTarget.length();
                return 0.8 + (ratio * 0.15); // 0.8 - 0.95
            }
            if (pattern.contains(normTarget)) {
                double ratio = (double)normTarget.length() / pattern.length();
                return 0.75 + (ratio * 0.15); // 0.75 - 0.9
            }
        }

        // Fuzzy match sederhana - hanya untuk string pendek
        if (normTarget.length() <= 15) {
            for (const QString &pattern : patterns) {
                if (pattern.length() <= 15) {
                    // Hitung common characters
                    int commonChars = 0;
                    int minLen = qMin(normTarget.length(), pattern.length());
                    int maxLen = qMax(normTarget.length(), pattern.length());

                    for (int i = 0; i < minLen; i++) {
                        if (normTarget[i] == pattern[i]) {
                            commonChars++;
                        }
                    }

                    double similarity = (double)commonChars / maxLen;
                    if (similarity > 0.6) {
                        return similarity * 0.7; // Max 0.7 untuk fuzzy
                    }
                }
            }
        }

        return 0.0;
    };

    // Cache untuk menghindari query berulang
    static QHash<QString, QVector<QPair<QString, int>>> titleCache;
    static QHash<QString, QVector<QPair<QString, int>>> appCache;
    static QString lastUserId;

    // Reset cache jika user berubah
    QString currentUser = QString::number(m_currentUserId);
    if (lastUserId != currentUser) {
        titleCache.clear();
        appCache.clear();
        lastUserId = currentUser;
    }

    // Buat pattern dari input
    QStringList titlePatterns = createSearchPatterns(windowTitle);
    QStringList appPatterns = createSearchPatterns(appName);

    double bestScore = 0.0;
    int bestType = 0;

    QSqlQuery query(m_productivityDb);

    // 1. Cek window title (prioritas tinggi)
    if (!titleCache.contains(currentUser)) {
        QVector<QPair<QString, int>> titleEntries;
        query.prepare(
            "SELECT window_title, jenis, for_user FROM aplikasi "
            "WHERE window_title IS NOT NULL AND window_title != ''"
            );

        if (query.exec()) {
            while (query.next()) {
                QString dbTitle = query.value(0).toString();
                int jenis = query.value(1).toInt();
                QString forUsers = query.value(2).toString();

                // Filter user
                if (forUsers == "0" || forUsers.split(',').contains(currentUser)) {
                    titleEntries.append({dbTitle, jenis});
                }
            }
        }
        titleCache[currentUser] = titleEntries;
    }

    // Match dengan title entries
    for (const auto &entry : titleCache[currentUser]) {
        double score = fastMatch(entry.first, titlePatterns);
        if (score > bestScore) {
            bestScore = score;
            bestType = entry.second;
        }
    }

    // 2. Cek aplikasi umum (hanya jika belum dapat match bagus dari title)
    if (bestScore < 0.9) {
        if (!appCache.contains(currentUser)) {
            QVector<QPair<QString, int>> appEntries;
            query.prepare(
                "SELECT aplikasi, jenis, for_user FROM aplikasi "
                "WHERE window_title IS NULL OR window_title = ''"
                );

            if (query.exec()) {
                while (query.next()) {
                    QString dbApp = query.value(0).toString();
                    int jenis = query.value(1).toInt();
                    QString forUsers = query.value(2).toString();

                    // Filter user
                    if (forUsers == "0" || forUsers.split(',').contains(currentUser)) {
                        appEntries.append({dbApp, jenis});
                    }
                }
            }
            appCache[currentUser] = appEntries;
        }

        // Match dengan app entries
        for (const auto &entry : appCache[currentUser]) {
            double score = fastMatch(entry.first, appPatterns);

            // Penalti sedikit untuk app match vs title match
            score *= 0.95;

            if (score > bestScore) {
                bestScore = score;
                bestType = entry.second;
            }
        }
    }

    // Return hasil jika score cukup tinggi
    if (bestScore >= 0.65) {
        return bestType;
    }

    return 0; // Default netral
}

QVariantList Logger::getPendingApplicationRequests() {
    QVariantList requests;

    QSqlQuery query(m_productivityDb);
    query.prepare("SELECT id, aplikasi, window_title, for_user FROM aplikasi WHERE jenis = 0");

    if (query.exec()) {
        while (query.next()) {
            QVariantMap request;
            request["id"] = query.value(0);
            request["app_name"] = query.value(1); // aplikasi
            request["window_title"] = query.value(2);
            requests.append(request);
        }
    } else {
        qDebug() << "Error getting pending requests:" << query.lastError().text();
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

    // 2. Periksa dan ambil token autentikasi jika belum ada
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

    // 3. Siapkan permintaan HTTP GET ke endpoint server
    QNetworkRequest request(QUrl("https://deskmon.pranala-dt.co.id/api/tasks/all"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_authToken.toUtf8());

    qDebug() << "Sending request with token:" << m_authToken;

    // 4. Kirim permintaan ke server dan hubungkan respons ke slot penanganan
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
        clearToken(); // Hapus token yang tidak valid
        emit authTokenError("Authentication token expired");
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

        // Tentukan durasi maksimum dari API, default ke 8 jam jika tidak ada
        QJsonValue durationValue = taskObj["duration"];
        int apiMaxTime = durationValue.isNull() ? 8 * 3600 : durationValue.toInt();

        bool taskExists = existingTasks.contains(taskId);

        if (taskExists) {
            // 15. Untuk tugas yang sudah ada, pertahankan max_time dan time_usage lokal
            QPair<int, int> currentValues = existingTasks[taskId];
            int currentMaxTime = currentValues.first;
            int currentTimeUsage = currentValues.second;

            // Update max_time hanya jika saat ini 0 atau nilai API lebih besar
            int finalMaxTime = (currentMaxTime == 0) ? apiMaxTime :
                                   (apiMaxTime > currentMaxTime) ? apiMaxTime : currentMaxTime;

            query.prepare("UPDATE task SET project_name = :projectName, task = :taskDesc, "
                          "max_time = :maxTime WHERE id = :id");
            query.bindValue(":id", taskId);
            query.bindValue(":projectName", projectName);
            query.bindValue(":taskDesc", taskDesc);
            query.bindValue(":maxTime", finalMaxTime);

            qDebug() << "Updating task ID" << taskId << "- preserving max_time:" << finalMaxTime
                     << "(current:" << currentMaxTime << ", API:" << apiMaxTime << ")";
        } else {
            // 16. Untuk tugas baru, masukkan dengan nilai default
            query.prepare("INSERT INTO task (id, project_name, task, max_time, time_usage, active, status, paused, user_id) "
                          "VALUES (:id, :projectName, :taskDesc, :maxTime, 0, 0, 'Preview', 0, :userId)");
            query.bindValue(":id", taskId);
            query.bindValue(":projectName", projectName);
            query.bindValue(":taskDesc", taskDesc);
            query.bindValue(":maxTime", apiMaxTime);
            query.bindValue(":userId", userId);

            qDebug() << "Inserting new task ID" << taskId << "with max_time:" << apiMaxTime;
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
            task["status"] = "Preview";
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

        // Hanya ubah status ke 'Preview' jika bukan 'review' atau 'completed'
        QString newStatus = (prevStatus == "review" || prevStatus == "completed") ? prevStatus : "preview";

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
            // Konversi m_taskStartTime dari qint64 ke QDateTime
            QString startTime = QDateTime::fromSecsSinceEpoch(m_taskStartTime).toString(Qt::ISODateWithMs);
            sendPausePlayDataToAPI(m_activeTaskId, startTime, currentTime, "pause");
        }

        stopPingTimer();
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
        sendPausePlayDataToAPI(taskId, currentTime, currentTime, "pause");

        // Set max time untuk tugas
        setMaxTimeForTask(taskId);
    }

    // Sinkronkan tugas
    fetchAndStoreTasks();
    syncActiveTask();

    // Update state
    m_activeTaskId = taskId;
    m_isTaskPaused = true;
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
    connect(reply, &QNetworkReply::finished, [this, reply, taskId]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            qDebug() << "Ping successful for task ID:" << taskId << "Response:" << response;
        } else {
            qWarning() << "Ping failed for task ID" << taskId << ":" << reply->errorString();
            qDebug() << "HTTP Status:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            qDebug() << "Response Body:" << reply->readAll();
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
    if (apiStatus == "created" || apiStatus == "on-progress") {
        dbStatus = "preview";
    } else if (apiStatus == "on-review") {
        dbStatus = "review";
        QString message = QString("Task is under review. Please wait for approval.").arg(taskName);
        emit taskReviewNotification(message);
    } else if (apiStatus == "completed") {
        dbStatus = "completed";
        finishTask(taskId);
    } else {
        qWarning() << "Unknown status for taskId" << taskId << ":" << apiStatus;
        reply->deleteLater();
        return;
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
            sendPausePlayDataToAPI(m_activeTaskId,
                                   m_lastPauseStartTime.toString(Qt::ISODateWithMs),
                                   currentTime,
                                   "play");
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
    connect(reply, &QNetworkReply::finished, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            qDebug() << "API Response:" << response;

            QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
            if (!jsonDoc.isNull() && jsonDoc.isObject()) {
                QJsonObject jsonObj = jsonDoc.object();

                // Handle case when server rejects the request
                if (!jsonObj["success"].toBool() &&
                    jsonObj["message"].toString().contains("User already has another task in on-progress status")) {
                    // Emit signal to show notification in UI
                    emit showNotification("Silahkan pause task sebelum berpindah ke task lain");

                    // Revert the task change in local database
                    revertTaskChange();
                }
            }
        } else {
            qWarning() << "API Error:" << reply->errorString();
            qDebug() << "HTTP Status:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            qDebug() << "Response Body:" << reply->readAll();

            // If it's a 400 Bad Request with the specific message
            if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 400) {
                QByteArray response = reply->readAll();
                QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
                if (!jsonDoc.isNull() && jsonDoc.isObject()) {
                    QJsonObject jsonObj = jsonDoc.object();
                    if (jsonObj["message"].toString().contains("User already has another task in on-progress status")) {
                        emit showNotification("Silahkan pause task sebelum berpindah ke task lain");
                        revertTaskChange();
                    }
                }
            }
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


void Logger::updateTaskTime()
{
    m_globalTimeUsage += 1;
    emit globalTimeUsageChanged();

    if (m_isTaskPaused || m_activeTaskId == -1 || !ensureProductivityDatabaseOpen()) {
        return;
    }

    QSqlQuery query(m_productivityDb);
    query.prepare("SELECT user_id, status FROM task WHERE id = :id");
    query.bindValue(":id", m_activeTaskId);
    if (!query.exec() || !query.next()) {
        qWarning() << "Task not found or invalid task ID:" << m_activeTaskId;
        return;
    }
    if (query.value(0).toInt() != m_currentUserId) {
        qWarning() << "Task ID" << m_activeTaskId << "does not belong to current user:" << m_currentUserId;
        return;
    }

    QString status = query.value(1).toString().toLower();
    if (status == "review") {
        qDebug() << "Skipping time update for task ID" << m_activeTaskId << "in Review status";
        return;
    }

    qint64 currentTime = QDateTime::currentSecsSinceEpoch();
    qint64 timeUsage = m_taskTimeOffset + (currentTime - m_taskStartTime);

    query.prepare("UPDATE task SET time_usage = :timeUsage WHERE id = :id");
    query.bindValue(":timeUsage", timeUsage);
    query.bindValue(":id", m_activeTaskId);
    if (!query.exec()) {
        qWarning() << "Failed to update task time_usage:" << query.lastError().text();
        return;
    }

    emit taskListChanged();
}



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
        // Jika API tidak support login dengan username, coba cari email dari database lokal terlebih dahulu
        QSqlQuery emailQuery(m_db);
        emailQuery.prepare("SELECT email FROM users WHERE username = :username");
        emailQuery.bindValue(":username", loginInput);

        if (emailQuery.exec() && emailQuery.next()) {
            QString foundEmail = emailQuery.value(0).toString();
            jsonPayload["email"] = foundEmail;
            qDebug() << "Found email for username" << loginInput << ":" << foundEmail;
        } else {
            // Jika tidak ditemukan di database lokal, gunakan username langsung
            // (tergantung apakah API mendukung login dengan username)
            jsonPayload["email"] = loginInput; // atau jsonPayload["username"] jika API support
            qDebug() << "No email found for username, using username directly";
        }
    }

    jsonPayload["password"] = password;
    QJsonDocument doc(jsonPayload);
    QByteArray data = doc.toJson();

    qDebug() << "Sending login request with payload:" << doc.toJson(QJsonDocument::Compact);

    // 2. Kirim request
    QNetworkReply *reply = m_networkManager->post(request, data);

    // 3. Tunggu response secara sinkron
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    bool authenticated = false;

    // 4. Handle response
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        QJsonDocument jsonResponse = QJsonDocument::fromJson(response);
        QJsonObject jsonObj = jsonResponse.object();

        qDebug() << "API Response:" << jsonResponse.toJson(QJsonDocument::Compact);

        if (jsonObj["success"].toBool()) {
            // 5. Parse data user dari response
            QJsonObject userData = jsonObj["user"].toObject();
            m_authToken = jsonObj["token"].toString();
            qDebug() << "API Login successful, token:" << m_authToken;

            // 6. Simpan data user ke database lokal
            int userId = userData["id"].toInt();
            QString username = userData["name"].toString();
            QString role = userData["role"].toObject()["rolename"].toString();
            QString userEmail = userData["email"].toString();

            // First, insert/update user data
            QSqlQuery userQuery(m_db);
            userQuery.prepare("INSERT OR REPLACE INTO users "
                              "(id, username, password, department, email, role, token) "
                              "VALUES (:id, :username, :password, :department, :email, :role, :token)");
            userQuery.bindValue(":id", userId);
            userQuery.bindValue(":username", username);
            userQuery.bindValue(":password", hashPassword(password));
            userQuery.bindValue(":department", role);
            userQuery.bindValue(":email", userEmail);
            userQuery.bindValue(":role", role);
            userQuery.bindValue(":token", m_authToken);

            if (!userQuery.exec()) {
                qWarning() << "Failed to store user data:" << userQuery.lastError().text();
            }


            // 7. Set current user info dan emit signals
            setCurrentUserInfo(userId, username, userEmail);

            // 8. Start timers and fetch tasks
            checkAndCreateNewDayRecord();
            loadWorkTimeData();
            startGlobalTimer();
            syncActiveTask();
            fetchAndStoreTasks(); // This will now properly preserve existing max_time values

            reply->deleteLater();
            return true;
        } else {
            qWarning() << "API Login failed:" << jsonObj["message"].toString();
        }
    } else {
        qWarning() << "Network error during API login:" << reply->errorString();
    }

    // Fallback ke login lokal jika HTTP request gagal
    qDebug() << "Attempting local login fallback for" << loginType << ":" << loginInput;
    QSqlQuery localQuery(m_db);

    // Query yang mendukung login dengan email atau username
    localQuery.prepare("SELECT id, username, email, password, token FROM users "
                       "WHERE email = :loginInput OR username = :loginInput");
    localQuery.bindValue(":loginInput", loginInput);

    if (!localQuery.exec()) {
        qWarning() << "Local query failed:" << localQuery.lastError().text();
        reply->deleteLater();
        return false;
    }

    if (!localQuery.next()) {
        qDebug() << "Local login failed: User not found with" << loginType << ":" << loginInput;
        reply->deleteLater();
        return false;
    }

    QString storedPassword = localQuery.value(3).toString();
    QString hashedInput = hashPassword(password);

    if (storedPassword == hashedInput) {
        int userId = localQuery.value(0).toInt();
        QString username = localQuery.value(1).toString();
        QString userEmail = localQuery.value(2).toString();

        // Get stored token for local login
        m_authToken = localQuery.value(4).toString();
        qDebug() << "Local login successful for user:" << username << "(" << userEmail << ")";
        qDebug() << "Using stored token:" << (m_authToken.isEmpty() ? "No token" : "Token available");

        // Set current user info dan emit signals
        setCurrentUserInfo(userId, username, userEmail);

        checkAndCreateNewDayRecord();
        loadWorkTimeData();
        startGlobalTimer();
        syncActiveTask();

        // Try to fetch tasks if we have a token
        if (!m_authToken.isEmpty()) {
            fetchAndStoreTasks();
        } else {
            qDebug() << "No stored token available for offline mode";
        }

        reply->deleteLater();
        return true;
    }

    qDebug() << "Local login failed: Password mismatch";
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
    query.prepare("SELECT department FROM users WHERE username = :username");
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
    query.prepare("INSERT INTO log (id_user, start_time, end_time, app_name, title) "
                  "VALUES (:id_user, :start, :end, :app, :title)");
    query.bindValue(":id_user", m_currentUserId);
    query.bindValue(":start", startTime);
    query.bindValue(":end", endTime);
    query.bindValue(":app", info.appName);
    query.bindValue(":title", info.title);

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
#else
    return getActiveWindowInfoLinux();
#endif
}

#ifdef Q_OS_WIN
Logger::WindowInfo Logger::getActiveWindowInfoWindows()
{
    WindowInfo info;

    HWND hwnd = GetForegroundWindow();
    if (hwnd == NULL) {
        info.appName = "Unknown";
        info.title = "No active window";
        return info;
    }

    wchar_t buffer[256];
    GetWindowTextW(hwnd, buffer, 256);
    info.title = QString::fromWCharArray(buffer);

    DWORD processId;
    GetWindowThreadProcessId(hwnd, &processId);

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (hProcess != NULL) {
        wchar_t exePath[MAX_PATH];
        if (GetModuleFileNameExW(hProcess, NULL, exePath, MAX_PATH)) {
            QFileInfo fileInfo(QString::fromWCharArray(exePath));
            info.appName = fileInfo.baseName();
        } else {
            info.appName = "Unknown";
        }
        CloseHandle(hProcess);
    } else {
        info.appName = "Unknown";
    }

    return info;
}
#elif define(Q_OS_LINUX)
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
            QStringList windows = QString(process.readAllStandardOutput()).split('\n');
            for (const QString &window : windows) {
                QStringList parts = window.split(' ', QString::SkipEmptyParts);
                if (parts.size() >= 4 && parts[0].contains("0x")) {
                    info.title = parts.mid(3).join(' ');
                    break;
                }
            }
        }
    }

    if (info.appName.isEmpty()) info.appName = "Unknown";
    if (info.title.isEmpty()) info.title = "No active window";

    return info;
}
#elif defined(Q_OS_MACOS)
Logger::WindowInfo Logger::getActiveWindowInfoMacOS() {
    WindowInfo info;

    // Gunakan AppleScript untuk mendapatkan info window di macOS
    QProcess process;
    process.start("osascript", {"-e", "tell application \"System Events\" to get name of first application process whose frontmost is true"});
    if (process.waitForFinished(100)) {
        info.appName = QString(process.readAllStandardOutput()).trimmed();
    }

    process.start("osascript", {"-e", "tell application \"System Events\" to get name of first window of (first application process whose frontmost is true)"});
    if (process.waitForFinished(100)) {
        info.title = QString(process.readAllStandardOutput()).trimmed();
    }

    if (info.appName.isEmpty()) info.appName = "Unknown";
    if (info.title.isEmpty()) info.title = "No active window";

    return info;
}
#endif
