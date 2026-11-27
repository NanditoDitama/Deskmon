#include "TaskRepository.h"
#include "DatabaseManager.h"
#include <QHash>
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

QString TaskRepository::mapServerStatus(const QString &apiStatus)
{
    const QString s = apiStatus.trimmed().toLower();
    if (s == QStringLiteral("on-progress")) {
        return QStringLiteral("on-progress");
    }
    if (s == QStringLiteral("on-review") || s == QStringLiteral("review")) {
        return QStringLiteral("Review");
    }
    if (s == QStringLiteral("need-review") || s == QStringLiteral("need review")) {
        return QStringLiteral("Need Review");
    }
    if (s == QStringLiteral("need-revise") || s == QStringLiteral("need revise")) {
        return QStringLiteral("Need Revise");
    }
    return QStringLiteral("Pending");
}

bool TaskRepository::syncTasksToDatabase(QSqlDatabase &db,
                                         int userId,
                                         const QVector<SyncedTaskRow> &tasks,
                                         const QSet<int> &serverTaskIds,
                                         int currentActiveTaskId,
                                         bool &outActiveTaskRemoved)
{
    outActiveTaskRemoved = false;
    if (!db.isOpen() || userId == -1) return false;

    const bool inTx = db.transaction();
    if (!inTx) {
        qWarning() << "Failed to start transaction in syncTasksToDatabase";
        return false;
    }

    QHash<int, QPair<int, int>> existingTasks;
    {
        QSqlQuery loadQ(db);
        loadQ.prepare(QStringLiteral("SELECT id, max_time, time_usage FROM tasks WHERE user_id = :user_id"));
        loadQ.bindValue(":user_id", userId);
        if (!loadQ.exec()) {
            qWarning() << "Failed to load existing tasks:" << loadQ.lastError().text();
            db.rollback();
            return false;
        }
        while (loadQ.next()) {
            existingTasks.insert(loadQ.value(0).toInt(),
                                 qMakePair(loadQ.value(1).toInt(), loadQ.value(2).toInt()));
        }
    }

    QSqlQuery deleteQ(db);
    deleteQ.prepare(QStringLiteral("DELETE FROM tasks WHERE id = :id AND user_id = :user_id"));

    for (auto it = existingTasks.constBegin(); it != existingTasks.constEnd(); ++it) {
        const int localTaskId = it.key();
        if (!serverTaskIds.contains(localTaskId)) {
            if (localTaskId == currentActiveTaskId && currentActiveTaskId != -1) {
                outActiveTaskRemoved = true;
            }
            deleteQ.bindValue(":id", localTaskId);
            deleteQ.bindValue(":user_id", userId);
            if (!deleteQ.exec()) {
                qWarning() << "Failed to delete stale task:" << deleteQ.lastError().text();
                db.rollback();
                return false;
            }
        }
    }

    QSqlQuery updateQ(db);
    updateQ.prepare(QStringLiteral("UPDATE tasks SET project_name = :projectName, task_desc = :taskDesc, "
                                   "max_time = :maxTime, time_usage = :timeUsage, created_at = :createdAt, "
                                   "status = :status "
                                   "WHERE id = :id AND user_id = :user_id"));

    QSqlQuery updateRestrictedQ(db);
    updateRestrictedQ.prepare(QStringLiteral("UPDATE tasks SET project_name = :projectName, task_desc = :taskDesc, "
                                             "max_time = :maxTime, time_usage = :timeUsage, created_at = :createdAt, "
                                             "status = :status, active = 0, paused = 0 "
                                             "WHERE id = :id AND user_id = :user_id"));

    QSqlQuery insertQ(db);
    insertQ.prepare(QStringLiteral("INSERT INTO tasks (id, user_id, project_name, task_desc, max_time, time_usage, active, status, paused, created_at) "
                                   "VALUES (:id, :user_id, :projectName, :taskDesc, :maxTime, :timeUsage, 0, :status, 0, :createdAt)"));

    for (const SyncedTaskRow &pt : tasks) {
        const QString finalStatus = pt.status.isEmpty() ? QStringLiteral("Pending") : pt.status;
        const bool isRestricted = (finalStatus == QStringLiteral("Review") ||
                                   finalStatus == QStringLiteral("Need Review") ||
                                   finalStatus == QStringLiteral("Need Revise"));

        if (existingTasks.contains(pt.id)) {
            const int currentMaxTime = existingTasks.value(pt.id).first;
            const int currentTimeUsage = existingTasks.value(pt.id).second;
            const int finalMaxTime = pt.hasDuration ? pt.serverMaxTime : currentMaxTime;
            int finalTimeUsage = pt.hasTotalDuration ? pt.serverTimeUsage : currentTimeUsage;
            if (pt.id == currentActiveTaskId && currentTimeUsage > finalTimeUsage) {
                finalTimeUsage = currentTimeUsage;
            }
            if (isRestricted && pt.id == currentActiveTaskId && currentActiveTaskId != -1) {
                outActiveTaskRemoved = true;
            }

            QSqlQuery &q = isRestricted ? updateRestrictedQ : updateQ;
            q.bindValue(":projectName", pt.projectName);
            q.bindValue(":taskDesc", pt.taskDesc);
            q.bindValue(":maxTime", finalMaxTime);
            q.bindValue(":timeUsage", finalTimeUsage);
            q.bindValue(":createdAt", pt.createdAt);
            q.bindValue(":status", finalStatus);
            q.bindValue(":id", pt.id);
            q.bindValue(":user_id", userId);
            if (!q.exec()) {
                qWarning() << "Failed to update task in batch sync:" << q.lastError().text();
                db.rollback();
                return false;
            }
        } else {
            insertQ.bindValue(":id", pt.id);
            insertQ.bindValue(":user_id", userId);
            insertQ.bindValue(":projectName", pt.projectName);
            insertQ.bindValue(":taskDesc", pt.taskDesc);
            insertQ.bindValue(":maxTime", pt.serverMaxTime);
            insertQ.bindValue(":timeUsage", pt.serverTimeUsage);
            insertQ.bindValue(":status", finalStatus);
            insertQ.bindValue(":createdAt", pt.createdAt);
            if (!insertQ.exec()) {
                qWarning() << "Failed to insert task in batch sync:" << insertQ.lastError().text();
                db.rollback();
                return false;
            }
        }
    }

    if (!db.commit()) {
        qWarning() << "Failed to commit syncTasksToDatabase transaction:" << db.lastError().text();
        db.rollback();
        return false;
    }

    return true;
}

bool TaskRepository::computeIsExpired(const QString &createdAtStr, int maxTimeSeconds, const QDate &today)
{
    QDateTime createdDateTime = QDateTime::fromString(createdAtStr, Qt::ISODate);
    if (!createdDateTime.isValid()) {
        return false;
    }

    QDate createdDate = createdDateTime.date();
    if (createdDate.year() < today.year() ||
        (createdDate.year() == today.year() && createdDate.month() < today.month())) {
        double workHoursPerDay = 8.0 * 3600.0;
        int daysDuration = static_cast<int>(std::ceil(static_cast<double>(maxTimeSeconds) / workHoursPerDay));
        QDate estimatedFinishDate = createdDate.addDays(daysDuration);

        if (estimatedFinishDate.year() < today.year() ||
            (estimatedFinishDate.year() == today.year() && estimatedFinishDate.month() < today.month())) {
            return true;
        }
    }
    return false;
}

bool TaskRepository::isTaskExpired(int taskId) const
{
    if (!DatabaseManager::instance().ensureOpen() || taskId <= 0) return false;

    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("SELECT created_at, max_time FROM tasks WHERE id = :id");
    query.bindValue(":id", taskId);
    if (query.exec() && query.next()) {
        return computeIsExpired(query.value(0).toString(), query.value(1).toInt());
    }
    return false;
}

QVariantList TaskRepository::getTasksForUser(int userId, bool isTaskPaused) const
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
        bool active = query.value(5).toBool();
        task["active"] = active;
        QString rawStatus = query.value(6).toString();
        QString createdAtStr = query.value(7).toString();

        if (rawStatus.toLower() == QStringLiteral("review")) {
            task["status"] = QStringLiteral("Review");
        } else if (active) {
            task["status"] = isTaskPaused ? QStringLiteral("Paused") : QStringLiteral("Is Running");
        } else if (rawStatus.toLower() == QStringLiteral("pending")) {
            task["status"] = QStringLiteral("Pending");
        } else {
            task["status"] = rawStatus;
        }

        task["isExpired"] = computeIsExpired(createdAtStr, maxTime, today);
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

bool TaskRepository::findStartupTask(int userId, int &outTaskId, bool &outIsPaused, int &outTimeUsage, QString &outStatus) const
{
    outTaskId = -1;
    outIsPaused = false;
    outTimeUsage = 0;
    outStatus.clear();

    if (!DatabaseManager::instance().ensureOpen() || userId == -1) return false;

    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("SELECT id, paused, time_usage, status FROM tasks "
                  "WHERE user_id = :user_id AND (active = 1 OR LOWER(status) = 'on-progress') "
                  "ORDER BY (LOWER(status) = 'on-progress') DESC LIMIT 1");
    query.bindValue(":user_id", userId);

    if (query.exec() && query.next()) {
        outTaskId = query.value(0).toInt();
        outIsPaused = query.value(1).toBool();
        outTimeUsage = query.value(2).toInt();
        outStatus = query.value(3).toString();
        return true;
    }
    return false;
}

bool TaskRepository::getTaskDetails(int taskId, int &outMaxTime, int &outTimeUsage, QString &outStatus) const
{
    outMaxTime = 0;
    outTimeUsage = 0;
    outStatus.clear();

    if (!DatabaseManager::instance().ensureOpen() || taskId <= 0) return false;

    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("SELECT max_time, time_usage, status FROM tasks WHERE id = :id");
    query.bindValue(":id", taskId);

    if (query.exec() && query.next()) {
        outMaxTime = query.value(0).toInt();
        outTimeUsage = query.value(1).toInt();
        outStatus = query.value(2).toString();
        return true;
    }
    return false;
}

int TaskRepository::findNextEligibleTaskId(int userId, int excludedTaskId) const
{
    if (!DatabaseManager::instance().ensureOpen() || userId == -1) return -1;

    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("SELECT id FROM tasks "
                  "WHERE user_id = :user_id AND id != :excluded AND status NOT IN ('Review', 'Need Review', 'Need Revise') "
                  "LIMIT 1");
    query.bindValue(":user_id", userId);
    query.bindValue(":excluded", excludedTaskId);

    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return -1;
}

bool TaskRepository::deactivateTask(int taskId, const QString &status, int timeUsage)
{
    if (!DatabaseManager::instance().ensureOpen() || taskId <= 0) return false;

    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("UPDATE tasks SET active = 0, status = :status, time_usage = :timeUsage, paused = 0 WHERE id = :id");
    query.bindValue(":status", status);
    query.bindValue(":timeUsage", timeUsage);
    query.bindValue(":id", taskId);
    return query.exec();
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

bool TaskRepository::archiveCompletedTask(int taskId, int userId, int overrideTimeUsage)
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
        int timeUsage = (overrideTimeUsage >= 0) ? overrideTimeUsage : selectQuery.value(3).toInt();
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
    query.prepare("SELECT COUNT(*) FROM tasks WHERE user_id = :user_id AND LOWER(status) = 'pending' AND time_usage > 0");
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
