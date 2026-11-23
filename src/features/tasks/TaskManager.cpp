#include "TaskManager.h"
#include "core/database/DatabaseManager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QPointer>
#include <QSet>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QThreadPool>
#include <QVector>
#include <QDebug>
#include <atomic>

TaskManager::TaskManager(ApiClient *apiClient,
                         TaskRepository *taskRepo,
                         AuthManager *authManager,
                         QObject *parent)
    : QObject(parent)
    , m_apiClient(apiClient)
    , m_taskRepo(taskRepo)
    , m_authManager(authManager)
{
    m_pingTimer.setInterval(30000);
    connect(&m_pingTimer, &QTimer::timeout, this, [this]() {
        sendPing((m_activeTaskId != -1 && !m_isTaskPaused) ? m_activeTaskId : -1);
    });

    m_taskRefreshTimer.setInterval(180000); // 3 menit
    connect(&m_taskRefreshTimer, &QTimer::timeout, this, &TaskManager::refreshTasks);

    connect(m_authManager, &AuthManager::loggedOut, this, [this]() {
        stopPingTimer();
        stopRefreshTimer();
        ++m_activationSequence;
        ++m_fetchSequence;
        m_activeTaskId = -1;
        m_isTaskPaused = false;
        m_taskStartTime = 0;
        m_taskTimeOffset = 0;
        m_pauseStartTime = 0;
        invalidateTaskCache();
        emit activeTaskChanged();
        emit taskPausedChanged();
        emit taskListChanged();
    });
}

void TaskManager::invalidateTaskCache()
{
    m_taskCacheValid = false;
}

void TaskManager::startGlobalTimer()
{
    m_globalTimeUsage++;
    emit globalTimeUsageChanged();
}

void TaskManager::startPingTimer()
{
    m_pingRetryCount = 0;
    m_pingTimer.setInterval(30000);
    sendPing((m_activeTaskId != -1 && !m_isTaskPaused) ? m_activeTaskId : -1);
    m_pingTimer.start();
}

void TaskManager::stopPingTimer()
{
    m_pingTimer.stop();
}

void TaskManager::startRefreshTimer()
{
    m_taskRefreshTimer.start();
}

void TaskManager::stopRefreshTimer()
{
    m_taskRefreshTimer.stop();
}

QVariantList TaskManager::taskList() const
{
    int userId = m_authManager->currentUserId();
    if (userId == -1 || !m_taskRepo) return QVariantList();

    if (!m_taskCacheValid || m_cachedUserId != userId || m_cachedPausedState != m_isTaskPaused) {
        m_cachedTaskList = m_taskRepo->getTasksForUser(userId, m_isTaskPaused);
        m_cachedUserId = userId;
        m_cachedPausedState = m_isTaskPaused;
        m_taskCacheValid = true;
    }

    if (m_activeTaskId != -1 && !m_isTaskPaused && m_taskStartTime > 0) {
        const qint64 liveUsage = m_taskTimeOffset + qMax<qint64>(0, QDateTime::currentSecsSinceEpoch() - m_taskStartTime);
        QVariantList result = m_cachedTaskList;
        for (int i = 0; i < result.size(); ++i) {
            QVariantMap item = result[i].toMap();
            if (item.value(QStringLiteral("id")).toInt() == m_activeTaskId) {
                item[QStringLiteral("time_usage")] = static_cast<int>(liveUsage);
                result[i] = item;
                break;
            }
        }
        return result;
    }

    return m_cachedTaskList;
}

void TaskManager::fetchAndStoreTasks()
{
    int userId = m_authManager->currentUserId();
    if (userId == -1 || m_authManager->authToken().isEmpty()) return;

    QUrl apiUrl = m_apiClient->buildUrl(QString("/task-by-user/%1").arg(userId));
    QNetworkReply *reply = m_apiClient->get(apiUrl, true);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        handleTaskFetchReply(reply);
    });
}

void TaskManager::handleTaskFetchReply(QNetworkReply *reply)
{
    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 401) {
        m_authManager->showAuthTokenErrorMessage();
        reply->deleteLater();
        return;
    }

    if (reply->error() != QNetworkReply::NoError) {
        reply->deleteLater();
        return;
    }

    QByteArray responseData = reply->readAll();
    reply->deleteLater();

    int userId = m_authManager->currentUserId();
    if (userId == -1 || !m_taskRepo) {
        return;
    }

    static std::atomic<quint64> s_taskSyncWorkerSeq{0};
    const quint64 seq = ++m_fetchSequence;
    const quint64 workerSeq = ++s_taskSyncWorkerSeq;
    const QString dbPath = DatabaseManager::instance().databasePath();
    const int currentActiveId = m_activeTaskId;
    QPointer<TaskManager> self(this);

    // Parse payload & sinkronisasi SQLite batch di background worker thread (QThreadPool)
    QThreadPool::globalInstance()->start([self, responseData, userId, seq, workerSeq, dbPath, currentActiveId]() {
        QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
        if (!jsonDoc.isObject()) return;

        QJsonObject jsonObj = jsonDoc.object();
        if (!jsonObj["success"].toBool()) return;

        QJsonArray tasksArray = jsonObj["data"].toArray();
        QSet<int> serverTaskIds;
        QVector<TaskRepository::SyncedTaskRow> parsedTasks;
        parsedTasks.reserve(tasksArray.size());

        for (const QJsonValue &taskValue : tasksArray) {
            QJsonObject taskObj = taskValue.toObject();
            if (taskObj.contains("id")) {
                serverTaskIds.insert(taskObj["id"].toInt());
            }
            if (taskObj["status"].toString() == "completed") continue;
            if (!taskObj.contains("id") || !taskObj.contains("title") ||
                !taskObj.contains("description") || !taskObj.contains("user_id")) {
                continue;
            }

            TaskRepository::SyncedTaskRow pt;
            pt.id = taskObj["id"].toInt();
            pt.projectName = taskObj["title"].toString();
            pt.taskDesc = taskObj["description"].toString();
            const int tUserId = taskObj["user_id"].toInt();
            if (tUserId != userId) continue;

            QJsonValue durationValue = taskObj["duration"];
            QJsonValue totalDurationValue = taskObj["total_duration"];
            pt.hasDuration = !durationValue.isNull();
            pt.serverMaxTime = pt.hasDuration ? qRound(durationValue.toVariant().toDouble() * 3600) : 0;
            pt.hasTotalDuration = !totalDurationValue.isNull();
            pt.serverTimeUsage = pt.hasTotalDuration ? qRound(totalDurationValue.toVariant().toDouble() * 3600) : 0;
            pt.createdAt = taskObj["created_at"].toString();
            parsedTasks.append(pt);
        }

        if (!self) return;

        bool syncedOnWorker = false;
        bool activeTaskRemoved = false;

        if (!dbPath.isEmpty()) {
            const QString connName = QStringLiteral("worker_tasks_%1").arg(workerSeq);
            {
                QSqlDatabase workerDb = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connName);
                workerDb.setDatabaseName(dbPath);
                if (workerDb.open()) {
                    QSqlQuery pragmaQ(workerDb);
                    pragmaQ.exec(QStringLiteral("PRAGMA busy_timeout = 5000;"));
                    pragmaQ.exec(QStringLiteral("PRAGMA foreign_keys = ON;"));
                    syncedOnWorker = TaskRepository::syncTasksToDatabase(
                        workerDb, userId, parsedTasks, serverTaskIds, currentActiveId, activeTaskRemoved);
                    workerDb.close();
                }
            }
            QSqlDatabase::removeDatabase(connName);
        }

        if (!self) return;

        QMetaObject::invokeMethod(self, [self, userId, seq, currentActiveId, syncedOnWorker, activeTaskRemoved, parsedTasks, serverTaskIds]() {
            if (!self || self->m_fetchSequence != seq || !self->m_taskRepo) return;
            if (self->m_authManager->currentUserId() != userId) return;

            bool removed = activeTaskRemoved;
            if (!syncedOnWorker) {
                QSqlDatabase mainDb = DatabaseManager::instance().database();
                if (!TaskRepository::syncTasksToDatabase(mainDb, userId, parsedTasks, serverTaskIds, self->m_activeTaskId, removed)) {
                    return;
                }
            }

            if (removed && self->m_activeTaskId == currentActiveId && currentActiveId != -1) {
                ++self->m_activationSequence;
                self->m_activeTaskId = -1;
                self->m_isTaskPaused = false;
                self->m_taskTimeOffset = 0;
                self->m_taskStartTime = 0;
                emit self->activeTaskChanged();
                emit self->taskPausedChanged();
            }

            self->invalidateTaskCache();
            self->syncActiveTask();
            emit self->taskListChanged();
        }, Qt::QueuedConnection);
    });
}

void TaskManager::refreshTasks()
{
    int userId = m_authManager->currentUserId();
    if (userId == -1 || !m_taskRepo) return;

    fetchAndStoreTasks();

    // Hanya cek status real-time untuk task yang relevan (maksimal 5 task teratas / aktif)
    // agar tidak membanjiri ratusan HTTP request paralel yang membuat UI freeze
    if (m_activeTaskId != -1) {
        updateTaskStatus(m_activeTaskId);
    } else {
        QList<int> ids = m_taskRepo->getTaskIdsForUser(userId);
        const int limit = qMin(ids.size(), 5);
        for (int i = 0; i < limit; ++i) {
            updateTaskStatus(ids[i]);
        }
    }

    syncActiveTask();
}

void TaskManager::syncActiveTask()
{
    int userId = m_authManager->currentUserId();
    if (userId == -1 || !m_taskRepo) return;

    const int prevActiveId = m_activeTaskId;
    const bool prevPaused = m_isTaskPaused;

    int activeId = -1;
    bool isPaused = false;
    int timeUsage = 0;

    if (m_taskRepo->findActiveTask(userId, activeId, isPaused, timeUsage)) {
        if (m_activeTaskId == activeId && !m_isTaskPaused && !isPaused && m_taskStartTime > 0) {
            // Pertahankan sesi timer yang sedang berjalan tanpa mereset m_taskStartTime
            m_taskTimeOffset = qMax<qint64>(m_taskTimeOffset, timeUsage);
        } else {
            m_activeTaskId = activeId;
            m_isTaskPaused = isPaused;
            m_taskTimeOffset = timeUsage;
            m_taskStartTime = QDateTime::currentSecsSinceEpoch();
        }
    } else {
        m_activeTaskId = -1;
        m_isTaskPaused = false;
        m_taskTimeOffset = 0;
        m_taskStartTime = 0;
    }

    if (m_activeTaskId != prevActiveId || m_isTaskPaused != prevPaused) {
        invalidateTaskCache();
    }
    if (m_activeTaskId != prevActiveId) {
        emit activeTaskChanged();
    }
    if (m_isTaskPaused != prevPaused) {
        emit taskPausedChanged();
    }
}

void TaskManager::setActiveTask(int taskId)
{
    int userId = m_authManager->currentUserId();
    if (userId == -1 || !m_taskRepo) return;

    if (m_activeTaskId != -1 && m_activeTaskId != taskId && !m_isTaskPaused) {
        emit requestTaskDetails(m_activeTaskId, QStringLiteral("switch"), taskId);
    }

    if (taskId == m_activeTaskId && !m_isTaskPaused) {
        return;
    }

    const quint64 expectedSeq = ++m_activationSequence;

    if (m_activeTaskId != -1) {
        int prevMaxTime = 0;
        int prevTimeUsage = 0;
        QString prevStatus;
        if (m_taskRepo->getTaskDetails(m_activeTaskId, prevMaxTime, prevTimeUsage, prevStatus)) {
            QStringList restrictedStatuses = {"review", "need review", "need revise", "completed"};
            QString newStatus = restrictedStatuses.contains(prevStatus.toLower()) ? prevStatus : QStringLiteral("Pending");

            qint64 currentEpoch = QDateTime::currentSecsSinceEpoch();
            qint64 timeUsed = m_taskTimeOffset + (m_isTaskPaused ? 0 : (currentEpoch - m_taskStartTime));

            m_taskRepo->deactivateTask(m_activeTaskId, newStatus, static_cast<int>(timeUsed));

            if (!m_isTaskPaused) {
                QString nowIso = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
                m_taskRepo->logPauseEvent(m_activeTaskId, nowIso);
                sendPausePlayDataToAPI(m_activeTaskId, m_lastPlayStartTime.toString(Qt::ISODateWithMs), nowIso, QStringLiteral("pause"));
                emit saveWorkTimeRequested();
            }
        }
    }

    if (taskId != -1) {
        int newMaxTime = 0;
        int newTimeUsage = 0;
        QString newStatus;
        if (!m_taskRepo->getTaskDetails(taskId, newMaxTime, newTimeUsage, newStatus)) {
            qWarning() << "Failed to fetch details for new task:" << taskId;
            return;
        }

        m_taskTimeOffset = newTimeUsage;
        m_taskStartTime = QDateTime::currentSecsSinceEpoch();

        m_taskRepo->resetActiveTaskStatus(taskId);
        m_taskRepo->updateTaskTiming(taskId, static_cast<int>(m_taskTimeOffset), true, true);
        m_taskRepo->updateTaskStatus(taskId, QStringLiteral("Paused"));
        setMaxTimeForTask(taskId);

        QTimer::singleShot(2000, this, [this, taskId, userId, expectedSeq]() {
            if (m_activationSequence != expectedSeq ||
                m_activeTaskId != taskId ||
                !m_isTaskPaused ||
                m_authManager->currentUserId() != userId) {
                return;
            }
            m_taskRepo->updateTaskTiming(taskId, static_cast<int>(m_taskTimeOffset), true, false);
            m_taskRepo->updateTaskStatus(taskId, QStringLiteral("on-progress"));

            m_isTaskPaused = false;
            m_taskStartTime = QDateTime::currentSecsSinceEpoch();
            m_pauseStartTime = 0;
            m_lastPlayStartTime = QDateTime::currentDateTime();

            m_taskRepo->logPlayEvent(taskId, m_lastPlayStartTime.toString(Qt::ISODateWithMs));
            sendPing(m_activeTaskId);

            invalidateTaskCache();
            emit taskPausedChanged();
            emit trackingActiveChanged();
            emit taskListChanged();
        });
    }

    m_activeTaskId = taskId;
    if (taskId != -1) {
        m_isTaskPaused = true;
        m_pauseStartTime = QDateTime::currentSecsSinceEpoch();
    } else {
        m_isTaskPaused = false;
        m_taskTimeOffset = 0;
        m_taskStartTime = 0;
        m_pauseStartTime = 0;
    }

    invalidateTaskCache();
    emit taskPausedChanged();
    emit trackingActiveChanged();
    emit activeTaskChanged();
    emit taskListChanged();
}

void TaskManager::stopActiveTaskForLogout()
{
    if (!m_taskRepo || m_activeTaskId == -1) return;

    ++m_activationSequence;

    qint64 currentEpoch = QDateTime::currentSecsSinceEpoch();
    qint64 timeUsed = m_taskTimeOffset + (m_isTaskPaused ? 0 : (currentEpoch - m_taskStartTime));

    if (!m_isTaskPaused) {
        QString nowIso = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
        m_taskRepo->logPauseEvent(m_activeTaskId, nowIso);
        sendPausePlayDataToAPI(m_activeTaskId, m_lastPlayStartTime.toString(Qt::ISODateWithMs), nowIso, QStringLiteral("pause"));
    }

    int maxTime = 0;
    int existingUsage = 0;
    QString prevStatus;
    QString newStatus = QStringLiteral("Pending");
    if (m_taskRepo->getTaskDetails(m_activeTaskId, maxTime, existingUsage, prevStatus)) {
        QStringList restrictedStatuses = {"review", "need review", "need revise", "completed"};
        if (restrictedStatuses.contains(prevStatus.toLower())) {
            newStatus = prevStatus;
        }
    }

    m_taskRepo->deactivateTask(m_activeTaskId, newStatus, static_cast<int>(timeUsed));

    m_activeTaskId = -1;
    m_isTaskPaused = false;
    m_taskTimeOffset = 0;
    m_taskStartTime = 0;
    m_pauseStartTime = 0;

    invalidateTaskCache();
    emit activeTaskChanged();
    emit taskPausedChanged();
    emit trackingActiveChanged();
    emit taskListChanged();
}

void TaskManager::finishTask(int taskId)
{
    int userId = m_authManager->currentUserId();
    if (userId == -1 || !m_taskRepo) return;

    int overrideTimeUsage = -1;
    if (m_activeTaskId == taskId && !m_isTaskPaused) {
        overrideTimeUsage = static_cast<int>(m_taskTimeOffset + (QDateTime::currentSecsSinceEpoch() - m_taskStartTime));
    }

    m_taskRepo->archiveCompletedTask(taskId, userId, overrideTimeUsage);

    if (m_activeTaskId == taskId) {
        ++m_activationSequence;
        m_activeTaskId = -1;
        m_isTaskPaused = false;
        m_taskTimeOffset = 0;
        m_taskStartTime = 0;
        m_pauseStartTime = 0;
        emit activeTaskChanged();
        emit taskPausedChanged();
        emit trackingActiveChanged();
    }

    invalidateTaskCache();
    emit taskListChanged();
}

void TaskManager::toggleTaskPause()
{
    if (!m_taskRepo || m_activeTaskId == -1) return;

    QString currentTime = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
    qint64 currentEpoch = QDateTime::currentSecsSinceEpoch();

    if (!DatabaseManager::instance().transaction()) {
        qWarning() << "Failed to start transaction in toggleTaskPause";
        return;
    }

    if (!m_isTaskPaused) {
        qint64 timeUsed = m_taskTimeOffset + (currentEpoch - m_taskStartTime);
        if (!m_taskRepo->updateTaskTiming(m_activeTaskId, static_cast<int>(timeUsed), true, true) ||
            !m_taskRepo->updateTaskStatus(m_activeTaskId, QStringLiteral("Paused")) ||
            !m_taskRepo->logPauseEvent(m_activeTaskId, currentTime)) {
            DatabaseManager::instance().rollback();
            return;
        }

        if (!DatabaseManager::instance().commit()) {
            DatabaseManager::instance().rollback();
            return;
        }

        ++m_activationSequence;
        sendPausePlayDataToAPI(m_activeTaskId, m_lastPlayStartTime.toString(Qt::ISODateWithMs), currentTime, QStringLiteral("pause"));
        emit saveWorkTimeRequested();

        m_isTaskPaused = true;
        m_taskTimeOffset = timeUsed;
        m_pauseStartTime = currentEpoch;
    } else {
        if (!m_taskRepo->updateTaskTiming(m_activeTaskId, static_cast<int>(m_taskTimeOffset), true, false) ||
            !m_taskRepo->updateTaskStatus(m_activeTaskId, QStringLiteral("on-progress")) ||
            !m_taskRepo->logPlayEvent(m_activeTaskId, currentTime)) {
            DatabaseManager::instance().rollback();
            return;
        }

        if (!DatabaseManager::instance().commit()) {
            DatabaseManager::instance().rollback();
            return;
        }

        ++m_activationSequence;
        sendPing(m_activeTaskId);

        m_isTaskPaused = false;
        m_taskStartTime = currentEpoch;
        m_pauseStartTime = 0;
        m_lastPlayStartTime = QDateTime::currentDateTime();
    }

    invalidateTaskCache();
    emit taskPausedChanged();
    emit trackingActiveChanged();
    emit taskListChanged();
}

void TaskManager::updateTaskStatus(int taskId)
{
    int userId = m_authManager->currentUserId();
    if (!m_taskRepo || userId == -1 || taskId <= 0) return;

    QUrl url = m_apiClient->buildUrl(QString("/get-current-task-status/%1").arg(taskId));
    QNetworkReply *reply = m_apiClient->get(url, true);
    connect(reply, &QNetworkReply::finished, this, [this, reply, taskId]() {
        handleTaskStatusReply(reply, taskId);
    });
}

void TaskManager::handleTaskStatusReply(QNetworkReply *reply, int taskId)
{
    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 401) {
        m_authManager->showAuthTokenErrorMessage();
        reply->deleteLater();
        return;
    }

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

    QString apiStatus;
    const QJsonValue dataVal = jsonObj.value("data");
    if (dataVal.isString()) {
        apiStatus = dataVal.toString().trimmed().toLower();
    } else if (dataVal.isObject()) {
        apiStatus = dataVal.toObject().value("status").toString().trimmed().toLower();
    }

    if (apiStatus.isEmpty()) {
        reply->deleteLater();
        return;
    }

    QString taskName = getTaskName(taskId);
    QString dbStatus;
    bool isReviewStatus = false;

    if (apiStatus == "created" || apiStatus == "pending") {
        dbStatus = QStringLiteral("Pending");
    } else if (apiStatus == "on-progress") {
        if (m_activeTaskId != taskId || m_isTaskPaused) {
            if (m_activeTaskId != taskId) {
                setActiveTask(taskId);
            }
            if (m_activeTaskId == taskId) {
                ++m_activationSequence;
                m_taskRepo->updateTaskTiming(taskId, static_cast<int>(m_taskTimeOffset), true, false);
                m_isTaskPaused = false;
                m_taskStartTime = QDateTime::currentSecsSinceEpoch();
                m_pauseStartTime = 0;
                m_lastPlayStartTime = QDateTime::currentDateTime();
                m_taskRepo->logPlayEvent(taskId, m_lastPlayStartTime.toString(Qt::ISODateWithMs));
                emit taskPausedChanged();
                emit trackingActiveChanged();
            }
        }
        dbStatus = QStringLiteral("on-progress");
    } else if (apiStatus == "on-review") {
        dbStatus = QStringLiteral("Review");
        isReviewStatus = true;
        emit taskReviewNotification(QString("Task '%1' is under system review").arg(taskName));
    } else if (apiStatus == "need-review") {
        dbStatus = QStringLiteral("Need Review");
    } else if (apiStatus == "need-revise") {
        dbStatus = QStringLiteral("Need Revise");
    } else if (apiStatus == "completed") {
        finishTask(taskId);
        reply->deleteLater();
        return;
    } else {
        qWarning() << "Unknown status for taskId" << taskId << ":" << apiStatus;
        reply->deleteLater();
        return;
    }

    if (isReviewStatus && m_activeTaskId == taskId) {
        ++m_activationSequence;
        m_taskRepo->updateTaskTiming(taskId, static_cast<int>(m_taskTimeOffset), false, true);
        m_activeTaskId = -1;
        m_isTaskPaused = false;
        emit activeTaskChanged();
        emit taskPausedChanged();
    }

    m_taskRepo->updateTaskStatus(taskId, dbStatus);
    emit taskStatusChanged(taskId, dbStatus);

    const QStringList restrictedStatuses = {"Review", "Need Review", "Need Revise"};
    if (restrictedStatuses.contains(dbStatus) && m_activeTaskId == taskId) {
        int userId = m_authManager->currentUserId();
        int nextTaskId = m_taskRepo->findNextEligibleTaskId(userId, taskId);
        setActiveTask(nextTaskId);
    }

    invalidateTaskCache();
    emit taskListChanged();
    reply->deleteLater();
}

QString TaskManager::getTaskName(int taskId)
{
    return m_taskRepo ? m_taskRepo->getTaskName(taskId) : "Unknown Task";
}

bool TaskManager::isTaskExpired(int taskId)
{
    return m_taskRepo ? m_taskRepo->isTaskExpired(taskId) : false;
}

int TaskManager::getPendingStartedTaskCount()
{
    int userId = m_authManager->currentUserId();
    return m_taskRepo ? m_taskRepo->getPendingStartedTaskCount(userId) : 0;
}

void TaskManager::revertTaskChange()
{
    if (m_taskRepo && m_activeTaskId != -1) {
        ++m_activationSequence;
        m_taskRepo->updateTaskTiming(m_activeTaskId, static_cast<int>(m_taskTimeOffset), true, true);
        m_isTaskPaused = true;
    }
    invalidateTaskCache();
    emit taskListChanged();
    emit taskPausedChanged();
    emit trackingActiveChanged();
    emit activeTaskChanged();
}

void TaskManager::setMaxTimeForTask(int taskId)
{
    if (!m_taskRepo || taskId <= 0) return;

    int maxTime = 0;
    int timeUsage = 0;
    QString status;
    if (m_taskRepo->getTaskDetails(taskId, maxTime, timeUsage, status) && maxTime <= 0) {
        m_taskRepo->updateTaskMaxTime(taskId, 8 * 3600);
        invalidateTaskCache();
    }
}

void TaskManager::checkTaskStatusBeforeStart()
{
    if (!m_taskRepo) return;

    int userId = m_authManager->currentUserId();
    if (userId == -1) return;

    ++m_activationSequence;

    int taskId = -1;
    bool isPaused = false;
    int timeUsage = 0;
    QString status;

    if (m_taskRepo->findStartupTask(userId, taskId, isPaused, timeUsage, status)) {
        m_activeTaskId = taskId;
        if (status.toLower() == QStringLiteral("on-progress")) {
            m_isTaskPaused = false;
            m_taskRepo->updateTaskTiming(taskId, timeUsage, true, false);
        } else {
            m_isTaskPaused = isPaused;
        }
        m_taskTimeOffset = timeUsage;
        if (!m_isTaskPaused) {
            m_taskStartTime = QDateTime::currentSecsSinceEpoch();
            m_lastPlayStartTime = QDateTime::currentDateTime();
        } else {
            m_taskStartTime = 0;
        }
    } else {
        m_activeTaskId = -1;
        m_isTaskPaused = false;
        m_taskTimeOffset = 0;
        m_taskStartTime = 0;
        m_pauseStartTime = 0;
    }

    invalidateTaskCache();
    emit activeTaskChanged();
    emit taskPausedChanged();
    emit trackingActiveChanged();
    emit taskListChanged();
}

void TaskManager::sendPing(int taskId)
{
    int userId = m_authManager->currentUserId();
    QString token = m_authManager->authToken();
    if (userId == -1 || token.isEmpty()) return;

    QJsonObject payload;
    if (taskId != -1) {
        payload["task_id"] = QString::number(taskId);
    } else {
        payload["user_id"] = userId;
    }

    QNetworkReply* reply = m_apiClient->post(m_apiClient->buildUrl("/ping"), QJsonDocument(payload).toJson(), true);
    QTimer::singleShot(30000, reply, &QNetworkReply::abort);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        QByteArray responseData = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
        QJsonObject jsonObj = jsonDoc.object();

        bool isAuthError = false;
        bool isConnectionError = false;
        QString connectionErrorMessage;

        if (reply->error() != QNetworkReply::NoError) {
            if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 401 ||
                reply->errorString().contains("Host requires authentication", Qt::CaseInsensitive)) {
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
    Q_UNUSED(startTime);
    Q_UNUSED(endTime);
    QString token = m_authManager->authToken();
    if (token.isEmpty() || status != "pause") return;

    QJsonObject payload;
    payload["status"] = "stop";

    QByteArray data = QJsonDocument(payload).toJson();
    QNetworkReply* reply = m_apiClient->put(m_apiClient->buildUrl(QString("/end-implementation/%1").arg(taskId)), data, true);
    QTimer::singleShot(30000, reply, &QNetworkReply::abort);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        QByteArray responseData = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
        QJsonObject jsonObj = jsonDoc.object();

        if (reply->error() != QNetworkReply::NoError) {
            if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 401 ||
                reply->errorString().contains("Host requires authentication", Qt::CaseInsensitive)) {
                m_authManager->showAuthTokenErrorMessage();
            } else {
                emit showPingErrorDialog("Koneksi gagal: " + reply->errorString());
            }
        } else {
            bool success = jsonObj.value("success").toBool();
            if (!success) {
                QString errMsg = jsonObj.value("message").toString("Terjadi kesalahan.");
                if (errMsg.contains("Host requires authentication", Qt::CaseInsensitive)) {
                    m_authManager->showAuthTokenErrorMessage();
                } else {
                    emit showPingErrorDialog(errMsg);
                }
            } else {
                emit hidePingErrorDialog();
            }
        }
        reply->deleteLater();
    });
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

    QNetworkReply *reply = m_apiClient->post(m_apiClient->buildUrl("/send-detail-pekerjaan"), QJsonDocument(payload).toJson(), true);
    QTimer::singleShot(15000, reply, &QNetworkReply::abort);

    connect(reply, &QNetworkReply::finished, this, [this, reply, taskId, details, action, nextTaskId]() {
        if (reply->error() != QNetworkReply::NoError) {
            QString errorMsg = "Network Error: " + reply->errorString();
            emit taskDetailsSubmissionFailed(errorMsg);
            emit taskDetailsSubmissionFailedWithRetry(errorMsg, taskId, details, action, nextTaskId);
            emit showNotification("warning", "Detail pekerjaan Gagal dikirim!");
        } else {
            const QByteArray responseData = reply->readAll();
            const QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
            const QJsonObject jsonObj = jsonDoc.isObject() ? jsonDoc.object() : QJsonObject();
            if (jsonDoc.isObject() && jsonObj.contains(QStringLiteral("success")) && !jsonObj.value(QStringLiteral("success")).toBool(true)) {
                QString errorMsg = jsonObj.value(QStringLiteral("message")).toString();
                if (errorMsg.isEmpty()) {
                    errorMsg = jsonObj.value(QStringLiteral("error")).toString();
                }
                if (errorMsg.isEmpty()) {
                    errorMsg = QStringLiteral("Gagal mengirim detail pekerjaan.");
                }
                emit taskDetailsSubmissionFailed(errorMsg);
                emit taskDetailsSubmissionFailedWithRetry(errorMsg, taskId, details, action, nextTaskId);
                emit showNotification("warning", "Detail pekerjaan Gagal dikirim!");
            } else {
                emit taskDetailsSubmissionSuccess();
                emit showNotification("success", "Detail pekerjaan berhasil dikirim!");
                if (action == "quit") emit readyToProceedWithQuit();
                else if (action == "switch") setActiveTask(nextTaskId);
                else if (action == "logout") emit readyToProceedWithLogout();
            }
        }
        reply->deleteLater();
    });
}
