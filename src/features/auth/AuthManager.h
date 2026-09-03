#ifndef AUTHMANAGER_H
#define AUTHMANAGER_H

#include <QObject>
#include <QString>
#include "core/network/ApiClient.h"
#include "core/database/UserRepository.h"

class AuthManager : public QObject
{
    Q_OBJECT
public:
    explicit AuthManager(ApiClient *apiClient, UserRepository *userRepo, QObject *parent = nullptr);
    ~AuthManager() override = default;

    int currentUserId() const { return m_currentUserId; }
    QString currentUsername() const { return m_currentUsername; }
    QString currentUserEmail() const { return m_currentUserEmail; }
    QString userEmail() const { return m_userEmail; }
    QString authToken() const { return m_authToken; }

    bool tryAutoLogin();
    QString authenticate(const QString &loginInput, const QString &password);
    void logout();
    void sendLogoutToAPI();

    QString savedUsername() const;
    QString savedPassword() const;
    QString getUserEmail(const QString &username) const;
    QString getUserDepartment(const QString &username) const;
    QString getUsernameById(int userId) const;
    QString getUserPassword(const QString &username) const;
    bool isUsernameTaken(const QString &username) const;

    void setCurrentUserInfo(int userId, const QString &username, const QString &email, const QString &token);
    void showAuthTokenErrorMessage();
    void clearToken();

signals:
    void currentUserIdChanged();
    void currentUsernameChanged();
    void currentUserEmailChanged();
    void userEmailChanged();
    void authTokenChanged();
    void authTokenError(const QString &message);
    void showAuthTokenErrorWindow(const QString &message);
    void readyToProceedWithLogout();
    void loggedIn();
    void loggedOut();

private:
    ApiClient *m_apiClient;
    UserRepository *m_userRepo;

    int m_currentUserId = -1;
    QString m_currentUsername;
    QString m_currentUserEmail;
    QString m_userEmail;
    QString m_authToken;
    bool m_isTokenErrorVisible = false;
};

#endif // AUTHMANAGER_H
