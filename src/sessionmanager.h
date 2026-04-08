#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlQuery>

class DataManager;

class SessionManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int currentUserId READ currentUserId NOTIFY currentUserIdChanged)
    Q_PROPERTY(QString currentUsername READ currentUsername NOTIFY currentUsernameChanged)
    Q_PROPERTY(QString currentUserEmail READ currentUserEmail NOTIFY currentUserEmailChanged)
    Q_PROPERTY(QString authToken READ authToken NOTIFY authTokenChanged)

public:
    explicit SessionManager(QObject *parent = nullptr);

    void setDataManager(DataManager *dm) { m_dataManager = dm; }

    void login(const QString &username, const QString &password);
    void logout();
    bool updateProfileImage(const QString &username, const QString &imagePath);
    QString getProfileImagePath(const QString &username);

    QString getSavedUsername();
    QString getSavedPassword();

    int currentUserId() const { return m_userId; }
    QString currentUsername() const { return m_username; }
    QString currentUserEmail() const { return m_email; }
    QString authToken() const { return m_token; }

signals:
    void loginResult(bool success, const QString &message, int userId, const QString &username, const QString &email, const QString &token);
    void logoutFinished();
    void profileImageChanged(const QString &username, const QString &newPath);
    void currentUserIdChanged();
    void currentUsernameChanged();
    void currentUserEmailChanged();
    void authTokenChanged();

private:
    void setCurrentUser(int id, const QString &username, const QString &email, const QString &token);
    void clearSession();

    int m_userId = -1;
    QString m_username;
    QString m_email;
    QString m_token;
    DataManager *m_dataManager = nullptr;
};

#endif // SESSIONMANAGER_H
