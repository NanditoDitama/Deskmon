#include "WorkTimeRepository.h"
#include "DatabaseManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

WorkTimeRepository::WorkTimeRepository(QObject *parent)
    : QObject(parent)
{
}

int WorkTimeRepository::getElapsedSeconds(int userId, const QString &date) const
{
    if (!DatabaseManager::instance().ensureOpen() || userId == -1) return 0;

    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("SELECT elapsed_seconds FROM work_time_records WHERE user_id = :user_id AND work_date = :date");
    query.bindValue(":user_id", userId);
    query.bindValue(":date", date);

    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

bool WorkTimeRepository::saveElapsedSeconds(int userId, const QString &date, int seconds)
{
    if (!DatabaseManager::instance().ensureOpen() || userId == -1) return false;

    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("INSERT OR REPLACE INTO work_time_records (user_id, work_date, elapsed_seconds) "
                  "VALUES (:user_id, :date, :seconds)");
    query.bindValue(":user_id", userId);
    query.bindValue(":date", date);
    query.bindValue(":seconds", seconds);

    if (!query.exec()) {
        qWarning() << "Failed to save work time record:" << query.lastError().text();
        return false;
    }
    return true;
}

bool WorkTimeRepository::hasRecordForDate(int userId, const QString &date) const
{
    if (!DatabaseManager::instance().ensureOpen() || userId == -1) return false;

    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("SELECT 1 FROM work_time_records WHERE user_id = :user_id AND work_date = :date LIMIT 1");
    query.bindValue(":user_id", userId);
    query.bindValue(":date", date);

    return query.exec() && query.next();
}
