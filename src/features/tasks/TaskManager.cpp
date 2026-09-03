#include "TaskManager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSet>
#include <QDebug>

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
        sendPing(m_activeTaskId);
    });

    m_taskRefreshTimer.setInterval(300000); // 5 menit
    connect(&m_taskRefreshTimer, &QTimer::timeout, this, &TaskManager::refreshTasks);

    connect(m_authManager, &AuthManager::loggedIn, this, [this]() {
        startPingTimer();
        m_taskRefreshTimer.start();
        refreshTasks();
    });

    connect(m_authManager, &AuthManager::loggedOut, this, [this]() {
        stopPingTimer();
        m_taskRefreshTimer.stop();
        m_activeTaskId = -1;
        m_isTaskPaused = false;
        m_taskStartTime = 0;
        m_taskTimeOffset = 0;
        emit activeTaskChanged();
        emit taskPausedChanged();
        emit taskListChanged();
    });
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
    m_pingTimer.start();
}

void TaskManager::stopPingTimer()
{
    m_pingTimer.stop();
}

QVariantList TaskManager::taskList() const
{
    int userId = m_authManager->currentUserId();
    if (userId == -1 || !m_taskRepo) return QVariantList();
    return m_taskRepo->getTasksForUser(userId);
}

void TaskManager::fetchAndStoreTasks()
{
    int userId = m_authManager->currentUserId();
    if (userId == -1 || m_authManager->authToken().isEmpty()) return;

    QUrl apiUrl(QString("https://deskmon.pranala-dt.co.id/api/task-by-user/%1").arg(userId));
    QNetworkReply *reply = m_apiClient->get(apiUrl, true);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        handleTaskFetchReply(reply);
    });
}

void TaskManager::handleTaskFetchReply(QNetworkReply *reply)
{
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
    if (!jsonObj["success"].toBool()) {
        reply->deleteLater();
        return;
    }

    int userId = m_authManager->currentUserId();
    if (userId == -1 || !m_taskRepo) {
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

    QMap<int, QPair<int, int>> existingTasks = m_taskRepo->getExistingTasksMap(userId);

    // Hapus task lokal yang sudah tidak ada di server
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
            m_taskRepo->deleteTask(localTaskId, userId);
        }
    }

    // Perbarui atau sisipkan task dari server
    for (const QJsonValue &taskValue : tasksArray) {
        QJsonObject taskObj = taskValue.toObject();
        if (taskObj["status"].toString() == "completed") continue;
        if (!taskObj.contains("id") || !taskObj.contains("title") ||
            !taskObj.contains("description") || !taskObj.contains("user_id")) {
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

            m_taskRepo->upsertTask(taskId, userId, projectName, taskDesc, finalMaxTime, finalTimeUsage, createdAt);
        } else {
            m_taskRepo->upsertTask(taskId, userId, projectName, taskDesc, serverMaxTime, serverTimeUsage, createdAt);
        }
    }

    syncActiveTask();
    emit taskListChanged();
    reply->deleteLater();
}

void TaskManager::refreshTasks()
{
    int userId = m_authManager->currentUserId();
    if (userId == -1 || !m_taskRepo) return;

    fetchAndStoreTasks();

    QList<int> ids = m_taskRepo->getTaskIdsForUser(userId);
    for (int taskId : ids) {
        updateTaskStatus(taskId);
    }

    syncActiveTask();
    emit taskListChanged();
}

void TaskManager::syncActiveTask()
{
    int userId = m_authManager->currentUserId();
    if (userId == -1 || !m_taskRepo) return;

    int activeId = -1;
    bool isPaused = false;
    int timeUsage = 0;

    if (m_taskRepo->findActiveTask(userId, activeId, isPaused, timeUsage)) {
        m_activeTaskId = activeId;
        m_isTaskPaused = isPaused;
        m_taskTimeOffset = timeUsage;
        m_taskStartTime = QDateTime::currentSecsSinceEpoch();
    } else {
        m_activeTaskId = -1;
        m_isTaskPaused = false;
        m_taskTimeOffset = 0;
        m_taskStartTime = 0;
    }

    emit activeTaskChanged();
    emit taskPausedChanged();
}

void TaskManager::setActiveTask(int taskId)
{
    int userId = m_authManager->currentUserId();
    if (userId == -1 || !m_taskRepo) return;

    if (m_activeTaskId == taskId && !m_isTaskPaused) {
        return;
    }

    m_taskRepo->resetActiveTaskStatus(taskId);
    m_taskRepo->updateTaskTiming(taskId, m_taskTimeOffset, true, false);
    m_taskRepo->updateTaskStatus(taskId, "started");

    m_activeTaskId = taskId;
    m_isTaskPaused = false;
    m_taskStartTime = QDateTime::currentSecsSinceEpoch();
    m_lastPlayStartTime = QDateTime::currentDateTime();

    QString currentTime = m_lastPlayStartTime.toString(Qt::ISODateWithMs);
    m_taskRepo->logPlayEvent(taskId, currentTime);

    sendPing(taskId);

    emit activeTaskChanged();
    emit taskPausedChanged();
    emit trackingActiveChanged();
    emit taskListChanged();
}

void TaskManager::finishTask(int taskId)
{
    int userId = m_authManager->currentUserId();
    if (userId == -1 || !m_taskRepo) return;

    m_taskRepo->archiveCompletedTask(taskId, userId);

    if (m_activeTaskId == taskId) {
        m_activeTaskId = -1;
        m_isTaskPaused = false;
        m_taskTimeOffset = 0;
        m_taskStartTime = 0;
        emit activeTaskChanged();
        emit taskPausedChanged();
    }

    emit taskListChanged();
}

void TaskManager::toggleTaskPause()
{
    if (!m_taskRepo || m_activeTaskId == -1) return;

    QString currentTime = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
    qint64 currentEpoch = QDateTime::currentSecsSinceEpoch();

    if (!m_isTaskPaused) {
        qint64 timeUsed = m_taskTimeOffset + (currentEpoch - m_taskStartTime);
        m_taskRepo->updateTaskTiming(m_activeTaskId, timeUsed, true, true);
        m_taskRepo->updateTaskStatus(m_activeTaskId, "Paused");
        m_taskRepo->logPauseEvent(m_activeTaskId, currentTime);

        sendPausePlayDataToAPI(m_activeTaskId, m_lastPlayStartTime.toString(Qt::ISODateWithMs), currentTime, "pause");

        m_isTaskPaused = true;
        m_taskTimeOffset = timeUsed;
        m_pauseStartTime = currentEpoch;
    } else {
        m_taskRepo->updateTaskTiming(m_activeTaskId, m_taskTimeOffset, true, false);
        m_taskRepo->updateTaskStatus(m_activeTaskId, "On Progress");
        m_taskRepo->logPlayEvent(m_activeTaskId, currentTime);

        sendPing(m_activeTaskId);

        m_isTaskPaused = false;
        m_taskStartTime = currentEpoch;
        m_pauseStartTime = 0;
        m_lastPlayStartTime = QDateTime::currentDateTime();
    }

    emit taskPausedChanged();
    emit trackingActiveChanged();
    emit taskListChanged();
}

void TaskManager::updateTaskStatus(int taskId)
{
    int userId = m_authManager->currentUserId();
    if (!m_taskRepo || userId == -1 || taskId <= 0) return;

    QUrl url(QString("https://deskmon.pranala-dt.co.id/api/get-current-task-status/%1").arg(taskId));
    QNetworkReply *reply = m_apiClient->get(url, true);
    connect(reply, &QNetworkReply::finished, this, [this, reply, taskId]() {
        handleTaskStatusReply(reply, taskId);
    });
}

void TaskManager::handleTaskStatusReply(QNetworkReply *reply, int taskId)
{
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
        m_taskRepo->updateTaskTiming(taskId, m_taskTimeOffset, false, true);
        m_activeTaskId = -1;
        m_isTaskPaused = false;
        emit activeTaskChanged();
        emit taskPausedChanged();
    }

    emit taskStatusChanged(taskId, dbStatus);
    m_taskRepo->updateTaskStatus(taskId, dbStatus);
    emit taskListChanged();
    reply->deleteLater();
}

QString TaskManager::getTaskName(int taskId)
{
    return m_taskRepo ? m_taskRepo->getTaskName(taskId) : "Unknown Task";
}

bool TaskManager::isTaskExpired(int taskId)
{
    Q_UNUSED(taskId);
    // Diperiksa otomatis di TaskRepository::getTasksForUser
    return false;
}

int TaskManager::getPendingStartedTaskCount()
{
    int userId = m_authManager->currentUserId();
    return m_taskRepo ? m_taskRepo->getPendingStartedTaskCount(userId) : 0;
}

void TaskManager::revertTaskChange()
{
    if (m_taskRepo && m_activeTaskId != -1) {
        m_taskRepo->updateTaskTiming(m_activeTaskId, m_taskTimeOffset, true, true);
        m_isTaskPaused = true;
    }
    emit taskListChanged();
    emit taskPausedChanged();
    emit trackingActiveChanged();
    emit activeTaskChanged();
}

void TaskManager::setMaxTimeForTask(int taskId)
{
    Q_UNUSED(taskId);
}

void TaskManager::checkTaskStatusBeforeStart()
{
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
