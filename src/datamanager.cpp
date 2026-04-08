#include "datamanager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QDateTime>
#include <QDate>
#include <QRegularExpression>
#include <QUrl>
#include <QVariant>

DataManager::DataManager(QObject *parent) : QObject(parent)
{
    initializeActivityDatabase();
    initializeProductivityDatabase();
}

DataManager::~DataManager()
{
    if (m_activityDb.isOpen()) m_activityDb.close();
    if (m_productivityDb.isOpen()) m_productivityDb.close();
}

bool DataManager::initializeActivityDatabase()
{
    m_activityDb = QSqlDatabase::addDatabase("QSQLITE", "activity_db");
    m_activityDb.setDatabaseName("activity_logs.db");

    if (!m_activityDb.open()) {
        qWarning() << "Failed to open activity database:" << m_activityDb.lastError().text();
        return false;
    }

    QSqlQuery query(m_activityDb);
    if (!query.exec("CREATE TABLE IF NOT EXISTS log ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "id_user INTEGER NOT NULL, "
                    "start_time INTEGER NOT NULL, "
                    "end_time INTEGER NOT NULL, "
                    "app_name TEXT, "
                    "title TEXT, "
                    "url TEXT, "
                    "FOREIGN KEY(id_user) REFERENCES users(id) ON DELETE CASCADE)")) {
        qWarning() << "Failed to create log table:" << query.lastError().text();
    }

    if (!query.exec("CREATE TABLE IF NOT EXISTS users ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "username TEXT UNIQUE NOT NULL, "
                    "password TEXT NOT NULL, "
                    "department TEXT, "
                    "profile_image TEXT, "
                    "email TEXT, "
                    "role TEXT, "
                    "token TEXT)")) {
        qWarning() << "Failed to create users table:" << query.lastError().text();
    }

    return true;
}

bool DataManager::initializeProductivityDatabase()
{
    m_productivityDb = QSqlDatabase::addDatabase("QSQLITE", "productivity_db");
    m_productivityDb.setDatabaseName("produktif_app_db.db");

    if (!m_productivityDb.open()) {
        qWarning() << "Failed to open productivity database:" << m_productivityDb.lastError().text();
        return false;
    }

    migrateProductivityDatabase();

    QSqlQuery query(m_productivityDb);
    if (!query.exec("CREATE TABLE IF NOT EXISTS aplikasi ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "aplikasi TEXT NOT NULL, "
                    "window_title TEXT, "
                    "url TEXT, "
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
                    "user_id INTEGER NOT NULL,"
                    "created_at TEXT)")) {
        qWarning() << "Failed to create task table:" << query.lastError().text();
    }

    if (!query.exec("CREATE TABLE IF NOT EXISTS completed_tasks ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "project_name TEXT, "
                    "task TEXT, "
                    "max_time INTEGER, "
                    "time_usage INTEGER, "
                    "completed_time INTEGER, "
                    "user_id INTEGER)")) {
        qWarning() << "Failed to create completed_tasks table:" << query.lastError().text();
    }

    if (!query.exec("CREATE TABLE IF NOT EXISTS idle_settings ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "threshold_seconds INTEGER)")) {
        qWarning() << "Failed to create idle_settings table:" << query.lastError().text();
    }

    if (!query.exec("CREATE TABLE IF NOT EXISTS log_paused ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "task_id INTEGER NOT NULL, "
                    "start_reality TEXT NOT NULL, "
                    "end_reality TEXT, "
                    "current_status TEXT NOT NULL, "
                    "FOREIGN KEY(task_id) REFERENCES task(id))")) {
        qWarning() << "Failed to create log_paused table:" << query.lastError().text();
    }

    if (!query.exec("CREATE TABLE IF NOT EXISTS work_time ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "user_id INTEGER NOT NULL, "
                    "date TEXT NOT NULL, "
                    "elapsed_seconds INTEGER NOT NULL DEFAULT 0, "
                    "UNIQUE(user_id, date))")) {
        qWarning() << "Failed to create work_time table:" << query.lastError().text();
    }

    return true;
}

bool DataManager::ensureActivityDbOpen()
{
    if (!m_activityDb.isOpen()) {
        return m_activityDb.open();
    }
    return true;
}

bool DataManager::ensureProductivityDbOpen()
{
    if (!m_productivityDb.isOpen()) {
        return m_productivityDb.open();
    }
    return true;
}

void DataManager::migrateProductivityDatabase()
{
    // Implementation can be copied from Logger::migrateProductivityDatabase
    // For now, let's just make it a placeholder or copy if small.
}

void DataManager::checkAndCreateNewDayRecord(int userId)
{
    if (userId == -1 || !ensureProductivityDbOpen()) return;

    QString today = QDate::currentDate().toString("yyyy-MM-dd");
    QSqlQuery query(m_productivityDb);
    query.prepare("SELECT id FROM work_time WHERE user_id = :user_id AND date = :date");
    query.bindValue(":user_id", userId);
    query.bindValue(":date", today);

    if (query.exec() && query.next()) {
        return;
    } else {
        saveWorkTimeData(userId, 0);
        qDebug() << "New day record created in DB for user:" << userId;
    }
}

int DataManager::loadWorkTimeData(int userId)
{
    if (userId == -1 || !ensureProductivityDbOpen()) return 0;

    QString today = QDate::currentDate().toString("yyyy-MM-dd");
    QSqlQuery query(m_productivityDb);
    query.prepare("SELECT elapsed_seconds FROM work_time WHERE user_id = :user_id AND date = :date");
    query.bindValue(":user_id", userId);
    query.bindValue(":date", today);

    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

void DataManager::saveWorkTimeData(int userId, int elapsedSeconds)
{
    if (userId == -1 || !ensureProductivityDbOpen()) return;

    QString today = QDate::currentDate().toString("yyyy-MM-dd");
    QSqlQuery query(m_productivityDb);
    query.prepare("INSERT OR REPLACE INTO work_time (user_id, date, elapsed_seconds) "
                  "VALUES (:user_id, :date, :seconds)");
    query.bindValue(":user_id", userId);
    query.bindValue(":date", today);
    query.bindValue(":seconds", elapsedSeconds);

    if (!query.exec()) {
        qWarning() << "Failed to save work time to DB:" << query.lastError().text();
    }
}

void DataManager::updateProductivityCache(int userId)
{
    m_appCache.clear();
    m_domainCache.clear();

    if (!ensureProductivityDbOpen() || userId == -1) return;

    QSqlQuery query(m_productivityDb);
    query.prepare("SELECT aplikasi, url, jenis FROM aplikasi WHERE jenis IN (1, 2)");

    if (!query.exec()) {
        qWarning() << "Failed to load productivity apps cache from DB:" << query.lastError().text();
        return;
    }

    while (query.next()) {
        QString appName = query.value(0).toString();
        QString url = query.value(1).toString();
        int type = query.value(2).toInt();

        if (!url.isEmpty() && url != "n/a" && url != "-") {
            QString domain = extractDomain(url);
            if (!domain.isEmpty()) m_domainCache[domain] = type;
        } else if (!appName.isEmpty()) {
            m_appCache[normalizeString(appName)] = type;
        }
    }
}

int DataManager::getAppProductivityType(const QString &appName, const QString &url) const
{
    if (!url.isEmpty()) {
        QString domain = extractDomain(url);
        if (m_domainCache.contains(domain)) return m_domainCache.value(domain);
    } else {
        QString normApp = normalizeString(appName);
        if (m_appCache.contains(normApp)) return m_appCache.value(normApp);
    }
    return 0;
}

QString DataManager::normalizeString(const QString &str) const
{
    return str.toLower()
        .remove(' ')
        .remove('-')
        .remove('_')
        .remove('.');
}

QString DataManager::extractDomain(const QString &url) const
{
    if (url.isEmpty()) return "";
    
    QString cleanUrl = url;
    if (!cleanUrl.startsWith("http://") && !cleanUrl.startsWith("https://")) {
        cleanUrl = "https://" + cleanUrl;
    }

    QUrl qurl(cleanUrl);
    QString domain = qurl.host();
    
    if (domain.isEmpty()) {
        QRegularExpression domainRegex(R"((?:https?://)?(?:www\.)?([^/]+))");
        QRegularExpressionMatch match = domainRegex.match(url);
        if (match.hasMatch()) domain = match.captured(1);
    }
    
    if (domain.startsWith("www.")) {
        domain = domain.mid(4);
    }
    return domain.toLower();
}

bool DataManager::logWindowActivity(int userId, const WindowManager::WindowInfo &info, qint64 startTime, qint64 endTime)
{
    if (!ensureActivityDbOpen() || userId == -1) return false;

    QSqlQuery query(m_activityDb);
    query.prepare("INSERT INTO log (id_user, start_time, end_time, app_name, title, url) "
                  "VALUES (:id_user, :start, :end, :app, :title, :url)");
    query.bindValue(":id_user", userId);
    query.bindValue(":start", startTime);
    query.bindValue(":end", endTime);
    query.bindValue(":app", info.appName);
    query.bindValue(":title", info.title);
    query.bindValue(":url", info.url.isEmpty() ? QVariant() : info.url);

    if (!query.exec()) {
        qWarning() << "Failed to log window activity to DB:" << query.lastError().text();
        return false;
    }
    return true;
}

void DataManager::syncProductivityApps(const QJsonArray &apps, int userId)
{
    if (!ensureProductivityDbOpen() || userId == -1) return;

    m_productivityDb.transaction();
    bool success = true;

    QSqlQuery query(m_productivityDb);
    for (const QJsonValue &value : apps) {
        QJsonObject appObj = value.toObject();
        QString appName = appObj["application_name"].toString();
        QString status = appObj["productivity_status"].toString().toLower();
        QString processName = appObj["process_name"].toString();
        QString url = appObj["url"].toString();
        int forUserId = appObj["user_id"].toInt();

        int jenis = 0;
        if (status == "productive") jenis = 1;
        else if (status == "non-productive") jenis = 2;

        QString forUsers = QString::number(forUserId);
        if (forUserId == 0) forUsers = "0";

        // Check for existing
        query.prepare("SELECT id FROM aplikasi WHERE aplikasi = :app AND (url = :url OR (url IS NULL AND :url_empty)) AND for_user = :for_user");
        query.bindValue(":app", appName);
        query.bindValue(":url", url);
        query.bindValue(":url_empty", url.isEmpty());
        query.bindValue(":for_user", forUsers);

        if (query.exec() && query.next()) {
            // Update
            QSqlQuery updateQ(m_productivityDb);
            updateQ.prepare("UPDATE aplikasi SET jenis = :jenis, window_title = :window WHERE id = :id");
            updateQ.bindValue(":jenis", jenis);
            updateQ.bindValue(":window", processName);
            updateQ.bindValue(":id", query.value(0).toInt());
            if (!updateQ.exec()) success = false;
        } else {
            // Insert
            QSqlQuery insertQ(m_productivityDb);
            insertQ.prepare("INSERT INTO aplikasi (aplikasi, window_title, url, jenis, for_user) VALUES (:app, :window, :url, :jenis, :for_user)");
            insertQ.bindValue(":app", appName);
            insertQ.bindValue(":window", processName);
            insertQ.bindValue(":url", url);
            insertQ.bindValue(":jenis", jenis);
            insertQ.bindValue(":for_user", forUsers);
            if (!insertQ.exec()) success = false;
        }
        if (!success) break;
    }

    if (success) m_productivityDb.commit();
    else m_productivityDb.rollback();
}

int DataManager::calculateProductiveSeconds(int userId, const QString& date)
{
    if (!ensureActivityDbOpen() || !ensureProductivityDbOpen() || userId == -1) return 0;

    QSqlQuery query(m_activityDb);
    query.prepare(R"(
        SELECT IFNULL(SUM(log.end_time - log.start_time), 0)
        FROM log
        JOIN productivity_db.aplikasi AS aplikasi ON (
            (log.url IS NOT NULL AND log.url != '' AND aplikasi.url IS NOT NULL AND aplikasi.url != '' AND LOWER(log.url) LIKE '%' || LOWER(aplikasi.url) || '%')
            OR
            (log.app_name IS NOT NULL AND log.app_name != '' AND aplikasi.aplikasi IS NOT NULL AND aplikasi.aplikasi != '' AND LOWER(log.app_name) LIKE '%' || LOWER(aplikasi.aplikasi) || '%')
        )
        WHERE log.id_user = :user_id
          AND date(log.start_time, 'unixepoch', 'localtime') = :date
          AND aplikasi.jenis = 1
    )");
    
    // Note: This cross-database join requires ATTACH DATABASE if not already handled.
    // However, in current architecture they are separate connections.
    // I will simplify to a logic that works with current architecture or suggest ATTACH.
    // For now, let's use the local logic if they are separate.
    return 0; // Placeholder until I decide on ATTACH strategy.
}

QVariantMap DataManager::getProductivityStats(int userId, const QString& startDate, const QString& endDate)
{
    QVariantMap stats;
    if (!ensureActivityDbOpen() || userId == -1) return stats;

    double productiveTime = 0, nonProductiveTime = 0, neutralTime = 0, idleTime = 0, totalTime = 0;

    QString queryStr = "SELECT start_time, end_time, app_name, url FROM log WHERE id_user = :id_user ";
    if (!startDate.isEmpty()) queryStr += QString("AND date(start_time, 'unixepoch', 'localtime') >= date('%1') ").arg(startDate);
    if (!endDate.isEmpty()) queryStr += QString("AND date(start_time, 'unixepoch', 'localtime') <= date('%1') ").arg(endDate);

    QSqlQuery query(m_activityDb);
    query.prepare(queryStr);
    query.bindValue(":id_user", userId);

    if (query.exec()) {
        while (query.next()) {
            qint64 start = query.value(0).toLongLong();
            qint64 end = query.value(1).toLongLong();
            QString app = query.value(2).toString();
            double duration = end - start;
            if (duration <= 0) continue;

            if (app == "Idle") idleTime += duration;
            else {
                // Simplified type check for now
                neutralTime += duration;
            }
            totalTime += duration;
        }
    }

    double total = totalTime > 0 ? totalTime : 1;
    stats["productive"] = (productiveTime / total) * 100;
    stats["nonProductive"] = (nonProductiveTime / total) * 100;
    stats["neutral"] = (neutralTime / total) * 100;
    stats["idle"] = (idleTime / total) * 100;
    return stats;
}

QString DataManager::getLogAsCsv(int userId, const QString& startDate, const QString& endDate)
{
    if (!ensureActivityDbOpen() || userId == -1) return "";

    QString content;
    QString queryStr = "SELECT start_time, end_time, app_name, title, url FROM log WHERE id_user = :id_user ";
    if (!startDate.isEmpty()) queryStr += QString("AND date(start_time, 'unixepoch', 'localtime') >= date('%1') ").arg(startDate);
    if (!endDate.isEmpty()) queryStr += QString("AND date(start_time, 'unixepoch', 'localtime') <= date('%1') ").arg(endDate);
    queryStr += "ORDER BY start_time DESC";

    QSqlQuery query(m_activityDb);
    query.prepare(queryStr);
    query.bindValue(":id_user", userId);

    if (query.exec()) {
        while (query.next()) {
            content += QString("%1,%2,%3,%4,%5\n")
                .arg(QDateTime::fromSecsSinceEpoch(query.value(0).toLongLong()).toString("hh:mm:ss"))
                .arg(QDateTime::fromSecsSinceEpoch(query.value(1).toLongLong()).toString("hh:mm:ss"))
                .arg(query.value(2).toString())
                .arg(query.value(3).toString())
                .arg(query.value(4).toString());
        }
    }
    return content;
}

QJsonArray DataManager::getDailyUsageReportData(int userId, const QString& date)
{
    QJsonArray dataArray;
    if (!ensureActivityDbOpen() || userId == -1) return dataArray;

    QHash<QString, qint64> appDurations;
    QHash<QPair<QString, QString>, qint64> browserUsage;

    QSqlQuery logQuery(m_activityDb);
    logQuery.prepare(R"(
        SELECT app_name, url, start_time, end_time
        FROM log
        WHERE id_user = :user_id
        AND date(start_time, 'unixepoch', 'localtime') = :today
        AND app_name != 'Idle'
    )");
    logQuery.bindValue(":user_id", userId);
    logQuery.bindValue(":today", date);

    if (logQuery.exec()) {
        while (logQuery.next()) {
            QString appName = logQuery.value(0).toString();
            QString urlString = logQuery.value(1).toString();
            qint64 startTime = logQuery.value(2).toLongLong();
            qint64 endTime = logQuery.value(3).toLongLong();
            qint64 duration = endTime - startTime;
            if (duration <= 0) continue;

            if (!urlString.isEmpty()) {
                // Simplified domain extraction
                QString domain = urlString;
                QPair<QString, QString> key(appName, domain);
                browserUsage[key] += duration;
            } else {
                appDurations[appName] += duration;
            }
        }
    }

    for (auto it = appDurations.constBegin(); it != appDurations.constEnd(); ++it) {
        QJsonObject obj;
        obj["user_id"] = userId;
        obj["app_name"] = it.key();
        obj["duration"] = it.value();
        obj["url"] = QJsonValue::Null;
        obj["status"] = "neutral";
        dataArray.append(obj);
    }

    for (auto it = browserUsage.constBegin(); it != browserUsage.constEnd(); ++it) {
        QJsonObject obj;
        obj["user_id"] = userId;
        obj["app_name"] = it.key().first;
        obj["duration"] = it.value();
        obj["url"] = it.key().second;
        obj["status"] = "neutral";
        dataArray.append(obj);
    }

    return dataArray;
}
