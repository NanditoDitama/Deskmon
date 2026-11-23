#include "ActivityTracker.h"
#include <QDateTime>
#include <QMetaObject>
#include <QPointer>
#include <QThreadPool>
#include <QDebug>

ActivityTracker::ActivityTracker(WorkLogRepository *workLogRepo,
                                 AuthManager *authManager,
                                 QObject *parent)
    : QObject(parent)
    , m_workLogRepo(workLogRepo)
    , m_authManager(authManager)
{
    if (m_authManager) {
        connect(m_authManager, &AuthManager::loggedIn, this, [this]() {
            ++m_sessionGeneration;
            m_isFirstCheck = true;
            setIsTrackingActive(true);
        });
        connect(m_authManager, &AuthManager::loggedOut, this, [this]() {
            ++m_sessionGeneration;
            m_isFirstCheck = true;
            m_lastActivityTime = 0;
            m_lastWindowInfo = WindowInfo();
            m_currentAppName.clear();
            m_currentWindowTitle.clear();
            setIsTrackingActive(false);
            emit currentAppNameChanged();
            emit currentWindowTitleChanged();
        });
    }
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
    if (!m_isTrackingActive || isTaskPaused || m_pollInFlight) {
        return;
    }

    int userId = m_authManager ? m_authManager->currentUserId() : -1;
    if (userId == -1 || !m_workLogRepo || !m_workLogRepo->ensureDatabaseOpen()) {
        return;
    }

    m_pollInFlight = true;
    QPointer<ActivityTracker> self(this);
    const quint64 gen = m_sessionGeneration;

    QThreadPool::globalInstance()->start([self, userId, gen]() {
        WindowInfo currentInfo = WindowInfoProvider::getActiveWindowInfo();
        if (!self) return;

        QMetaObject::invokeMethod(self, [self, userId, gen, currentInfo]() {
            if (!self) return;
            self->m_pollInFlight = false;

            if (self->m_sessionGeneration != gen ||
                !self->m_isTrackingActive ||
                !self->m_authManager ||
                self->m_authManager->currentUserId() != userId ||
                !self->m_workLogRepo ||
                !self->m_workLogRepo->ensureDatabaseOpen()) {
                return;
            }

            qint64 currentTime = QDateTime::currentSecsSinceEpoch();

            if (self->m_isFirstCheck) {
                self->m_lastWindowInfo = currentInfo;
                self->m_lastActivityTime = currentTime;
                self->m_isFirstCheck = false;
            } else if (currentInfo.appName != self->m_lastWindowInfo.appName ||
                       currentInfo.title != self->m_lastWindowInfo.title) {
                self->m_workLogRepo->logWindowChange(self->m_lastWindowInfo, self->m_lastActivityTime, currentTime - 1, userId);
                self->m_lastActivityTime = currentTime;
                self->m_lastWindowInfo = currentInfo;
                emit self->windowChanged(currentInfo);
            }

            const bool appChanged = (self->m_currentAppName != currentInfo.appName);
            const bool titleChanged = (self->m_currentWindowTitle != currentInfo.title);
            if (appChanged) {
                self->m_currentAppName = currentInfo.appName;
                emit self->currentAppNameChanged();
                emit self->currentAppIconPathChanged();
            }
            if (titleChanged) {
                self->m_currentWindowTitle = currentInfo.title;
                emit self->currentWindowTitleChanged();
            }
        }, Qt::QueuedConnection);
    });
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
    emit refreshRequested();
}
