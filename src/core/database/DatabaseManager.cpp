#include "DatabaseManager.h"

#include <QCoreApplication>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QDebug>

DatabaseManager& DatabaseManager::instance()
{
    static DatabaseManager s_instance;
    return s_instance;
}

DatabaseManager::DatabaseManager(QObject *parent)
    : QObject(parent)
{
}

DatabaseManager::~DatabaseManager()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
}

QString DatabaseManager::databasePath() const
{
    return m_dbPath;
}

QSqlDatabase DatabaseManager::database() const
{
    return m_db;
}

bool DatabaseManager::ensureOpen() const
{
    if (!m_db.isValid()) {
        m_db = QSqlDatabase::database("deskmon_main_db");
    }
    if (!m_db.isOpen()) {
        if (!m_db.open()) {
            qCritical() << "Failed to open Deskmon database:" << m_db.lastError().text();
            return false;
        }
    }
    return true;
}

bool DatabaseManager::initialize()
{
    // Gunakan lokasi standar pengguna (AppData/Local di Windows)
    // Memisahkan database dari folder aplikasi agar aman saat update/uninstall
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(dataDir);

    m_dbPath = dataDir + "/deskmon.db";
    bool isNewDatabase = !QFileInfo::exists(m_dbPath);

    m_db = QSqlDatabase::addDatabase("QSQLITE", "deskmon_main_db");
    m_db.setDatabaseName(m_dbPath);

    if (!m_db.open()) {
        qCritical() << "Could not open SQLite database at" << m_dbPath << ":" << m_db.lastError().text();
        return false;
    }

    qInfo() << "=== Deskmon Database Initialized at:" << m_dbPath << "===";

    // Aktifkan WAL mode dan foreign keys untuk performa maksimal
    QSqlQuery pragmaQuery(m_db);
    pragmaQuery.exec("PRAGMA journal_mode = WAL;");
    pragmaQuery.exec("PRAGMA foreign_keys = ON;");

    if (!createTables()) {
        return false;
    }

    createViews();

    if (isNewDatabase) {
        migrateLegacyDatabases();
    }

    return true;
}

bool DatabaseManager::createTables()
{
    QSqlQuery query(m_db);

    // 1. Domain User & Autentikasi
    const char *usersTable = R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT UNIQUE NOT NULL,
            password TEXT NOT NULL,
            email TEXT,
            department TEXT,
            role TEXT,
            token TEXT,
            profile_image TEXT
        );
    )";
    if (!query.exec(usersTable)) {
        qWarning() << "Failed to create users table:" << query.lastError().text();
        return false;
    }

    // 2. Domain Pencatatan Aktivitas Window
    const char *activityLogsTable = R"(
        CREATE TABLE IF NOT EXISTS activity_logs (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            start_time INTEGER NOT NULL,
            end_time INTEGER NOT NULL,
            app_name TEXT,
            title TEXT,
            url TEXT,
            FOREIGN KEY(user_id) REFERENCES users(id) ON DELETE CASCADE
        );
    )";
    if (!query.exec(activityLogsTable)) {
        qWarning() << "Failed to create activity_logs table:" << query.lastError().text();
        return false;
    }

    // 3. Domain Klasifikasi Produktivitas
    const char *productivityAppsTable = R"(
        CREATE TABLE IF NOT EXISTS productivity_apps (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            app_name TEXT NOT NULL,
            window_title TEXT,
            url TEXT,
            productivity_type INTEGER NOT NULL, -- 1: Productive, 2: Non-Productive, 0: Neutral
            for_user TEXT NOT NULL DEFAULT '0'
        );
    )";
    if (!query.exec(productivityAppsTable)) {
        qWarning() << "Failed to create productivity_apps table:" << query.lastError().text();
        return false;
    }

    // 4. Domain Pengaturan Idle
    const char *idleSettingsTable = R"(
        CREATE TABLE IF NOT EXISTS idle_settings (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            threshold_seconds INTEGER NOT NULL DEFAULT 180
        );
    )";
    if (!query.exec(idleSettingsTable)) {
        qWarning() << "Failed to create idle_settings table:" << query.lastError().text();
        return false;
    }

    // Inisialisasi default idle threshold jika kosong
    query.exec("SELECT COUNT(*) FROM idle_settings");
    if (query.next() && query.value(0).toInt() == 0) {
        query.exec("INSERT INTO idle_settings (threshold_seconds) VALUES (180)");
    }

    // 5. Domain Tasks
    const char *tasksTable = R"(
        CREATE TABLE IF NOT EXISTS tasks (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            project_name TEXT NOT NULL,
            task_desc TEXT,
            max_time INTEGER NOT NULL DEFAULT 0,
            time_usage INTEGER NOT NULL DEFAULT 0,
            active BOOLEAN NOT NULL DEFAULT 0,
            status TEXT NOT NULL DEFAULT 'pending',
            paused BOOLEAN NOT NULL DEFAULT 0,
            created_at TEXT
        );
    )";
    if (!query.exec(tasksTable)) {
        qWarning() << "Failed to create tasks table:" << query.lastError().text();
        return false;
    }

    // 6. Domain Completed Tasks
    const char *completedTasksTable = R"(
        CREATE TABLE IF NOT EXISTS completed_tasks (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            project_name TEXT,
            task_desc TEXT,
            max_time INTEGER,
            time_usage INTEGER,
            completed_time INTEGER
        );
    )";
    if (!query.exec(completedTasksTable)) {
        qWarning() << "Failed to create completed_tasks table:" << query.lastError().text();
        return false;
    }

    // 7. Domain Task Pause Logs
    const char *taskPauseLogsTable = R"(
        CREATE TABLE IF NOT EXISTS task_pause_logs (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            task_id INTEGER NOT NULL,
            paused_at TEXT NOT NULL,
            resumed_at TEXT,
            status TEXT NOT NULL,
            FOREIGN KEY(task_id) REFERENCES tasks(id) ON DELETE CASCADE
        );
    )";
    if (!query.exec(taskPauseLogsTable)) {
        qWarning() << "Failed to create task_pause_logs table:" << query.lastError().text();
        return false;
    }

    // 8. Domain Waktu Kerja Harian
    const char *workTimeTable = R"(
        CREATE TABLE IF NOT EXISTS work_time_records (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            work_date TEXT NOT NULL,
            elapsed_seconds INTEGER NOT NULL DEFAULT 0,
            UNIQUE(user_id, work_date)
        );
    )";
    if (!query.exec(workTimeTable)) {
        qWarning() << "Failed to create work_time_records table:" << query.lastError().text();
        return false;
    }

    return true;
}

bool DatabaseManager::createViews()
{
    QSqlQuery query(m_db);

    // View untuk kompatibilitas backward
    query.exec(R"(
        CREATE VIEW IF NOT EXISTS log AS
        SELECT id, user_id AS id_user, start_time, end_time, app_name, title, url
        FROM activity_logs;
    )");

    query.exec(R"(
        CREATE VIEW IF NOT EXISTS aplikasi AS
        SELECT id, app_name AS aplikasi, window_title, url, productivity_type AS jenis, 0 AS productivity, for_user
        FROM productivity_apps;
    )");

    query.exec(R"(
        CREATE VIEW IF NOT EXISTS task AS
        SELECT id, project_name, task_desc AS task, max_time, time_usage, active, status, paused, user_id, created_at
        FROM tasks;
    )");

    query.exec(R"(
        CREATE VIEW IF NOT EXISTS work_time AS
        SELECT id, user_id, work_date AS date, elapsed_seconds
        FROM work_time_records;
    )");

    return true;
}

void DatabaseManager::migrateLegacyDatabases()
{
    qInfo() << "Checking for legacy databases to migrate...";

    auto findFile = [](const QString &filename) -> QString {
        QStringList candidates = {
            QDir::current().filePath(filename),
            QCoreApplication::applicationDirPath() + "/" + filename,
            QDir(QCoreApplication::applicationDirPath() + "/..").filePath(filename)
        };
        for (const QString &path : candidates) {
            if (QFileInfo::exists(path)) return QFileInfo(path).absoluteFilePath();
        }
        return "";
    };

    QString legacyActivityDb = findFile("activity_logs.db");
    QString legacyProductivityDb = findFile("produktif_app_db.db");

    // 1. Migrasi dari activity_logs.db
    if (!legacyActivityDb.isEmpty()) {
        qInfo() << "Found legacy activity_logs.db at" << legacyActivityDb << ", migrating data...";
        QSqlQuery query(m_db);
        if (query.exec(QString("ATTACH DATABASE '%1' AS legacy_act").arg(legacyActivityDb))) {
            query.exec(R"(
                INSERT OR IGNORE INTO users (id, username, password, department, profile_image, email, role, token)
                SELECT id, username, password, department, profile_image, email, role, token FROM legacy_act.users;
            )");
            query.exec(R"(
                INSERT OR IGNORE INTO activity_logs (id, user_id, start_time, end_time, app_name, title, url)
                SELECT id, id_user, start_time, end_time, app_name, title, url FROM legacy_act.log;
            )");
            query.exec("DETACH DATABASE legacy_act");
            qInfo() << "Migration from activity_logs.db completed successfully.";
        }
    }

    // 2. Migrasi dari produktif_app_db.db
    if (!legacyProductivityDb.isEmpty()) {
        qInfo() << "Found legacy produktif_app_db.db at" << legacyProductivityDb << ", migrating data...";
        QSqlQuery query(m_db);
        if (query.exec(QString("ATTACH DATABASE '%1' AS legacy_prod").arg(legacyProductivityDb))) {
            query.exec(R"(
                INSERT OR IGNORE INTO productivity_apps (id, app_name, window_title, url, productivity_type, for_user)
                SELECT id, aplikasi, window_title, url, jenis, for_user FROM legacy_prod.aplikasi;
            )");
            query.exec(R"(
                INSERT OR IGNORE INTO tasks (id, user_id, project_name, task_desc, max_time, time_usage, active, status, paused, created_at)
                SELECT id, user_id, project_name, task, max_time, time_usage, active, status, paused, created_at FROM legacy_prod.task;
            )");
            query.exec(R"(
                INSERT OR IGNORE INTO completed_tasks (id, user_id, project_name, task_desc, max_time, time_usage, completed_time)
                SELECT id, user_id, project_name, task, max_time, time_usage, completed_time FROM legacy_prod.completed_tasks;
            )");
            query.exec(R"(
                INSERT OR IGNORE INTO task_pause_logs (id, task_id, paused_at, resumed_at, status)
                SELECT id, task_id, start_reality, end_reality, current_status FROM legacy_prod.log_paused;
            )");
            query.exec(R"(
                INSERT OR IGNORE INTO work_time_records (id, user_id, work_date, elapsed_seconds)
                SELECT id, user_id, date, elapsed_seconds FROM legacy_prod.work_time;
            )");
            query.exec(R"(
                INSERT OR REPLACE INTO idle_settings (id, threshold_seconds)
                SELECT id, threshold_seconds FROM legacy_prod.idle_settings LIMIT 1;
            )");
            query.exec("DETACH DATABASE legacy_prod");
            qInfo() << "Migration from produktif_app_db.db completed successfully.";
        }
    }
}

bool DatabaseManager::transaction()
{
    return m_db.transaction();
}

bool DatabaseManager::commit()
{
    return m_db.commit();
}

bool DatabaseManager::rollback()
{
    return m_db.rollback();
}
