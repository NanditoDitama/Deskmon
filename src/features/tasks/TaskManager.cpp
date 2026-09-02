#include "TaskManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QBuffer>
#include <QSet>
#include <cmath>
#include <QDebug>

TaskManager::TaskManager(ApiClient *apiClient,
                         ProductivityAppRepository *prodRepo,
                         AuthManager *authManager,
                         QObject *parent)
    : QObject(parent)
    , m_apiClient(apiClient)
    , m_prodRepo(prodRepo)
    , m_authManager(authManager)
{
    m_pingTimer.setInterval(30000);
    connect(&m_pingTimer, &QTimer::timeout, this, [this]() {
        if (m_authManager->currentUserId() != -1) {
            if (m_activeTaskId != -1 && !m_isTaskPaused) {
                sendPing(m_activeTaskId);
            } else {
                sendPing(-1);
            }
        }
    });

    m_taskRefreshTimer.setInterval(180000);
    connect(&m_taskRefreshTimer, &QTimer::timeout, this, &TaskManager::refreshTasks);
    m_taskRefreshTimer.start();
}

void TaskManager::startGlobalTimer()
{
    m_globalTimeUsage = 0;
    emit globalTimeUsageChanged();
}

void TaskManager::startPingTimer()
{
    sendPing(m_activeTaskId);
    m_pingTimer.start();
}

void TaskManager::stopPingTimer()
{
    m_pingTimer.stop();
}

QVariantList TaskManager::taskList() const
{
    if (!m_prodRepo->ensureDatabaseOpen()) return QVariantList();
    int userId = m_authManager->currentUserId();
    if (userId == -1) return QVariantList();

    QVariantList tasks;
    QSqlQuery query(m_prodRepo->database());
    query.prepare("SELECT id, project_name, task, max_time, time_usage, active, status, created_at FROM task WHERE user_id = :user_id");
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
            if (createdDate.year() < today.year() || (createdDate.year() == today.year() && createdDate.month() < today.month())) {
                double workHoursPerDay = 8.0 * 3600.0;
                int daysDuration = std::ceil((double)maxTime / workHoursPerDay);
                QDate estimatedFinishDate = createdDate.addDays(daysDuration);

                if (estimatedFinishDate.year() < today.year() || (estimatedFinishDate.year() == today.year() && estimatedFinishDate.month() < today.month())) {
                    isExpired = true;
                }
            }
        }
        task["isExpired"] = isExpired;
        tasks.append(task);
    }
    return tasks;
}

void TaskManager::fetchAndStoreTasks()
{
    if (!m_prodRepo->ensureDatabaseOpen()) return;
    int userId = m_authManager->currentUserId();
    if (userId == -1) return;

    QString token = m_authManager->authToken();
    if (token.isEmpty()) return;

    QUrl apiUrl(QString("https://deskmon.pranala-dt.co.id/api/task-by-user/%1").arg(userId));
    QNetworkReply *reply = m_apiClient->get(apiUrl, true);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        handleTaskFetchReply(reply);
    });
}

void TaskManager::handleTaskFetchReply(QNetworkReply *reply)
{
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Failed to fetch tasks: Network error:" << reply->errorString();
        reply->deleteLater();
        return;
    }

    if (statusCode == 401) {
        m_authManager->showAuthTokenErrorMessage();
        reply->deleteLater();
        return;
    }

    QByteArray responseData = reply->readAll();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
    if (jsonDoc.isNull() || !jsonDoc.isObject()) {
        reply->deleteLater();
        return;
    }

    QJsonObject jsonObj = jsonDoc.object();
    if (!jsonObj["success"].toBool()) {
        reply->deleteLater();
        return;
    }

    QJsonArray tasksArray = jsonObj["data"].toArray();
    QSet<int> serverTaskIds;
    for (const QJsonValue &taskValue : tasksArray) {
        QJsonObject taskObj = taskValue.toObject();
        if (taskObj.contains("id")) {
            serverTaskIds.insert(taskObj["id"].toInt());
        }
    }

    int userId = m_authManager->currentUserId();
    QSqlQuery query(m_prodRepo->database());
    m_prodRepo->database().transaction();

    QMap<int, QPair<int, int>> existingTasks;
    query.prepare("SELECT id, max_time, time_usage FROM task WHERE user_id = :user_id");
    query.bindValue(":user_id", userId);
    if (query.exec()) {
        while (query.next()) {
            existingTasks.insert(query.value(0).toInt(), QPair<int, int>(query.value(1).toInt(), query.value(2).toInt()));
        }
    }

    for (int localTaskId : existingTasks.keys()) {
        if (!serverTaskIds.contains(localTaskId)) {
            if (localTaskId == m_activeTaskId) {
                m_activeTaskId = -1;
                m_isTaskPaused = false;
                m_taskTimeOffset = 0;
                m_taskStartTime = 0;
                emit activeTaskChanged();
                emit taskPausedChanged();
            }
            QSqlQuery deleteQuery(m_prodRepo->database());
            deleteQuery.prepare("DELETE FROM task WHERE id = :id AND user_id = :user_id");
            deleteQuery.bindValue(":id", localTaskId);
            deleteQuery.bindValue(":user_id", userId);
            deleteQuery.exec();
        }
    }

    for (const QJsonValue &taskValue : tasksArray) {
        QJsonObject taskObj = taskValue.toObject();
        if (taskObj["status"].toString() == "completed") continue;
        if (!taskObj.contains("id") || !taskObj.contains("title") || !taskObj.contains("description") || !taskObj.contains("user_id")) {
            continue;
        }

        int taskId = taskObj["id"].toInt();
        QString projectName = taskObj["title"].toString();
        QString taskDesc = taskObj["description"].toString();
        int tUserId = taskObj["user_id"].toInt();
        if (tUserId != userId) continue;

        QJsonValue durationValue = taskObj["duration"];
        QJsonValue totalDurationValue = taskObj["total_duration"];

        int serverMaxTime = !durationValue.isNull() ? qRound(durationValue.toVariant().toDouble() * 3600) : 0;
        int serverTimeUsage = !totalDurationValue.isNull() ? qRound(totalDurationValue.toVariant().toDouble() * 3600) : 0;
        QString createdAt = taskObj["created_at"].toString();

        if (existingTasks.contains(taskId)) {
            int currentMaxTime = existingTasks[taskId].first;
            int currentTimeUsage = existingTasks[taskId].second;
            int finalMaxTime = !durationValue.isNull() ? serverMaxTime : currentMaxTime;
            int finalTimeUsage = !totalDurationValue.isNull() ? serverTimeUsage : currentTimeUsage;

            query.prepare("UPDATE task SET project_name = :projectName, task = :taskDesc, "
                          "max_time = :maxTime, time_usage = :timeUsage, created_at = :createdAt WHERE id = :id");
            query.bindValue(":id", taskId);
            query.bindValue(":projectName", projectName);
            query.bindValue(":taskDesc", taskDesc);
            query.bindValue(":maxTime", finalMaxTime);
            query.bindValue(":timeUsage", finalTimeUsage);
            query.bindValue(":createdAt", createdAt);
        } else {
            query.prepare("INSERT INTO task (id, project_name, task, max_time, time_usage, active, status, paused, user_id, created_at) "
                          "VALUES (:id, :projectName, :taskDesc, :maxTime, :timeUsage, 0, 'Pending', 0, :userId, :createdAt)");
            query.bindValue(":id", taskId);
            query.bindValue(":projectName", projectName);
            query.bindValue(":taskDesc", taskDesc);
            query.bindValue(":maxTime", serverMaxTime);
            query.bindValue(":timeUsage", serverTimeUsage);
            query.bindValue(":userId", userId);
            query.bindValue(":createdAt", createdAt);
        }
        query.exec();
    }

    m_prodRepo->database().commit();
    emit taskListChanged();
    reply->deleteLater();
}

void TaskManager::refreshTasks()
{
    int userId = m_authManager->currentUserId();
    if (userId == -1 || !m_prodRepo->ensureDatabaseOpen()) return;

    fetchAndStoreTasks();

    QSqlQuery query(m_prodRepo->database());
    query.prepare("SELECT id FROM task WHERE user_id = :user_id");
    query.bindValue(":user_id", userId);
    if (query.exec()) {
        while (query.next()) {
            updateTaskStatus(query.value(0).toInt());
        }
    }

    syncActiveTask();
    emit taskListChanged();
}

void TaskManager::syncActiveTask()
{
    int userId = m_authManager->currentUserId();
    if (userId == -1 || !m_prodRepo->ensureDatabaseOpen()) return;

    QSqlQuery query(m_prodRepo->database());
    query.prepare("SELECT id, paused, time_usage, active FROM task WHERE user_id = :user_id");
    query.bindValue(":user_id", userId);
    if (!query.exec()) return;

    QList<int> taskIds;
    bool hasActiveTask = false;
    while (query.next()) {
        int taskId = query.value(0).toInt();
        if (taskId <= 0) continue;
        taskIds.append(taskId);

        if (query.value(3).toBool()) {
            if (hasActiveTask) {
                QSqlQuery resetQuery(m_prodRepo->database());
                resetQuery.prepare("UPDATE task SET active = 0, paused = 0 WHERE id = :id");
                resetQuery.bindValue(":id", m_activeTaskId);
                resetQuery.exec();
            }
            m_activeTaskId = taskId;
            m_isTaskPaused = query.value(1).toBool();
            m_taskTimeOffset = query.value(2).toInt();
            m_taskStartTime = QDateTime::currentSecsSinceEpoch();
            hasActiveTask = true;
        }
    }

    if (!hasActiveTask) {
        m_activeTaskId = -1;
        m_isTaskPaused = false;
        m_taskTimeOffset = 0;
        m_taskStartTime = 0;
    }

    fetchAndStoreTasks();
    for (int taskId : taskIds) {
        updateTaskStatus(taskId);
    }

    emit activeTaskChanged();
    emit taskPausedChanged();
    emit taskListChanged();
}

void TaskManager::checkTaskStatusBeforeStart()
{
    int userId = m_authManager->currentUserId();
    if (userId == -1 || !m_prodRepo->ensureDatabaseOpen()) return;

    QSqlQuery query(m_prodRepo->database());
    query.prepare("SELECT id, paused, time_usage, status FROM task WHERE user_id = :user_id AND (active = 1 OR status = 'on-progress') ORDER BY status = 'on-progress' DESC LIMIT 1");
    query.bindValue(":user_id", userId);

    if (query.exec() && query.next()) {
        m_activeTaskId = query.value(0).toInt();
        m_isTaskPaused = query.value(1).toBool();
        QString status = query.value(3).toString();

        if (status == "on-progress") {
            m_isTaskPaused = false;
            QSqlQuery updateQuery(m_prodRepo->database());
            updateQuery.prepare("UPDATE task SET paused = 0 WHERE id = :id");
            updateQuery.bindValue(":id", m_activeTaskId);
            updateQuery.exec();
        }

        if (!m_isTaskPaused) {
            m_taskStartTime = QDateTime::currentSecsSinceEpoch();
            QSqlQuery timeQuery(m_prodRepo->database());
            timeQuery.prepare("SELECT time_usage FROM task WHERE id = :id");
            timeQuery.addBindValue(m_activeTaskId);
            if (timeQuery.exec() && timeQuery.next()) {
                m_taskTimeOffset = timeQuery.value(0).toInt();
            }
        }
    } else {
        m_activeTaskId = -1;
        m_isTaskPaused = false;
    }

    emit activeTaskChanged();
    emit taskPausedChanged();
    emit trackingActiveChanged();
    emit taskListChanged();
}

void TaskManager::setActiveTask(int taskId)
{
    if (m_activeTaskId != -1 && m_activeTaskId != taskId && !m_isTaskPaused) {
        emit requestTaskDetails(m_activeTaskId, "switch", taskId);
    }

    if (taskId == m_activeTaskId && !m_isTaskPaused) {
        return;
    }

    if (!m_prodRepo->ensureDatabaseOpen()) return;
    QSqlQuery query(m_prodRepo->database());

    if (m_activeTaskId != -1) {
        query.prepare("SELECT status, task FROM task WHERE id = :id");
        query.bindValue(":id", m_activeTaskId);
        if (query.exec() && query.next()) {
            QString prevStatus = query.value(0).toString().toLower();
            QStringList restrictedStatuses = {"Review", "Need Review", "Need Revise", "completed"};
            QString newStatus = restrictedStatuses.contains(prevStatus) ? prevStatus : "Pending";

            qint64 currentEpoch = QDateTime::currentSecsSinceEpoch();
            qint64 timeUsed = m_taskTimeOffset + (m_isTaskPaused ? 0 : (currentEpoch - m_taskStartTime));

            query.prepare("UPDATE task SET active = 0, status = :status, time_usage = :timeUsage, paused = 0 WHERE id = :id");
            query.bindValue(":status", newStatus);
            query.bindValue(":timeUsage", timeUsed);
            query.bindValue(":id", m_activeTaskId);
            query.exec();
        }
        if (!m_isTaskPaused) {
            toggleTaskPause();
        }
    }

    if (taskId != -1) {
        query.prepare("SELECT time_usage FROM task WHERE id = :id");
        query.bindValue(":id", taskId);
        if (query.exec() && query.next()) {
            m_taskTimeOffset = query.value(0).toInt();
            m_taskStartTime = QDateTime::currentSecsSinceEpoch();
        }

        query.prepare("UPDATE task SET active = 1, status = 'Paused', paused = 1 WHERE id = :id");
        query.bindValue(":id", taskId);
        query.exec();

        setMaxTimeForTask(taskId);

        QTimer::singleShot(2000, this, [this, taskId]() {
            if (m_activeTaskId == taskId) {
                QSqlQuery resumeQuery(m_prodRepo->database());
                resumeQuery.prepare("UPDATE task SET paused = 0, status = 'on-progress' WHERE id = :id");
                resumeQuery.bindValue(":id", taskId);
                resumeQuery.exec();

                m_isTaskPaused = false;
                m_taskStartTime = QDateTime::currentSecsSinceEpoch();
                m_pauseStartTime = 0;

                QString newTime = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
                QSqlQuery logQuery(m_prodRepo->database());
                logQuery.prepare("UPDATE log_paused SET end_reality = ? WHERE task_id = ? AND current_status = 'pause' AND end_reality IS NULL");
                logQuery.addBindValue(newTime);
                logQuery.addBindValue(taskId);
                logQuery.exec();

                logQuery.prepare("INSERT INTO log_paused (task_id, start_reality, end_reality, current_status) VALUES (?, ?, NULL, 'play')");
                logQuery.addBindValue(taskId);
                logQuery.addBindValue(newTime);
                logQuery.exec();
                sendPing(m_activeTaskId);

                emit taskPausedChanged();
                emit trackingActiveChanged();
                emit taskListChanged();
            }
        });
    }

    m_activeTaskId = taskId;
    m_isTaskPaused = true;
    m_pauseStartTime = QDateTime::currentSecsSinceEpoch();

    emit taskPausedChanged();
    emit trackingActiveChanged();
    emit activeTaskChanged();
    emit taskListChanged();
}

void TaskManager::setMaxTimeForTask(int taskId)
{
    if (!m_prodRepo->ensureDatabaseOpen()) return;
    QSqlQuery query(m_prodRepo->database());
    query.prepare("SELECT max_time FROM task WHERE id = :id");
    query.bindValue(":id", taskId);
    if (query.exec() && query.next()) {
        if (query.value(0).toInt() == 0) {
            query.prepare("UPDATE task SET max_time = :maxTime WHERE id = :id");
            query.bindValue(":maxTime", 8 * 3600);
            query.bindValue(":id", taskId);
            query.exec();
        }
    }
}

void TaskManager::finishTask(int taskId)
{
    int userId = m_authManager->currentUserId();
    if (!m_prodRepo->ensureDatabaseOpen() || userId == -1) return;

    QSqlQuery query(m_prodRepo->database());
    query.prepare("SELECT user_id, project_name, task, max_time, time_usage FROM task WHERE id = :id");
    query.bindValue(":id", taskId);
    if (!query.exec() || !query.next() || query.value(0).toInt() != userId) return;

    QString projectName = query.value(1).toString();
    QString taskDesc = query.value(2).toString();
    int maxTime = query.value(3).toInt();
    int timeUsage = (m_activeTaskId == taskId && !m_isTaskPaused)
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
    query.bindValue(":user_id", userId);
    query.exec();

    query.prepare("DELETE FROM task WHERE id = :id");
    query.bindValue(":id", taskId);
    query.exec();

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

void TaskManager::toggleTaskPause()
{
    if (!m_prodRepo->ensureDatabaseOpen() || m_activeTaskId == -1) return;

    QString currentTime = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
    QSqlQuery query(m_prodRepo->database());
    m_prodRepo->database().transaction();

    try {
        if (!m_isTaskPaused) {
            qint64 currentEpoch = QDateTime::currentSecsSinceEpoch();
            qint64 timeUsed = m_taskTimeOffset + (currentEpoch - m_taskStartTime);

            query.prepare("UPDATE task SET time_usage = ?, paused = 1, status = 'Paused' WHERE id = ?");
            query.addBindValue(timeUsed);
            query.addBindValue(m_activeTaskId);
            query.exec();

            query.prepare("UPDATE log_paused SET end_reality = ? WHERE task_id = ? AND current_status = 'play' AND end_reality IS NULL");
            query.addBindValue(currentTime);
            query.addBindValue(m_activeTaskId);
            query.exec();

            query.prepare("INSERT INTO log_paused (task_id, start_reality, end_reality, current_status) VALUES (?, ?, NULL, 'pause')");
            query.addBindValue(m_activeTaskId);
            query.addBindValue(currentTime);
            query.exec();

            sendPausePlayDataToAPI(m_activeTaskId, m_lastPlayStartTime.toString(Qt::ISODateWithMs), currentTime, "pause");

            m_isTaskPaused = true;
            m_taskTimeOffset = timeUsed;
            m_pauseStartTime = currentEpoch;
        } else {
            query.prepare("UPDATE log_paused SET end_reality = ? WHERE task_id = ? AND current_status = 'pause' AND end_reality IS NULL");
            query.addBindValue(currentTime);
            query.addBindValue(m_activeTaskId);
            query.exec();

            query.prepare("INSERT INTO log_paused (task_id, start_reality, end_reality, current_status) VALUES (?, ?, NULL, 'play')");
            query.addBindValue(m_activeTaskId);
            query.addBindValue(currentTime);
            query.exec();

            sendPing(m_activeTaskId);
            m_isTaskPaused = false;
            m_taskStartTime = QDateTime::currentSecsSinceEpoch();
            m_pauseStartTime = 0;
        }

        m_prodRepo->database().commit();
        emit taskPausedChanged();
        emit trackingActiveChanged();
        emit taskListChanged();
    } catch (...) {
        m_prodRepo->database().rollback();
    }
}

void TaskManager::updateTaskStatus(int taskId)
{
    int userId = m_authManager->currentUserId();
    if (!m_prodRepo->ensureDatabaseOpen() || userId == -1 || taskId <= 0) return;

    QUrl url(QString("https://deskmon.pranala-dt.co.id/api/get-current-task-status/%1").arg(taskId));
    QNetworkReply *reply = m_apiClient->get(url, true);
    connect(reply, &QNetworkReply::finished, this, [this, reply, taskId]() {
        handleTaskStatusReply(reply, taskId);
    });
}

void TaskManager::handleTaskStatusReply(QNetworkReply *reply, int taskId)
{
    int userId = m_authManager->currentUserId();
    if (reply->error() != QNetworkReply::NoError) {
        reply->deleteLater();
        return;
    }

    QByteArray responseData = reply->readAll();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
    if (!jsonDoc.isObject()) {
        reply->deleteLater();
        return;
    }

    QJsonObject jsonObj = jsonDoc.object();
    if (!jsonObj.value("success").toBool()) {
        reply->deleteLater();
        return;
    }

    QString apiStatus = jsonObj.value("data").toObject().value("status").toString().toLower();
    QString taskName = getTaskName(taskId);
    QString dbStatus;

    if (apiStatus == "created" || apiStatus == "pending") {
        dbStatus = "Pending";
    } else if (apiStatus == "on-progress") {
        setActiveTask(taskId);
        m_isTaskPaused = false;
        m_taskStartTime = QDateTime::currentSecsSinceEpoch();
        dbStatus = "On Progress";
    } else if (apiStatus == "on-review") {
        dbStatus = "Review";
        emit taskReviewNotification(QString("Task '%1' is under system review").arg(taskName));
    } else if (apiStatus == "need-review") {
        dbStatus = "Need Review";
    } else if (apiStatus == "need-revise") {
        dbStatus = "Need Revise";
    } else if (apiStatus == "completed") {
        finishTask(taskId);
        reply->deleteLater();
        return;
    }

    if (dbStatus == "Review" && m_activeTaskId == taskId) {
        QSqlQuery pauseQuery(m_prodRepo->database());
        pauseQuery.prepare("UPDATE task SET active = 0, paused = 1 WHERE id = :id");
        pauseQuery.bindValue(":id", taskId);
        if (pauseQuery.exec()) {
            m_activeTaskId = -1;
            m_isTaskPaused = false;
            emit activeTaskChanged();
            emit taskPausedChanged();
        }
    }

    emit taskStatusChanged(taskId, dbStatus);

    QSqlQuery query(m_prodRepo->database());
    query.prepare("UPDATE task SET status = :status WHERE id = :id AND user_id = :user_id");
    query.bindValue(":status", dbStatus);
    query.bindValue(":id", taskId);
    query.bindValue(":user_id", userId);
    query.exec();

    emit taskListChanged();
    reply->deleteLater();
}

QString TaskManager::getTaskName(int taskId)
{
    if (!m_prodRepo->ensureDatabaseOpen()) return "Unknown Task";
    QSqlQuery query(m_prodRepo->database());
    query.prepare("SELECT task FROM task WHERE id = :id AND user_id = :user_id");
    query.bindValue(":id", taskId);
    query.bindValue(":user_id", m_authManager->currentUserId());
    if (query.exec() && query.next()) {
        return query.value(0).toString();
    }
    return "Unknown Task";
}

bool TaskManager::isTaskExpired(int taskId)
{
    if (!m_prodRepo->ensureDatabaseOpen()) return false;
    QSqlQuery query(m_prodRepo->database());
    query.prepare("SELECT created_at, max_time FROM task WHERE id = :id");
    query.bindValue(":id", taskId);

    if (query.exec() && query.next()) {
        QString dateStr = query.value(0).toString();
        int maxTimeSeconds = query.value(1).toInt();

        QDateTime createdDateTime = QDateTime::fromString(dateStr, Qt::ISODate);
        if (!createdDateTime.isValid()) return false;

        QDate createdDate = createdDateTime.date();
        QDate today = QDate::currentDate();

        if (createdDate.year() > today.year()) return false;
        if (createdDate.year() == today.year() && createdDate.month() >= today.month()) return false;

        double workHoursPerDay = 8.0 * 3600.0;
        int daysDuration = std::ceil((double)maxTimeSeconds / workHoursPerDay);
        QDate estimatedFinishDate = createdDate.addDays(daysDuration);

        if (estimatedFinishDate.year() > today.year()) return false;
        if (estimatedFinishDate.year() == today.year() && estimatedFinishDate.month() >= today.month()) return false;

        return true;
    }
    return false;
}

int TaskManager::getPendingStartedTaskCount()
{
    int userId = m_authManager->currentUserId();
    if (!m_prodRepo->ensureDatabaseOpen() || userId == -1) return 0;
    QSqlQuery query(m_prodRepo->database());
    query.prepare("SELECT COUNT(*) FROM task WHERE user_id = :uid AND status = 'Pending' AND time_usage > 0");
    query.bindValue(":uid", userId);
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

void TaskManager::revertTaskChange()
{
    if (!m_prodRepo->ensureDatabaseOpen()) return;
    if (m_activeTaskId != -1) {
        QSqlQuery query(m_prodRepo->database());
        query.prepare("UPDATE task SET active = 1, paused = 1 WHERE id = :id");
        query.bindValue(":id", m_activeTaskId);
        query.exec();
        m_isTaskPaused = true;
    }
    emit taskListChanged();
    emit taskPausedChanged();
    emit trackingActiveChanged();
    emit activeTaskChanged();
}

void TaskManager::sendPing(int taskId)
{
    int userId = m_authManager->currentUserId();
    QString token = m_authManager->authToken();
    if (userId == -1 || token.isEmpty() || !m_prodRepo->ensureDatabaseOpen()) return;

    QJsonObject payload;
    if (taskId != -1) {
        payload["task_id"] = QString::number(taskId);
    } else {
        payload["user_id"] = userId;
    }

    QNetworkReply* reply = m_apiClient->post(QUrl("https://deskmon.pranala-dt.co.id/api/ping"), QJsonDocument(payload).toJson(), true);
    QTimer::singleShot(30000, reply, &QNetworkReply::abort);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        QByteArray responseData = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
        QJsonObject jsonObj = jsonDoc.object();

        bool isAuthError = false;
        bool isConnectionError = false;
        QString connectionErrorMessage;

        if (reply->error() != QNetworkReply::NoError) {
            if (reply->errorString().contains("Host requires authentication", Qt::CaseInsensitive)) {
                isAuthError = true;
            } else {
                isConnectionError = true;
                connectionErrorMessage = "Koneksi gagal: " + reply->errorString();
            }
        } else {
            if (!jsonObj.value("success").toBool()) {
                QString errMsg = jsonObj.value("message").toString("Terjadi kesalahan.");
                if (errMsg.contains("Host requires authentication", Qt::CaseInsensitive)) {
                    isAuthError = true;
                } else {
                    isConnectionError = true;
                    connectionErrorMessage = errMsg;
                }
            }
        }

        if (isAuthError) {
            m_pingTimer.setInterval(30000);
            m_pingRetryCount = 0;
            m_authManager->showAuthTokenErrorMessage();
        } else if (isConnectionError) {
            if (m_pingTimer.interval() == 30000) {
                m_pingRetryCount = 0;
                m_pingTimer.setInterval(5000);
                emit showPingErrorDialog(connectionErrorMessage);
            } else {
                m_pingRetryCount++;
                if (m_pingRetryCount >= 22) {
                    m_pingTimer.setInterval(30000);
                    m_pingRetryCount = 0;
                    emit hidePingErrorDialog();
                }
            }
        } else {
            if (m_pingTimer.interval() != 30000) {
                m_pingTimer.setInterval(30000);
                m_pingRetryCount = 0;
            }
            emit hidePingErrorDialog();
        }

        reply->deleteLater();
    });
}

void TaskManager::sendPausePlayDataToAPI(int taskId, const QString& startTime, const QString& endTime, const QString& status)
{
    QString token = m_authManager->authToken();
    if (token.isEmpty() || status != "pause") return;

    QJsonObject payload;
    payload["status"] = "stop";

    QByteArray data = QJsonDocument(payload).toJson();
    QNetworkReply* reply = m_apiClient->put(QUrl(QString("https://deskmon.pranala-dt.co.id/api/end-implementation/%1").arg(taskId)), data, true);
    QTimer::singleShot(30000, reply, &QNetworkReply::abort);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
}

void TaskManager::taskDetailsDialogClosed(const QString &action)
{
    if (action == "quit") {
        emit readyToProceedWithQuit();
    } else if (action == "logout") {
        emit readyToProceedWithLogout();
    }
}

void TaskManager::submitTaskDetails(int taskId, const QString &details, const QString &action, int nextTaskId)
{
    sendTaskDetailsToAPI(taskId, details, action, nextTaskId);
}

void TaskManager::sendTaskDetailsToAPI(int taskId, const QString &details, const QString &action, int nextTaskId)
{
    QString token = m_authManager->authToken();
    if (token.isEmpty()) {
        emit taskDetailsSubmissionFailed("Authentication token not found.");
        emit showNotification("error", "Gagal: Token otentikasi tidak ditemukan.");
        if (action == "quit") emit readyToProceedWithQuit();
        return;
    }

    QJsonObject payload;
    payload["task_id"] = taskId;
    payload["user_id"] = m_authManager->currentUserId();
    payload["message"] = details;

    QNetworkReply *reply = m_apiClient->post(QUrl("https://deskmon.pranala-dt.co.id/api/send-detail-pekerjaan"), QJsonDocument(payload).toJson(), true);
    QTimer::singleShot(15000, reply, &QNetworkReply::abort);

    connect(reply, &QNetworkReply::finished, this, [this, reply, taskId, details, action, nextTaskId]() {
        if (reply->error() != QNetworkReply::NoError) {
            emit taskDetailsSubmissionFailed("Network Error: " + reply->errorString());
            emit showNotification("warning", "Detail pekerjaan Gagal dikirim!");
            if (action == "quit") emit readyToProceedWithQuit();
        } else {
            emit taskDetailsSubmissionSuccess();
            emit showNotification("success", "Detail pekerjaan berhasil dikirim!");
            if (action == "quit") emit readyToProceedWithQuit();
            else if (action == "switch") setActiveTask(nextTaskId);
            else if (action == "logout") emit readyToProceedWithLogout();
        }
        reply->deleteLater();
    });
}
