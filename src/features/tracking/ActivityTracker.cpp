#include "ActivityTracker.h"
#include <QDateTime>
#include <QDebug>

ActivityTracker::ActivityTracker(WorkLogRepository *workLogRepo,
                                 AuthManager *authManager,
                                 QObject *parent)
    : QObject(parent)
    , m_workLogRepo(workLogRepo)
    , m_authManager(authManager)
{
}

void ActivityTracker::setIsTrackingActive(bool active)
{
    if (m_isTrackingActive != active) {
        m_isTrackingActive = active;
        emit trackingActiveChanged();
    }
}

void ActivityTracker::logActiveWindow(bool isTaskPaused)
{
    if (!m_isTrackingActive || isTaskPaused) {
        return;
    }

    int userId = m_authManager->currentUserId();
    if (userId == -1 || !m_workLogRepo->ensureDatabaseOpen()) {
        return;
    }

    qint64 currentTime = QDateTime::currentSecsSinceEpoch();
    WindowInfo currentInfo = WindowInfoProvider::getActiveWindowInfo();

    if (m_isFirstCheck) {
        m_lastWindowInfo = currentInfo;
        m_lastActivityTime = currentTime;
        m_isFirstCheck = false;
        return;
    }

    if (currentInfo.appName != m_lastWindowInfo.appName ||
        currentInfo.title != m_lastWindowInfo.title) {
        m_workLogRepo->logWindowChange(m_lastWindowInfo, m_lastActivityTime, currentTime - 1, userId);
        m_lastActivityTime = currentTime;
        m_lastWindowInfo = currentInfo;
        emit windowChanged(currentInfo);
    }

    m_currentAppName = currentInfo.appName;
    m_currentWindowTitle = currentInfo.title;
    emit currentAppNameChanged();
    emit currentWindowTitleChanged();
    emit currentAppIconPathChanged();
}

void ActivityTracker::logIdle(qint64 startTime, qint64 endTime)
{
    int userId = m_authManager->currentUserId();
    if (userId == -1 || !m_workLogRepo->ensureDatabaseOpen()) {
        return;
    }

    WindowInfo idleInfo;
    idleInfo.appName = "Idle";
    idleInfo.title = "No active window";
    idleInfo.url = "";

    m_workLogRepo->logWindowChange(idleInfo, startTime, endTime, userId);
}
