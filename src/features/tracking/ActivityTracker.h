#ifndef ACTIVITYTRACKER_H
#define ACTIVITYTRACKER_H

#include <QObject>
#include <QString>
#include "core/platform/WindowInfoProvider.h"
#include "core/database/WorkLogRepository.h"
#include "features/auth/AuthManager.h"

class ActivityTracker : public QObject
{
    Q_OBJECT
public:
    explicit ActivityTracker(WorkLogRepository *workLogRepo,
                             AuthManager *authManager,
                             QObject *parent = nullptr);
    ~ActivityTracker() override = default;

    QString currentAppName() const { return m_currentAppName; }
    QString currentWindowTitle() const { return m_currentWindowTitle; }
    bool isTrackingActive() const { return m_isTrackingActive; }
    void setIsTrackingActive(bool active);

    void logActiveWindow(bool isTaskPaused = false);
    void logIdle(qint64 startTime, qint64 endTime);

signals:
    void currentAppNameChanged();
    void currentWindowTitleChanged();
    void trackingActiveChanged();
    void currentAppIconPathChanged();
    void windowChanged(const WindowInfo &info);

private:
    WorkLogRepository *m_workLogRepo;
    AuthManager *m_authManager;

    QString m_currentAppName;
    QString m_currentWindowTitle;
    WindowInfo m_lastWindowInfo;
    qint64 m_lastActivityTime = 0;
    bool m_isFirstCheck = true;
    bool m_isTrackingActive = true;
};

#endif // ACTIVITYTRACKER_H
