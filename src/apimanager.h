#ifndef APIMANAGER_H
#define APIMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

class ApiManager : public QObject
{
    Q_OBJECT
public:
    explicit ApiManager(QObject *parent = nullptr);

    void setAuthToken(const QString &token);
    QString authToken() const { return m_authToken; }

    QNetworkReply* post(const QString &url, const QJsonObject &payload);
    QNetworkReply* get(const QString &url);

    QNetworkAccessManager* networkManager() const { return m_networkManager; }

    // Network Methods
    void fetchTasks(int userId);
    void fetchProductivityApps(int userId);
    void submitWorkTime(int userId, int elapsedSeconds);
    void logout();
    void fetchWorkTime(int userId);
    void submitEarlyLeaveReason(const QString &email, const QString &reason);
    void submitEarlyLeaveReason(const QString &email, const QString &reason, const QString &token);
    void submitTaskDetails(int taskId, const QString &details, const QString &action, int nextTaskId = -1);
    void sendLogout(const QString &token);
    void sendDailyUsageReport(const QJsonArray &data, const QString &token);

signals:
    void errorOccurred(const QString &message);
    void tasksFetched(const QJsonArray &tasks);
    void productivityAppsFetched(const QJsonArray &apps);
    void workTimeFetched(int seconds);
    void logoutFinished(bool success);
    void earlyLeaveSubmitted(bool success, const QString &message);
    void earlyLeaveReasonSubmitted();
    void taskDetailsSubmitted(bool success, const QString &message);

private:
    QNetworkAccessManager *m_networkManager;
    QString m_authToken;
};

#endif // APIMANAGER_H
