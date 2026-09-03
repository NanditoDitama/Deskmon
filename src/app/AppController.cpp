#include "AppController.h"
#include <QApplication>
#include <QProcess>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QScopedPointer>
#include <QVersionNumber>
#include <QDebug>

AppController::AppController(QObject *parent)
    : QObject(parent)
    , m_apiClient(new ApiClient(this))
    , m_userRepo(new UserRepository(this))
    , m_taskRepo(new TaskRepository(this))
    , m_workTimeRepo(new WorkTimeRepository(this))
    , m_workLogRepo(new WorkLogRepository(this))
    , m_prodRepo(new ProductivityAppRepository(this))
    , m_authManager(new AuthManager(m_apiClient, m_userRepo, this))
    , m_profileManager(new UserProfileManager(m_userRepo, m_authManager, this))
    , m_workTimeTracker(new WorkTimeTracker(m_apiClient, m_workTimeRepo, m_authManager, this))
    , m_activityTracker(new ActivityTracker(m_workLogRepo, m_authManager, this))
    , m_taskManager(new TaskManager(m_apiClient, m_taskRepo, m_authManager, this))
    , m_prodStatsService(new ProductivityStatsService(m_apiClient, m_workLogRepo, m_prodRepo, m_authManager, this))
{
    DatabaseManager::instance().initialize();
    m_workLogRepo->initialize();
    m_prodRepo->initialize();

    setupConnections();

    // Check auto login
    if (m_authManager->tryAutoLogin()) {
        m_workTimeTracker->checkAndCreateNewDayRecord();
        m_workTimeTracker->loadWorkTimeData();
        m_workTimeTracker->startSyncTimer();
        m_prodRepo->updateProductivityCache(m_authManager->currentUserId());
        m_taskManager->checkTaskStatusBeforeStart();
        m_prodStatsService->startTimers();
    }
}

AppController::~AppController()
{
}

void AppController::setupConnections()
{
    // Activity Tracker Signals
    connect(m_activityTracker, &ActivityTracker::currentAppNameChanged, this, &AppController::currentAppNameChanged);
    connect(m_activityTracker, &ActivityTracker::currentWindowTitleChanged, this, &AppController::currentWindowTitleChanged);
    connect(m_activityTracker, &ActivityTracker::trackingActiveChanged, this, &AppController::trackingActiveChanged);
    connect(m_activityTracker, &ActivityTracker::currentAppIconPathChanged, this, &AppController::currentAppIconPathChanged);
    connect(m_activityTracker, &ActivityTracker::windowChanged, this, [this](const WindowInfo &) {
        emit logCountChanged();
        emit logContentChanged();
        emit productivityStatsChanged();
    });

    // Work Log Repo Signals
    connect(m_workLogRepo, &WorkLogRepository::logsChanged, this, [this]() {
        emit logCountChanged();
        emit logContentChanged();
        emit productivityStatsChanged();
    });

    // Productivity App Repo Signals
    connect(m_prodRepo, &ProductivityAppRepository::productivityAppsChanged, this, &AppController::productivityAppsChanged);
    connect(m_prodRepo, &ProductivityAppRepository::idleThresholdChanged, this, &AppController::idleThresholdChanged);

    // Auth Manager Signals
    connect(m_authManager, &AuthManager::currentUserIdChanged, this, &AppController::currentUserIdChanged);
    connect(m_authManager, &AuthManager::currentUsernameChanged, this, &AppController::currentUsernameChanged);
    connect(m_authManager, &AuthManager::currentUserEmailChanged, this, &AppController::currentUserEmailChanged);
    connect(m_authManager, &AuthManager::userEmailChanged, this, &AppController::userEmailChanged);
    connect(m_authManager, &AuthManager::authTokenChanged, this, &AppController::authTokenChanged);
    connect(m_authManager, &AuthManager::authTokenError, this, &AppController::authTokenError);
    connect(m_authManager, &AuthManager::showAuthTokenErrorWindow, this, &AppController::showAuthTokenErrorWindow);
    connect(m_authManager, &AuthManager::readyToProceedWithLogout, this, &AppController::readyToProceedWithLogout);

    connect(m_authManager, &AuthManager::loggedIn, this, [this]() {
        m_workTimeTracker->checkAndCreateNewDayRecord();
        m_workTimeTracker->loadWorkTimeData();
        m_workTimeTracker->startSyncTimer();
        m_prodRepo->updateProductivityCache(m_authManager->currentUserId());
        m_taskManager->startPingTimer();
        m_taskManager->checkTaskStatusBeforeStart();
        m_taskManager->fetchAndStoreTasks();
        m_prodStatsService->startTimers();
        fetchAndStoreProductivityApps();
    });

    connect(m_authManager, &AuthManager::loggedOut, this, [this]() {
        m_workTimeTracker->stopSyncTimer();
        m_taskManager->stopPingTimer();
        m_prodStatsService->stopTimers();
        m_prodStatsService->sendDailyUsageReport();
        emit taskListChanged();
        emit activeTaskChanged();
        emit taskPausedChanged();
    });

    // Profile Manager Signals
    connect(m_profileManager, &UserProfileManager::profileImageChanged, this, &AppController::profileImageChanged);

    // Work Time Tracker Signals
    connect(m_workTimeTracker, &WorkTimeTracker::workTimeElapsedSecondsChanged, this, &AppController::workTimeElapsedSecondsChanged);
    connect(m_workTimeTracker, &WorkTimeTracker::earlyLeaveReasonSubmitted, this, &AppController::earlyLeaveReasonSubmitted);

    // Task Manager Signals
    connect(m_taskManager, &TaskManager::taskListChanged, this, &AppController::taskListChanged);
    connect(m_taskManager, &TaskManager::activeTaskChanged, this, &AppController::activeTaskChanged);
    connect(m_taskManager, &TaskManager::taskPausedChanged, this, &AppController::taskPausedChanged);
    connect(m_taskManager, &TaskManager::globalTimeUsageChanged, this, &AppController::globalTimeUsageChanged);
    connect(m_taskManager, &TaskManager::trackingActiveChanged, this, &AppController::trackingActiveChanged);
    connect(m_taskManager, &TaskManager::taskStatusChanged, this, &AppController::taskStatusChanged);
    connect(m_taskManager, &TaskManager::taskReviewNotification, this, &AppController::taskReviewNotification);
    connect(m_taskManager, &TaskManager::showNotification, this, QOverload<const QString&, const QString&>::of(&AppController::showNotification));
    connect(m_taskManager, &TaskManager::showPingErrorDialog, this, &AppController::showPingErrorDialog);
    connect(m_taskManager, &TaskManager::hidePingErrorDialog, this, &AppController::hidePingErrorDialog);
    connect(m_taskManager, &TaskManager::requestTaskDetails, this, &AppController::requestTaskDetails);
    connect(m_taskManager, &TaskManager::taskDetailsSubmissionSuccess, this, &AppController::taskDetailsSubmissionSuccess);
    connect(m_taskManager, &TaskManager::taskDetailsSubmissionFailed, this, &AppController::taskDetailsSubmissionFailed);
    connect(m_taskManager, &TaskManager::readyToProceedWithQuit, this, &AppController::readyToProceedWithQuit);
    connect(m_taskManager, &TaskManager::readyToProceedWithLogout, this, &AppController::readyToProceedWithLogout);

    // Productivity Stats Signals
    connect(m_prodStatsService, &ProductivityStatsService::productivityStatsChanged, this, &AppController::productivityStatsChanged);
}

// Property Getters
QString AppController::appVersion() const {
#ifdef APP_VERSION
    return QStringLiteral(APP_VERSION);
#else
    return QStringLiteral("1.0.3.4");
#endif
}
QString AppController::currentAppName() const { return m_activityTracker->currentAppName(); }
QString AppController::currentWindowTitle() const { return m_activityTracker->currentWindowTitle(); }
int AppController::logCount() const { return m_workLogRepo->logCount(m_authManager->currentUserId()); }
QString AppController::logContent() const { return m_workLogRepo->logContent(m_authManager->currentUserId()); }
QVariantMap AppController::productivityStats() const { return m_prodStatsService->productivityStats(m_startDateFilter, m_endDateFilter); }
QVariantList AppController::taskList() const { return m_taskManager->taskList(); }
int AppController::activeTaskId() const { return m_taskManager->activeTaskId(); }
bool AppController::isTaskPaused() const { return m_taskManager->isTaskPaused(); }
qint64 AppController::globalTimeUsage() const { return m_taskManager->globalTimeUsage(); }
bool AppController::isTrackingActive() const { return m_activityTracker->isTrackingActive(); }
int AppController::currentUserId() const { return m_authManager->currentUserId(); }
QAbstractItemModel* AppController::productiveAppsModel() const { return m_prodRepo->productiveAppsModel(); }
QAbstractItemModel* AppController::nonProductiveAppsModel() const { return m_prodRepo->nonProductiveAppsModel(); }
QString AppController::authToken() const { return m_authManager->authToken(); }
QString AppController::userEmail() const { return m_authManager->userEmail(); }
QString AppController::currentUsername() const { return m_authManager->currentUsername(); }
QString AppController::currentUserEmail() const { return m_authManager->currentUserEmail(); }
int AppController::workTimeElapsedSeconds() const { return m_workTimeTracker->workTimeElapsedSeconds(); }
QSqlQueryModel* AppController::logModel() const { return m_workLogRepo->logModel(); }

// Auth & Profile Delegations
QString AppController::authenticate(const QString &loginInput, const QString &password) { return m_authManager->authenticate(loginInput, password); }
void AppController::logout() {
    m_workTimeTracker->saveWorkTimeData();
    m_workTimeTracker->sendWorkTimeToAPI();
    m_authManager->logout();
}
QString AppController::savedUsername() const { return m_authManager->savedUsername(); }
QString AppController::savedPassword() const { return m_authManager->savedPassword(); }
bool AppController::isUsernameTaken(const QString &username) { return m_authManager->isUsernameTaken(username); }
QString AppController::getUserPassword(const QString &username) { return m_authManager->getUserPassword(username); }
QString AppController::getUserDepartment(const QString &username) { return m_authManager->getUserDepartment(username); }
QString AppController::getUserEmail(const QString &username) { return m_authManager->getUserEmail(username); }
QString AppController::getUsernameById(int userId) const { return m_authManager->getUsernameById(userId); }
QString AppController::getCurrentUsername() const { return m_authManager->currentUsername(); }
QString AppController::getCurrentUserEmail() const { return m_authManager->currentUserEmail(); }
QString AppController::updateUserProfile(const QString &currentUsername, const QString &newUsername, const QString &newPassword) {
    return m_profileManager->updateUserProfile(currentUsername, newUsername, newPassword);
}
QString AppController::cropProfileImage(const QString &imagePath, qreal x, qreal y, qreal imageWidth, qreal imageHeight, qreal cropWidth, qreal cropHeight) {
    return m_profileManager->cropProfileImage(imagePath, x, y, imageWidth, imageHeight, cropWidth, cropHeight);
}
bool AppController::updateProfileImage(const QString &username, const QString &imagePath) {
    return m_profileManager->updateProfileImage(username, imagePath);
}
QString AppController::getProfileImagePath(const QString &username) {
    return m_profileManager->getProfileImagePath(username);
}
bool AppController::validateFilePath(const QString &filePath) {
    return m_profileManager->validateFilePath(filePath);
}

// Tasks Delegations
void AppController::fetchAndStoreTasks() { m_taskManager->fetchAndStoreTasks(); }
void AppController::refreshTasks() { m_taskManager->refreshTasks(); }
void AppController::setActiveTask(int taskId) { m_taskManager->setActiveTask(taskId); }
void AppController::finishTask(int taskId) { m_taskManager->finishTask(taskId); }
void AppController::toggleTaskPause() { m_taskManager->toggleTaskPause(); }
void AppController::updateTaskStatus(int taskId) { m_taskManager->updateTaskStatus(taskId); }
void AppController::submitTaskDetails(int taskId, const QString &details, const QString &action, int nextTaskId) {
    m_taskManager->submitTaskDetails(taskId, details, action, nextTaskId);
}
void AppController::taskDetailsDialogClosed(const QString &action) { m_taskManager->taskDetailsDialogClosed(action); }
bool AppController::isTaskExpired(int taskId) { return m_taskManager->isTaskExpired(taskId); }
int AppController::getPendingStartedTaskCount() { return m_taskManager->getPendingStartedTaskCount(); }
QString AppController::getTaskName(int taskId) { return m_taskManager->getTaskName(taskId); }
void AppController::sendPing(int taskId) { m_taskManager->sendPing(taskId); }
void AppController::sendPausePlayDataToAPI(int taskId, const QString& startTime, const QString& endTime, const QString& status) {
    m_taskManager->sendPausePlayDataToAPI(taskId, startTime, endTime, status);
}
void AppController::startPingTimer() { m_taskManager->startPingTimer(); }

// Work Time Delegations
void AppController::loadWorkTimeData() { m_workTimeTracker->loadWorkTimeData(); }
void AppController::checkAndCreateNewDayRecord() { m_workTimeTracker->checkAndCreateNewDayRecord(); }
void AppController::saveWorkTimeData() { m_workTimeTracker->saveWorkTimeData(); }
void AppController::sendWorkTimeToAPI() { m_workTimeTracker->sendWorkTimeToAPI(); }
void AppController::submitEarlyLeaveReason(const QString &reason) { m_workTimeTracker->submitEarlyLeaveReason(reason); }
int AppController::totalWorkSeconds() const { return m_workTimeTracker->totalWorkSeconds(); }

// Tracking & Logs Delegations
void AppController::logActiveWindow() { m_activityTracker->logActiveWindow(m_taskManager->isTaskPaused()); }
void AppController::logIdle(qint64 startTime, qint64 endTime) { m_activityTracker->logIdle(startTime, endTime); }
void AppController::startGlobalTimer() { m_taskManager->startGlobalTimer(); }
void AppController::setIdleThreshold(int seconds) { m_prodRepo->setIdleThreshold(seconds); }
int AppController::getIdleThreshold() const { return m_prodRepo->getIdleThreshold(); }
void AppController::showLogs() { m_workLogRepo->showLogs(m_authManager->currentUserId()); }
void AppController::clearLogFilter() {
    m_startDateFilter.clear();
    m_endDateFilter.clear();
    m_workLogRepo->clearLogFilter(m_authManager->currentUserId());
    emit logContentChanged();
    emit logCountChanged();
    emit productivityStatsChanged();
}
void AppController::setLogFilter(const QString &startDate, const QString &endDate) {
    m_startDateFilter = startDate;
    m_endDateFilter = endDate;
    m_workLogRepo->setLogFilter(startDate, endDate, m_authManager->currentUserId());
    emit logContentChanged();
    emit logCountChanged();
    emit productivityStatsChanged();
}
QString AppController::debugShowRawData() const { return m_workLogRepo->debugShowRawData(m_authManager->currentUserId()); }
QString AppController::formatDuration(int seconds) const {
    if (seconds < 60) {
        return QString("%1s").arg(seconds);
    } else if (seconds < 3600) {
        int minutes = seconds / 60;
        int secs = seconds % 60;
        return QString("%1m %2s").arg(minutes).arg(secs);
    } else {
        int hours = seconds / 3600;
        int mins = (seconds % 3600) / 60;
        int secs = seconds % 60;
        return QString("%1h %2m %3s").arg(hours).arg(mins).arg(secs);
    }
}

// Productivity & Apps Delegations
int AppController::calculateTodayProductiveSeconds() const { return m_prodStatsService->calculateTodayProductiveSeconds(); }
void AppController::sendProductiveTimeToAPI() { m_prodStatsService->sendProductiveTimeToAPI(); }
void AppController::sendDailyUsageReport() { m_prodStatsService->sendDailyUsageReport(); }
QVariantList AppController::getAvailableApps() const { return m_prodRepo->getAvailableApps(); }
QVariantList AppController::getProductivityApps() const { return m_prodRepo->getProductivityApps(m_authManager->currentUserId()); }

void AppController::addProductivityApp(const QString &appName, const QString &windowTitle, const QString &url, int productivityType) {
    if (m_prodRepo->addProductivityApp(appName, windowTitle, url, productivityType)) {
        sendProductivityAppToAPI(appName, windowTitle, url, productivityType);
        m_prodRepo->refreshProductivityModels(m_authManager->currentUserId());
        m_prodRepo->updateProductivityCache(m_authManager->currentUserId());
    }
}

void AppController::sendProductivityAppToAPI(const QString &appName, const QString &windowTitle, const QString &url, int productivityType) {
    int userId = m_authManager->currentUserId();
    if (userId == -1 || m_authManager->authToken().isEmpty()) return;

    QString status = (productivityType == 1) ? "productive" : (productivityType == 2 ? "non-productive" : "neutral");
    QJsonObject payload;
    payload["application_name"] = appName;
    payload["productivity_status"] = status;
    payload["user_id"] = userId;
    if (!windowTitle.isEmpty()) payload["process_name"] = windowTitle;
    if (!url.isEmpty()) payload["url"] = url;

    QNetworkReply *reply = m_apiClient->post(QUrl("https://deskmon.pranala-dt.co.id/api/app-request/store"), QJsonDocument(payload).toJson(), true);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
}

void AppController::fetchAndStoreProductivityApps() {
    int userId = m_authManager->currentUserId();
    if (userId == -1 || m_authManager->authToken().isEmpty()) return;

    QNetworkReply *reply = m_apiClient->get(QUrl("https://deskmon.pranala-dt.co.id/api/app-request/all"), true);
    connect(reply, &QNetworkReply::finished, this, [this, reply, userId]() {
        QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> replyPtr(reply);
        if (reply->error() != QNetworkReply::NoError) return;

        QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll());
        if (!jsonDoc.isObject()) return;

        QJsonObject jsonObj = jsonDoc.object();
        if (!jsonObj["success"].toBool()) return;

        QJsonArray appsArray = jsonObj["data"].toArray();
        m_prodRepo->storeProductivityAppsFromApi(appsArray, userId);
    });
}

int AppController::getAppProductivityType(const QString &appName, const QString &url) const {
    return m_prodRepo->getAppProductivityType(appName, url, m_authManager->currentUserId());
}

QVariantList AppController::getPendingApplicationRequests() {
    return m_prodRepo->getPendingApplicationRequests();
}

// General / System
void AppController::notify(const QString &type, const QString &message) {
    emit showNotification(type, message);
}

void AppController::launchMaintenanceTool() {
    QString appDir = QApplication::applicationDirPath();
    QString maintenanceToolPath = QDir(appDir).filePath("DeskmonTool.exe");

    if (!QFile::exists(maintenanceToolPath)) {
        QMessageBox::critical(nullptr, "Error", "File update (DeskmonTool.exe) tidak ditemukan.");
        return;
    }
    QProcess::startDetached(maintenanceToolPath);
    QApplication::quit();
}

void AppController::checkForUpdates() {
    const QString currentVersion = appVersion();
    QUrl url("https://raw.githubusercontent.com/NanditoDitama/DeskmonUpdateRepo/main/version.json");
    QNetworkReply *reply = m_apiClient->get(url, false);

    connect(reply, &QNetworkReply::finished, this, [this, reply, currentVersion]() {
        if (reply->error() != QNetworkReply::NoError) {
            emit showNotification("error", "Gagal mengecek update: " + reply->errorString());
            reply->deleteLater();
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isObject()) {
            emit showNotification("warning", "Format file update di server tidak valid.");
            reply->deleteLater();
            return;
        }

        QJsonObject obj = doc.object();
        QString serverVersion = obj.value("version").toString();

        QVersionNumber serverVer = QVersionNumber::fromString(serverVersion);
        QVersionNumber currentVer = QVersionNumber::fromString(currentVersion);

        if (serverVer > currentVer) {
            QString notes = obj.value("releaseNotes").toString();
#ifdef Q_OS_MAC
            emit showStatusMessage(QStringLiteral("Update %1 tersedia.").arg(serverVersion));
#else
            emit updateAvailable(serverVersion, notes);
#endif
        } else {
            emit showNotification("success", "Aplikasi Anda sudah versi terbaru.");
        }
        reply->deleteLater();
    });
}

QString AppController::statusMessage() const {
    return m_statusMessage;
}

void AppController::refreshAll() {
    int userId = m_authManager->currentUserId();
    if (userId == -1) return;

    m_taskManager->fetchAndStoreTasks();
    fetchAndStoreProductivityApps();
    m_workTimeTracker->fetchWorkTimeFromAPI();
    m_activityTracker->logActiveWindow(m_taskManager->isTaskPaused());
    m_taskManager->syncActiveTask();

    emit taskListChanged();
    emit logContentChanged();
    emit logCountChanged();
    emit productivityStatsChanged();
    emit currentAppNameChanged();
    emit currentWindowTitleChanged();
    emit globalTimeUsageChanged();
    emit trackingActiveChanged();
    emit taskPausedChanged();
    emit activeTaskChanged();
}
