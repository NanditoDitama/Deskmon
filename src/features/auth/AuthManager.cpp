#include "AuthManager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QEventLoop>
#include <QTimer>
#include <QDebug>

AuthManager::AuthManager(ApiClient *apiClient, UserRepository *userRepo, QObject *parent)
    : QObject(parent)
    , m_apiClient(apiClient)
    , m_userRepo(userRepo)
{
}

bool AuthManager::tryAutoLogin()
{
    int userId = -1;
    QString username, email, token;
    if (m_userRepo && m_userRepo->findUserWithToken(userId, username, email, token)) {
        setCurrentUserInfo(userId, username, email, token);
        qDebug() << "Auto login as user ID:" << userId << "Username:" << username;
        emit loggedIn();
        return true;
    }
    return false;
}

void AuthManager::setCurrentUserInfo(int userId, const QString &username, const QString &email, const QString &token)
{
    m_currentUserId = userId;
    m_currentUsername = username;
    m_currentUserEmail = email;
    m_userEmail = email;
    m_authToken = token;

    if (m_apiClient) {
        m_apiClient->setAuthToken(token);
    }

    emit currentUserIdChanged();
    emit currentUsernameChanged();
    emit currentUserEmailChanged();
    emit userEmailChanged();
    emit authTokenChanged();

    qDebug() << "Current user set - ID:" << userId << ", Username:" << username << ", Email:" << email;
}

QString AuthManager::authenticate(const QString &loginInput, const QString &password)
{
    bool isEmail = loginInput.contains("@");
    QString loginType = isEmail ? "email" : "username";
    qDebug() << "Attempting login with" << loginType << ":" << loginInput;

    QJsonObject jsonPayload;
    if (isEmail) {
        jsonPayload["email"] = loginInput;
    } else {
        QString email = m_userRepo ? m_userRepo->getEmail(loginInput) : "";
        jsonPayload["email"] = email.isEmpty() ? loginInput : email;
    }
    jsonPayload["password"] = password;
    QJsonDocument doc(jsonPayload);
    QByteArray data = doc.toJson();

    qDebug() << "Sending login request with payload:" << doc.toJson(QJsonDocument::Compact);

    QNetworkReply *reply = m_apiClient->post(QUrl("https://deskmon.pranala-dt.co.id/api/login"), data, false);
    QEventLoop loop;
    QTimer::singleShot(10000, &loop, &QEventLoop::quit);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (!reply->isFinished()) {
        reply->abort();
        reply->deleteLater();
        return "Koneksi ke server gagal atau timeout.\n Periksa koneksi internet Anda.";
    }

    QByteArray response = reply->readAll();
    QJsonDocument jsonResponse = QJsonDocument::fromJson(response);

    if (jsonResponse.isNull() || !jsonResponse.isObject()) {
        qWarning() << "Invalid JSON response. Network error:" << reply->errorString();
        reply->deleteLater();
        return "Koneksi ke server gagal. Periksa koneksi \n internet, username, dan pasword Anda.";
    }

    QJsonObject jsonObj = jsonResponse.object();
    qDebug() << "API Response:" << jsonResponse.toJson(QJsonDocument::Compact);

    if (jsonObj.contains("success") && jsonObj["success"].toBool()) {
        qDebug() << "API login successful. Storing user data...";

        QJsonObject userData = jsonObj["user"].toObject();
        int userId = userData["id"].toInt();
        QString username = userData["name"].toString();
        QString userEmail = userData["email"].toString();
        QString role = userData["role"].toObject()["rolename"].toString();
        QString department = userData["department"].toObject()["rolename"].toString();
        QString token = jsonObj["token"].toString();

        if (m_userRepo) {
            m_userRepo->saveUser(userId, username, password, userEmail, department, role, token);
        }

        setCurrentUserInfo(userId, username, userEmail, token);
        emit loggedIn();

        reply->deleteLater();
        return ""; // Success
    } else {
        QString errorMsg = jsonObj.value("message").toString("Login gagal. Periksa kembali username dan password Anda.");
        reply->deleteLater();
        return errorMsg;
    }
}

void AuthManager::logout()
{
    sendLogoutToAPI();

    if (m_currentUserId != -1 && m_userRepo) {
        m_userRepo->clearToken(m_currentUserId);
    }

    m_currentUserId = -1;
    m_currentUsername.clear();
    m_currentUserEmail.clear();
    m_userEmail.clear();
    m_authToken.clear();
    if (m_apiClient) m_apiClient->clearAuthToken();

    emit currentUserIdChanged();
    emit currentUsernameChanged();
    emit currentUserEmailChanged();
    emit userEmailChanged();
    emit authTokenChanged();
    emit loggedOut();

    qDebug() << "User logged out";
}

void AuthManager::sendLogoutToAPI()
{
    if (m_currentUserId == -1 || m_authToken.isEmpty()) return;

    QEventLoop loop;
    QNetworkReply *reply = m_apiClient->post(QUrl("https://deskmon.pranala-dt.co.id/api/logout"), QJsonDocument(QJsonObject()).toJson(), true);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() == QNetworkReply::NoError) {
        qDebug() << "Logout API success:" << reply->readAll();
    } else {
        qWarning() << "Logout API failed:" << reply->errorString();
    }
    reply->deleteLater();
}

QString AuthManager::savedUsername() const
{
    int userId = -1;
    QString username, email, token;
    if (m_userRepo && m_userRepo->findUserWithToken(userId, username, email, token)) {
        return email;
    }
    return "";
}

QString AuthManager::savedPassword() const
{
    int userId = -1;
    QString username, email, token;
    if (m_userRepo && m_userRepo->findUserWithToken(userId, username, email, token)) {
        return m_userRepo->getPassword(username);
    }
    return "";
}

QString AuthManager::getUserEmail(const QString &username) const
{
    return m_userRepo ? m_userRepo->getEmail(username) : "";
}

QString AuthManager::getUserDepartment(const QString &username) const
{
    return m_userRepo ? m_userRepo->getDepartment(username) : "";
}

QString AuthManager::getUsernameById(int userId) const
{
    return m_userRepo ? m_userRepo->getUsernameById(userId) : "";
}

QString AuthManager::getUserPassword(const QString &username) const
{
    return m_userRepo ? m_userRepo->getPassword(username) : "";
}

bool AuthManager::isUsernameTaken(const QString &username) const
{
    return m_userRepo ? m_userRepo->isUsernameTaken(username) : false;
}

void AuthManager::showAuthTokenErrorMessage()
{
    if (m_isTokenErrorVisible) return;
    m_isTokenErrorVisible = true;
    emit showAuthTokenErrorWindow("Sesi Anda telah berakhir atau tidak valid.\nSilakan login ulang untuk melanjutkan.");
}

void AuthManager::clearToken()
{
    m_authToken.clear();
    if (m_apiClient) m_apiClient->clearAuthToken();
    if (m_currentUserId > 0 && m_userRepo) {
        m_userRepo->clearToken(m_currentUserId);
    }
}
