#ifndef WORKTIMETRACKER_H
#define WORKTIMETRACKER_H

#include <QObject>
#include <QTimer>
#include "core/network/ApiClient.h"
#include "core/database/WorkTimeRepository.h"
#include "features/auth/AuthManager.h"

class WorkTimeTracker : public QObject
{
    Q_OBJECT
public:
    explicit WorkTimeTracker(ApiClient *apiClient,
                             WorkTimeRepository *workTimeRepo,
                             AuthManager *authManager,
                             QObject *parent = nullptr);
    ~WorkTimeTracker() override;

    int workTimeElapsedSeconds() const { return m_workTimeElapsedSeconds; }
    int totalWorkSeconds() const { return 9 * 60 * 60; }

    void loadWorkTimeData();
    void saveWorkTimeData();
    void sendWorkTimeToAPI();
    void fetchWorkTimeFromAPI();
    void checkAndCreateNewDayRecord();
    void submitEarlyLeaveReason(const QString &reason);

    void startSyncTimer();
    void stopSyncTimer();

signals:
    void workTimeElapsedSecondsChanged();
    void earlyLeaveReasonSubmitted();

private:
    ApiClient *m_apiClient;
    WorkTimeRepository *m_workTimeRepo;
    AuthManager *m_authManager;

    int m_workTimeElapsedSeconds = 0;
    QTimer m_apiWorkTimeTimer;
};

#endif // WORKTIMETRACKER_H
