#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H

#include <QObject>
#include <QSqlQueryModel>
#include <QVariantMap>
#include <QVariantList>
#include <QString>
#include <QTimer>

#include "core/network/ApiClient.h"
#include "core/database/DatabaseManager.h"
#include "core/database/UserRepository.h"
#include "core/database/TaskRepository.h"
#include "core/database/WorkTimeRepository.h"
#include "core/database/WorkLogRepository.h"
#include "core/database/ProductivityAppRepository.h"
#include "features/auth/AuthManager.h"
#include "features/worktime/WorkTimeTracker.h"
#include "features/tracking/ActivityTracker.h"
#include "features/tasks/TaskManager.h"
#include "features/productivity/ProductivityStatsService.h"
#include "features/updater/UpdateManager.h"

class AppController : public QObject
{
    Q_OBJECT

    // QML Properties (persis sama dengan Logger lama)
    Q_PROPERTY(QString currentAppName READ currentAppName NOTIFY currentAppNameChanged)
    Q_PROPERTY(QString currentWindowTitle READ currentWindowTitle NOTIFY currentWindowTitleChanged)
    Q_PROPERTY(QString currentWindowUrl READ currentWindowUrl NOTIFY currentWindowUrlChanged)
    Q_PROPERTY(int logCount READ logCount NOTIFY logCountChanged)
    Q_PROPERTY(QString logContent READ logContent NOTIFY logContentChanged)
    Q_PROPERTY(int todayLogCount READ todayLogCount NOTIFY logCountChanged)
    Q_PROPERTY(QString todayLogContent READ todayLogContent NOTIFY logContentChanged)
    Q_PROPERTY(QVariantMap productivityStats READ productivityStats NOTIFY productivityStatsChanged)
    Q_PROPERTY(QVariantList taskList READ taskList NOTIFY taskListChanged)
    Q_PROPERTY(QVariantList pendingNeedReviewTaskIds READ pendingNeedReviewTaskIds NOTIFY pendingNeedReviewTasksChanged)
    Q_PROPERTY(int activeTaskId READ activeTaskId NOTIFY activeTaskChanged)
    Q_PROPERTY(bool isTaskPaused READ isTaskPaused NOTIFY taskPausedChanged)
    Q_PROPERTY(qint64 globalTimeUsage READ globalTimeUsage NOTIFY globalTimeUsageChanged)
    Q_PROPERTY(bool isTrackingActive READ isTrackingActive NOTIFY trackingActiveChanged)
    Q_PROPERTY(int currentUserId READ currentUserId NOTIFY currentUserIdChanged)
    Q_PROPERTY(QAbstractItemModel* productiveAppsModel READ productiveAppsModel NOTIFY productivityAppsChanged)
    Q_PROPERTY(QAbstractItemModel* nonProductiveAppsModel READ nonProductiveAppsModel NOTIFY productivityAppsChanged)

    Q_PROPERTY(QString authToken READ authToken NOTIFY authTokenChanged)
    Q_PROPERTY(QString userEmail READ userEmail NOTIFY userEmailChanged)
    Q_PROPERTY(QString currentUsername READ currentUsername NOTIFY currentUsernameChanged)
    Q_PROPERTY(QString currentUserEmail READ currentUserEmail NOTIFY currentUserEmailChanged)

    Q_PROPERTY(int workTimeElapsedSeconds READ workTimeElapsedSeconds NOTIFY workTimeElapsedSecondsChanged)
    Q_PROPERTY(QSqlQueryModel* logModel READ logModel CONSTANT)
    Q_PROPERTY(QString appVersion READ appVersion CONSTANT)
    Q_PROPERTY(QString updateState READ updateState NOTIFY updateStateChanged)
    Q_PROPERTY(int updateProgress READ updateProgress NOTIFY updateProgressChanged)
    Q_PROPERTY(QString availableUpdateVersion READ availableUpdateVersion NOTIFY availableUpdateVersionChanged)
    Q_PROPERTY(QString updateReleaseNotes READ updateReleaseNotes NOTIFY updateReleaseNotesChanged)
    Q_PROPERTY(QString updateReleasePageUrl READ updateReleasePageUrl NOTIFY updateReleasePageUrlChanged)
    Q_PROPERTY(QString updateErrorMessage READ updateErrorMessage NOTIFY updateErrorMessageChanged)
    Q_PROPERTY(bool isUpdateBusy READ isUpdateBusy NOTIFY updateStateChanged)
    Q_PROPERTY(bool isUpdateReadyToRestart READ isUpdateReadyToRestart NOTIFY updateStateChanged)

public:
    explicit AppController(QObject *parent = nullptr);
    ~AppController() override;

    // Property Getters
    QString appVersion() const;
    QString currentAppName() const;
    QString currentWindowTitle() const;
    QString currentWindowUrl() const;
    int logCount() const;
    QString logContent() const;
    int todayLogCount() const;
    QString todayLogContent() const;
    QVariantMap productivityStats() const;
    QVariantList taskList() const;
    QVariantList pendingNeedReviewTaskIds() const;
    int activeTaskId() const;
    bool isTaskPaused() const;
    qint64 globalTimeUsage() const;
    bool isTrackingActive() const;
    int currentUserId() const;
    QAbstractItemModel* productiveAppsModel() const;
    QAbstractItemModel* nonProductiveAppsModel() const;
    QString authToken() const;
    QString userEmail() const;
    QString currentUsername() const;
    QString currentUserEmail() const;
    int workTimeElapsedSeconds() const;
    QSqlQueryModel* logModel() const;

    // Q_INVOKABLE Methods (Auth & Profile)
    Q_INVOKABLE QString authenticate(const QString &loginInput, const QString &password);
    Q_INVOKABLE void logout();
    Q_INVOKABLE QString savedUsername() const;
    Q_INVOKABLE QString savedPassword() const;
    Q_INVOKABLE bool isUsernameTaken(const QString &username);
    Q_INVOKABLE QString getUserPassword(const QString &username);
    Q_INVOKABLE QString getUserDepartment(const QString &username);
    Q_INVOKABLE QString getUserEmail(const QString &username);
    Q_INVOKABLE QString getUsernameById(int userId) const;
    Q_INVOKABLE QString getCurrentUsername() const;
    Q_INVOKABLE QString getCurrentUserEmail() const;

    // Q_INVOKABLE Methods (Tasks)
    Q_INVOKABLE void fetchAndStoreTasks();
    Q_INVOKABLE void refreshTasks();
    Q_INVOKABLE void setActiveTask(int taskId);
    Q_INVOKABLE void finishTask(int taskId);
    Q_INVOKABLE void toggleTaskPause();
    Q_INVOKABLE void updateTaskStatus(int taskId);
    Q_INVOKABLE void revertTaskChange();
    Q_INVOKABLE void submitTaskDetails(int taskId, const QString &details, const QString &action, int nextTaskId = -1);
    Q_INVOKABLE void taskDetailsDialogClosed(const QString &action);
    Q_INVOKABLE void submitNeedReview(int taskId, const QString &reason);
    Q_INVOKABLE bool isTaskExpired(int taskId);
    Q_INVOKABLE int getPendingStartedTaskCount();
    Q_INVOKABLE QString getTaskName(int taskId);
    Q_INVOKABLE void sendPing(int taskId);
    Q_INVOKABLE void sendPausePlayDataToAPI(int taskId, const QString& startTime, const QString& endTime, const QString& status);
    Q_INVOKABLE void startPingTimer();

    // Q_INVOKABLE Methods (Work Time)
    Q_INVOKABLE void loadWorkTimeData();
    Q_INVOKABLE void checkAndCreateNewDayRecord();
    Q_INVOKABLE void saveWorkTimeData();
    Q_INVOKABLE void sendWorkTimeToAPI();
    Q_INVOKABLE void submitEarlyLeaveReason(const QString &reason);
    Q_INVOKABLE int totalWorkSeconds() const;

    // Q_INVOKABLE Methods (Tracking & Logs)
    Q_INVOKABLE void logActiveWindow();
    Q_INVOKABLE void logIdle(qint64 startTime, qint64 endTime);
    Q_INVOKABLE void startGlobalTimer();
    Q_INVOKABLE void setIdleThreshold(int seconds);
    Q_INVOKABLE int getIdleThreshold() const;
    Q_INVOKABLE void showLogs();
    Q_INVOKABLE void clearLogFilter();
    Q_INVOKABLE void setLogFilter(const QString &startDate, const QString &endDate);
    Q_INVOKABLE QString debugShowRawData() const;
    Q_INVOKABLE QString formatDuration(int seconds) const;

    // Q_INVOKABLE Methods (Productivity Apps & Stats)
    Q_INVOKABLE int calculateTodayProductiveSeconds() const;
    Q_INVOKABLE void sendProductiveTimeToAPI();
    Q_INVOKABLE void sendDailyUsageReport();
    Q_INVOKABLE QVariantList getAvailableApps() const;
    Q_INVOKABLE QVariantList getProductivityApps() const;
    Q_INVOKABLE void addProductivityApp(const QString &appName, const QString &windowTitle, const QString &url, int productivityType);
    Q_INVOKABLE void sendProductivityAppToAPI(const QString &appName, const QString &windowTitle, const QString &url, int productivityType);
    Q_INVOKABLE void fetchAndStoreProductivityApps();
    Q_INVOKABLE int getAppProductivityType(const QString &appName, const QString &url) const;
    Q_INVOKABLE QVariantList getPendingApplicationRequests();

    // Q_INVOKABLE Methods (General / System)
    Q_INVOKABLE void notify(const QString &type, const QString &message);
    Q_INVOKABLE void launchMaintenanceTool();
    Q_INVOKABLE void checkForUpdates();
    Q_INVOKABLE void checkForUpdatesSilent();
    Q_INVOKABLE void restartToApplyUpdate();
    Q_INVOKABLE void openUpdateReleasePage();
    Q_INVOKABLE QString apiBaseUrl() const;
    QString statusMessage() const;

    QString updateState() const;
    int updateProgress() const;
    QString availableUpdateVersion() const;
    QString updateReleaseNotes() const;
    QString updateReleasePageUrl() const;
    QString updateErrorMessage() const;
    bool isUpdateBusy() const;
    bool isUpdateReadyToRestart() const;

    void armApplyUpdateOnQuitIfReady();
    bool launchArmedExitUpdateHelper();

    static QString resolveInstanceKey();

public slots:
    void refreshAll();

signals:
    void currentAppNameChanged();
    void currentWindowTitleChanged();
    void currentWindowUrlChanged();
    void logCountChanged();
    void logContentChanged();
    void productivityStatsChanged();
    void taskListChanged();
    void pendingNeedReviewTasksChanged();
    void activeTaskChanged();
    void taskPausedChanged();
    void globalTimeUsageChanged();
    void trackingActiveChanged();
    void idleThresholdChanged();
    void currentUserIdChanged();
    void productivityAppsChanged();
    void loginCompleted(bool success, const QString &message);
    void authTokenChanged();
    void userEmailChanged();
    void currentUsernameChanged();
    void currentUserEmailChanged();
    void authTokenError(const QString& message);
    void taskStatusChanged(int taskId, const QString& newStatus);
    void taskReviewNotification(const QString& message);
    void taskTimeUpdated(int taskId, int timeUsage);
    void showNotification(const QString &message);
    void showNotification(const QString &type, const QString &message);
    void workTimeElapsedSecondsChanged();
    void showTimeWarning(const QString &message);
    void earlyLeaveReasonSubmitted();
    void earlyLeaveSubmitFailed(const QString &errorMessage);
    void showStatusMessage(const QString &message);
    void statusMessageChanged();
    void updateAvailable(const QString &newVersion, const QString &releaseNotes);
    void updateFailed(const QString &toastSummary);
    void updateStateChanged();
    void updateProgressChanged();
    void availableUpdateVersionChanged();
    void updateReleaseNotesChanged();
    void updateReleasePageUrlChanged();
    void updateErrorMessageChanged();
    void updateReadyToRestart(const QString &newVersion);
    void requestLoginPage();
    void currentAppIconPathChanged();
    void showPingErrorDialog(const QString &message);
    void hidePingErrorDialog();
    void showAuthTokenErrorWindow(const QString &message);
    void requestTaskDetails(int taskId, const QString &action, int nextTaskId);
    void taskDetailsSubmissionSuccess();
    void taskDetailsSubmissionFailed(const QString &error);
    void readyToProceedWithQuit();
    void taskDetailsSubmissionFailedWithRetry(const QString &errorMessage, int taskId, const QString &details, const QString &action, int nextTaskId);
    void readyToProceedWithLogout();

private:
    void setupConnections();

    ApiClient *m_apiClient;
    UserRepository *m_userRepo;
    TaskRepository *m_taskRepo;
    WorkTimeRepository *m_workTimeRepo;
    WorkLogRepository *m_workLogRepo;
    ProductivityAppRepository *m_prodRepo;
    AuthManager *m_authManager;
    WorkTimeTracker *m_workTimeTracker;
    ActivityTracker *m_activityTracker;
    TaskManager *m_taskManager;
    ProductivityStatsService *m_prodStatsService;
    UpdateManager *m_updateManager;

    QString m_startDateFilter;
    QString m_endDateFilter;
    QString m_statusMessage;
};

#endif // APPCONTROLLER_H
