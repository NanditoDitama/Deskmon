#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QTimer>
#include <QSqlQueryModel>

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QEventLoop>


class Logger : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString currentAppName READ currentAppName NOTIFY currentAppNameChanged)
    Q_PROPERTY(QString currentWindowTitle READ currentWindowTitle NOTIFY currentWindowTitleChanged)
    Q_PROPERTY(int logCount READ logCount NOTIFY logCountChanged)
    Q_PROPERTY(QString logContent READ logContent NOTIFY logContentChanged)
    Q_PROPERTY(QVariantMap productivityStats READ productivityStats NOTIFY productivityStatsChanged)
    Q_PROPERTY(QVariantList taskList READ taskList NOTIFY taskListChanged)
    Q_PROPERTY(int activeTaskId READ activeTaskId NOTIFY activeTaskChanged)
    Q_PROPERTY(bool isTaskPaused READ isTaskPaused NOTIFY taskPausedChanged)
    Q_PROPERTY(qint64 globalTimeUsage READ globalTimeUsage NOTIFY globalTimeUsageChanged)
    Q_PROPERTY(bool isTrackingActive READ isTrackingActive NOTIFY trackingActiveChanged) // Tambah properti
    Q_PROPERTY(int currentUserId READ currentUserId NOTIFY currentUserIdChanged)
    Q_PROPERTY(QAbstractItemModel* productiveAppsModel READ productiveAppsModel NOTIFY productivityAppsChanged)
    Q_PROPERTY(QAbstractItemModel* nonProductiveAppsModel READ nonProductiveAppsModel NOTIFY productivityAppsChanged)

    Q_PROPERTY(QString authToken READ authToken NOTIFY authTokenChanged)
    Q_PROPERTY(QString userEmail READ userEmail NOTIFY userEmailChanged)
    Q_PROPERTY(QString currentUsername READ currentUsername NOTIFY currentUsernameChanged)
    Q_PROPERTY(QString currentUserEmail READ currentUserEmail NOTIFY currentUserEmailChanged)


public:
    explicit Logger(QObject *parent = nullptr);
    ~Logger();

    struct WindowInfo {
        QString appName;
        QString title;
    };
    QString currentAppName() const;
    QString currentWindowTitle() const;
    int logCount() const;
    QString logContent() const;
    QVariantMap productivityStats() const;
    QVariantList taskList() const;
    int activeTaskId() const { return m_activeTaskId; }
    bool isTaskPaused() const { return m_isTaskPaused; }
    qint64 globalTimeUsage() const { return m_globalTimeUsage; }
    bool isTrackingActive() const { return m_isTrackingActive; } // Tambah getter
    int getIdleThreshold() const;
    int currentUserId() const { return m_currentUserId; }
    QString getUsernameById(int userId) const;
    QString getTaskName(int taskId);


    // Fungsi lainnya tetap sama
    Q_INVOKABLE QString debugShowRawData() const;
    Q_INVOKABLE void showLogs();
    Q_INVOKABLE bool isUsernameTaken(const QString &username);
    Q_INVOKABLE QString updateUserProfile(const QString &currentUsername, const QString &newUsername,
                                          const QString &newPassword);

    Q_INVOKABLE QString cropProfileImage(const QString &imagePath, qreal x, qreal y,
                                         qreal imageWidth, qreal imageHeight,
                                         qreal cropWidth, qreal cropHeight);
    Q_INVOKABLE void clearLogFilter();
    Q_INVOKABLE bool validateFilePath(const QString &filePath);
    Q_INVOKABLE void setLogFilter(const QString &startDate, const QString &endDate);
    Q_INVOKABLE bool updateProfileImage(const QString &username, const QString &imagePath);
    Q_INVOKABLE QString getProfileImagePath(const QString &username);
    Q_INVOKABLE void setActiveTask(int taskId);
    Q_INVOKABLE void finishTask(int taskId);
    Q_INVOKABLE void toggleTaskPause();
    Q_INVOKABLE QString formatDuration(int seconds) const;
    Q_INVOKABLE void startGlobalTimer();
    Q_INVOKABLE QString getUserPassword(const QString &username);
    Q_INVOKABLE void setIdleThreshold(int seconds);
    Q_INVOKABLE QVariantList getAvailableApps() const;
    Q_INVOKABLE void addProductivityApp(const QString &appName,
                                        const QString &windowTitle,
                                        int productivityType);
    Q_INVOKABLE QVariantList getProductivityApps() const;

    QAbstractItemModel* productiveAppsModel() const { return m_productiveAppsModel; }
    QAbstractItemModel* nonProductiveAppsModel() const { return m_nonProductiveAppsModel; }

    Q_INVOKABLE bool authenticate(const QString &email, const QString &password);
    QString authToken() const { return m_authToken; }
    QString userEmail() const { return m_userEmail; }

    QString currentUsername() const { return m_currentUsername; }
    QString currentUserEmail() const { return m_currentUserEmail; }
    Q_INVOKABLE QString getUserDepartment(const QString &username);
    Q_INVOKABLE QString getCurrentUsername() const { return m_currentUsername; }
    Q_INVOKABLE QString getCurrentUserEmail() const { return m_currentUserEmail; }
    Q_INVOKABLE QString getUserEmail(const QString &username);
    Q_INVOKABLE void fetchAndStoreTasks(); // Fungsi baru

    QString getCurrentToken() const;  // Untuk mendapatkan token saat ini
    void clearToken();
    Q_INVOKABLE void updateTaskStatus(int taskId);






public slots:
    void logActiveWindow();
    void logIdle(qint64 startTime, qint64 endTime);
    void updateTaskTime();
    void refreshAll();
    void handleTaskStatusReply(QNetworkReply *reply, int taskId);
    // Di bagian public slots:
    // Tambahkan di bagian public slots:
    Q_INVOKABLE QVariantList getPendingApplicationRequests();


private slots:
    void handleTaskFetchReply(QNetworkReply *reply);


signals:
    void currentAppNameChanged();
    void currentWindowTitleChanged();
    void logCountChanged();
    void logContentChanged();
    void productivityStatsChanged();
    void taskListChanged();
    void activeTaskChanged();
    void taskPausedChanged();
    void globalTimeUsageChanged();
    void trackingActiveChanged(); // Tambah sinyal
    void idleThresholdChanged();
    void currentUserIdChanged();
    void productivityAppsChanged();
    void loginCompleted(bool success, const QString &message); // Sinyal baru
    void authTokenChanged();
    void userEmailChanged();

    void currentUsernameChanged();
    void currentUserEmailChanged();
    void authTokenError(const QString& message);

   void profileImageChanged(const QString &username, const QString &newPath);
    void taskStatusChanged(int taskId, const QString& newStatus);
    void taskReviewNotification(const QString& message);

private:
    void syncActiveTask();
    void initializeDatabase();
    bool ensureDatabaseOpen() const;
    WindowInfo getActiveWindowInfo();
    void logWindowChange(const WindowInfo &info, qint64 startTime, qint64 endTime);
    QString hashPassword(const QString &password);
    void initializeProductivityDatabase();
    bool ensureProductivityDatabaseOpen() const;
    int getAppProductivityType(const QString &appName, const QString &windowTitle) const;
    void setMaxTimeForTask(int taskId);
    void checkTaskStatusBeforeStart();
    void migrateProductivityDatabase();
    QSqlQueryModel* m_productiveAppsModel; // Tambahkan model
    QSqlQueryModel* m_nonProductiveAppsModel;

    QNetworkAccessManager *m_networkManager;
    QString m_authToken;
    QString m_userEmail;


    qint64 m_taskStartTime = 0;
    qint64 m_taskTimeOffset = 0;
    bool m_isTrackingActive = false; // Tambah variabel untuk status tracking
    int m_currentUserId = -1;

    QString m_currentUsername;
    QString m_currentUserEmail;

    void setCurrentUserInfo(int userId, const QString &username, const QString &email);


    void handleLoginResponse(QNetworkReply* reply, const QString& username);
    // Tambahkan member variables baru
    QDateTime m_lastPlayStartTime;
    QDateTime m_lastPauseStartTime;

    // Tambahkan method baru
    void sendPausePlayDataToAPI(int taskId, const QString& startTime,
                                const QString& endTime, const QString& status);

    QTimer m_pingTimer; // Timer untuk ping reguler
    void sendPing(int taskId);
    void startPingTimer(int taskId);
    void stopPingTimer();

#ifdef Q_OS_WIN
    WindowInfo getActiveWindowInfoWindows();
#else
    WindowInfo getActiveWindowInfoLinux();
#endif

    mutable QSqlDatabase m_db;
    mutable QSqlDatabase m_productivityDb;
    QString m_currentAppName;
    QString m_currentWindowTitle;
    WindowInfo m_lastWindowInfo;
    qint64 m_lastActivityTime;
    QString m_startDateFilter;
    QString m_endDateFilter;
    bool m_isFirstCheck = true;
    QTimer m_taskTimer;
    int m_activeTaskId = -1;
    bool m_isTaskPaused = false;
    qint64 m_pauseStartTime = 0;
    qint64 m_globalTimeUsage = 0;

};

#endif // LOGGER_H

