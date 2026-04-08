// deskmon_auth.cpp
// Menangani semua logika autentikasi dan sesi pengguna:
//   - Login (API + fallback local)
//   - Logout dan kirim ke server
//   - Manajemen token
//   - Ambil info user: email, department
//   - Submit alasan early leave
#include "deskmon.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QEventLoop>
#include <QTimer>
#include <QDebug>

// ─────────────────────────────────────────────────
//  Login — Autentikasi via API
// ─────────────────────────────────────────────────

QString Deskmon::authenticate(const QString &loginInput, const QString &password)
{
    if (!ensureDatabaseOpen()) {
        qWarning() << "Cannot authenticate: Database is not open";
        return "Database is not open";
    }

    bool isEmail = loginInput.contains("@");
    QString loginType = isEmail ? "email" : "username";
    qDebug() << "Attempting login with" << loginType << ":" << loginInput;

    QNetworkRequest request(QUrl("https://deskmon.pranala-dt.co.id/api/login"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject jsonPayload;
    if (isEmail) {
        jsonPayload["email"] = loginInput;
    } else {
        QSqlQuery emailQuery(m_dataManager->activityDb());
        emailQuery.prepare("SELECT email FROM users WHERE username = :username");
        emailQuery.bindValue(":username", loginInput);
        if (emailQuery.exec() && emailQuery.next()) {
            jsonPayload["email"] = emailQuery.value(0).toString();
        } else {
            jsonPayload["email"] = loginInput;
        }
    }
    jsonPayload["password"] = password;

    QByteArray data = QJsonDocument(jsonPayload).toJson();
    qDebug() << "Sending login request...";

    QNetworkReply *reply = m_apiManager->networkManager()->post(request, data);
    QEventLoop loop;
    QTimer::singleShot(10000, &loop, &QEventLoop::quit);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (!reply->isFinished()) {
        reply->abort();
        reply->deleteLater();
        return "Koneksi ke server gagal atau timeout.\n Periksa koneksi internet Anda.";
    }

    QJsonDocument jsonResponse = QJsonDocument::fromJson(reply->readAll());
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

        m_authToken = token;
        emit authTokenChanged();

        QSqlQuery query(m_dataManager->activityDb());
        query.prepare("INSERT OR REPLACE INTO users "
                      "(id, username, password, email, department, role, token) "
                      "VALUES (:id, :username, :password, :email, :department, :role, :token)");
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

        m_pingTimer.start();
        sendPing(m_activeTaskId);
        m_isTokenErrorVisible = false;
        updateProductivityCache();
        setCurrentUserInfo(userId, username, userEmail);
        checkAndCreateNewDayRecord();
        loadWorkTimeData();
        startGlobalTimer();
        syncActiveTask();
        checkForUpdates();
        fetchAndStoreTasks();
        m_usageReportTimer.start();
        m_isTrackingActive = true;
        m_isTaskPaused = false;
        m_pauseStartTime = 0;

        reply->deleteLater();
        return "";
    } else {
        QString serverMessage = jsonObj["message"].toString();

        if (serverMessage == "Login not allowed from this IP address") {
            QString ipAddress = jsonObj["your_ip"].toString();
            if (!ipAddress.isEmpty())
                serverMessage.append(". " + ipAddress);
        }

        reply->deleteLater();
        return serverMessage.isEmpty() ? "Username atau password salah." : serverMessage;
    }

    qDebug() << "Login failed completely.";
    reply->deleteLater();
    return "Terjadi kesalahan yang tidak diketahui.";
}

// ─────────────────────────────────────────────────
//  Logout
// ─────────────────────────────────────────────────

void Deskmon::logout()
{
    m_sessionManager->logout();

    m_taskRefreshTimer.stop();
    m_apiWorkTimeTimer.stop();
    m_taskTimer.stop();
    m_pingTimer.stop();

    m_isTrackingActive = false;
    m_isTaskPaused = true;
    m_activeTaskId = -1;

    emit trackingActiveChanged();
    emit taskPausedChanged();
    emit activeTaskChanged();
    emit currentUserIdChanged();
}

void Deskmon::sendLogoutToAPI()
{
    m_apiManager->sendLogout(m_authToken);
}

void Deskmon::submitEarlyLeaveReason(const QString &reason)
{
    m_apiManager->submitEarlyLeaveReason(m_currentUserEmail, reason, m_authToken);
}

// ─────────────────────────────────────────────────
//  Manajemen Sesi & Token
// ─────────────────────────────────────────────────

void Deskmon::setCurrentUserInfo(int userId, const QString &username, const QString &email)
{
    m_currentUserId = userId;
    m_currentUsername = username;
    m_currentUserEmail = email;
    m_userEmail = email;

    if (m_apiManager) {
        m_apiManager->setAuthToken(m_authToken);
    }

    emit currentUserIdChanged();
    emit currentUsernameChanged();
    emit currentUserEmailChanged();
    emit userEmailChanged();

    qDebug() << "Current user set - ID:" << userId << ", Username:" << username;
}

QString Deskmon::getCurrentToken() const
{
    return m_authToken;
}

void Deskmon::clearToken()
{
    m_authToken.clear();

    if (m_currentUserId > 0) {
        QSqlQuery query(m_dataManager->activityDb());
        query.prepare("UPDATE users SET token = NULL WHERE id = ?");
        query.addBindValue(m_currentUserId);
        if (!query.exec())
            qWarning() << "Failed to clear token from database:" << query.lastError().text();
    }

    qDebug() << "Token cleared from memory and database";
}

// ─────────────────────────────────────────────────
//  Ambil Info User dari Database
// ─────────────────────────────────────────────────

QString Deskmon::getUserEmail(const QString &username)
{
    if (!ensureDatabaseOpen()) {
        qWarning() << "Cannot get user email: Database is not open";
        return "";
    }

    QSqlQuery query(m_dataManager->activityDb());
    query.prepare("SELECT email FROM users WHERE username = :username");
    query.bindValue(":username", username);

    if (query.exec() && query.next()) {
        QString email = query.value(0).toString();
        qDebug() << "Found email for user" << username << ":" << email;
        return email;
    }

    qWarning() << "Failed to get email for user:" << username;
    return "";
}

QString Deskmon::getUserDepartment(const QString &username)
{
    if (!ensureDatabaseOpen()) {
        qWarning() << "Cannot get user department: Database is not open";
        return "";
    }

    QSqlQuery query(m_dataManager->activityDb());
    query.prepare("SELECT role FROM users WHERE username = :username");
    query.bindValue(":username", username);

    if (query.exec() && query.next()) {
        QString dept = query.value(0).toString();
        qDebug() << "Found department for user" << username << ":" << dept;
        return dept;
    }

    qWarning() << "Failed to get department for user:" << username;
    return "";
}

QString Deskmon::getUserPassword(const QString &username)
{
    QSqlQuery query(m_dataManager->activityDb());
    query.prepare("SELECT password FROM users WHERE username = ?");
    query.addBindValue(username);

    if (query.exec() && query.next()) {
        return query.value(0).toString();
    }
    return "";
}

QString Deskmon::savedUsername() const
{
    return m_sessionManager->getSavedUsername();
}

QString Deskmon::savedPassword() const
{
    return m_sessionManager->getSavedPassword();
}
