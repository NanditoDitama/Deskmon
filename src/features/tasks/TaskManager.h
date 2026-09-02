#ifndef TASKMANAGER_H
#define TASKMANAGER_H

#include <QObject>
#include <QVariantList>
#include <QTimer>
#include <QDateTime>
#include "core/network/ApiClient.h"
#include "core/database/ProductivityAppRepository.h"
#include "features/auth/AuthManager.h"

class TaskManager : public QObject
{
    Q_OBJECT
public:
    explicit TaskManager(ApiClient *apiClient,
                         ProductivityAppRepository *prodRepo,
                         AuthManager *authManager,
                         QObject *parent = nullptr);
    ~TaskManager() override = default;

    QVariantList taskList() const;
    int activeTaskId() const { return m_activeTaskId; }
    bool isTaskPaused() const { return m_isTaskPaused; }
    qint64 globalTimeUsage() const { return m_globalTimeUsage; }
    void startGlobalTimer();

    void fetchAndStoreTasks();
    void syncActiveTask();
    void refreshTasks();
    void setActiveTask(int taskId);
    void finishTask(int taskId);
    void toggleTaskPause();
    void updateTaskStatus(int taskId);
    void revertTaskChange();
    void setMaxTimeForTask(int taskId);
    void checkTaskStatusBeforeStart();

    void submitTaskDetails(int taskId, const QString &details, const QString &action, int nextTaskId = -1);
    void taskDetailsDialogClosed(const QString &action);
    bool isTaskExpired(int taskId);
    int getPendingStartedTaskCount();
    QString getTaskName(int taskId);

    void sendPing(int taskId);
    void startPingTimer();
    void stopPingTimer();
    void sendPausePlayDataToAPI(int taskId, const QString& startTime, const QString& endTime, const QString& status);

signals:
    void taskListChanged();
    void activeTaskChanged();
    void taskPausedChanged();
    void globalTimeUsageChanged();
    void trackingActiveChanged();
    void taskStatusChanged(int taskId, const QString &newStatus);
    void taskReviewNotification(const QString &message);
    void showNotification(const QString &type, const QString &message);
    void showPingErrorDialog(const QString &message);
    void hidePingErrorDialog();
    void requestTaskDetails(int taskId, const QString &action, int nextTaskId);
    void taskDetailsSubmissionSuccess();
    void taskDetailsSubmissionFailed(const QString &error);
    void readyToProceedWithQuit();
    void readyToProceedWithLogout();

private slots:
    void handleTaskFetchReply(QNetworkReply *reply);
    void handleTaskStatusReply(QNetworkReply *reply, int taskId);

private:
    void sendTaskDetailsToAPI(int taskId, const QString &details, const QString &action, int nextTaskId);

    ApiClient *m_apiClient;
    ProductivityAppRepository *m_prodRepo;
    AuthManager *m_authManager;

    int m_activeTaskId = -1;
    bool m_isTaskPaused = false;
    qint64 m_taskStartTime = 0;
    qint64 m_taskTimeOffset = 0;
    qint64 m_pauseStartTime = 0;
    qint64 m_globalTimeUsage = 0;
    QDateTime m_lastPlayStartTime;

    QTimer m_pingTimer;
    int m_pingRetryCount = 0;
    QTimer m_taskRefreshTimer;
};

#endif // TASKMANAGER_H
