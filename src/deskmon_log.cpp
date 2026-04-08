// deskmon_log.cpp
// Menangani semua logika pencatatan log aktivitas, filter log, dan laporan:
//   - Hitung dan ambil log aktivitas dari database
//   - Filter log berdasarkan tanggal
//   - Format durasi & debug data
#include "deskmon.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QDate>
#include <QDebug>

// ─────────────────────────────────────────────────
//  Log Model & Properties
// ─────────────────────────────────────────────────

QSqlQueryModel* Deskmon::logModel() const
{
    return m_logModel;
}

int Deskmon::logCount() const
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

    QSqlQuery query(m_dataManager->activityDb());
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

QString Deskmon::logContent() const
{
    return m_dataManager->getLogAsCsv(m_currentUserId, m_startDateFilter, m_endDateFilter);
}

void Deskmon::showLogs()
{
    emit logContentChanged();
}

// ─────────────────────────────────────────────────
//  Filter Tanggal
// ─────────────────────────────────────────────────

void Deskmon::setLogFilter(const QString &startDate, const QString &endDate)
{
    qDebug() << "Setting log filter - Start:" << startDate << "End:" << endDate;
    m_startDateFilter = startDate;
    m_endDateFilter = endDate;

    if (!m_logModel || !ensureDatabaseOpen()) {
        qWarning() << "Log model or database is not available for filtering.";
        return;
    }

    QString queryStr = "SELECT start_time, end_time, app_name, title, url FROM log "
                       "WHERE id_user = :id_user ";

    if (!m_startDateFilter.isEmpty())
        queryStr += "AND date(start_time, 'unixepoch', 'localtime') >= :startDate ";
    if (!m_endDateFilter.isEmpty())
        queryStr += "AND date(start_time, 'unixepoch', 'localtime') <= :endDate ";

    queryStr += "ORDER BY start_time DESC";

    QSqlQuery query(m_dataManager->activityDb());
    query.prepare(queryStr);
    query.bindValue(":id_user", m_currentUserId);

    if (!m_startDateFilter.isEmpty())
        query.bindValue(":startDate", m_startDateFilter);
    if (!m_endDateFilter.isEmpty())
        query.bindValue(":endDate", m_endDateFilter);

    if (m_logModel->lastError().isValid())
        qWarning() << "Failed to update log model query:" << m_logModel->lastError();

    emit logCountChanged();
    emit productivityStatsChanged();
    emit logContentChanged();
}

void Deskmon::clearLogFilter()
{
    m_startDateFilter = "";
    m_endDateFilter = "";
    emit logContentChanged();
    emit logCountChanged();
    emit productivityStatsChanged();
}

// ─────────────────────────────────────────────────
//  Debug & Format Helpers
// ─────────────────────────────────────────────────

QString Deskmon::formatDuration(int seconds) const
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

QString Deskmon::debugShowRawData() const
{
    if (!ensureDatabaseOpen()) {
        qWarning() << "Cannot fetch raw data: Database is not open";
        return "";
    }

    QString result;
    QSqlQuery query(m_dataManager->activityDb());
    query.prepare("SELECT start_time, datetime(start_time, 'unixepoch', 'localtime') as start_date, "
                  "app_name, title FROM log ORDER BY start_time DESC LIMIT 10");
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
