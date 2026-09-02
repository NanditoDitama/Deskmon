#include "WorkLogRepository.h"
#include <QSqlError>
#include <QDateTime>
#include <QDebug>
#include <QVariant>

WorkLogRepository::WorkLogRepository(QObject *parent)
    : QObject(parent)
    , m_logModel(new QSqlQueryModel(this))
{
}

bool WorkLogRepository::ensureDatabaseOpen() const
{
    if (!m_db.isValid()) {
        m_db = QSqlDatabase::database("activity_db");
    }
    if (!m_db.isOpen()) {
        if (!m_db.open()) {
            qWarning() << "Failed to open activity database:" << m_db.lastError().text();
            return false;
        }
    }
    return true;
}

bool WorkLogRepository::initialize()
{
    m_db = QSqlDatabase::addDatabase("QSQLITE", "activity_db");
    m_db.setDatabaseName("activity_logs.db");

    if (!m_db.open()) {
        qWarning() << "Failed to open activity database:" << m_db.lastError().text();
        return false;
    }

    QSqlQuery query(m_db);
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

    emit logsChanged();
    return true;
}

void WorkLogRepository::logWindowChange(const WindowInfo &info, qint64 startTime, qint64 endTime, int userId)
{
    if (!ensureDatabaseOpen()) {
        qWarning() << "Cannot log window change: Database is not open";
        return;
    }

    if (endTime <= startTime) {
        qWarning() << "Invalid window change period: endTime (" << endTime << ") <= startTime (" << startTime << ")";
        return;
    }
    if (userId == -1) {
        qWarning() << "Cannot log window change: No user logged in";
        return;
    }

    QString urlToLog = info.url;
    if (!urlToLog.isEmpty() && (urlToLog.contains(' ') || !urlToLog.contains('.'))) {
        qDebug() << "URL Ditolak (dianggap query pencarian):" << urlToLog;
        urlToLog.clear();
    }

    QSqlQuery query(m_db);
    query.prepare("INSERT INTO log (id_user, start_time, end_time, app_name, title, url) "
                  "VALUES (:id_user, :start, :end, :app, :title, :url)");
    query.bindValue(":id_user", userId);
    query.bindValue(":start", startTime);
    query.bindValue(":end", endTime);
    query.bindValue(":app", info.appName);
    query.bindValue(":title", info.title);
    query.bindValue(":url", urlToLog.isEmpty() ? QVariant() : urlToLog);

    if (!query.exec()) {
        qWarning() << "Failed to log window change:" << query.lastError().text();
    } else {
        emit logsChanged();
    }
}

int WorkLogRepository::logCount(int userId) const
{
    if (!ensureDatabaseOpen() || userId == -1) {
        return 0;
    }

    QString queryStr = "SELECT COUNT(*) FROM log WHERE app_name IS NOT NULL AND title IS NOT NULL AND id_user = :id_user";
    if (!m_startDateFilter.isEmpty()) {
        queryStr += QString(" AND date(start_time, 'unixepoch', 'localtime') >= date('%1')").arg(m_startDateFilter);
    }
    if (!m_endDateFilter.isEmpty()) {
        queryStr += QString(" AND date(start_time, 'unixepoch', 'localtime') <= date('%1')").arg(m_endDateFilter);
    }

    QSqlQuery query(m_db);
    query.prepare(queryStr);
    query.bindValue(":id_user", userId);
    if (!query.exec()) {
        qWarning() << "Failed to count logs:" << query.lastError().text();
        return 0;
    }
    if (query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

QString WorkLogRepository::logContent(int userId) const
{
    if (!ensureDatabaseOpen() || userId == -1) {
        return "";
    }

    QString content;
    QString queryStr = "SELECT start_time, end_time, app_name, title, url FROM log "
                       "WHERE app_name IS NOT NULL AND title IS NOT NULL AND id_user = :id_user ";

    if (!m_startDateFilter.isEmpty()) {
        queryStr += QString("AND date(start_time, 'unixepoch', 'localtime') >= date('%1') ").arg(m_startDateFilter);
    }
    if (!m_endDateFilter.isEmpty()) {
        queryStr += QString("AND date(start_time, 'unixepoch', 'localtime') <= date('%1') ").arg(m_endDateFilter);
    }

    queryStr += "ORDER BY start_time DESC";

    QSqlQuery query(m_db);
    query.prepare(queryStr);
    query.bindValue(":id_user", userId);
    if (!query.exec()) {
        qWarning() << "Failed to fetch log content:" << query.lastError().text();
        return content;
    }

    while (query.next()) {
        qint64 start = query.value(0).toLongLong();
        qint64 end = query.value(1).toLongLong();
        QString app = query.value(2).toString();
        QString title = query.value(3).toString();
        QString url = query.value(4).toString();

        content += QString("%1,%2,%3,%4,%5\n")
                       .arg(QDateTime::fromSecsSinceEpoch(start).toString("hh:mm:ss"))
                       .arg(QDateTime::fromSecsSinceEpoch(end).toString("hh:mm:ss"))
                       .arg(app)
                       .arg(title)
                       .arg(url);
    }
    return content;
}

void WorkLogRepository::setLogFilter(const QString &startDate, const QString &endDate, int userId)
{
    m_startDateFilter = startDate;
    m_endDateFilter = endDate;

    if (!m_logModel || !ensureDatabaseOpen()) {
        return;
    }

    QString queryStr = "SELECT start_time, end_time, app_name, title, url FROM log WHERE id_user = :id_user ";
    if (!m_startDateFilter.isEmpty()) {
        queryStr += "AND date(start_time, 'unixepoch', 'localtime') >= :startDate ";
    }
    if (!m_endDateFilter.isEmpty()) {
        queryStr += "AND date(start_time, 'unixepoch', 'localtime') <= :endDate ";
    }
    queryStr += "ORDER BY start_time DESC";

    QSqlQuery query(m_db);
    query.prepare(queryStr);
    query.bindValue(":id_user", userId);
    if (!m_startDateFilter.isEmpty()) query.bindValue(":startDate", m_startDateFilter);
    if (!m_endDateFilter.isEmpty()) query.bindValue(":endDate", m_endDateFilter);

    if (query.exec()) {
        m_logModel->setQuery(std::move(query));
    }

    emit logsChanged();
}

void WorkLogRepository::clearLogFilter(int userId)
{
    setLogFilter("", "", userId);
}

void WorkLogRepository::showLogs(int userId)
{
    clearLogFilter(userId);
}

QString WorkLogRepository::debugShowRawData(int userId) const
{
    if (!ensureDatabaseOpen()) return "Database not open";
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM log WHERE id_user = :id_user ORDER BY start_time DESC LIMIT 50");
    query.bindValue(":id_user", userId);
    if (!query.exec()) return "Query failed";

    QString result;
    while (query.next()) {
        result += QString("ID: %1 | User: %2 | Start: %3 | End: %4 | App: %5 | Title: %6 | URL: %7\n")
                      .arg(query.value(0).toString())
                      .arg(query.value(1).toString())
                      .arg(query.value(2).toString())
                      .arg(query.value(3).toString())
                      .arg(query.value(4).toString())
                      .arg(query.value(5).toString())
                      .arg(query.value(6).toString());
    }
    return result;
}
