#include "TaskRepository.h"
#include "DatabaseManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QDate>
#include <QDebug>
#include <cmath>

TaskRepository::TaskRepository(QObject *parent)
    : QObject(parent)
{
}

QVariantList TaskRepository::getTasksForUser(int userId) const
{
    QVariantList tasks;
    if (!DatabaseManager::instance().ensureOpen() || userId == -1) return tasks;

    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("SELECT id, project_name, task_desc, max_time, time_usage, active, status, created_at "
                  "FROM tasks WHERE user_id = :user_id");
    query.bindValue(":user_id", userId);

    if (!query.exec()) {
        qWarning() << "Failed to fetch tasks:" << query.lastError().text();
        return tasks;
    }

    QDate today = QDate::currentDate();

    while (query.next()) {
        QVariantMap task;
        int taskId = query.value(0).toInt();
        task["id"] = taskId;
        task["project_name"] = query.value(1).toString();
        task["task"] = query.value(2).toString();
        int maxTime = query.value(3).toInt();
        task["max_time"] = maxTime;
        task["time_usage"] = query.value(4).toInt();
        task["active"] = query.value(5).toBool();
        task["status"] = query.value(6).toString();
        QString createdAtStr = query.value(7).toString();

        bool isExpired = false;
        QDateTime createdDateTime = QDateTime::fromString(createdAtStr, Qt::ISODate);

        if (createdDateTime.isValid()) {
            QDate createdDate = createdDateTime.date();
            if (createdDate.year() < today.year() ||
                (createdDate.year() == today.year() && createdDate.month() < today.month())) {
                double workHoursPerDay = 8.0 * 3600.0;
                int daysDuration = std::ceil((double)maxTime / workHoursPerDay);
                QDate estimatedFinishDate = createdDate.addDays(daysDuration);

                if (estimatedFinishDate.year() < today.year() ||
                    (estimatedFinishDate.year() == today.year() && estimatedFinishDate.month() < today.month())) {
                    isExpired = true;
                }
            }
        }
        task["isExpired"] = isExpired;
        tasks.append(task);
    }
    return tasks;
}

QMap<int, QPair<int, int>> TaskRepository::getExistingTasksMap(int userId) const
{
    QMap<int, QPair<int, int>> existingTasks;
    if (!DatabaseManager::instance().ensureOpen() || userId == -1) return existingTasks;

    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("SELECT id, max_time, time_usage FROM tasks WHERE user_id = :user_id");
    query.bindValue(":user_id", userId);

    if (query.exec()) {
        while (query.next()) {
            existingTasks.insert(query.value(0).toInt(),
                                 QPair<int, int>(query.value(1).toInt(), query.value(2).toInt()));
        }
    }
    return existingTasks;
}

bool TaskRepository::deleteTask(int taskId, int userId)
{
    if (!DatabaseManager::instance().ensureOpen()) return false;

    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("DELETE FROM tasks WHERE id = :id AND user_id = :user_id");
    query.bindValue(":id", taskId);
    query.bindValue(":user_id", userId);
    return query.exec();
}

bool TaskRepository::upsertTask(int taskId, int userId, const QString &projectName, const QString &taskDesc,
                                int maxTime, int timeUsage, const QString &createdAt, const QString &status)
{
    if (!DatabaseManager::instance().ensureOpen()) return false;

    QSqlQuery checkQuery(DatabaseManager::instance().database());
    checkQuery.prepare("SELECT 1 FROM tasks WHERE id = :id");
    checkQuery.bindValue(":id", taskId);
    bool exists = checkQuery.exec() && checkQuery.next();

    QSqlQuery query(DatabaseManager::instance().database());
    if (exists) {
        query.prepare("UPDATE tasks SET project_name = :projectName, task_desc = :taskDesc, "
                      "max_time = :maxTime, time_usage = :timeUsage, created_at = :createdAt "
                      "WHERE id = :id AND user_id = :user_id");
    } else {
        query.prepare("INSERT INTO tasks (id, user_id, project_name, task_desc, max_time, time_usage, active, status, paused, created_at) "
                      "VALUES (:id, :user_id, :projectName, :taskDesc, :maxTime, :timeUsage, 0, :status, 0, :createdAt)");
        query.bindValue(":status", status);
    }

    query.bindValue(":id", taskId);
    query.bindValue(":user_id", userId);
    query.bindValue(":projectName", projectName);
    query.bindValue(":taskDesc", taskDesc);
    query.bindValue(":maxTime", maxTime);
    query.bindValue(":timeUsage", timeUsage);
    query.bindValue(":createdAt", createdAt);

    return query.exec();
}

QList<int> TaskRepository::getTaskIdsForUser(int userId) const
{
    QList<int> ids;
    if (!DatabaseManager::instance().ensureOpen() || userId == -1) return ids;

    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("SELECT id FROM tasks WHERE user_id = :user_id");
    query.bindValue(":user_id", userId);

    if (query.exec()) {
        while (query.next()) {
            ids.append(query.value(0).toInt());
        }
    }
    return ids;
}

bool TaskRepository::findActiveTask(int userId, int &outTaskId, bool &outIsPaused, int &outTimeUsage)
{
    outTaskId = -1;
    outIsPaused = false;
    outTimeUsage = 0;

    if (!DatabaseManager::instance().ensureOpen() || userId == -1) return false;

    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("SELECT id, paused, time_usage, active FROM tasks WHERE user_id = :user_id");
    query.bindValue(":user_id", userId);

    if (!query.exec()) return false;

    bool found = false;
    while (query.next()) {
        int taskId = query.value(0).toInt();
        bool active = query.value(3).toBool();
        if (active) {
            outTaskId = taskId;
            outIsPaused = query.value(1).toBool();
            outTimeUsage = query.value(2).toInt();
            found = true;
            break;
        }
    }
    return found;
}

bool TaskRepository::resetActiveTaskStatus(int taskIdToKeep)
{
    if (!DatabaseManager::instance().ensureOpen()) return false;

    QSqlQuery query(DatabaseManager::instance().database());
    if (taskIdToKeep > 0) {
        query.prepare("UPDATE tasks SET active = 0, paused = 0 WHERE id != :id");
        query.bindValue(":id", taskIdToKeep);
    } else {
        query.prepare("UPDATE tasks SET active = 0, paused = 0");
    }
    return query.exec();
}

bool TaskRepository::updateTaskStatus(int taskId, const QString &status)
{
    if (!DatabaseManager::instance().ensureOpen()) return false;

    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("UPDATE tasks SET status = :status WHERE id = :id");
    query.bindValue(":status", status);
    query.bindValue(":id", taskId);
    return query.exec();
}

bool TaskRepository::updateTaskTiming(int taskId, int timeUsage, bool active, bool paused)
{
    if (!DatabaseManager::instance().ensureOpen()) return false;

    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("UPDATE tasks SET time_usage = :timeUsage, active = :active, paused = :paused WHERE id = :id");
    query.bindValue(":timeUsage", timeUsage);
    query.bindValue(":active", active);
    query.bindValue(":paused", paused);
    query.bindValue(":id", taskId);
    return query.exec();
}

bool TaskRepository::updateTaskMaxTime(int taskId, int maxTime)
{
    if (!DatabaseManager::instance().ensureOpen()) return false;

    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("UPDATE tasks SET max_time = :maxTime WHERE id = :id");
    query.bindValue(":maxTime", maxTime);
    query.bindValue(":id", taskId);
    return query.exec();
}

bool TaskRepository::logPauseEvent(int taskId, const QString &startTime)
{
    if (!DatabaseManager::instance().ensureOpen()) return false;

    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("UPDATE task_pause_logs SET resumed_at = :endTime WHERE task_id = :taskId AND status = 'play' AND resumed_at IS NULL");
    query.bindValue(":endTime", startTime);
    query.bindValue(":taskId", taskId);
    query.exec();

    query.prepare("INSERT INTO task_pause_logs (task_id, paused_at, resumed_at, status) VALUES (:taskId, :startTime, NULL, 'pause')");
    query.bindValue(":taskId", taskId);
    query.bindValue(":startTime", startTime);
    return query.exec();
}

bool TaskRepository::logPlayEvent(int taskId, const QString &startTime)
{
    if (!DatabaseManager::instance().ensureOpen()) return false;

    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("UPDATE task_pause_logs SET resumed_at = :endTime WHERE task_id = :taskId AND status = 'pause' AND resumed_at IS NULL");
    query.bindValue(":endTime", startTime);
    query.bindValue(":taskId", taskId);
    query.exec();

    query.prepare("INSERT INTO task_pause_logs (task_id, paused_at, resumed_at, status) VALUES (:taskId, :startTime, NULL, 'play')");
    query.bindValue(":taskId", taskId);
    query.bindValue(":startTime", startTime);
    return query.exec();
}

bool TaskRepository::logPausePlayData(int taskId, const QString &startTime, const QString &endTime, const QString &status)
{
    if (!DatabaseManager::instance().ensureOpen()) return false;

    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("INSERT INTO task_pause_logs (task_id, paused_at, resumed_at, status) VALUES (:taskId, :start, :end, :status)");
    query.bindValue(":taskId", taskId);
    query.bindValue(":start", startTime);
    query.bindValue(":end", endTime.isEmpty() ? QVariant() : endTime);
    query.bindValue(":status", status);
    return query.exec();
}

bool TaskRepository::archiveCompletedTask(int taskId, int userId)
{
    if (!DatabaseManager::instance().ensureOpen()) return false;

    QSqlQuery selectQuery(DatabaseManager::instance().database());
    selectQuery.prepare("SELECT project_name, task_desc, max_time, time_usage FROM tasks WHERE id = :id AND user_id = :user_id");
    selectQuery.bindValue(":id", taskId);
    selectQuery.bindValue(":user_id", userId);

    if (selectQuery.exec() && selectQuery.next()) {
        QString projectName = selectQuery.value(0).toString();
        QString taskDesc = selectQuery.value(1).toString();
        int maxTime = selectQuery.value(2).toInt();
        int timeUsage = selectQuery.value(3).toInt();
        qint64 completedTime = QDateTime::currentSecsSinceEpoch();

        QSqlQuery insertQuery(DatabaseManager::instance().database());
        insertQuery.prepare("INSERT INTO completed_tasks (user_id, project_name, task_desc, max_time, time_usage, completed_time) "
                            "VALUES (:user_id, :projectName, :taskDesc, :maxTime, :timeUsage, :completedTime)");
        insertQuery.bindValue(":user_id", userId);
        insertQuery.bindValue(":projectName", projectName);
        insertQuery.bindValue(":taskDesc", taskDesc);
        insertQuery.bindValue(":maxTime", maxTime);
        insertQuery.bindValue(":timeUsage", timeUsage);
        insertQuery.bindValue(":completedTime", completedTime);
        insertQuery.exec();

        QSqlQuery deleteQuery(DatabaseManager::instance().database());
        deleteQuery.prepare("DELETE FROM tasks WHERE id = :id AND user_id = :user_id");
        deleteQuery.bindValue(":id", taskId);
        deleteQuery.bindValue(":user_id", userId);
        return deleteQuery.exec();
    }
    return false;
}

int TaskRepository::getPendingStartedTaskCount(int userId) const
{
    if (!DatabaseManager::instance().ensureOpen() || userId == -1) return 0;

    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("SELECT COUNT(*) FROM tasks WHERE user_id = :user_id AND status IN ('pending', 'started')");
    query.bindValue(":user_id", userId);

    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

QString TaskRepository::getTaskName(int taskId) const
{
    if (!DatabaseManager::instance().ensureOpen() || taskId <= 0) return "";

    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("SELECT task_desc FROM tasks WHERE id = :id");
    query.bindValue(":id", taskId);

    if (query.exec() && query.next()) {
        return query.value(0).toString();
    }
    return "";
}
