#include "WorkLogRepository.h"
#include "DatabaseManager.h"
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
    return DatabaseManager::instance().ensureOpen();
}

bool WorkLogRepository::initialize()
{
    bool ok = DatabaseManager::instance().ensureOpen();
    emit logsChanged();
    return ok;
}

void WorkLogRepository::logWindowChange(const WindowInfo &info, qint64 startTime, qint64 endTime, int userId)
{
    if (!ensureDatabaseOpen()) {
        qWarning() << "Cannot log window change: Database is not open";
        return;
    }

    if (endTime <= startTime) {
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

    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("INSERT INTO activity_logs (user_id, start_time, end_time, app_name, title, url) "
                  "VALUES (:user_id, :start, :end, :app, :title, :url)");
    query.bindValue(":user_id", userId);
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

    QString queryStr = "SELECT COUNT(*) FROM activity_logs WHERE app_name IS NOT NULL AND title IS NOT NULL AND user_id = :user_id";
    if (!m_startDateFilter.isEmpty()) {
        queryStr += QString(" AND date(start_time, 'unixepoch', 'localtime') >= date('%1')").arg(m_startDateFilter);
    }
    if (!m_endDateFilter.isEmpty()) {
        queryStr += QString(" AND date(start_time, 'unixepoch', 'localtime') <= date('%1')").arg(m_endDateFilter);
    }

    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare(queryStr);
    query.bindValue(":user_id", userId);
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
    QString queryStr = "SELECT start_time, end_time, app_name, title, url FROM activity_logs "
                       "WHERE app_name IS NOT NULL AND title IS NOT NULL AND user_id = :user_id ";

    if (!m_startDateFilter.isEmpty()) {
        queryStr += QString("AND date(start_time, 'unixepoch', 'localtime') >= date('%1') ").arg(m_startDateFilter);
    }
    if (!m_endDateFilter.isEmpty()) {
        queryStr += QString("AND date(start_time, 'unixepoch', 'localtime') <= date('%1') ").arg(m_endDateFilter);
    }

    queryStr += "ORDER BY start_time DESC";

    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare(queryStr);
    query.bindValue(":user_id", userId);
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

    QString queryStr = "SELECT start_time, end_time, app_name, title, url FROM activity_logs WHERE user_id = :user_id ";
    if (!m_startDateFilter.isEmpty()) {
        queryStr += "AND date(start_time, 'unixepoch', 'localtime') >= :startDate ";
    }
    if (!m_endDateFilter.isEmpty()) {
        queryStr += "AND date(start_time, 'unixepoch', 'localtime') <= :endDate ";
    }
    queryStr += "ORDER BY start_time DESC";

    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare(queryStr);
    query.bindValue(":user_id", userId);
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
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("SELECT * FROM activity_logs WHERE user_id = :user_id ORDER BY start_time DESC LIMIT 50");
    query.bindValue(":user_id", userId);
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
