#include "WorkLogRepository.h"
#include "DatabaseManager.h"
#include <QSqlError>
#include <QDate>
#include <QDateTime>
#include <QTime>
#include <QDebug>
#include <QVariant>

WorkLogRepository::WorkLogRepository(QObject *parent)
    : QObject(parent)
    , m_logModel(new QSqlQueryModel(this))
{
}

void WorkLogRepository::dateRangeToEpoch(const QString &startDate, const QString &endDate,
                                         qint64 &outStartEpoch, bool &hasStart,
                                         qint64 &outEndExclusiveEpoch, bool &hasEnd)
{
    hasStart = false;
    hasEnd = false;
    outStartEpoch = 0;
    outEndExclusiveEpoch = 0;

    if (!startDate.isEmpty()) {
        const QDate startD = QDate::fromString(startDate, Qt::ISODate);
        if (startD.isValid()) {
            outStartEpoch = QDateTime(startD, QTime(0, 0, 0)).toSecsSinceEpoch();
            hasStart = true;
        }
    }
    if (!endDate.isEmpty()) {
        const QDate endD = QDate::fromString(endDate, Qt::ISODate);
        if (endD.isValid()) {
            outEndExclusiveEpoch = QDateTime(endD.addDays(1), QTime(0, 0, 0)).toSecsSinceEpoch();
            hasEnd = true;
        }
    }
}

void WorkLogRepository::invalidateCache()
{
    m_countCacheValid = false;
    m_contentCacheValid = false;
}

bool WorkLogRepository::ensureDatabaseOpen() const
{
    return DatabaseManager::instance().ensureOpen();
}

bool WorkLogRepository::initialize()
{
    bool ok = DatabaseManager::instance().ensureOpen();
    invalidateCache();
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
        invalidateCache();
        emit logsChanged();
    }
}

int WorkLogRepository::logCount(int userId) const
{
    if (!ensureDatabaseOpen() || userId == -1) {
        return 0;
    }

    if (m_countCacheValid && m_cachedCountUserId == userId) {
        return m_cachedLogCount;
    }

    qint64 startEpoch = 0;
    qint64 endExclusiveEpoch = 0;
    bool hasStartEpoch = false;
    bool hasEndEpoch = false;
    dateRangeToEpoch(m_startDateFilter, m_endDateFilter, startEpoch, hasStartEpoch, endExclusiveEpoch, hasEndEpoch);

    QString queryStr = "SELECT COUNT(*) FROM activity_logs WHERE app_name IS NOT NULL AND title IS NOT NULL AND user_id = :user_id";
    if (!m_startDateFilter.isEmpty()) {
        queryStr += hasStartEpoch ? " AND start_time >= :startEpoch"
                                  : " AND date(start_time, 'unixepoch', 'localtime') >= date(:startDate)";
    }
    if (!m_endDateFilter.isEmpty()) {
        queryStr += hasEndEpoch ? " AND start_time < :endEpoch"
                                : " AND date(start_time, 'unixepoch', 'localtime') <= date(:endDate)";
    }

    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare(queryStr);
    query.bindValue(":user_id", userId);
    if (!m_startDateFilter.isEmpty()) {
        if (hasStartEpoch) query.bindValue(":startEpoch", startEpoch);
        else query.bindValue(":startDate", m_startDateFilter);
    }
    if (!m_endDateFilter.isEmpty()) {
        if (hasEndEpoch) query.bindValue(":endEpoch", endExclusiveEpoch);
        else query.bindValue(":endDate", m_endDateFilter);
    }
    if (!query.exec()) {
        qWarning() << "Failed to count logs:" << query.lastError().text();
        return 0;
    }
    int count = 0;
    if (query.next()) {
        count = query.value(0).toInt();
    }
    m_cachedLogCount = count;
    m_cachedCountUserId = userId;
    m_countCacheValid = true;
    return count;
}

QString WorkLogRepository::logContent(int userId) const
{
    if (!ensureDatabaseOpen() || userId == -1) {
        return "";
    }

    if (m_contentCacheValid && m_cachedContentUserId == userId) {
        return m_cachedLogContent;
    }

    qint64 startEpoch = 0;
    qint64 endExclusiveEpoch = 0;
    bool hasStartEpoch = false;
    bool hasEndEpoch = false;
    dateRangeToEpoch(m_startDateFilter, m_endDateFilter, startEpoch, hasStartEpoch, endExclusiveEpoch, hasEndEpoch);

    QString content;
    content.reserve(4096);
    QString queryStr = "SELECT start_time, end_time, app_name, title, url FROM activity_logs "
                       "WHERE app_name IS NOT NULL AND title IS NOT NULL AND user_id = :user_id ";

    if (!m_startDateFilter.isEmpty()) {
        queryStr += hasStartEpoch ? "AND start_time >= :startEpoch "
                                  : "AND date(start_time, 'unixepoch', 'localtime') >= date(:startDate) ";
    }
    if (!m_endDateFilter.isEmpty()) {
        queryStr += hasEndEpoch ? "AND start_time < :endEpoch "
                                : "AND date(start_time, 'unixepoch', 'localtime') <= date(:endDate) ";
    }

    queryStr += "ORDER BY start_time DESC";

    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare(queryStr);
    query.bindValue(":user_id", userId);
    if (!m_startDateFilter.isEmpty()) {
        if (hasStartEpoch) query.bindValue(":startEpoch", startEpoch);
        else query.bindValue(":startDate", m_startDateFilter);
    }
    if (!m_endDateFilter.isEmpty()) {
        if (hasEndEpoch) query.bindValue(":endEpoch", endExclusiveEpoch);
        else query.bindValue(":endDate", m_endDateFilter);
    }
    if (!query.exec()) {
        qWarning() << "Failed to fetch log content:" << query.lastError().text();
        return content;
    }

    const QString timeFmt = QStringLiteral("hh:mm:ss");
    while (query.next()) {
        const qint64 start = query.value(0).toLongLong();
        const qint64 end = query.value(1).toLongLong();
        content.append(QDateTime::fromSecsSinceEpoch(start).toString(timeFmt));
        content.append(QLatin1Char(','));
        content.append(QDateTime::fromSecsSinceEpoch(end).toString(timeFmt));
        content.append(QLatin1Char(','));
        content.append(query.value(2).toString());
        content.append(QLatin1Char(','));
        content.append(query.value(3).toString());
        content.append(QLatin1Char(','));
        content.append(query.value(4).toString());
        content.append(QLatin1Char('\n'));
    }

    m_cachedLogContent = content;
    m_cachedContentUserId = userId;
    m_contentCacheValid = true;
    return content;
}

void WorkLogRepository::setLogFilter(const QString &startDate, const QString &endDate, int userId)
{
    m_startDateFilter = startDate;
    m_endDateFilter = endDate;
    invalidateCache();

    if (m_logModelUsed && m_logModel && ensureDatabaseOpen()) {
        qint64 startEpoch = 0;
        qint64 endExclusiveEpoch = 0;
        bool hasStartEpoch = false;
        bool hasEndEpoch = false;
        dateRangeToEpoch(m_startDateFilter, m_endDateFilter, startEpoch, hasStartEpoch, endExclusiveEpoch, hasEndEpoch);

        QString queryStr = "SELECT start_time, end_time, app_name, title, url FROM activity_logs WHERE user_id = :user_id ";
        if (!m_startDateFilter.isEmpty()) {
            queryStr += hasStartEpoch ? "AND start_time >= :startEpoch "
                                      : "AND date(start_time, 'unixepoch', 'localtime') >= :startDate ";
        }
        if (!m_endDateFilter.isEmpty()) {
            queryStr += hasEndEpoch ? "AND start_time < :endEpoch "
                                    : "AND date(start_time, 'unixepoch', 'localtime') <= :endDate ";
        }
        queryStr += "ORDER BY start_time DESC";

        QSqlQuery query(DatabaseManager::instance().database());
        query.prepare(queryStr);
        query.bindValue(":user_id", userId);
        if (!m_startDateFilter.isEmpty()) {
            if (hasStartEpoch) query.bindValue(":startEpoch", startEpoch);
            else query.bindValue(":startDate", m_startDateFilter);
        }
        if (!m_endDateFilter.isEmpty()) {
            if (hasEndEpoch) query.bindValue(":endEpoch", endExclusiveEpoch);
            else query.bindValue(":endDate", m_endDateFilter);
        }

        if (query.exec()) {
            m_logModel->setQuery(std::move(query));
        }
    }

    emit logsChanged();
}

void WorkLogRepository::clearLogFilter(int userId)
{
    setLogFilter("", "", userId);
}

void WorkLogRepository::showLogs(int userId)
{
    Q_UNUSED(userId);
    invalidateCache();
    emit logsChanged();
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
