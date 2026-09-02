#include "AuthManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QEventLoop>
#include <QTimer>
#include <QDebug>

AuthManager::AuthManager(ApiClient *apiClient, WorkLogRepository *workLogRepo, QObject *parent)
    : QObject(parent)
    , m_apiClient(apiClient)
    , m_workLogRepo(workLogRepo)
{
}

bool AuthManager::tryAutoLogin()
{
    if (!m_workLogRepo->ensureDatabaseOpen()) return false;

    QSqlQuery query(m_workLogRepo->database());
    if (query.exec("SELECT id, username, email, token FROM users WHERE token IS NOT NULL AND token != '' LIMIT 1")) {
        if (query.next()) {
            int userId = query.value(0).toInt();
            QString username = query.value(1).toString();
            QString email = query.value(2).toString();
            QString token = query.value(3).toString();

            setCurrentUserInfo(userId, username, email, token);
            qDebug() << "Auto login as user ID:" << userId << "Username:" << username;
            emit loggedIn();
            return true;
        }
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
    if (!m_workLogRepo->ensureDatabaseOpen()) {
        qWarning() << "Cannot authenticate: Database is not open";
        return "Database is not open";
    }

    bool isEmail = loginInput.contains("@");
    QString loginType = isEmail ? "email" : "username";
    qDebug() << "Attempting login with" << loginType << ":" << loginInput;

    QJsonObject jsonPayload;
    if (isEmail) {
        jsonPayload["email"] = loginInput;
    } else {
        QSqlQuery emailQuery(m_workLogRepo->database());
        emailQuery.prepare("SELECT email FROM users WHERE username = :username");
        emailQuery.bindValue(":username", loginInput);
        if (emailQuery.exec() && emailQuery.next()) {
            jsonPayload["email"] = emailQuery.value(0).toString();
        } else {
            jsonPayload["email"] = loginInput;
        }
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

        QSqlQuery query(m_workLogRepo->database());
        query.prepare(
            "INSERT OR REPLACE INTO users "
            "(id, username, password, email, department, role, token) "
            "VALUES (:id, :username, :password, :email, :department, :role, :token)"
        );
        query.bindValue(":id", userId);
        query.bindValue(":username", username);
        query.bindValue(":password", password);
        query.bindValue(":email", userEmail);
        query.bindValue(":role", role);
        query.bindValue(":department", department);
        query.bindValue(":token", token);

        if (!query.exec()) {
            qWarning() << "Gagal menyimpan user ke database lokal:" << query.lastError();
        } else {
            qDebug() << "Data user tersimpan di database lokal. ID:" << userId;
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

    if (m_currentUserId != -1 && m_workLogRepo->ensureDatabaseOpen()) {
        QSqlQuery clearTokenQuery(m_workLogRepo->database());
        clearTokenQuery.prepare("UPDATE users SET token = '' WHERE id = :id");
        clearTokenQuery.bindValue(":id", m_currentUserId);
        clearTokenQuery.exec();
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
    if (!m_workLogRepo->ensureDatabaseOpen()) return "";
    QSqlQuery query(m_workLogRepo->database());
    if (query.exec("SELECT email FROM users WHERE token IS NOT NULL AND token != '' LIMIT 1") && query.next()) {
        return query.value(0).toString();
    }
    return "";
}

QString AuthManager::savedPassword() const
{
    if (!m_workLogRepo->ensureDatabaseOpen()) return "";
    QSqlQuery query(m_workLogRepo->database());
    if (query.exec("SELECT password FROM users WHERE token IS NOT NULL AND token != '' LIMIT 1") && query.next()) {
        return query.value(0).toString();
    }
    return "";
}

QString AuthManager::getUserEmail(const QString &username) const
{
    if (!m_workLogRepo->ensureDatabaseOpen()) return "";
    QSqlQuery query(m_workLogRepo->database());
    query.prepare("SELECT email FROM users WHERE username = :username");
    query.bindValue(":username", username);
    if (query.exec() && query.next()) {
        return query.value(0).toString();
    }
    return "";
}

QString AuthManager::getUserDepartment(const QString &username) const
{
    if (!m_workLogRepo->ensureDatabaseOpen()) return "";
    QSqlQuery query(m_workLogRepo->database());
    query.prepare("SELECT role FROM users WHERE username = :username");
    query.bindValue(":username", username);
    if (query.exec() && query.next()) {
        return query.value(0).toString();
    }
    return "";
}

QString AuthManager::getUsernameById(int userId) const
{
    if (!m_workLogRepo->ensureDatabaseOpen()) return "";
    QSqlQuery query(m_workLogRepo->database());
    query.prepare("SELECT username FROM users WHERE id = :id");
    query.bindValue(":id", userId);
    if (query.exec() && query.next()) {
        return query.value(0).toString();
    }
    return "";
}

QString AuthManager::getUserPassword(const QString &username) const
{
    if (!m_workLogRepo->ensureDatabaseOpen()) return "";
    QSqlQuery query(m_workLogRepo->database());
    query.prepare("SELECT password FROM users WHERE username = ?");
    query.addBindValue(username);
    if (query.exec() && query.next()) {
        return query.value(0).toString();
    }
    return "";
}

bool AuthManager::isUsernameTaken(const QString &username) const
{
    if (!m_workLogRepo->ensureDatabaseOpen()) return false;
    QSqlQuery query(m_workLogRepo->database());
    query.prepare("SELECT COUNT(*) FROM users WHERE username = :username");
    query.bindValue(":username", username);
    if (query.exec() && query.next()) {
        return query.value(0).toInt() > 0;
    }
    return false;
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
    if (m_currentUserId > 0 && m_workLogRepo->ensureDatabaseOpen()) {
        QSqlQuery query(m_workLogRepo->database());
        query.prepare("UPDATE users SET token = NULL WHERE id = ?");
        query.addBindValue(m_currentUserId);
        query.exec();
    }
}
